# 第 4 周：多人联机与复制验证

## 本周目标

把第 3 周的单机行为改成真正的客户端-服务器模式，重点理解角色复制、RPC、Ownership、Relevancy 和服务器权威。完成一个可以被面试官观察和追问的网络实验场。

## 验收目标

- Dedicated Server 可以运行一局对局；
- 两个客户端能同时加入并看到对方移动；
- 开枪输入由客户端发起，伤害和死亡由服务器确认；
- 客户端无法通过修改本地生命值、弹药或奖励直接改变最终状态；
- 能展示 Server RPC、Client RPC、Multicast 或 Replicated Property 各自的用途；
- 在模拟延迟和丢包时，系统不会重复扣血、重复发奖励或进入不可恢复状态；
- 完成一份网络实验记录，包含现象、原因和修复方式。

## 操作步骤

### 1. 建立网络角色调试层

在 HUD 或日志中显示：

- `HasAuthority()`；
- `GetLocalRole()`；
- `GetRemoteRole()`；
- Actor 所属 Controller；
- 当前连接的 PlayerState；
- 当前对局 ID。

这样每次看到“为什么客户端不执行”时，先确认代码运行在哪一端。

### 2. 给行为划分发起者和权威者

以射击为例：

```text
本地输入
  -> 客户端预测枪口和声音
  -> Server RPC 提交射击意图/时间/瞄准信息
  -> 服务器校验弹药、冷却、武器拥有权
  -> 服务器检测命中并修改生命值
  -> 复制生命值变化
  -> 客户端播放确认后的受击/死亡表现
```

客户端可以立即播放表现，但不能直接写最终生命值、库存或结算奖励。

### 3. 实现最小的服务器校验

服务器至少校验：

- RPC 的调用者是否拥有对应角色；
- 武器是否属于调用者；
- 当前是否处于可射击状态；
- 射击间隔是否满足；
- 弹匣是否还有弹药；
- 请求中的方向和距离是否合理；
- 是否超出服务器允许的时间窗口。

失败时返回一个明确的失败原因，并记录结构化日志。不要静默忽略所有错误。

### 4. 做三组网络实验

实验 A：正常网络。记录两客户端移动、射击和死亡的行为。

实验 B：增加延迟。观察客户端预测、服务器确认和最终修正。

实验 C：模拟丢包或短暂断线。观察玩家状态、武器状态和 UI 是否能恢复。

每组实验记录：

- 网络参数；
- 操作序列；
- 客户端看到的状态；
- 服务器看到的状态；
- 最终是否一致；
- 修复或接受的原因。

### 5. 开始关注复制成本

不要把整个库存、所有日志字段和临时表现变量都标记为 Replicated。区分：

- 需要其他玩家看到的状态；
- 只需要拥有者看到的状态；
- 只在服务器内部存在的状态；
- 只在本地表现的状态。

为远处 Actor 和不相关 Actor 保留后续使用 Replication Graph 的空间。

## 实现原理

Unreal 的多人模式不是“把变量同步过去”这么简单。RPC 解决的是事件或意图传递，Replication 解决的是状态传播；服务器权威解决的是最终可信来源。正确的设计通常是客户端发送意图，服务器执行规则，客户端接收结果。

Ownership 决定谁有资格发送某些 RPC。Relevancy 决定某个客户端是否需要看到某个 Actor。把所有操作都做成 Multicast 会产生不必要的广播，也会让客户端拥有过多的控制权。

## 常见问题

### Server RPC 没有执行

检查：

- 调用对象是否被当前客户端拥有；
- RPC 是否声明在正确的 Actor/Component 上；
- 是否在客户端对象上调用；
- Actor 是否已经复制和建立连接；
- 函数参数是否可复制。

### 两个客户端看到的结果不一致

先判断是表现不同还是最终状态不同。表现可以因延迟不同而不同，但最终生命值、死亡、库存和结算必须以服务器状态为准。

### 服务器被客户端请求打爆

对 RPC 做频率限制、状态校验和参数边界检查。日志中记录调用者、请求类型、频率和失败原因。

## 本周作品集产出

- 两客户端 + Dedicated Server 演示视频；
- 一张射击请求的时序图；
- 一份网络实验报告；
- 一段展示非法客户端请求被服务器拒绝的日志；
- 一篇“RPC 与 Replicated Property 的取舍”文章。

## 参考资料

- [Multiplayer Programming Quick Start](https://dev.epicgames.com/documentation/en-us/unreal-engine/multiplayer-programming-quick-start-for-unreal-engine)
- [Networking and Multiplayer](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-and-multiplayer-in-unreal-engine)
- [Replication Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/replication-graph-in-unreal-engine)
- [Unreal Engine Replication](https://dev.epicgames.com/documentation/en-us/unreal-engine/replication-in-unreal-engine)

