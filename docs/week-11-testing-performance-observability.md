# 第 11 周：测试、性能与可观测性

## 本周目标

让项目从“功能能演示”变成“问题可复现、性能可测量、故障可定位”。这是把个人作品从 Demo 提升为工程项目的关键一周。

## 验收目标

- 有客户端、Dedicated Server 和 Go 后台的启动检查；
- 有至少一套多人冒烟测试；
- 有拾取、死亡、撤离和结算的自动化或半自动化测试；
- 有结构化日志和贯穿请求链路的 `request_id/match_id/run_id`；
- 有 2 人、4 人和带额外 AI/可复制物体的性能基线；
- 有一次 Unreal Insights 分析；
- 有一次网络复制量分析；
- 有一次优化前后数据对比；
- 能通过日志找到模拟的重连失败和重复结算问题。

## 操作步骤

### 1. 建立测试分层

建议分为：

#### 单元级

- 物品堆叠和拆分；
- 背包容量；
- 结算状态转移；
- 幂等键生成；
- Token 过期判断。

#### UE 功能级

- Ability 激活失败；
- 生命归零进入死亡；
- 拾取距离校验；
- 撤离状态机；
- 断线恢复快照。

#### 联机冒烟级

- 两客户端加入；
- 射击和伤害；
- 同时争抢物品；
- 一个玩家死亡，一个玩家撤离；
- 结算回大厅。

#### 故障级

- 网络延迟和丢包；
- 后台超时；
- 数据库延迟；
- Server 进程退出；
- 重复请求和乱序请求。

### 2. 建立统一日志字段

UE 和 Go 日志都尽量包含：

```text
timestamp
service
level
event
request_id
player_id
match_id
run_id
server_instance_id
error_code
duration_ms
```

日志事件要记录事实，不要只记录自然语言。例如：

```text
event=settlement_received
run_id=...
idempotency_key=...
result=already_processed
```

### 3. 建立性能基线

固定测试条件：

- CPU、GPU、内存；
- 分辨率和画质；
- 地图；
- 玩家数；
- AI/可复制物体数量；
- 网络 RTT 和丢包率；
- Server Tick 目标；
- 测试时长。

记录：

- 客户端 FPS、Game Thread、Render Thread；
- Dedicated Server Tick 和帧耗时；
- 网络发送/接收带宽；
- Replication Actor 数量；
- 后台 API p50/p95/p99；
- 结算成功率和重试次数。

### 4. 使用 Unreal Insights

先抓取一个正常 60 秒会话，再抓取一个 4 人压力会话。按照以下顺序查看：

1. Game Thread 是否持续超预算；
2. Tick 中是否有不必要的高频工作；
3. 复制和序列化是否出现峰值；
4. 加载、GC、动画或 UI 是否产生长帧；
5. 优化前后是否有可量化差异。

不要只截图工具界面，要在报告中写出问题、假设、改动和数据。

### 5. 做一次有证据的优化

可选方向：

- 降低无关 Actor 的复制频率；
- 只复制拥有者需要的背包字段；
- 把周期 Tick 改成事件驱动；
- 减少 UI 全量刷新；
- 对远处 Actor 使用 relevancy 或 Replication Graph；
- 给后台请求设置合理超时，避免线程堆积。

优化前先测量，优化后再测量。没有数据的“优化”不要写进作品集。

## 实现原理

可观测性不是把日志越写越多，而是让一个业务事件能跨客户端、专服和后台关联起来。`match_id` 标识一局，`run_id` 标识玩家在一局中的运行，`request_id` 标识一次请求，`server_instance_id` 标识承载对局的进程。

性能优化也不是盲目降低所有更新频率。多人游戏要在体验、准确性、带宽、CPU 和可维护性之间做取舍，因此必须先定义测试条件和指标。

## 常见问题

### 日志很多但查不到问题

检查是否缺少稳定 ID、事件名和错误码。自然语言日志适合阅读，但跨服务排查必须有结构化字段。

### FPS 低但不知道是谁造成的

分别看 Game Thread、Render Thread、GPU 和网络，避免把所有性能问题都归因于客户端渲染。

### 只在本机测试通过

固定两台或多个客户端的测试方式，并记录网络条件。多人行为不能只用单进程 PIE 代表。

## 本周作品集产出

- 测试矩阵；
- 一套启动检查或冒烟脚本；
- Unreal Insights 报告；
- Replication 和带宽报告；
- 优化前后对比图；
- 一次端到端故障定位记录。

## 参考资料

- [Unreal Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-in-unreal-engine)
- [Testing and Optimizing Your Content](https://dev.epicgames.com/documentation/en-us/unreal-engine/testing-and-optimizing-your-content)
- [Functional Testing](https://dev.epicgames.com/documentation/en-us/unreal-engine/functional-testing-in-unreal-engine)
- [Replication Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/replication-graph-in-unreal-engine)
- [Gameplay Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-insights-in-unreal-engine)

