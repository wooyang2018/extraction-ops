# Lyra 启动与架构主链：源码采集报告

## 研究元数据

- 研究日期：2026-08-13
- 研究角色：deep-study 源码采集角色；本文件是结构化证据，不是 `docs/learn` 最终教学文档。
- 研究范围：当前仓库的 `Source/` 与非 ExtractionOps 的 `Plugins/`；为解释直接基类，补读 `D:/Software/UE_5.8/Engine/Source` 与 UE 5.8 内置 `ModularGameplay`、`GameFeatures` 插件源码。
- 排除：`Plugins/GameFeatures/ExtractionOps/`、Content 资产的内部实现，以及跨 UE 版本历史比较。
- 来源口径：本轮全部结论来自本地当前版本源码，均为 T0。不同文件/模块被视为独立源码证据组。
- 置信度口径：H = 源码直接表达且有至少两组 T0 证据交叉；M = 当前源码单点事实或存在资产/配置不可见环节；L = 推断或 TODO 暗示，未作为核心事实。

## 一页架构结论

Lyra 的核心不是“一个很大的 GameMode”，而是五层职责叠加：

1. **构建装配层**：Target 决定产物类型和编入模块；`LyraGame` 模块负责注册主游戏模块。它们不负责某一局游戏的运行时逻辑。
2. **进程/会话层**：`ULyraGameEngine` 目前仅透传 `UGameEngine::Init`；`ULyraGameInstance` 跨地图存在，初始化 GameInstance 子系统并注册 Lyra 的组件初始化状态链。
3. **世界权威层**：服务端 `ALyraGameMode` 选择 Experience，`ALyraGameState` 作为所有端可见的全局复制锚点，持有 ExperienceManager 与全局 ASC。
4. **玩法组合层**：Experience 是 Primary Data Asset，组合 PawnData、GameFeature 插件、Actions 与 ActionSets；ExperienceManager 按状态机加载、激活并广播就绪。
5. **实体扩展层**：ModularGameplay 的 receiver/request/event/init-state 四种机制，使已存在与未来生成的 Actor 都能被 GameFeature 注入组件；Pawn 的初始化不依赖单一回调顺序，而由多个可重复触发的门槛共同推进。

关键主链可概括为：

```text
Target -> LyraGame module -> UGameEngine::Init
  -> 创建/初始化 ULyraGameInstance
  -> 注册 Lyra InitState 链
  -> StartGameInstance/Browse/LoadMap
  -> 服务端 UWorld::SetGameMode -> ALyraGameMode
  -> GameMode 创建 ALyraGameState
  -> GameMode 选择 ExperienceId
  -> GameState.ExperienceManager 复制 CurrentExperience
  -> 两端各自异步加载资产与 GameFeatures
  -> 执行 Experience Actions -> Loaded 广播
  -> PlayerState 获得 PawnData/Abilities
  -> GameMode 解锁 RestartPlayer，延迟构造 Pawn 并先注入 PawnData
  -> Controller/PlayerState/Input 复制或建立后，Pawn features 推进至 GameplayReady
```

---

## 维度一：Target、Module、Engine、GameInstance、GameMode 启动链与边界

### 发现 1.1：Target 与 Module 是构建/加载边界，不是玩法生命周期对象

- **内容**：`LyraGameTarget` 将产物声明为 `TargetType.Game`，把 `LyraGame` 加入 `ExtraModuleNames`，并集中应用共享构建设置；Editor Target 同时编入 `LyraGame` 与 `LyraEditor`。`LyraGame.Build.cs` 显式依赖 `ModularGameplay`、`ModularGameplayActors`、`GameFeatures` 等模块。`IMPLEMENT_PRIMARY_GAME_MODULE` 只注册主模块，当前 `StartupModule/ShutdownModule` 为空。因此不要把 Target、Module、GameInstance 混成同一层：前两者解决“如何构建和加载代码”，后者才是运行时实例。
- **来源**：本地仓库 `Source/LyraGame.Target.cs`、`Source/LyraEditor.Target.cs`、`Source/LyraGame/LyraGame.Build.cs`、`Source/LyraGame/LyraGameModule.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame.Target.cs:11-19` — Game Target 与 `LyraGame` 模块装配。
  - `Source/LyraEditor.Target.cs:6-20` — Editor Target 额外装配 `LyraEditor`。
  - `Source/LyraGame/LyraGame.Build.cs:22-46` — 运行时公共模块依赖包含 ModularGameplay/GameFeatures。
  - `Source/LyraGame/LyraGameModule.cpp:9-20` — 空模块生命周期与主模块注册。
- **备注**：Target 中 `ConfigureGameFeaturePlugins` 决定哪些 GameFeature 插件会参与构建，但“编入/可用”不等于运行时已激活；运行时激活由 Experience 链完成。

### 发现 1.2：Lyra 自定义 Engine 目前没有新增启动语义，真正的进程启动工作仍在 `UGameEngine`

- **内容**：`ULyraGameEngine::Init` 只调用 `Super::Init`。UE 的 `UGameEngine::Init` 从 `UGameMapsSettings::GameInstanceClass` 加载类、创建唯一 standalone GameInstance 并调用 `InitializeStandalone`；`UGameEngine::Start` 再调用 `StartGameInstance`。因此 LyraGameEngine 是可扩展钩子/类型选择点，目前不能夸大为自定义启动框架。
- **来源**：`Source/LyraGame/System/LyraGameEngine.cpp`；UE 5.8 `Engine/Source/Runtime/Engine/Private/GameEngine.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/System/LyraGameEngine.cpp:15-18` — Lyra override 仅透传。
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Private/GameEngine.cpp:1221-1267` — Engine 初始化并创建配置指定的 GameInstance。
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Private/GameEngine.cpp:1333-1339` — Engine Start 调用 `GameInstance->StartGameInstance()`。
- **备注**：项目配置边界证据是 `Config/DefaultEngine.ini:27` 的 `GameEngine=/Script/LyraGame.LyraGameEngine`。Config 不在最终学习源码范围，但没有它就无法解释自定义类如何被选中，应在教学文档中作为“外部装配点”侧栏引用，而非纳入源码覆盖计数。

### 发现 1.3：GameInstance 是跨地图服务容器；Lyra 在这里注册可插拔组件的统一初始化词汇

- **内容**：UE 的 `UGameInstance::Init` 最后初始化 `SubsystemCollection`。Lyra 先调用 Super，再取得 `UGameFrameworkComponentManager`，依次注册 `Spawned -> DataAvailable -> DataInitialized -> GameplayReady`。这解释了为什么 Pawn 组件可以在异步复制、动态注入、控制器建立等事件顺序不固定时仍共享一致的初始化协议。
- **来源**：`Source/LyraGame/System/LyraGameInstance.cpp`；UE 5.8 `GameInstance.cpp`；UE 5.8 ModularGameplay header
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Private/GameInstance.cpp:97-130` — 基类初始化 GameInstance subsystems。
  - `Source/LyraGame/System/LyraGameInstance.cpp:88-101` — Lyra 注册四阶段 InitState 链。
  - `D:/Software/UE_5.8/Engine/Plugins/Runtime/ModularGameplay/Source/ModularGameplay/Public/Components/GameFrameworkComponentManager.h:89-102` — Manager 是 `UGameInstanceSubsystem`。
- **备注**：Lyra 必须在 `Super::Init()` 之后取 subsystem；当前源码确实如此。

### 发现 1.4：初始地图与服务端 GameMode 的创建属于 Engine/World；Experience 选择属于 LyraGameMode

- **内容**：`StartGameInstance` 解析默认地图并 Browse。服务端/Standalone 的 `UWorld::SetGameMode` 通过 GameInstance 为 URL 创建 GameMode，纯客户端不会创建 AuthorityGameMode。`UWorld::InitializeActorsForPlay` 调用 GameMode `InitGame`，Actor 初始化过程中 `AGameModeBase::PreInitializeComponents` 生成 GameState 并调用 `InitGameState`。LyraGameMode 随后把 Experience 选择延迟到下一 tick，避免把启动设置时序硬编码进构造阶段。
- **来源**：UE 5.8 `GameInstance.cpp`、`World.cpp`、`GameModeBase.cpp`；`LyraGameMode.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Private/GameInstance.cpp:626-688` — 默认地图与 Browse。
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Private/World.cpp:5927-5934` — 非客户端才创建 AuthorityGameMode。
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Private/World.cpp:6031-6043` — `InitGame` 后路由 Actor 初始化。
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Private/GameModeBase.cpp:116-143` — GameMode 创建 GameState 并执行 `InitGameState`。
  - `Source/LyraGame/GameModes/LyraGameMode.cpp:80-85` — Lyra 下一 tick 分配 Experience。
- **备注**：当前配置选择 `B_LyraGameMode`、`B_LyraGameInstance` 和前端地图（`Config/DefaultEngine.ini:71-73`），这些 Blueprint 的资产内部不在本轮源码研究范围。

### 启动阶段职责矩阵

| 层级 | 生命周期/可见性 | 主要职责 | 明确不负责 |
|---|---|---|---|
| TargetRules | 构建时 | 产物类型、模块、插件构建策略、编译宏 | 地图内玩法与复制 |
| Module | 进程模块加载期 | 注册模块、可选全局启动/关闭钩子 | 玩家与比赛状态 |
| GameEngine | 进程级 | Engine 初始化、创建 GameInstance、启动 Browse | 具体 Experience/Pawn 配置 |
| GameInstance | 进程/应用会话级，跨地图 | 子系统容器、用户/Session、Lyra InitState 词汇 | 某一 World 的权威比赛规则 |
| World | 地图实例级 | Actor 生命周期、GameMode/GameState 创建与 BeginPlay | 跨地图持久用户设置 |
| GameMode | 每个权威 World；客户端无实例 | 规则、登录、Experience 选择、出生 | 向所有客户端复制全局状态 |
| GameState | 每个 World，服务端生成并复制 | 全局可见状态、Experience 锚点、全局 ASC | 服务器专属规则裁决 |

---

## 维度二：Experience 选择、异步加载、GameFeature 激活与玩家初始化

### 发现 2.1：Experience 是组合根，不是 GameMode 的子类替代品

- **内容**：`ULyraExperienceDefinition` 是 const PrimaryDataAsset，组合启用的 GameFeature 名、默认 PawnData、Actions 与 ActionSets；ActionSet 又能组合额外 actions/features。GameMode 保持稳定，具体玩法通过数据与插件组合进入同一架构壳。
- **来源**：`LyraExperienceDefinition.h`、`LyraExperienceActionSet.h`、`LyraPawnData.h`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/GameModes/LyraExperienceDefinition.h:15-51` — Experience 四类组合数据。
  - `Source/LyraGame/GameModes/LyraExperienceActionSet.h:14-40` — ActionSet 的 actions/features。
  - `Source/LyraGame/Character/LyraPawnData.h:19-53` — PawnClass、AbilitySets、输入与相机的 Pawn 配方。
- **备注**：`LyraExperienceDefinition.cpp:54` 的校验信息明确建议用 ActionSets 组合，而非 Blueprint Experience 的多层继承，体现“组合优先”。

### 发现 2.2：ExperienceId 的选择是权威端策略，优先级明确但注释中尚有未实现入口

- **内容**：GameMode 的当前代码按 URL `?Experience=`、PIE DeveloperSettings、命令行、WorldSettings、Dedicated Server host、硬编码默认值依次尝试；然后验证 PrimaryAsset 是否存在，并把最终 ID 交给 GameState 上的 ExperienceManager。注释把 Matchmaking assignment 列为最高优先级，但当前函数里没有实际读取该值。
- **来源**：`LyraGameMode.cpp`、`LyraWorldSettings.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H（实际代码）；M（注释所述 matchmaking 入口）
- **代码证据**：
  - `Source/LyraGame/GameModes/LyraGameMode.cpp:88-164` — 选择优先级、资产验证与默认 fallback。
  - `Source/LyraGame/GameModes/LyraGameMode.cpp:289-297` — 写入 ExperienceManager。
  - `Source/LyraGame/GameModes/LyraWorldSettings.cpp:18-30` — SoftClassPath 转 PrimaryAssetId。
- **备注**：教学材料必须区分“注释宣称的设计顺序”和“当前已实现分支”。

### 发现 2.3：ExperienceManager 是复制锚点 + 每端本地加载状态机

- **内容**：组件默认复制，服务端把 `CurrentExperience` 设为 Experience CDO；客户端收到复制后由 `OnRep_CurrentExperience` 启动同一加载过程。状态机为 `Unloaded -> Loading -> LoadingGameFeatures -> [LoadingChaosTestingDelay] -> ExecutingActions -> Loaded -> Deactivating -> Unloaded`。换言之，复制的是“选择了哪个 Experience”，不是把服务端已加载对象图直接复制给客户端；各端按自己的 client/server bundle 与 net mode 本地加载、激活。
- **来源**：`LyraExperienceManagerComponent.h/.cpp`、`LyraGameState.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/GameModes/LyraExperienceManagerComponent.h:18-30` — 完整状态枚举与组件基类。
  - `Source/LyraGame/GameModes/LyraExperienceManagerComponent.h:82-88` — `CurrentExperience` RepNotify 与本地状态字段。
  - `Source/LyraGame/GameModes/LyraExperienceManagerComponent.cpp:50-67` — 服务端设值并加载。
  - `Source/LyraGame/GameModes/LyraExperienceManagerComponent.cpp:118-169` — 客户端 OnRep 与按 net mode 选 bundle。
  - `Source/LyraGame/GameModes/LyraExperienceManagerComponent.cpp:373-377` — `CurrentExperience` 注册复制。
  - `Source/LyraGame/GameModes/LyraGameState.cpp:22-33` — Manager 固定挂在 GameState。
- **备注**：`SetCurrentExperience` 的 `TryLoad()` 是同步解析 Experience 类本身；随后 bundle/action-set 资源才走 streamable async load。不要笼统说“Experience 全流程纯异步”。

### 发现 2.4：插件激活先于 Experience Actions，Actions 完成后才对外宣布 Loaded

- **内容**：资产 bundle 完成后，Manager 从 Experience 与 ActionSets 汇总去重的插件 URL，逐一调用 `LoadAndActivateGameFeaturePlugin`。全部回调后，构造带 WorldContext 限制的 `FGameFeatureActivatingContext`，依次对 Experience/ActionSet actions 调用 Registering、Loading、Activating，最后切为 Loaded，并按 High/Normal/Low 三档广播。这三档是显式初始化排序机制，不是线程优先级。
- **来源**：`LyraExperienceManagerComponent.cpp`、UE 5.8 GameFeatures API 使用点
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/GameModes/LyraExperienceManagerComponent.cpp:214-275` — 汇总 URL 与插件激活。
  - `Source/LyraGame/GameModes/LyraExperienceManagerComponent.cpp:289-345` — WorldContext 限制、action 生命周期、切 Loaded。
  - `Source/LyraGame/GameModes/LyraExperienceManagerComponent.cpp:347-354` — High/Normal/Low 顺序广播并 Clear。
- **备注**：`OnGameFeaturePluginLoadComplete` 接收 `Result` 却未检查成功与否，见不确定项 U2。

### 发现 2.5：Experience 就绪同时解锁 PlayerState 配方与 Pawn 出生，PawnData 在 FinishSpawning 前注入

- **内容**：服务端 PlayerState 在 `PostInitializeComponents` 注册普通优先级 Experience callback，加载后从 GameMode 获取 PawnData，并把 AbilitySets 授予位于 PlayerState 的 ASC。GameMode 同样注册普通优先级 callback：已连接但尚无 Pawn 的玩家会被 Restart。新登录玩家在 Experience 未加载时被 `HandleStartingNewPlayer` 挡住。真正 Spawn 时采用 deferred construction，先给 PawnExtension 设置 PawnData，再 `FinishSpawning`，避免 BeginPlay 时关键数据缺失。
- **来源**：`LyraPlayerState.cpp`、`LyraGameMode.cpp`、`LyraPawnExtensionComponent.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/Player/LyraPlayerState.cpp:167-182` — 服务端订阅 Experience。
  - `Source/LyraGame/Player/LyraPlayerState.cpp:109-121,185-213` — 确定 PawnData、授予 abilities、发送 AbilityReady。
  - `Source/LyraGame/GameModes/LyraGameMode.cpp:305-320` — 对早到玩家统一 Restart。
  - `Source/LyraGame/GameModes/LyraGameMode.cpp:345-370` — deferred spawn，先 PawnData 后 FinishSpawning。
  - `Source/LyraGame/GameModes/LyraGameMode.cpp:391-399` — Experience 未加载时阻断出生。
- **备注**：普通优先级 delegates 的添加顺序不应被当成强契约。真正对 Pawn 安全性的保证来自 PlayerState/PawnData 的权威写入、复制和 Pawn init-state 门槛。

### Experience 状态与主要副作用

| 状态 | 入口 | 核心副作用 | 离开条件 |
|---|---|---|---|
| Unloaded | 初始/卸载完成 | 无 CurrentExperience 或尚未处理 | 服务端 Set 或客户端 OnRep |
| Loading | `StartExperienceLoad` | 加载 Experience/ActionSet bundles | streamable handle 完成 |
| LoadingGameFeatures | `OnExperienceLoadComplete` | Load+Activate 所需插件 | 插件回调计数归零 |
| LoadingChaosTestingDelay | 可选测试分支 | 人工延迟验证时序鲁棒性 | timer 到期 |
| ExecutingActions | `OnExperienceFullLoadCompleted` | Register/Load/Activate actions，限定 world context | action 调用结束 |
| Loaded | actions 完成 | 广播三档就绪委托、解除 loading screen | World EndPlay |
| Deactivating | EndPlay | Action Deactivate/Unregister，插件引用计数释放 | pauser 归零 |

---

## 维度三：ModularGameplay / GameFrameworkComponentManager 的可插拔机制

### 发现 3.1：动态组件注入是“请求—接收者”双向协议，并覆盖先有 Actor 与后有 Actor

- **内容**：GameFeature Action 在适用 World 中向每个 GameInstance 的 ComponentManager 添加 `(ActorClass, ComponentClass)` 请求；已有且已初始化的目标 Actor 会被遍历并立即补组件。未来 Actor 必须在合适时机调用 AddReceiver，Manager 沿其继承链匹配请求并创建组件。RequestHandle 生命周期控制撤销，且相同 actor/component 请求被引用计数。
- **来源**：UE 5.8 `GameFeatureAction_AddComponents.cpp`、`GameFrameworkComponentManager.h/.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `D:/Software/UE_5.8/Engine/Plugins/Runtime/GameFeatures/Source/GameFeatures/Private/GameFeatureAction_AddComponents.cpp:155-183` — net-mode 过滤并创建组件请求。
  - `D:/Software/UE_5.8/Engine/Plugins/Runtime/ModularGameplay/Source/ModularGameplay/Private/Components/GameFrameworkComponentManager.cpp:251-305` — 请求引用计数与给现有 Actor 补组件。
  - 同文件 `:176-207` — 新 receiver 沿类继承链获取请求与 extension handler。
  - `.../GameFrameworkComponentManager.h:79-87,118-127` — 官方类注释与 handle 契约。
- **备注**：禁止把 request 目标设为过宽的 `AActor`，Manager 在 `:251-256` 直接拒绝，原因是性能。

### 发现 3.2：ModularGameplayActors 把 receiver 协议封装进 Actor 基类生命周期

- **内容**：`AModularCharacter/Pawn/PlayerState/GameState/PlayerController` 等在 `PreInitializeComponents` 注册 receiver，通常在 BeginPlay 或 Controller 收到 Player 后发送 `GameActorReady`，EndPlay 移除 receiver。Lyra 类型继承这些 Modular 基类，因此 GameFeature 可以安全注入组件而无需每个 Lyra 类重复协议代码。
- **来源**：项目插件 `Plugins/ModularGameplayActors`；UE 5.8 ComponentManager
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Plugins/ModularGameplayActors/Source/ModularGameplayActors/Private/ModularCharacter.cpp:8-26` — Character 完整 receiver 生命周期。
  - `Plugins/ModularGameplayActors/Source/ModularGameplayActors/Private/ModularPlayerState.cpp:10-28` — PlayerState 同型协议。
  - `Plugins/ModularGameplayActors/Source/ModularGameplayActors/Private/ModularPlayerController.cpp:10-27` — Controller 在 `ReceivedPlayer` 发送 ready。
  - `D:/Software/UE_5.8/Engine/Plugins/Runtime/ModularGameplay/Source/ModularGameplay/Private/Components/GameFrameworkComponentManager.cpp:218-248` — 移除 receiver 会销毁由 Manager 实例化的组件并发 removed event。
- **备注**：不同 Actor 的 `GameActorReady` 时点不是完全一致；插件代码应按目标 Actor 的语义响应，不应假设统一发生于 BeginPlay 之后某个固定帧。

### 发现 3.3：Extension Event 与 InitState 是两套互补机制

- **内容**：Extension Event 是类过滤的瞬时消息（ReceiverAdded、ExtensionAdded、GameActorReady、LyraAbilityReady、BindInputsNow 等）；InitState 是按 Actor/Feature 保存的可查询状态，支持依赖判断、立即回调和跨 feature 协调。前者适合“此刻执行绑定/安装”，后者适合“数据是否已满足门槛”。
- **来源**：UE 5.8 ComponentManager；Lyra PawnExtension/Hero/PlayerState
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `.../GameFrameworkComponentManager.h:130-161` — 标准扩展事件与任意事件 API。
  - `.../GameFrameworkComponentManager.cpp:448-479` — event 沿继承链同步派发。
  - `.../GameFrameworkComponentManager.cpp:787-817,1070-1121` — 保存 feature state 并通知匹配 delegate。
  - `Source/LyraGame/Player/LyraPlayerState.cpp:200-213` — PawnData/Ability 授予后发 `LyraAbilityReady`。
  - `Source/LyraGame/Character/LyraHeroComponent.cpp:300-301` — 输入准备后向 Controller 与 Pawn 发 `BindInputsNow`。
- **备注**：事件本身不缓存；若需要处理“注册时对象已就绪”，必须依赖 `AddExtensionHandler` 对现有 Actor 发 `ExtensionAdded`、显式查询状态，或使用可立即调用的 InitState/Experience delegate。

### 发现 3.4：Pawn 初始化链把网络事件乱序转化为可重试的条件判断

- **内容**：PawnExtension 与 HeroComponent 都在 OnRegister 注册 feature、BeginPlay 进入 Spawned，并在 Controller changed、PlayerState Replicated、Input setup、其他 feature state changed 时反复 `CheckDefaultInitialization`。PawnExtension 只有取得 PawnData，且 authority/local pawn 已有 Controller，才能进 DataAvailable；全部 features 到 DataAvailable 后才进 DataInitialized。Hero 还要求 PlayerState、Controller 与 PlayerState ownership 配对，本地人类玩家还需 InputComponent/LocalPlayer，并等待 PawnExtension DataInitialized 后初始化 ASC、输入、相机。
- **来源**：`LyraPawnExtensionComponent.cpp`、`LyraHeroComponent.cpp`、`LyraCharacter.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/Character/LyraPawnExtensionComponent.cpp:41-65,185-221` — 注册、事件触发点与状态链。
  - 同文件 `:224-267` — PawnData、Controller、全 feature barrier。
  - `Source/LyraGame/Character/LyraHeroComponent.cpp:76-140` — PlayerState/Controller/Input/Extension 的门槛。
  - 同文件 `:145-183` — DataInitialized 时将 PlayerState ASC 绑定为 Pawn avatar，并初始化输入/相机。
  - `Source/LyraGame/Character/LyraCharacter.cpp:212-267` — Possess、Controller Rep、PlayerState Rep、Input setup 都回推 PawnExtension。
- **备注**：这是一种“状态收敛”而非“回调串行化”。面试回答应强调幂等检查和多入口重试。

---

## 维度四：LocalPlayer、PlayerController、PlayerState、Pawn、Character 的所有权、复制与初始化依赖

### 发现 4.1：五类对象代表不同身份/寿命；Lyra 把持久战斗状态放 PlayerState，把躯体放 Pawn

- **内容**：LocalPlayer 只代表当前客户端/Listen Server 上的本地人，持有本地设置并通过 UPlayer 绑定 Controller。PlayerController 代表控制通道，在服务端为每个玩家存在、客户端只存在本地拥有者对应实例；PlayerState 向所有客户端复制，适合姓名、队伍、PawnData、ASC 等玩家级持久状态；Pawn/Character 是可更换的世界躯体。Lyra 的 ASC/attributes 位于 PlayerState，PawnExtension 把当前 Pawn 设置为 avatar，因此死亡换 Pawn 不必重建玩家级能力所有权。
- **来源**：UE 5.8 class headers；Lyra LocalPlayer/PlayerState/HeroComponent
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Classes/Engine/LocalPlayer.h:164-169` — 每个本地活跃玩家一个 LocalPlayer。
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Classes/GameFramework/PlayerController.h:250-256` — Controller 网络存在范围。
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Classes/GameFramework/PlayerState.h:38` — PlayerState 复制给所有客户端。
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Classes/GameFramework/Pawn.h:177-186` — Pawn 保存 PlayerState，因为 Controller 不向非拥有客户端复制。
  - `Source/LyraGame/Player/LyraPlayerState.h:146-153` — PawnData 与 ASC 位于 PlayerState。
  - `Source/LyraGame/Character/LyraHeroComponent.cpp:162-165` — 明确注释持久数据/ASC 在 PlayerState，并初始化 Owner=PlayerState、Avatar=Pawn。
- **备注**：PlayerState 会跨死亡，但是否跨 seamless travel/断线重连要结合 `CopyProperties`、inactive player 流程具体分析，不能泛化为永远不销毁。

### 发现 4.2：网络所有权与“队伍来源”沿 LocalPlayer/Controller/PlayerState/Pawn 链传播

- **内容**：权威端真正写队伍的是 PlayerState；Controller 监听 PlayerState 的 team changed 并把自身表现为 team agent；LocalPlayer 再监听 Controller；Character 在 Possess 时从 Controller 获取队伍并监听变化，同时把 `MyTeamID` 复制给观察端。这使只拥有 Controller 的本地系统、只看到 PlayerState 的全局系统、只看到 Pawn 的感知/伤害系统都能查询团队。
- **来源**：`LyraPlayerState.cpp`、`LyraPlayerController.cpp`、`LyraLocalPlayer.cpp`、`LyraCharacter.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/Player/LyraPlayerState.cpp:236-264` — 仅 authority 设置 team，OnRep 广播。
  - `Source/LyraGame/Player/LyraPlayerController.cpp:237-266` — Controller 解绑旧 PS、绑定新 PS 并转播 team。
  - `Source/LyraGame/Player/LyraLocalPlayer.cpp:53-80,162-169` — LocalPlayer 绑定 Controller team（符号行号由当前文件核验）。
  - `Source/LyraGame/Character/LyraCharacter.cpp:212-226,249-260,484-522` — Character 从 Controller 取 team、响应复制与 controller/playerstate 到达。
- **备注**：Pawn 的 Controller 并不对所有客户端存在，因此 Pawn 自己复制 team 是面向远端观察者的重要冗余，不应简单删除为“重复状态”。

### 发现 4.3：初始化依赖不是单一顺序，而是 Authority 与各类 Proxy 的不同门槛

- **内容**：服务端先由 Experience 给 PlayerState 设置 PawnData，随后 deferred spawn 把同一配方放入 PawnExtension。客户端可能先收到 PlayerState/ASC，后收到自己的 PlayerController；Lyra 在 Controller `OnRep_PlayerState` 显式刷新 ASC actor info 并重试 on-spawn ability。Pawn 端则在 Controller、PlayerState、Input 三类回调后重试状态链。Simulated Proxy 不要求本地 Controller/Input；Authority 和 Autonomous Proxy 要求控制关系完整。
- **来源**：`LyraGameMode.cpp`、`LyraPlayerController.cpp`、`LyraPawnExtensionComponent.cpp`、`LyraHeroComponent.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/Player/LyraPlayerController.cpp:281-300` — PC 晚于 PS/ASC 复制的显式恢复路径。
  - `Source/LyraGame/Character/LyraPawnExtensionComponent.cpp:237-263` — authority/local 与 simulated proxy 的差异门槛。
  - `Source/LyraGame/Character/LyraHeroComponent.cpp:90-135` — 本地人类玩家额外等待 input/local player。
  - `Source/LyraGame/GameModes/LyraGameMode.cpp:345-368` — Spawn 前注入 PawnData。
- **备注**：`OnRep_PawnData` 在 PlayerState 中为空（`:216-218`），这不是遗漏的直接证据；PawnData 的客户端消费主要通过 replicated property、ASC grant 的复制效果以及其他组件/事件路径。需要具体追踪某项能力才能判定是否有缺口。

### 对象关系与复制矩阵

| 对象 | 典型存在位置 | UE/Lyra owner/绑定关系 | 关键复制/初始化职责 |
|---|---|---|---|
| `ULyraLocalPlayer` | 本地客户端/Listen Server | `UPlayer` 绑定本地 PlayerController | 本地设置、输入用户、把 Controller team 投影到本地层；不作为网络 Actor 复制 |
| `ALyraPlayerController` | Server 每玩家 + owning client | 拥有 PlayerState，Possess Pawn；客户端的 `Player` 是 LocalPlayer | 控制/RPC/输入；监听 PlayerState，晚复制时刷新 ASC |
| `ALyraPlayerState` | Server + 所有客户端 | Server 上 owner 通常为 Controller；关联当前 Pawn | 复制 PawnData、team、squad、stats；持有玩家级 ASC/attributes |
| `APawn`/`ALyraCharacter` | World 中按 relevancy 复制 | 被 Controller possess；引用 PlayerState | 当前躯体、movement/mesh/collision；转接 PlayerState ASC 为 avatar |
| `ULyraPawnExtensionComponent` | Pawn 子对象 | Pawn feature implementer | 复制 PawnData，协调所有 Pawn features 的 init-state |
| `ULyraHeroComponent` | 玩家 Pawn 子对象/可注入 | 依赖 PS、PC、PawnExtension、Input | 初始化 ASC avatar、输入、相机；到 GameplayReady |

---

## 维度五：World、GameState、GamePhase、Team 的全局玩法协作

### 发现 5.1：GameState 是全局可复制玩法服务的宿主，GameMode 是权威编排者

- **内容**：LyraGameMode 构造时指定 GameState/Controller/PlayerState/Pawn/HUD 类；GameState 构造时创建全局 ASC 与 ExperienceManager。GameMode 负责 Experience 选择、玩家初始化、Spawn/Restart；GameState 提供所有端可见的 Experience 选择、ServerFPS、全局消息桥和适合全局 ability 的 ASC。
- **来源**：`LyraGameMode.cpp`、`LyraGameState.cpp`、UE `GameModeBase.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/GameModes/LyraGameMode.cpp:33-42` — 稳定框架类装配。
  - `Source/LyraGame/GameModes/LyraGameState.cpp:22-33` — 全局 ASC 与 ExperienceManager。
  - `Source/LyraGame/GameModes/LyraGameState.cpp:85-113` — ServerFPS 复制与 multicast-to-message-router。
  - `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Private/GameModeBase.cpp:107-143` — GameMode 生成/初始化 GameState。
- **备注**：把所有全局逻辑都写进 GameState 也不是 Lyra 的意图；GameState 是复制锚点，复杂服务进一步拆为 Component 或 WorldSubsystem。

### 发现 5.2：GamePhase 用 GameState ASC 承载“有生命周期、可取消、可层级互斥”的全局状态

- **内容**：`ULyraGamePhaseSubsystem` 是 Game/PIE WorldSubsystem。`StartPhase` 向 GameState ASC 临时授予并立即激活一项 PhaseAbility；PhaseAbility 只允许 ServerInitiated/ServerOnly。PhaseTag 的层级关系决定并存与取消：父相同的更深子 phase 可与父 phase 共存；切到不匹配的分支会取消旧 phase。Subsystem 保存 active spec handle 与 observer，而真正生命周期由 GAS ability 激活/结束驱动。
- **来源**：`LyraGamePhaseSubsystem.cpp`、`LyraGamePhaseAbility.cpp`、`LyraGameState.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/AbilitySystem/Phases/LyraGamePhaseSubsystem.cpp:45-69` — World 类型与 GiveAbilityAndActivateOnce。
  - 同文件 `:136-180` — tag 层级判断与取消不相容 phase。
  - 同文件 `:193-210` — ability 结束驱动清理与通知。
  - `Source/LyraGame/AbilitySystem/Phases/LyraGamePhaseAbility.cpp:16-43` — ServerOnly 策略及 begin/end 回调。
  - `Source/LyraGame/GameModes/LyraGameState.cpp:28-30` — GameState ASC 复制。
- **备注**：PhaseSubsystem 的 `ActivePhaseMap` 本身未注册复制；客户端观察 phase 的具体方式依赖 GAS tag/effect/ability 相关复制或其他 gameplay 消息，不能宣称 subsystem map 直接同步到客户端。

### 发现 5.3：Team 是“权威创建/分配 + 复制 Actor 数据 + 每 World 查询索引”的三层设计

- **内容**：Experience Loaded 的高优先级回调触发 TeamCreationComponent：服务端先 Spawn public/private TeamInfo Actors，再给已存在玩家分队，并订阅 GameMode 的新玩家初始化事件。TeamInfo always relevant 且复制 TeamId/tags/display asset；每端 TeamInfo 在 BeginPlay/OnRep_TeamId 时注册进 `ULyraTeamSubsystem` 的 TeamMap。Subsystem 统一提供 actor->team 解析、关系/伤害判断、display asset 与 team tag stack 查询。
- **来源**：`LyraTeamCreationComponent.cpp`、`LyraTeamInfoBase.cpp`、`LyraTeamSubsystem.cpp`、`LyraPlayerState.cpp`
- **访问日期**：2026-08-13
- **来源层级**：T0
- **置信度**：H
- **代码证据**：
  - `Source/LyraGame/Teams/LyraTeamCreationComponent.cpp:35-54` — HighPriority Experience callback 与 authority gate。
  - 同文件 `:59-129` — 创建 TeamInfo、分配已有玩家、监听后续玩家。
  - `Source/LyraGame/Teams/LyraTeamInfoBase.cpp:13-35,70-83` — always relevant、复制与注册时点。
  - `Source/LyraGame/Teams/LyraTeamSubsystem.cpp:91-150` — TeamMap 注册与统一改队入口。
  - 同文件 `:153-180` — 从 interface/instigator/team-info/player-state 多级解析 team。
  - `Source/LyraGame/Player/LyraPlayerState.cpp:236-264` — authority 写 team、OnRep 广播。
- **备注**：HighPriority 的意义在此很具体：Team 在普通优先级的 GameMode 玩家 Restart 与 PlayerState 后续流程前先创建/分配，减少 Pawn 初始化时 team 尚缺失的窗口。

### 全局状态协作图

```text
Authority World
  ALyraGameMode
    ├─ 选择 ExperienceId ───────────────┐
    ├─ GenericPlayerInitialization event ─┼─> TeamCreationComponent
    └─ Restart/Spawn Pawn                │
                                         v
  ALyraGameState (replicated anchor)
    ├─ ExperienceManagerComponent -> 各端本地激活 features/actions
    ├─ AbilitySystemComponent -> server-only GamePhaseAbility 生命周期
    └─ 可复制全局数据/消息

  TeamCreationComponent (由 Experience action 注入/配置)
    ├─ Spawn Public/Private TeamInfo Actors
    └─ 写 PlayerState.TeamId

Each World side
  LyraTeamSubsystem <- TeamInfo BeginPlay/OnRep 注册索引
  PlayerState team -> Controller -> LocalPlayer / Pawn 转播
```

---

## 关键调用链（适合后续转成教学时序图）

### A. 进程到 World

1. Target 编入 `LyraGame`：`Source/LyraGame.Target.cs:13-19`。
2. 主模块注册：`Source/LyraGame/LyraGameModule.cpp:9-20`。
3. `ULyraGameEngine::Init -> UGameEngine::Init`：Lyra `:15-18`，Engine `GameEngine.cpp:1221-1267`。
4. Engine 依据配置创建 GameInstance；`UGameInstance::Init` 初始化 subsystems：`GameInstance.cpp:97-130`。
5. `ULyraGameInstance::Init` 注册四段 init states：`LyraGameInstance.cpp:88-101`。
6. `UGameEngine::Start -> UGameInstance::StartGameInstance -> Browse`：`GameEngine.cpp:1333-1339`，`GameInstance.cpp:626-688`。
7. 服务端 `UWorld::SetGameMode` 创建 AuthorityGameMode：`World.cpp:5927-5934`。
8. `InitializeActorsForPlay -> GameMode.InitGame -> Actor initialize -> GameMode.PreInitializeComponents -> Spawn GameState -> InitGameState`：`World.cpp:6031-6043`，`GameModeBase.cpp:116-143`。

### B. Experience 到 Pawn GameplayReady

1. `ALyraGameMode::InitGame` 安排下一 tick 选择：`LyraGameMode.cpp:80-85`。
2. `HandleMatchAssignmentIfNotExpectingOne` 选 PrimaryAssetId：`:88-164`。
3. `OnMatchAssignmentGiven -> ExperienceManager.SetCurrentExperience`：`:289-297`。
4. Experience manager 同步解析 definition CDO、异步加载 bundles：`LyraExperienceManagerComponent.cpp:56-67,123-202`。
5. 解析并 Load+Activate GameFeature plugins：`:214-286`。
6. 执行 Experience/ActionSet actions：`:289-343`。
7. `Loaded` 后 High -> Normal -> Low 广播：`:345-354`。
8. High：Team 创建/分配：`LyraTeamCreationComponent.cpp:35-54`。
9. Normal：PlayerState 设 PawnData/授予 ability；GameMode Restart 等待中的玩家：`LyraPlayerState.cpp:109-121,185-213`；`LyraGameMode.cpp:305-320`。
10. GameMode deferred spawn，在 FinishSpawning 前设 PawnExtension.PawnData：`LyraGameMode.cpp:345-368`。
11. Pawn/Character 的 Controller、PlayerState、Input 回调不断重试状态链：`LyraCharacter.cpp:212-267`。
12. PawnExtension 与 Hero 收敛到 GameplayReady：`LyraPawnExtensionComponent.cpp:213-267`；`LyraHeroComponent.cpp:76-215`。

### C. GameFeature 动态组件注入

1. Experience Action `OnGameFeatureActivating`：`LyraExperienceManagerComponent.cpp:320-343`。
2. `UGameFeatureAction_AddComponents::OnGameFeatureActivating` 处理已有/未来 GameInstance：UE `GameFeatureAction_AddComponents.cpp:36-79`。
3. `AddToWorld` 依据 net mode 建 component requests：同文件 `:155-183`。
4. Manager 给已存在 actor 补组件：UE `GameFrameworkComponentManager.cpp:251-305`。
5. Modular Actor 在 PreInitializeComponents AddReceiver，使未来 actor 接受请求：项目 `ModularCharacter.cpp:8-12` 等。
6. BeginPlay/ReceivedPlayer 发 GameActorReady，extension handlers 响应：项目 ModularGameplayActors 对应 `.cpp`。
7. Action deactivation 释放 request handles；引用计数归零后移除动态组件：UE ComponentManager `:308-355`。

---

## 矛盾、不确定项与需定向补证之处

### U1：Experience 选择优先级注释与当前实现不完全一致

- **矛盾描述**：`LyraGameMode.cpp:93-100` 把 Matchmaking assignment 写成最高优先级，但 `HandleMatchAssignmentIfNotExpectingOne` 当前没有读取 matchmaking assignment 的实现，只从空 `ExperienceId` 开始检查 URL 等来源。
- **来源 A**：同文件注释，T0，访问日期 2026-08-13。
- **来源 B**：同文件 `:88-164` 实际控制流，T0，访问日期 2026-08-13。
- **判定**：以实际代码为准；注释表示预留/目标设计。最终教学文档应明确标“当前快照未接入”。
- **置信度**：H。

### U2：GameFeature 激活失败是否阻止 Experience Loaded

- **不确定描述**：`OnGameFeaturePluginLoadComplete(const FResult& Result)` 在 `LyraExperienceManagerComponent.cpp:278-286` 只递减计数，不读取 Result；因此从这段源码看，失败也会推进到 actions/Loaded。GameFeaturesSubsystem 内部是否在失败前采取其他终止/错误恢复，需进一步沿 `LoadAndActivateGameFeaturePlugin` 状态机定向追踪。
- **当前判定**：不能宣称 Lyra Experience 对插件失败 fail-closed；当前直接调用点表现为“回调完成即继续”。
- **置信度**：M。

### U3：Experience 的“异步加载”不是全链路异步

- **易混描述**：架构介绍常把 Experience 说成异步加载；但 `SetCurrentExperience` 用 `AssetPath.TryLoad()` 同步加载 definition class，后续 `ChangeBundleStateForPrimaryAssets` 才异步。
- **判定**：两者并存，不是源码矛盾；教学文档应精确拆成“definition 解析”与“bundle/plugin 加载”。
- **置信度**：H。

### U4：GamePhase 客户端可见性的边界

- **不确定描述**：PhaseSubsystem 在 Game/PIE world 均创建，但 StartPhase/PhaseAbility 是 authority-only/server-only；`ActivePhaseMap` 没有直接复制。客户端究竟通过哪些 replicated GAS tags/effects 或 gameplay messages 获取特定 phase 表现，必须针对实际 PhaseAbility 蓝图/资产与 GAS 复制继续追踪。
- **当前判定**：服务端是 phase 真相源；禁止写成“WorldSubsystem 自动复制 phase”。
- **置信度**：H（否定直接复制）；M（具体客户端观察路径）。

### U5：资产层决定“哪些组件实际被注入”

- **不确定描述**：C++ 解释了 Experience/Action/ComponentManager 的机制，但具体 Experience Blueprint、GameFeatureData、AddComponents entries 位于 Content，不在用户本次学习范围。仅凭 C++ 不能完整列出运行时所有注入组件。
- **当前判定**：最终文档可解释插槽与代码入口，不应把未读取资产配置伪造成已核实实例。
- **置信度**：H。

---

## 面试追问链（5 题）

| # | 追问 | 答题方向 |
|---|---|---|
| 1 | 为什么 Lyra 不直接为每种玩法写一个 GameMode 子类，而要引入 Experience？ | 从稳定 GameMode 壳、PrimaryDataAsset 组合根、ActionSets 组合优于继承、GameFeature 动态激活、PawnData 配方五点回答；强调 Experience 不是复制 GameMode 的规则类，而是装配方案。回指发现 2.1、2.4。 |
| 2 | 客户端没有 GameMode，它如何知道应加载哪个 Experience？这是否意味着服务端把所有资产对象复制过去？ | GameState 上的 ExperienceManager 复制 `CurrentExperience` 引用/选择；客户端 OnRep 后按自己的 bundle/net mode 本地加载与激活。不是复制整个对象图。回指发现 2.3。 |
| 3 | 为什么 ASC 放 PlayerState 而不是 Character？换 Pawn 时 OwnerActor 与 AvatarActor 分别是谁？ | 玩家级能力/属性需要跨死亡与 Pawn 替换；Owner=PlayerState，Avatar=当前 Pawn。PawnExtension 处理旧 avatar 清退、ActorInfo 刷新，Character 初始化 health/tags。回指发现 4.1 与 PawnExtension `:105-182`。 |
| 4 | AddComponents action 激活时 Actor 已经存在，或 Actor 晚于插件生成，分别怎样保证组件被添加？卸载怎样移除？ | 已有 Actor：AddComponentRequest 遍历已初始化实例；未来 Actor：Modular Actor 在 PreInitializeComponents AddReceiver，沿继承链匹配请求；RequestHandle 引用计数归零时撤销并销毁 Manager 创建的实例。回指发现 3.1、3.2。 |
| 5 | 为什么 Lyra Pawn 初始化需要四阶段状态机，而不是依赖 BeginPlay -> PossessedBy -> OnRep 的固定顺序？ | 多人网络下 Controller、PlayerState、PawnData、Input、动态组件到达顺序因 authority/autonomous/simulated proxy 不同；多个事件只负责触发幂等重试，CanChangeInitState 表达门槛，all-features barrier 保证跨组件一致性。回指发现 3.4、4.3。 |

### 额外刁钻追问预判

1. **High/Normal/Low Experience delegate 是否等价于线程优先级？** 不等价，只是同一完成点上的显式同步广播顺序；应说明各层当前消费者（例如 Team High，GameMode/PlayerState Normal）。
2. **GameFeature 已经 Active，为什么还要 Experience 自己手动调用 Actions 的 Registering/Loading/Activating？** 插件自身激活与 Experience 内嵌 actions/action sets 是两组来源；ExperienceManager 对后者显式驱动生命周期，并用 required world context 限定应用世界。
3. **InitState 是否自动按父 tag 推进？** 状态关系由 GameInstance 注册，实际推进仍由 feature implementer 的 `CanChangeInitState`/`TryToChangeInitState`/`ContinueInitStateChain` 驱动；Manager 保存顺序与通知，不替业务判断依赖是否满足。

---

## 来源清单

| # | 来源 | 层级 | 访问日期 | 用途 |
|---|---|---|---|---|
| 1 | `Source/LyraGame.Target.cs`、`Source/LyraEditor.Target.cs` | T0 | 2026-08-13 | Target 装配边界 |
| 2 | `Source/LyraGame/LyraGame.Build.cs`、`LyraGameModule.cpp` | T0 | 2026-08-13 | 模块依赖与注册 |
| 3 | `Source/LyraGame/System/LyraGameEngine.cpp`、`LyraGameInstance.cpp` | T0 | 2026-08-13 | Lyra 进程/会话扩展点 |
| 4 | `Source/LyraGame/GameModes/LyraGameMode.cpp`、`LyraGameState.cpp` | T0 | 2026-08-13 | World 权威编排与复制锚点 |
| 5 | `Source/LyraGame/GameModes/LyraExperience*.h/.cpp` | T0 | 2026-08-13 | Experience 数据与状态机 |
| 6 | `Source/LyraGame/Character/LyraPawnExtensionComponent.cpp`、`LyraHeroComponent.cpp`、`LyraCharacter.cpp` | T0 | 2026-08-13 | Pawn 初始化收敛链 |
| 7 | `Source/LyraGame/Player/LyraLocalPlayer.*`、`LyraPlayerController.*`、`LyraPlayerState.*` | T0 | 2026-08-13 | 玩家对象所有权/复制 |
| 8 | `Plugins/ModularGameplayActors/Source/ModularGameplayActors` | T0 | 2026-08-13 | Receiver 生命周期封装 |
| 9 | `Source/LyraGame/AbilitySystem/Phases` | T0 | 2026-08-13 | 全局 GamePhase 生命周期 |
| 10 | `Source/LyraGame/Teams` | T0 | 2026-08-13 | Team 创建、复制与索引 |
| 11 | `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Private/GameEngine.cpp`、`GameInstance.cpp`、`World.cpp`、`GameModeBase.cpp` | T0 | 2026-08-13 | UE 启动与 World/GameMode 基类链 |
| 12 | `D:/Software/UE_5.8/Engine/Source/Runtime/Engine/Classes/Engine/LocalPlayer.h`、`Classes/GameFramework/{PlayerController,PlayerState,Pawn}.h` | T0 | 2026-08-13 | 官方类边界与网络可见性 |
| 13 | `D:/Software/UE_5.8/Engine/Plugins/Runtime/ModularGameplay/Source/ModularGameplay` | T0 | 2026-08-13 | request/receiver/event/init-state 底层 |
| 14 | `D:/Software/UE_5.8/Engine/Plugins/Runtime/GameFeatures/Source/GameFeatures/.../GameFeatureAction_AddComponents.*` | T0 | 2026-08-13 | GameFeature 到动态组件请求桥梁 |

## 最低来源覆盖核查

- 启动链：项目 Target/Module + Lyra Engine/GameInstance + UE Engine/GameInstance/World/GameModeBase，至少 3 组独立 T0。
- Experience：Lyra GameMode + ExperienceManager/Definition + PlayerState/Pawn + UE GameFeatures，至少 4 组独立 T0。
- ModularGameplay：项目 ModularGameplayActors + UE ComponentManager + UE AddComponents Action + Lyra Pawn features，至少 4 组独立 T0。
- 玩家对象：UE class contract + Lyra Player classes + Lyra Pawn components，至少 3 组独立 T0。
- 全局状态：Lyra GameState + GamePhase + Team + UE GameModeBase，至少 4 组独立 T0。

## 本轮停止条件

- 五个指定问题均有结构化发现、调用链、路径/符号/当前行号与置信度。
- 核心结论均达到至少两组 T0 交叉；未闭合处已进入 U1-U5，不伪装为已证实。
- 下一轮若继续，应只针对 U2（GameFeature 激活失败状态机）、U4（Phase 客户端观察路径）和具体资产配置做定向追踪，不再重复泛扫启动主链。
