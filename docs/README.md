# Extraction Ops 学习文档

这是基于 Lyra Starter Game 的 UE5 多人撤离射击学习与实践文档。项目采用 20 周 Vertical Slice 蓝图；原 12 周文档保留为按技术主题查阅的实施手册。

## 项目定位

本项目基于当前目录中的 Lyra Starter Game（Unreal Engine 5.8），目标是做出一个可运行、可讲解、可展示的第三人称 1–2 人合作 PvE Vertical Slice。信号终端用更高 Threat 换取物资与撤离情报，构成“继续扫描还是立即撤离”的核心决策。

最终作品不追求内容规模，而追求一条完整且可信的链路：

~~~text
登录 → 大厅 → 加入对局 → 角色与武器 → 搜刮物资 → 背包 → 撤离/死亡
  → 服务端结算 → 业务后台持久化 → 返回大厅
~~~

项目的个人定位是：

> 以游戏后台和分布式系统为纵深，补齐 UE 客户端、多人网络和游戏玩法，使用 AI 工具提升研发效率。

## 当前工程事实

- 工程文件：`LyraStarterGame.uproject`
- 引擎版本：`5.8`
- C++ 模块：`LyraGame`、`LyraEditor`
- 已启用的重要能力：`GameplayAbilities`、`ReplicationGraph`、`CommonUI`、`EnhancedInput`、`GameFeatures`、`OnlineServices`、`ShooterCore`、`ShooterMaps`、`ShooterTests`
- 已提供的构建目标：`LyraClient`、`LyraServer`、`LyraEditor` 及 Steam/EOS 变体
- 主要代码位置：`Source/LyraGame`

## 使用方式

本路线按 UE 初学者、每周 15–20 小时编写。12 周是目标节奏，不是必须压缩完成的截止日期；某周关键验收未通过时，应顺延当前周，不能删掉测试或带着不一致状态进入下一周。

路径约定：

- `<RepoRoot>`：仓库根目录，当前示例为 `D:\Document\AI\Codex\extraction-ops`；
- `<UE_ROOT>`：Unreal Engine 5.8 安装目录；
- `<LyraSource>`：从 Fab/Epic Games Launcher 创建的同版本 Lyra 原始工程；
- 文档中标注为“计划新增”的路径，需要在对应周实施时创建，不代表当前仓库已经存在。

每周按下面顺序执行：

1. 检查“前置条件与周门槛”，确认上周关键链路已经通过；
2. 阅读“先读什么”，沿项目已有调用链定位扩展点；
3. 按时间预算依次完成五个工作单元，不并行堆叠未验证功能；
4. 每完成一个单元立即检查成功信号，并记录第一条真实错误；
5. 用“验收目标”逐条复测正常、非法和网络/服务异常路径；
6. 保存日志、截图、录屏、架构图和本周学习记录；
7. 所有关键门槛通过后才进入下一周，可选美化不得阻塞核心闭环。

## 统一工作规则

1. 每周开始前创建 `week-XX` 分支或标签，并记录本周基线。
2. 新玩法默认进入 `Plugins/GameFeatures/ExtractionOps`，减少直接改动 Lyra 核心代码。
3. 优先编写 C++ 核心逻辑；Blueprint 负责资产配置、表现、组合和快速迭代。
4. UI 只发出意图并消费状态，不直接修改生命、背包、撤离或结算。
5. Dedicated Server 负责实时战局权威，Go Backend 负责身份、房间、服务器注册、票据、永久数据和结算。
6. 每个功能同时记录客户端、游戏服务器、业务后台的职责和失败行为。
7. 跨进程统一使用 `request_id`、`player_id`、`room_id`、`match_id`、`run_id`、`server_instance_id` 和 `idempotency_key`。
8. Backend 使用 Go、HTTP/JSON 和 SQLite；SQLite 启用外键、WAL、busy timeout、transaction 和唯一约束。
9. 每周至少保留一个可运行版本和一个屏幕录制片段，不以“代码写完”为验收，而以“能复现、能解释、能演示”为验收。
10. 遇到文档与当前 UE 5.8 API/资产名不一致时，先以项目源码和实际资产为准，并在学习记录中写明差异。

## 执行路线

范围、20 周阶段门槛、技术证据链和量化验收以 [Vertical Slice 执行蓝图](vertical-slice-blueprint.md) 为准。以下 12 篇文档是技术专题索引，不代表必须逐周完成的交付排期。

已经验证的实现与当前硬门槛见 [实施状态](implementation-status.md)。

### 九条实现内容的周落点

| 实现内容 | 主工作周 | 进入条件 | 本周结束时应看到什么 |
| --- | --- | --- | --- |
| 可编译、可复制、可测试的撤离底座 | 第 2 周 | Editor 构建可用 | GameFeature 自动注入 Match/Run，规则测试通过 |
| 专属 Extraction Experience | 第 2 周 | 插件边界明确 | 加载 Experience 自动激活 ExtractionOps |
| 两把武器与枪感 | 第 3 周 | Experience 可玩 | 两把武器定位不同，5 分钟战斗无状态卡死 |
| Dedicated Server 双客户端 | 第 1、4、9 周 | 支持 Server Target 的源码引擎 | 先直连验证权威，再接房间/Ticket 生命周期 |
| GAS Armor 与交互能力接线 | 第 5 周 | ASC/输入链可解释 | ArmorSet 正确复制；玩家 Ability 经服务器校验启动终端，不能重复激活 |
| 战利品与背包 | 第 6 周 | GAS 战斗终态稳定 | 12 格背包、8 种战利品、物品实例守恒 |
| 可玩灰盒地图 | 第 7 周 | 枪战/背包可用 | 从 5 分钟玩法房扩到 12–15 分钟地图合同 |
| Threat 驱动 AI | 第 7 周 | 终端/Threat 状态可复制 | 三档 Threat 改变 AI 调度并影响玩家决策 |
| Go 后台与幂等结算 | 第 8–10 周 | 核心玩法门槛通过 | 双人房间、Ticket、仓库、90 秒重连、结算不重复 |

第 11 周统一做自动化、故障和性能证据，第 12 周完成外测与作品集包装。

## 技术专题索引

| 周次 | 主题 | 关键产出 |
| --- | --- | --- |
| 1 | 环境基线与可重复构建 | 能打开工程、编译 Client/Server、建立 Git 基线 |
| 2 | Lyra 架构阅读与工程边界 | 架构图、模块阅读笔记、ExtractionOps 功能插件 |
| 3 | 客户端输入、角色、武器与 HUD | 一套可玩的客户端战斗回路 |
| 4 | 多人联机与复制验证 | Dedicated Server + 两客户端 + 网络实验记录 |
| 5 | GAS 战斗能力与数据驱动 | 射击、换弹、Armor/Health、交互、治疗与死亡的 GAS 化实现 |
| 6 | 物品、背包与客户端状态 | 拾取、拖拽/移动、装备和 UI 状态同步 |
| 7 | 地图交互、搜刮与撤离 | 一局游戏可完成搜刮、撤离或死亡 |
| 8 | Go 后台接入 | 登录、资料、库存、对局入口和错误处理 |
| 9 | 专用服务器与对局会话 | 服务器启动、加入、关闭和基本匹配流程 |
| 10 | 结算、持久化、重连与幂等 | 奖励不重复、断线可恢复、失败可补偿 |
| 11 | 测试、性能和可观测性 | 自动化冒烟、网络压测、Unreal Insights 报告 |
| 12 | 打磨作品集与面试 | 演示视频、架构图、技术文章、面试讲解稿 |

- [第 1 周：环境基线与可重复构建](week-01-environment-baseline.md)
- [第 2 周：Lyra 架构阅读与工程边界](week-02-lyra-architecture.md)
- [第 3 周：客户端输入、角色、武器与 HUD](week-03-client-gameplay.md)
- [第 4 周：多人联机与复制验证](week-04-multiplayer-replication.md)
- [第 5 周：GAS 战斗能力与数据驱动](week-05-gas-combat.md)
- [第 6 周：物品、背包与客户端状态](week-06-items-inventory-ui.md)
- [第 7 周：地图交互、搜刮与撤离循环](week-07-loot-extraction-loop.md)
- [第 8 周：Go 后台接入与对局控制面](week-08-go-backend-integration.md)
- [第 9 周：Dedicated Server、房间和对局会话](week-09-dedicated-server-sessions.md)
- [第 10 周：断线重连、结算幂等与数据一致性](week-10-reconnect-idempotency.md)
- [第 11 周：测试、性能与可观测性](week-11-testing-performance-observability.md)
- [第 12 周：作品集包装与面试验收](week-12-portfolio-interview.md)
- [参考资料库](references.md)
- [专题：Unreal MCP、UE 与游戏客户端开发](extra/unreal-mcp-ue-client-development.md)

## 最终验收

最终验收不以系统数量判断，而以外部测试者是否理解并真实经历风险选择、双客户端 Dedicated Server 是否稳定一致，以及是否具备可复核的网络/性能证据为准。量化门槛见 [Vertical Slice 执行蓝图](vertical-slice-blueprint.md#最终验收)。

## 当前工程基线

- 引擎版本：UE 5.8
- 项目文件：[LyraStarterGame.uproject](../LyraStarterGame.uproject)
- C++ 代码：[Source/LyraGame](../Source/LyraGame)
- Server Target：[Source/LyraServer.Target.cs](../Source/LyraServer.Target.cs)
- 关键插件：GameplayAbilities、ReplicationGraph、CommonUI、EnhancedInput、GameFeatures、OnlineServices、ShooterCore、ShooterMaps、ShooterTests

## 项目边界

这套指南面向个人作品集、转岗学习和可上架原型，不把 v1 假设成生产级在线游戏。不做开放世界、多区域自动扩容、完整反作弊、支付系统和大规模经济系统。完成一条可信且好玩的多人闭环，比拥有大量未完成玩法更重要。
