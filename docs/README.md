# Extraction Ops 学习文档

这是一套基于当前 Lyra Starter Game 工程的 12 周 UE5 多人撤离射击学习与实践指南。

## 使用方式

每周按下面顺序执行：

1. 先读本周的“学习目标”和“实现原理”；
2. 按“操作步骤”完成最小可运行功能；
3. 用“验收目标”逐条检查；
4. 录制本周作品集片段；
5. 把失败原因、取舍和下一周风险写进自己的学习日志；
6. 通过后再扩展功能，不要跳过验收直接堆内容。

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

