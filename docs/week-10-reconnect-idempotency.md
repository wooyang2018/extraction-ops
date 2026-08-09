# 第 10 周：断线重连、结算幂等与数据一致性

## 本周目标

处理真实网络环境中最容易暴露工程能力的部分：断线、重试、超时、重复请求、结算失败和恢复。目标是做到“请求可以重复，结果不能重复”。

## 验收目标

- 玩家断线后在宽限时间内能够回到同一局；
- 重连不会创建重复角色；
- 重连后生命、装备、背包和对局 ID 恢复正确；
- 对局结束后重连不能修改结算结果；
- 同一结算请求发送 10 次只发放一次奖励；
- 并发结算请求最终只有一个有效结果；
- 数据库写入成功但响应丢失时，重试能够返回已处理结果；
- 后台不可用时，Dedicated Server 能进入明确的待重试或失败状态。

## 操作步骤

### 1. 设计断线状态机

```text
Connected
  -> Disconnected
  -> GracePeriod
      -> Reconnected -> Restoring -> Connected
      -> Timeout -> Abandoned
```

明确每个状态：

- 角色是否保留；
- 是否可以被攻击；
- 其他玩家是否可以拾取其物品；
- 是否继续消耗撤离倒计时；
- 是否允许新的连接占用槽位。

建议 MVP 保留角色 90 秒，重连时依据稳定的 `player_id` 恢复，而不是依据临时连接 ID。

### 2. 实现重连票据

重连需要一个短期有效的凭证，包含：

- `player_id`；
- `match_id`；
- `run_id`；
- `server_instance_id`；
- 过期时间；
- 当前状态版本。

服务器收到重连请求后，先确认玩家仍属于对局，再发送完整快照或增量恢复数据。

### 3. 设计结算事件

Dedicated Server 生成服务端可验证的结算事件：

```text
SettlementEvent {
  run_id
  match_id
  player_id
  result
  extracted_items
  lost_items
  server_instance_id
  settlement_version
  idempotency_key
}
```

不要接受客户端自行上传“我击杀了多少人、我带出了什么”的最终结算结果。客户端可以上传遥测或表现数据，但奖励依据服务端状态。

### 4. 实现幂等消费

采用：

> 至少一次投递 + 服务端幂等消费 + 数据库唯一约束。

推荐表：

- `settlement_events`；
- `player_rewards`；
- `runs`；
- `inventory_ledger`。

关键唯一约束可以是：

```text
unique(run_id, player_id, settlement_version)
```

或：

```text
unique(idempotency_key)
```

在同一个数据库事务中完成：写入结算事件、写入奖励流水、更新 Run 状态。处理重复请求时返回 `already_processed`，不要返回含糊的成功或失败。

### 5. 做故障注入

至少模拟：

- 结算请求超时；
- 数据库写成功但 HTTP 响应丢失；
- 同一请求并发 10 次；
- 成功结算后再次发送失败结算；
- 玩家死亡和撤离请求几乎同时到达；
- 玩家在撤离倒计时中断线；
- Go 服务短暂不可用；
- 客户端进程被杀死后重新启动。

## 实现原理

网络系统很难在所有边界上保证严格的 exactly-once 传输。工程上更可靠的做法是允许消息至少到达一次，同时让业务处理具备幂等性。唯一键、状态机和事务共同保证最终结果不重复。

重连的关键不是“重新连上 Socket”，而是恢复业务身份和对局状态。玩家的身份应由稳定的 `player_id` 绑定，当前对局由 `match_id/run_id` 绑定，恢复操作应检查状态版本。

## 常见问题

### 为什么重试后奖励重复

通常是“先发奖励，再记录已处理”，或者数据库没有唯一约束。把结果记录和奖励流水放进同一个事务，数据库约束作为最后一道保护。

### 成功和失败结算同时到达

使用明确的状态转移规则，例如 `InProgress -> Extracted` 或 `InProgress -> Dead`，终态只能写入一次，后到事件只能记录为冲突，不得覆盖终态。

### 重连时恢复了旧背包

给快照和操作增加版本号。服务器只接受当前版本范围内的恢复请求，客户端不能用旧快照覆盖新状态。

## 本周作品集产出

- 重连状态机图；
- 结算事件 schema；
- 数据库唯一约束和事务说明；
- 10 次重复结算的自动化测试结果；
- 断线、重试、恢复的演示视频。

## 参考资料

- [Networking and Multiplayer](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-and-multiplayer-in-unreal-engine)
- [Replication in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/replication-in-unreal-engine)
- [HTTP API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/HTTP)
- Go 标准库：[context](https://pkg.go.dev/context)、[database/sql](https://pkg.go.dev/database/sql)

