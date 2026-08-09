# 第 10 周：断线重连、结算幂等与数据一致性

## 本周目标

让网络和 HTTP 请求可以重复、最终结果不能重复：玩家断线后 90 秒内恢复同一 Pawn/Run；Dedicated Server 生成权威结算；SQLite transaction、状态条件和唯一约束保证重复或并发结算只发奖一次。

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

重连 Ticket 绑定 player_id、match_id、run_id、server_instance_id、用途=`reconnect`、snapshot_version 和 expiry。Backend 只在 Match 仍 InGame、Run 非终态且宽限期内签发。

Server 校验 Ticket 后按顺序：找到现有 run → 拒绝第二个活跃连接 → 重新绑定 Controller/PlayerState/Pawn → 发送当前完整快照 → Client 应用 UI/表现 → Server 标记 Connected。

Client 旧版本不得覆盖 Server 新状态；恢复期间禁止 Fire/Inventory Command，直到收到 `restore_complete(snapshot_version)`。

测试：10 秒重连、89 秒重连、91 秒重连、同 Ticket 双开、重连前 Pawn 死亡、撤离成功后重连、恢复期间发背包命令。

## 工作单元 3：权威结算与 SQLite 幂等事务

Dedicated Server 生成：

```text
SettlementEvent {
  idempotency_key, settlement_version,
  server_instance_id, match_id, run_id, player_id,
  result, extracted_items, lost_items, occurred_at
}
```

固定唯一约束：`UNIQUE(run_id, settlement_version)` 与 `UNIQUE(idempotency_key)`。新增 `settlement_events`、`player_rewards`、`inventory_ledger`，并扩展 runs 终态。

一个短 transaction 中：

1. 验证 Server 身份与 Match/Run 关系；
2. 尝试插入 settlement_event；
3. 用条件更新 `runs SET status=终态 WHERE status='InProgress'`；
4. 写 inventory_ledger 和 player_rewards；
5. 保存可重复返回的响应；
6. Commit。

遇到唯一冲突，读取原事件/响应并返回 `already_processed=true`；不得再次发奖。死亡和撤离竞争时，只有第一个合法 `InProgress -> Dead/Extracted` 获胜，后到事件记录 conflict 且不覆盖终态。

## 工作单元 4：Server 重试与明确失败状态

Server 结算发送采用至少一次：固定 idempotency_key，指数退避加抖动，最多例如 1/2/4/8/16 秒后进入 PendingRetry；进程关闭前把未确认事件写入本地受控 spool 文件，重启后重放。只有 Backend 返回不可重试的验证错误才进入 FailedManualReview。

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

每次实验运行 SQL 查询证明事件、奖励、流水计数和 Run 终态，而不仅看 HTTP 200。

## 验收目标

- [ ] 90 秒规则、Pawn 保留和受攻击行为明确；
- [ ] 重连复用同一 Pawn/Run，不创建重复角色；
- [ ] Health、Armor、装备、背包/version 和撤离状态恢复正确；
- [ ] 终态 Run 重连不能更改结果；
- [ ] 10 次顺序/并发结算只发奖一次；
- [ ] Commit 后响应丢失可返回原处理结果；
- [ ] Dead/Extracted 竞态只有一个终态；
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
