# 第 10 周：断线重连、结算幂等与数据一致性

## 本周目标

让网络和 HTTP 请求可以重复、最终结果不能重复：玩家断线后 90 秒内恢复同一 Pawn/Run；Dedicated Server 生成权威结算；SQLite transaction、状态条件和唯一约束保证重复或并发结算只发奖一次。

## 执行基线

开始前完整阅读[12 周执行基线](execution-baseline.md)。本文中的 Dedicated Server 指 Editor Dedicated Process；只使用 `D:\Software\UE_5.8`，不得访问受保护的 ue5-main 源码目录。结算正确性不依赖 Server Target 的打包形式。

## 与总蓝图同步：Go 后台与幂等结算闭环

本周要闭合的是“至少一次发送、至多一次产生经济效果”。网络超时后 Server 无法判断 Backend 是否已提交，因此一定会重试；正确性必须来自同一个 `idempotency_key`、数据库唯一约束和单个短事务，而不是假设请求只到达一次。

完整数据流：

```text
Run 进入 Extracted / Dead / Abandoned
  -> Dedicated Server 冻结权威结果快照
  -> 生成一次 SettlementEvent 和稳定 idempotency_key
  -> POST /v1/matches/{match_id}/settlements
  -> Backend 校验 Server、Match、Run 与事件终态
  -> SQLite 短事务写事件、Run 终态、库存流水和可重复响应
  -> Server 收到确认；超时则使用同一 key 重试
```

实现前固定不变量：

- 一个 `run_id` 只能从进行中进入一个终态；
- 同一个 `item_instance_id` 不能同时进入永久仓库和丢失清单；
- Client 永远不能提供 `extracted_items` 或奖励倍率作为权威输入；
- 重复 key 返回第一次保存的响应，不能重新计算当下仓库状态；
- `RunState`、SettlementEvent、库存 Ledger 和奖励必须同事务提交或一起回滚；
- 响应丢失、Server 重启和并发提交都不得重复发奖。

建议先写数据库级测试，再写 HTTP：同 key 顺序重复、同 key 并发重复、不同 key 竞争同一 run、死亡/撤离竞态、事务中途错误、Commit 成功但响应丢失。随后才接 UE Server 的重试队列和本地 spool。

90 秒重连与结算共享同一个 `run_id`。宽限期内 Pawn 仍可死亡或完成已经开始的世界规则；一旦 Run 进入终态，重连只能恢复结果界面，不能生成新 Pawn 或第二份 Settlement。

## 前置条件与周门槛

- 第 9 周并发分配和 Ticket 身份链通过。
- 重连身份使用稳定 player_id/match_id/run_id，不使用 NetConnection 或临时 Controller ID。
- 结算内容只接受 Dedicated Server 生成的数据，Client 不上传权威奖励。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 断线状态机与 Server 快照 | 4 小时 |
| 2 | 重连 Ticket 与 Pawn 恢复 | 4 小时 |
| 3 | 结算 schema、事务和幂等 | 4–5 小时 |
| 4 | 重试队列与失败状态 | 3 小时 |
| 5 | 并发、响应丢失和竞态实验 | 2–4 小时 |

## 先读什么

- UE GameMode Logout/PreLogin/PostLogin、PlayerState inactive/reconnect 行为；
- 第 7 周 RunState/InventoryState 和结果快照；
- 第 8–9 周 Ticket、Match/Run schema；
- SQLite transaction、unique constraint、UPSERT 与 Go `database/sql` 错误处理。

## 工作单元 1：断线状态机和快照

固定状态：

```text
Connected -> Disconnected -> GracePeriod
GracePeriod -> Reconnected -> Restoring -> Connected
GracePeriod -> Timeout -> Abandoned
```

MVP 规则：宽限期 90 秒；Pawn 留在世界，可被攻击，倒计时继续；槽位不释放给其他身份；物品不可因断线复制/自动保全；若死亡则 Run 进入 Dead，重连只能看结果。

Server 为每个 run 保存恢复快照：run_state、Pawn 位置/旋转、Health/Armor、装备、完整本局 InventoryState/version、Extraction 状态/结束时间、snapshot_version。快照由权威状态更新驱动，不从 Client 上传。

断线时解绑临时 Controller，但保留 run 与 Pawn 的唯一关联。相同 player_id/run_id 不得生成第二个 Pawn。

## 工作单元 2：重连 Ticket 与恢复

每次重连请求都签发新的 Ticket，绑定 ticket_id、player_id、match_id、run_id、server_instance_id/generation、purpose=`reconnect`、snapshot_version 和 expiry。Backend 只在 Match 仍 InGame、Run 非终态且宽限期内签发；首次 connect Ticket 即使仍未过期也不能用于 reconnect。

Server 按第 8–9 周同一原子协议处理：验证 reconnect purpose → 找到现有 run → 原子预留 Ticket 和 run 连接槽 → 拒绝第二个活跃连接 → 重新绑定 Controller/PlayerState/Pawn → PostLogin 成功后消费 Ticket → 发送当前完整快照 → Client 应用 UI/表现 → Server 标记 Connected。恢复失败时释放预留但不创建第二个 Pawn。

Client 旧版本不得覆盖 Server 新状态；恢复期间禁止 Fire/Inventory Command，直到收到 `restore_complete(snapshot_version)`。

测试：10 秒重连、89 秒重连、91 秒重连、首次 connect Ticket 冒充 reconnect、同 reconnect Ticket 双开、两个 reconnect Ticket 竞争同一 run、握手失败后重试、重连前 Pawn 死亡、撤离成功后重连、恢复期间发背包命令。

## 工作单元 3：权威结算与 SQLite 幂等事务

Dedicated Server 生成：

```text
SettlementEvent {
  idempotency_key, settlement_version,
  server_instance_id, match_id, run_id, player_id,
  result, extracted_items, lost_items, occurred_at
}
```

固定唯一约束：`UNIQUE(run_id, settlement_version)`、作用域化的 `UNIQUE(actor_scope, route, idempotency_key)`，以及事件内 `UNIQUE(settlement_event_id, item_instance_id)`。幂等记录保存规范化 `request_hash` 和第一次完整响应。新增 `settlement_events`、`player_rewards`、`inventory_ledger`、权威 `item_instances` 所有权/version，并扩展 runs 终态。

提交前拒绝 extracted/lost 列表内部重复或彼此相交。对每个物品使用条件更新：只有 `owner_kind='run' AND owner_id=当前 run_id AND version=预期值` 才能转移到 stash 或 lost；受影响行数必须等于请求物品数，否则整个事务回滚为 `ItemOwnershipConflict`。Ledger 对 `(item_instance_id, from_version)` 唯一，防止同一所有权版本产生两次经济移动。

一个短 transaction 中：

1. 验证 Server 身份与 Match/Run 关系；
2. 检查作用域化 idempotency key 与 request_hash；
3. 用条件更新抢占 Run 终态；
4. 插入 settlement_event；
5. 条件转移 item_instances，并写 inventory_ledger 和 player_rewards；
6. 保存可重复返回的响应；
7. Commit。

冲突分类固定为：同 scope/key 且同 request_hash 返回原响应并标记 `already_processed=true`；同 key 不同 hash 返回 `IdempotencyMismatch`；不同 key 提交已经终态的 run 返回 `RunConflict`。后两者都不能伪装成成功重复。死亡和撤离竞争时，只有第一个合法 `InProgress -> Dead/Extracted` 获胜。

## 工作单元 4：Server 重试与明确失败状态

Server 结算发送采用至少一次：生成事件后、第一次 HTTP 发送前，先用“临时文件写完并 Flush -> 原子 rename”持久化 spool；Backend 确认后再删除。spool 保存 payload、request_hash、固定 idempotency_key，以及 Backend 分配时签发、绑定 match/server 且短期有效的 Settlement Replay Token。spool 目录限制为运行账号可读写，禁止写日志或进入版本控制。这样异常 kill 不依赖关闭回调，新进程可在 recovery 模式授权重放旧实例事件；损坏文件隔离到 quarantine 并进入 FailedManualReview，不能静默丢弃。

指数退避加抖动使用例如 1/2/4/8/16 秒，耗尽后保留 PendingRetry。只有 Backend 返回不可重试的验证错误才进入 FailedManualReview。Replay Token 过期时停止自动重放并报警，不能退回使用普通 Client Token 或只信任旧 `server_instance_id`。

区分：HTTP 超时/5xx/SQLite busy 可重试；401/403、schema invalid、Run conflict 不自动重试。响应丢失时保持同一 key，不创建新事件。

日志含 request_id、match_id、run_id、idempotency_key、attempt、result，不记录完整物品隐私 payload 或密钥。

## 工作单元 5：故障注入和自动化验收

固定实验：

1. 同一结算顺序提交 10 次：一条 event、一组 reward/ledger；
2. 10 goroutine 并发提交：只有一个 `processed`，其余 `already_processed`；
3. DB Commit 后故意丢弃 HTTP 响应，再重试得到原结果；
4. Extracted/Dead 几乎同时提交：终态只有一个；
5. Backend 停止 20 秒再恢复：Server 用同 key 完成重试；
6. Server 在 pending 时退出并重启：spool 重放一次；
7. 撤离倒计时断线并在 90 秒内/外重连；
8. Client 伪造 extracted_items：Server/Backend 不接受 Client 权威结算。
9. 同一 key 改变 result/物品 body：返回 `IdempotencyMismatch`，仓库不变；
10. 不同 key 竞争同一 run：失败者返回 `RunConflict`，不能得到伪成功；
11. extracted/lost 重复、相交或物品不属于该 run：整个 transaction 回滚；
12. 首次发送前强杀 Server：重启后仍能从已经原子落盘的 spool 恢复。

每次实验运行 SQL 查询证明事件、奖励、流水计数和 Run 终态，而不仅看 HTTP 200。

## 验收目标

- [ ] 90 秒规则、Pawn 保留和受攻击行为明确；
- [ ] 重连复用同一 Pawn/Run，不创建重复角色；
- [ ] Health、Armor、装备、背包/version 和撤离状态恢复正确；
- [ ] 终态 Run 重连不能更改结果；
- [ ] 10 次顺序/并发结算只发奖一次；
- [ ] Commit 后响应丢失可返回原处理结果；
- [ ] Dead/Extracted 竞态只有一个终态；
- [ ] 幂等 key/body 不匹配、RunConflict 和真正重放能被明确区分；
- [ ] 物品归属/version 条件更新保证 Ledger 与仓库不复制物品；
- [ ] Backend 不可用时有 PendingRetry/spool/Failed 状态；
- [ ] Client 无法上传权威奖励。

## 实现原理

网络无法可靠保证 exactly-once 传输，因此使用至少一次投递、固定幂等键、Server 权威事件、数据库唯一约束和单 transaction。重连恢复的是稳定业务身份和权威状态，而不是简单重开 Socket。

## 常见问题与停止条件

- 重连生成第二个 Pawn：按 player_id/run_id 查现有实体后再绑定。
- 背包恢复旧版本：Server 快照版本永远胜过 Client 缓存。
- 奖励重复：事件、Run 终态、流水和响应必须在同一 transaction。
- 重试换新 key：始终复用原 idempotency_key。

重复奖励、终态覆盖或重连复制角色任一存在时，不进入性能与作品集阶段。

## 本周作品集产出

- 重连状态机和恢复时序图；
- Settlement schema/事务图；
- 10 次并发结算测试输出；
- 响应丢失与重放证据；
- 断线重连视频。

## 参考资料

- [Unreal Networking](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-and-multiplayer-in-unreal-engine)
- [Go context](https://pkg.go.dev/context)
- [Go database/sql](https://pkg.go.dev/database/sql)
