# 参考资料库

## 使用原则

资料优先级：Epic 官方文档和官方样例 > EOS、PlayFab、Agones 等官方文档 > 高质量开源项目和专业文章。

当前工程的 `LyraStarterGame.uproject` 使用 UE `5.8`。如果本机实际使用 UE 5.5、5.6 或 5.7，应在 Epic 文档页面切换版本，并优先阅读 Lyra 升级文档；不要把不同小版本的配置和 API 直接混用。

## 推荐阅读顺序

```text
Gameplay Framework
  -> Lyra Tour / Lyra Sample
  -> Enhanced Input / CommonUI
  -> Lyra Inventory and Equipment
  -> GAS
  -> Replication / RPC / Ownership
  -> Dedicated Server
  -> Online Services / EOS
  -> Unreal Insights / Networking Insights
  -> 后台匹配、库存和服务器编排
```

## UE 基础

- [Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine)：理解 GameMode、GameState、PlayerState、PlayerController、Pawn 的职责边界。
- [Gameplay Framework Quick Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-quick-reference-in-unreal-engine)：需要快速查生命周期和对象职责时使用。
- [Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)：处理射击、瞄准、冲刺、交互、按键重绑定和输入上下文。
- [Common UI Plugin](https://dev.epicgames.com/documentation/en-us/unreal-engine/common-ui-plugin-for-advanced-user-interfaces)：学习跨平台 UI、输入路由和界面层级。
- [Game Features and Modular Gameplay](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-features-and-modular-gameplay-in-unreal-engine)：学习把玩法拆成可启停 Feature Plugin。
- [Data Assets](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-assets-in-unreal-engine)：为武器、物品、任务和撤离点配置建立数据驱动模型。
- [Asset Management](https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-management-in-unreal-engine)：理解 Primary Asset、异步加载和 Cook 后资源管理。
- [World Partition](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine)：只有在地图规模确实需要时再读，不要在 MVP 阶段提前引入复杂性。
- [Epic Sample Game Projects](https://dev.epicgames.com/documentation/en-us/unreal-engine/sample-game-projects-for-unreal-engine)：官方样例总入口。

## Lyra

- [Lyra Sample Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine?lang=en-US)：整体架构、模块化、多人 FPS、GAS、UI、武器和在线服务。
- [A Tour of Lyra](https://dev.epicgames.com/documentation/en-us/unreal-engine/tour-of-lyra-in-unreal-engine)：理解前端、Session、游戏模式、设置、地图和网络统计。
- [Abilities in Lyra](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)：学习 HeroComponent、Pawn 扩展、武器能力、目标数据、预测和服务端校验。
- [Lyra Inventory and Equipment](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-inventory-and-equipment-in-unreal-engine)：最贴近撤离射击背包、装备、武器、弹药和物品碎片系统。
- [Common User Plugin for Lyra](https://dev.epicgames.com/documentation/en-us/unreal-engine/common-user-plugin-in-unreal-engine-for-lyra-sample-game?lang=en-US)：学习登录、权限、Session 创建和 OSS 抽象。
- [Upgrading Lyra](https://dev.epicgames.com/documentation/en-us/unreal-engine/upgrading-the-lyra-starter-game-to-the-latest-engine-release-in-unreal-engine)：解决 Lyra 与引擎版本升级后的配置和插件差异。

## GAS

- [Gameplay Ability System](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine)：官方总览。
- [Understanding GAS](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-the-unreal-engine-gameplay-ability-system)：理解 Ability、Attribute、Gameplay Effect、Gameplay Cue、Tag 和预测。
- [Using Gameplay Abilities](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-abilities-in-unreal-engine)：查 Ability 激活、取消、任务和输入。
- [Gameplay Attributes and Attribute Sets](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-attributes-and-attribute-sets-for-the-gameplay-ability-system-in-unreal-engine)：实现生命、护甲、资源和状态属性。
- [Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine)：设计武器状态、伤害类型和行为阻断。
- [tranek/GASDocumentation](https://github.com/tranek/GASDocumentation)：社区高质量 GAS 文档，适合补充初始化、预测、复制和调试细节。

## 多人网络

- [Networking Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-overview-for-unreal-engine)：Authority、Replication、RPC、Ownership、Dormancy 和网络设计。
- [Multiplayer Programming Quick Start](https://dev.epicgames.com/documentation/en-us/unreal-engine/multiplayer-programming-quick-start-for-unreal-engine)：从最小联机程序入门。
- [Replicate Actor Properties](https://dev.epicgames.com/documentation/en-us/unreal-engine/replicate-actor-properties-in-unreal-engine)：复制属性的声明、条件和使用边界。
- [Remote Procedure Calls](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-procedure-calls-in-unreal-engine)：RPC 方向、可靠性和 Ownership。
- [Detailed Actor Replication Flow](https://dev.epicgames.com/documentation/en-us/unreal-engine/detailed-actor-replication-flow-in-unreal-engine)：深入理解服务器如何为连接复制 Actor。
- [Replication Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/replication-graph-in-unreal-engine)：多人场景下按空间和相关性控制复制。
- [Iris Replication System](https://dev.epicgames.com/documentation/en-us/unreal-engine/iris-replication-system-in-unreal-engine?lang=en-US)：了解新一代复制系统，先测量再决定是否使用。
- [Testing and Debugging Networked Games](https://dev.epicgames.com/documentation/en-us/unreal-engine/testing-and-debugging-networked-games-in-unreal-engine)：延迟、丢包、多人测试和 Gauntlet。
- [Console Commands for Network Debugging](https://dev.epicgames.com/documentation/en-us/unreal-engine/console-commands-for-network-debugging-in-unreal-engine)：网络实验和问题定位。
- [Gabriel Gambetta: Client-Server Game Architecture](https://www.gabrielgambetta.com/client-server-game-architecture.html)：理解客户端预测、服务器校正和插值。
- [Gaffer on Games: What Every Programmer Needs to Know About Game Networking](https://gafferongames.com/post/what_every_programmer_needs_to_know_about_game_networking/)：补充 Tick、快照和网络带宽思维。

## Dedicated Server 与在线服务

- [Setting Up Dedicated Servers](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers-in-unreal-engine)：构建、Cook、启动和测试 Dedicated Server。
- [Packaging Unreal Engine Projects](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-unreal-engine-projects)：打包客户端和服务端。
- [Command-Line Arguments](https://dev.epicgames.com/documentation/en-us/unreal-engine/command-line-arguments-in-unreal-engine)：统一地图、端口、版本和日志启动参数。
- [Online Services Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-online-services-in-unreal-engine)：理解旧 Online Subsystem 与新 Online Services 的关系。
- [Enable and Configure Online Services EOS](https://dev.epicgames.com/documentation/en-us/unreal-engine/enable-and-configure-online-services-eos-in-unreal-engine)：EOS 产品、Sandbox、Deployment、Client 和 Artifact。
- [Online Subsystem EOS Plugin](https://dev.epicgames.com/documentation/en-us/unreal-engine/online-subsystem-eos-plugin-in-unreal-engine)：适合现有 Lyra/OSSv1 工作流。
- [Using Lyra with EOS](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-lyra-with-epic-online-services-in-unreal-engine)：登录、Session 和 Lyra 联机流程。
- [Replay System](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-replay-system-in-unreal-engine)：战局复盘、QA 重现和作弊调查的扩展方向。

## 性能和可观测性

- [Performance Profiling Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine)：建立 CPU、GPU、内存、Tick、渲染和网络分析流程。
- [Unreal Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-in-unreal-engine)：Game Thread、Render Thread、Task Graph 和长帧分析。
- [Networking Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-insights-in-unreal-engine?lang=en-US)：定位 Actor、Property、RPC、NetGUID 和 Packet 的带宽成本。
- [Network Profiler](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-network-profiler-in-unreal-engine?lang=en-US)：快速查看高带宽 Actor、RPC 和属性。
- [Gameplay Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-insights-in-unreal-engine)：将 Gameplay 事件和性能分析联系起来。
- [Gauntlet Testing](https://dev.epicgames.com/documentation/en-us/unreal-engine/testing-and-debugging-networked-games-in-unreal-engine)：自动启动服务器、多客户端和异常网络测试。

## 游戏后台和服务器编排

以下资料用于理解生产系统方向，12 周 MVP 不需要全部接入：

- [EOS Developer Documentation](https://dev.epicgames.com/docs/epic-online-services)：身份、好友、Session、Stats、Presence 和跨平台能力。
- [PlayFab Multiplayer](https://learn.microsoft.com/en-us/xbox/playfab/multiplayer/)：匹配、Lobby、Party、QoS 和托管服务器。
- [PlayFab Player Data](https://learn.microsoft.com/en-us/xbox/playfab/player-progression/player-data/)：玩家数据权限和服务端/客户端边界。
- [PlayFab Economy Inventory](https://learn.microsoft.com/en-us/gaming/playfab/economy-monetization/economy-v2/inventory/quickstart)：永久仓库、物品、货币和流水。
- [Open Match](https://open-match.dev/site/docs/)：研究可自定义匹配规则和队伍组装。
- [Agones](https://www.agones.dev/site/docs/)：研究 Kubernetes 上的游戏服务器 Fleet、分配、健康检查和回收。
- [Amazon GameLift Servers Unreal Plugin](https://docs.aws.amazon.com/gameliftservers/latest/developerguide/unreal-plugin.html)：研究服务器注册、Game Session 生命周期和云端部署。
- [Nakama](https://heroiclabs.com/docs/nakama/)：研究自托管账号、存储、社交、排行榜和匹配。

## 后台边界建议

- UE Dedicated Server：实时战局、射击判定、掉落、撤离和对局权威状态。
- Go Backend：账号、永久仓库、货币、任务、结算、审计和跨战局进度。
- EOS：身份、社交、Session 和跨平台接入层。
- GameLift/Agones：服务器实例生命周期。
- Open Match/PlayFab：匹配和服务器分配。

## 资料使用记录模板

每读一份资料，记录：

```text
资料：
适用周次：
它解决的问题：
我在项目中对应的模块：
我验证过的结论：
仍然不清楚的问题：
```

不要只收藏链接。每份资料至少要对应一个代码搜索、一个实验或一个设计决定。

