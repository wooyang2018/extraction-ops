# Extraction Ops 学习文档

这是一套基于当前 Lyra Starter Game 工程的 12 周 UE5 多人撤离射击学习与实践指南。

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

## 跨周实现约定

- 新玩法默认进入 `Plugins/ExtractionOps`，优先复用 Lyra 的 Experience、GAS、Inventory、Interaction、UI 和网络能力；
- Blueprint 负责资产配置、UI 和表现，C++ 负责权威状态、规则、校验和服务访问；
- UI 只发出意图并消费状态，不直接修改生命、背包、撤离或结算；
- Dedicated Server 负责实时战局权威，Go Backend 负责身份、房间、服务器注册、票据、永久数据和结算；
- 跨进程统一使用 `request_id`、`player_id`、`room_id`、`match_id`、`run_id`、`server_instance_id` 和 `idempotency_key`；
- Backend 使用 Go、HTTP/JSON 和 SQLite；SQLite 启用外键、WAL、busy timeout、transaction 和唯一约束；
- 遇到文档与当前 UE 5.8 API/资产名不一致时，先以项目源码和实际资产为准，并在学习记录中写明差异，不盲目套用其他引擎版本教程。

## 文档索引

- [项目总览与统一工作规则](00-roadmap.md)
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

## 当前工程基线

- 引擎版本：UE 5.8
- 项目文件：[LyraStarterGame.uproject](../LyraStarterGame.uproject)
- C++ 代码：[Source/LyraGame](../Source/LyraGame)
- Server Target：[Source/LyraServer.Target.cs](../Source/LyraServer.Target.cs)
- 关键插件：GameplayAbilities、ReplicationGraph、CommonUI、EnhancedInput、GameFeatures、OnlineServices、ShooterCore、ShooterMaps、ShooterTests

## 项目边界

这套指南面向个人作品集和学习实践，不把项目假设成生产级商业游戏。12 周内不做开放世界、多区域自动扩容、完整反作弊、支付系统和大规模经济系统。完成一条可信的多人闭环，比拥有大量未完成玩法更重要。
