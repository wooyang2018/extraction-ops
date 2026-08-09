# 第 12 周：作品集包装与面试验收

## 本周目标

把前 11 周的工程工作整理成别人能运行、能理解、能追问的求职作品。面试官不会因为代码量大就自动觉得项目厉害；真正有说服力的是可运行证据、清晰取舍和真实问题。

## 验收目标

- 有 Client、Dedicated Server、Go Backend 的一键启动说明；
- 有一段不超过 3 分钟的演示视频；
- 有一张清晰架构图和至少三张时序图；
- 有断线重连、重复结算、网络延迟和性能分析证据；
- 有完整 README，陌生人能够按文档复现核心流程；
- 有一份简历项目描述和一份 5 分钟口述稿；
- 能回答至少 10 个客户端、网络和后台追问；
- 明确项目当前限制，不夸大为生产级游戏。

## 操作步骤

### 1. 整理仓库结构

建议：

```text
LyraStarterGame/
  Source/
  Plugins/ExtractionOps/
  Config/
  Scripts/
    launch-backend.ps1
    launch-server.ps1
    launch-client.ps1
  docs/
  README.md
```

清理：

- 无关实验代码；
- 无法运行的半成品功能；
- 本机绝对路径；
- Token、账号和私有资源；
- 过时截图和错误说明。

### 2. 制作 3 分钟演示视频

建议时间线：

```text
00:00-00:20  项目目标和技术栈
00:20-01:10  两个客户端进入同一局，移动、射击和搜刮
01:10-01:40  背包、装备和撤离倒计时
01:40-02:00  一名玩家断线并重连
02:00-02:20  成功撤离和结算
02:20-02:40  重复结算请求被幂等处理
02:40-03:00  架构图、性能指标和 AI 工具
```

视频重点展示功能和工程证据，不要使用过度剪辑掩盖不可运行的环节。

### 3. 写架构说明

必须解释：

- UE Client 负责什么；
- Dedicated Server 负责什么；
- Go Backend 负责什么；
- 哪些数据由谁最终负责；
- 为什么实时状态不放进普通 HTTP 后台；
- 为什么结算必须服务端权威；
- 断线和重复请求如何处理；
- 如何扩展到多个 Server 实例。

### 4. 写三张时序图

至少包括：

1. 登录到进入对局；
2. 客户端射击到服务端确认伤害；
3. 撤离到结算和持久化。

每张图标注请求方向、权威来源、失败路径和重试行为。

### 5. 写简历项目描述

推荐格式：

```text
多人撤离射击 Vertical Slice | UE5 / C++ / Lyra / GAS / Go

- 基于 Lyra 搭建 2-4 人 Dedicated Server 多人玩法，完成移动、武器、GAS 伤害、搜刮、背包、撤离和结算闭环。
- 设计客户端、Dedicated Server 与 Go 控制面的职责边界，由服务端权威校验伤害、拾取、撤离和奖励。
- 使用 player_id/match_id/run_id 和数据库唯一约束实现断线恢复、重复结算幂等和可追踪日志。
- 通过 Unreal Insights、stat net 和结构化日志定位网络与性能问题，并沉淀内部 AI 诊断工具。
```

只填写自己真正完成并能现场解释的内容。指标必须能说明测试条件。

### 6. 准备面试追问

每个项目点都准备：

- 目标是什么；
- 方案有哪些；
- 为什么选当前方案；
- 失败过什么；
- 如何测试；
- 如何量化结果；
- 如果规模扩大，哪里会成为瓶颈。

重点问题：

1. 为什么使用 Dedicated Server？
2. RPC 和 Replicated Property 如何选择？
3. 客户端预测失败如何处理？
4. 如何防止客户端伪造伤害和奖励？
5. 如何实现重连而不重复创建角色？
6. 数据库写成功但响应丢失怎么办？
7. 如何防止同一结算发奖两次？
8. 如何定位 Server Tick 下降？
9. 为什么背包不能只放在客户端？
10. 如果增加更多服务器和多个区域，当前架构怎么扩展？

## 实现原理

作品集的核心是降低面试官验证你能力的成本。可运行包证明你做过，架构图证明你理解，故障实验证明你不是只做 Happy Path，性能报告证明你会测量，取舍说明证明你能承担工程责任。

“懂游戏 + 懂前台 + 懂后台”不等于把所有系统都写一遍，而是能从玩家体验出发，在客户端、实时服务器和业务后台之间做正确的边界划分。

## 常见问题

### README 写得很长但没人能跑起来

先写一条最短路径：安装依赖、启动后台、启动服务器、启动两个客户端、进入对局。完整文档可以放在 `docs`，README 只保留入口。

### 展示了很多功能却没有深入点

把“网络权威 + 幂等结算 + 重连恢复”作为三条主线，其他功能为它们服务。少展示十种枪，多展示一次故障如何被正确处理。

### 把 Lyra 的能力全部说成自己实现

明确说明哪些来自 Lyra，哪些是你扩展的。面试官更看重你能否理解并正确扩展成熟框架。

## 本周作品集产出

- 最终 README；
- 3 分钟视频；
- 架构图和三张时序图；
- 性能、重连、幂等测试报告；
- 简历项目描述；
- 5 分钟口述稿；
- 10 个问题的模拟面试录音。

## 参考资料

- [Lyra Sample Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)
- [Unreal Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-in-unreal-engine)
- [Multiplayer Programming](https://dev.epicgames.com/documentation/en-us/unreal-engine/multiplayer-programming-quick-start-for-unreal-engine)
- [Game Features](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-features-in-unreal-engine)

