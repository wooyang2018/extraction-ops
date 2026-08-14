# Lyra 源码研究采集：插件、UI/设置/用户流程与编辑器支撑

## 研究范围与证据约定

- 研究对象：当前仓库 `Plugins/` 下所有含源码插件（排除 `Plugins/GameFeatures/ExtractionOps/`），以及连接这些插件所必需的 `Source/LyraGame/UI/`、`Source/LyraGame/Settings/`、`Source/LyraEditor/`。
- 引擎下钻：`D:/Software/UE_5.8/Engine/Source/` 与 `D:/Software/UE_5.8/Engine/Plugins/`，重点是 `PreLoadScreen`、`CommonUI`、`ModularGameplay`、`GameFeatures`。
- 访问日期：2026-08-13。
- 来源层级：仓库和本机 UE 5.8 源码均为 T0。
- 置信度：H 表示由当前版本中至少两处源码/描述文件互证；M 表示单处源码直接支持，或设计动机属于基于实现的推断；L 仅用于代码中尚未完成或未启用的路径。
- 本文件是结构化采集记录，不是 `docs/learn` 最终教程。

## 一、插件全景、模块职责与依赖

当前范围共 15 个含源码插件、16 个模块；`GameplayMessageRouter` 同时包含 Runtime 与 UncookedOnly 节点模块。`RedRoom`、`GreenRoom`、`LyraExampleContent`、`ShooterMaps`、`ShooterExplorer` 等没有源码模块，不列入本表。

| 插件 / 模块 | 源码职责 | 关键模块依赖 | 插件级依赖 | 证据 | 置信度 |
|---|---|---|---|---|---|
| AsyncMixin / `AsyncMixin` | 把多次软引用加载、Primary Asset bundle 预载、条件等待和回调按请求顺序串行化；宿主析构/取消时切断回调 | Core、CoreUObject、Engine | 无 | `Plugins/AsyncMixin/Source/Public/AsyncMixin.h:31-70,72-299`；`Private/AsyncMixin.cpp:11-57`；`AsyncMixin.Build.cs:9-26` | H |
| CommonGame / `CommonGame` | 通用 GameInstance、LocalPlayer、UI Policy、每玩家根布局、CommonUI layer push/pop、对话框和异步 Widget Action | CommonUI、CommonInput、CommonUser、GameplayTags、ModularGameplayActors、UMG | CommonUI、CommonUser、ModularGameplayActors、OnlineFramework | `Plugins/CommonGame/CommonGame.uplugin`；`Source/CommonGame.Build.cs:11-26`；`Private/CommonGameInstance.cpp:52-104`；`Private/GameUIPolicy.cpp:45-206` | H |
| CommonLoadingScreen / `CommonLoadingScreen` | 运行期/地图切换加载屏：逐帧汇总 World、GameState、PlayerController、组件和外部 processor 的阻塞理由，并控制输入、渲染、streaming 与 heartbeat | Engine、Slate、PreLoadScreen、RenderCore、DeveloperSettings、UMG | 无 | `Plugins/CommonLoadingScreen/CommonLoadingScreen.uplugin`；`LoadingScreenManager.h:25-130`；`LoadingScreenManager.cpp:128-698` | H |
| CommonStartupLoadingScreen / `CommonStartupLoadingScreen` | 引擎启动早期、GameInstance/World 尚不可用时注册 EngineLoadingScreen 类型的 Slate 预加载屏 | MoviePlayer、PreLoadScreen、Slate、DeveloperSettings、CommonLoadingScreen | CommonLoadingScreen | `Plugins/CommonStartupLoadingScreen/CommonStartupLoadingScreen.uplugin`；`Private/CommonStartupLoadingScreen.cpp:10-70`；`Private/CommonPreLoadScreen.h:9-15` | H |
| CommonUser / `CommonUser` | 平台用户、输入设备、登录状态、权限查询、Guest、OSSv1/v2 适配，以及 Host/Find/QuickPlay/Join/Travel 的 Session 外观层 | CoreOnline、GameplayTags、OnlineSubsystemUtils；当前编译选择 OnlineSubsystem v1 | OnlineSubsystem、OnlineSubsystemUtils、OnlineServices | `Plugins/CommonUser/CommonUser.uplugin`；`CommonUser.Build.cs:11,27-47`；`CommonUserSubsystem.cpp:979-1103,1342-1507`；`CommonSessionSubsystem.cpp:432-1584` | H |
| GameplayMessageRouter / `GameplayMessageRuntime` | GameInstance 范围的、GameplayTag 分频道、UScriptStruct 类型检查的进程内消息总线 | Engine、GameplayTags | GameplayTagsEditor | `GameplayMessageRuntime.Build.cs:11-23`；`GameplayMessageSubsystem.h:105-226`；`.cpp:45-190` | H |
| GameplayMessageRouter / `GameplayMessageNodes` | 为异步监听节点生成动态 payload pin，并在 K2 编译期展开临时变量、GetPayload 与赋值节点 | BlueprintGraph、KismetCompiler、PropertyEditor、GameplayMessageRuntime、UnrealEd | 同上 | `GameplayMessageNodes.Build.cs:11-29`；`K2Node_AsyncAction_ListenForGameplayMessages.cpp:89-190` | H |
| GameSettings / `GameSettings` | 设置项对象模型、Collection/Registry、动态数据源、编辑条件、过滤、脏状态、Apply/Restore，以及 CommonUI 设置页 Widget | CommonUI、CommonInput、GameplayTags、PropertyPath、UMG | CommonUI | `GameSettings.Build.cs:11-33`；`GameSettingRegistry.cpp:21-162`；`GameSettingRegistryChangeTracker.cpp:22-100`；`GameSettingScreen.cpp:33-70` | H |
| GameSubtitles / `GameSubtitles` | UMG/Slate 字幕显示、格式选项 GameInstanceSubsystem、Media Overlay 到引擎 `FSubtitleManager` 的桥接 | Overlay、MediaAssets、MediaUtils、GameplayTags、UMG、Slate | 无 | `GameSubtitles.Build.cs:9-37`；`SSubtitleDisplay.cpp:12-70`；`MediaSubtitlesPlayer.cpp:26-68` | H |
| ModularGameplayActors / `ModularGameplayActors` | 给 GameMode/GameState/Controller/PlayerState/Pawn/Character/AIController 提供会自动登记到 ModularGameplay ComponentManager 的基类 | Engine、ModularGameplay、AIModule | ModularGameplay | `ModularGameplayActors.Build.cs:26-43`；`ModularGameState.cpp:10-63`；其余 `Private/Modular*.cpp` | H |
| PocketWorlds / `PocketWorlds` | 为每 LocalPlayer 在远离主场景的位置流式加载“口袋关卡”，并用 SceneCapture2D 生成物品/角色缩略图 RenderTarget | Core、CoreUObject、Engine | 无 | `PocketWorlds.Build.cs:9-27`；`PocketLevelSystem.cpp:10-33`；`PocketLevelInstance.cpp:19-131`；`PocketCapture.cpp:24-318` | H |
| UIExtension / `UIExtension` | World 范围的 UI 插槽注册/内容注入系统；按 GameplayTag、ContextObject、数据类型契约匹配，并由 ExtensionPointWidget 实例化/移除 Widget | CommonUI、CommonGame、GameplayTags、UMG | CommonGame | `UIExtension.Build.cs:9-33`；`UIExtensionSystem.h:21-251`；`.cpp:97-304` | H |
| ShooterCore / `ShooterCoreRuntime` | Shooter GameFeature 的通用玩法包：助攻/连杀消息处理、准星辅助、TDM 出生点、交互拾取、勋章 UI | LyraGame、ModularGameplay、CommonGame；私有依赖 GAS、GameplayMessageRuntime、AsyncMixin、EnhancedInput、GameSubtitles 等 | GameplayAbilities、GameplayMessageRouter、AsyncMixin、CommonUI、CommonGame、GameSubtitles、EnhancedInput、LyraExampleContent | `ShooterCore.uplugin`；`ShooterCoreRuntime.Build.cs:25-55`；各子目录实现 | H |
| TopDownArena / `TopDownArenaRuntime` | 独立玩法变体示例：顶视摄像机、GAS AttributeSet、由 Attribute/Tag 驱动的移动组件、拾取 UI 数据 | LyraGame；私有依赖 GAS、GameplayTags、Niagara | GameplayAbilities、LyraExampleContent、Niagara | `TopDownArena.uplugin`；`TopDownArenaRuntime.Build.cs:25-45`；`TopDownArenaAttributeSet.cpp:11-80`；`TopDownArenaMovementComponent.cpp:15-34` | H |
| ShooterTests / `ShooterTestsRuntime` | CQTest 的地图、输入、动画和多人 PIE 网络测试辅助；消费 Lyra 角色/GAS 与 ShooterCore 内容 | LyraGame、GAS、ModularGameplay、AsyncMessageSystem、EnhancedInput、CQTest | ShooterCore、AsyncMessageSystem、AsyncMessageSystemTests、LyraExampleContent、CQTest 等 | `ShooterTests.uplugin`；`ShooterTestsRuntime.Build.cs:11-46`；`ShooterTestsMapTests.cpp:18-121`；`ShooterTestsActorNetworkTests.cpp:7-115` | H |
| LyraExtTool / `LyraExtTool` | 极薄 Editor 插件；唯一实际工具是批量把 `UStaticMesh::StaticMaterials` 改成同一材质，并触发事务/编辑后刷新 | Core、CoreUObject、Engine、Slate | 无 | `LyraExtTool.uplugin`；`LyraExtTool.Build.cs:25-42`；`BPFunctionLibrary.cpp:11-25` | H |

### 构建文件异常

- `CommonStartupLoadingScreen` 同时存在 `Source/CommonStartupLoadingScreen.Build.cs` 与 `Source/CommonStartupLoadingScreen/CommonStartupLoadingScreen.Build.cs`；前者私有依赖额外包含 `CommonLoadingScreen`（34-45），后者没有（34-44）。模块目录约定指向后者，但插件描述又声明依赖 `CommonLoadingScreen`，最终文档应把它标成“仓库布局异常/需构建验证”，不能把两份规则混成一个事实。置信度 M。
- 当前 `CommonUser.Build.cs:11` 把 `bUseOnlineSubsystemV1` 硬编码为 `true`，因此虽然源码保留 OSSv2 分支，当前项目实际编译的是 OSSv1。置信度 H。

## 二、登录、Session 与加载屏的完整链

### 2.1 三层加载屏不是同一个系统

1. **引擎启动前加载屏**：`CommonStartupLoadingScreen` 在 `PreEarlyLoadingScreen` 阶段启动；创建 `FCommonPreLoadScreen`，其类型是 `EPreLoadScreenTypes::EngineLoadingScreen`，并注册到 `FPreLoadScreenManager`。此时不能依赖 GameInstance、World 或 UMG。证据：`CommonStartupLoadingScreen.uplugin` 模块 LoadingPhase；`CommonStartupLoadingScreen.cpp:34-47`；`CommonPreLoadScreen.cpp:10-15`；UE `Engine/Source/Runtime/PreLoadScreen/Public/PreLoadScreen.h:9-71`。置信度 H。
2. **运行期加载屏**：GameInstance 创建 `ULoadingScreenManager`；其 tick 每帧调用 `ShouldShowLoadingScreen()`。它覆盖无 WorldContext、无 World/GameState、Pre/PostLoadMap 区间、pending travel/net game、未 BeginPlay、seamless travel、GameState/PlayerController/组件实现的 `ILoadingProcessInterface`、以及显式注册的 external processor。证据：`LoadingScreenManager.cpp:128-223,255-405`。置信度 H。
3. **业务流程阻塞者**：例如 `ULyraFrontendStateComponent` 自身实现 `ILoadingProcessInterface`，在 Experience 加载后执行 ControlFlow；直到 Press Start 或 Main Screen push 完成才把 `bShouldShowLoadingScreen=false`。证据：`LyraFrontendStateComponent.cpp:28-78,110-166,228-253`。置信度 H。

交接与互斥：运行期 manager 在 `ShowLoadingScreen()` 中发现仍有 `EngineLoadingScreen` 时直接返回（`LoadingScreenManager.cpp:473-484`），并用 `IsShowingInitialLoadingScreen()` 查询 `FPreLoadScreenManager`（467-470）。这防止两个系统同时覆盖 viewport。启动模块在 PreLoadScreen manager cleanup 回调中释放 Slate 资源（`CommonStartupLoadingScreen.cpp:45-55`）。置信度 H。

运行期显示/隐藏副作用：显示时注册最高优先级 Slate input preprocessor、创建 UMG loading widget、关闭 3D world rendering、把 ShaderPipelineCache 切到 Fast、提高关卡 streaming 优先级并暂停 hitch heartbeat；隐藏时先 GC、移除 widget、恢复渲染/cache/heartbeat（`LoadingScreenManager.cpp:473-599,640-697`）。这是“遮罩”之外的重要工程职责。置信度 H。

显式阻塞任务链：`ULoadingProcessTask::CreateLoadingScreenProcessTask()` 以 manager 为 Outer 创建对象并注册为 external processor；只要对象未 `Unregister()`，`ShouldShowLoadingScreen()` 恒真并返回其 reason（`LoadingProcessTask.cpp:13-46`）。置信度 H。

### 2.2 前端用户初始化链

调用链：

```text
Experience loaded
  -> ULyraFrontendStateComponent::OnExperienceLoaded
  -> ControlFlow: reset/cleanup -> press start or auto init -> requested session -> main screen
  -> UCommonUserSubsystem::TryToInitializeForLocalPlay
  -> TryToInitializeUser
  -> LoginLocalUser / ProcessLoginRequest (OSSv1 in current build)
  -> HandleLoginForUserInitialize
  -> optional CreateLocalPlayer
  -> SetLocalPlayerUserInfo
  -> HandleUserInitializeSucceeded
  -> OnUserInitializeComplete
  -> frontend flow ContinueFlow
```

- Frontend 先判断玩家 0 是否已经处于 `LoggedInLocalOnly/LoggedInOnline`；无需 Press Start 时直接用默认 input device 初始化；需要 Press Start 时 push 对应 CommonUI 页面，页面自己触发用户选择。证据：`LyraFrontendStateComponent.cpp:110-166`。置信度 H。
- `TryToInitializeUser` 校验 LocalPlayer 顺序、最大本地人数、输入设备与平台用户映射、防止同设备绑定两个用户，并禁止主玩家作为 Guest；随后区分 initial login 与 network login（`CommonUserSubsystem.cpp:1007-1103`）。置信度 H。
- 登录成功后允许创建 LocalPlayer，并设置用户信息；回调统一延迟到下一 tick，避免在 Online delegate 深层调用栈内继续修改玩家结构（`CommonUserSubsystem.cpp:1342-1413`）。这是基于实现的设计动机推断，置信度 M。
- `UCommonGameInstance::Init()` 不主动登录，它只在所有 subsystem 初始化后把 PlatformTraits、system message、privilege、user initialized、session invite/destroy 事件接起来（`CommonGameInstance.cpp:82-103`）。置信度 H。
- `UAsyncAction_CommonUserInitialize` 是蓝图友好包装：构造参数、注册到 GameInstance 保活、Activate 时绑定一次完成委托，失败则 next tick 广播（`AsyncAction_CommonUserInitialize.cpp:10-103`）。置信度 H。

### 2.3 Session 链

Host：`ULyraUserFacingExperienceDefinition::CreateHostingRequest()` 生成模式/地图/最大人数等请求（`Source/LyraGame/GameModes/LyraUserFacingExperienceDefinition.cpp:13-55`）→ `UCommonSessionSubsystem::HostSession()` → Online 时 `CreateOnlineSessionInternal()` → OSSv1 `CreateSession`，把 GameMode/Map/timeout/template 写入 settings（`CommonSessionSubsystem.cpp:459-559`）→ create/start 完成 → 通知创建结果；Travel 模式直接 `ServerTravel`。置信度 H。

Quick Play：建立 SearchRequest，强制主 Session 使用 presence/lobby 默认值，Find 完成后有结果则 Join 第一个，没有结果则 Host（`CommonSessionSubsystem.cpp:878-978`）。代码自己注明尚未按 ping/其他因素选择最佳结果，所以“best”目前只是首项。置信度 H。

Join：`JoinSession()` 更新 presence 信息 → OSSv1 `IOnlineSession::JoinSession` → `FinishJoinSession()`；可选先走 PartyBeacon reservation，否则广播 join success 并 `InternalTravelToSession()`；最后 resolve connect string、允许 `OnPreClientTravelEvent` 改 URL，再 `PlayerController->ClientTravel(TRAVEL_Absolute)`（`CommonSessionSubsystem.cpp:1157-1214,1309-1326,1523-1584`）。置信度 H。

Invite/requested session：Online delegate → `NotifyUserRequestedSession` → `UCommonGameInstance::OnUserRequestedSession` → `SetRequestedSession`；前端 ControlFlow 在 `FlowStep_TryJoinRequestedSession` 绑定 join completion，成功则取消“去主菜单”的 flow，失败才继续显示主界面（`CommonGameInstance.cpp:129-193`；`LyraFrontendStateComponent.cpp:192-225`）。置信度 H。

退出/硬断线：普通 `ReturnToMainMenu()` 调 `ResetUserAndSessionState()`；前端 hard disconnect 才重置 user，但每次都清 session，意图是保留普通返回菜单时的平台登录而清理旧 match session（`CommonGameInstance.cpp:106-127` 与 `LyraFrontendStateComponent.cpp:80-107` 存在策略差异，最终文档需指出调用场景）。置信度 H。

## 三、CommonUI、UIExtension 与 GameSettings 消费链

### 3.1 每玩家根 UI 与 Layer 栈

```text
UGameInstance::CreateLocalPlayer
  -> UCommonGameInstance::AddLocalPlayer
  -> UGameUIManagerSubsystem::NotifyPlayerAdded
  -> UGameUIPolicy::NotifyPlayerAdded
  -> CreateLayoutWidget(UPrimaryGameLayout)
  -> AddToPlayerScreen(z=1000)
  -> Blueprint layout calls RegisterLayer(GameplayTag, CommonActivatableWidgetContainer)
  -> PushWidgetToLayerStack[Async]
  -> CommonUI container creates/activates widget and routes focus/input
```

证据：`CommonGameInstance.cpp:52-64`；`GameUIManagerSubsystem.cpp:13-20,45-50`；`GameUIPolicy.cpp:51-76,124-132,189-206`；`PrimaryGameLayout.cpp:89-117`；UE `Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Private/Widgets/CommonActivatableWidgetContainer.cpp:191-214,271-318`。置信度 H。

设计边界：`UGameUIManagerSubsystem` 负责选 policy，policy 负责 local-player 到 root layout 生命周期，`UPrimaryGameLayout` 只负责 gameplay-tag layer 到 CommonUI container 的映射。`UCommonUIExtensions` 是便捷入口，并在异步 stream widget 时用 CommonUI action router suspend/resume input。证据：`CommonUIExtensions.cpp:55-115,129-171`。置信度 H。

### 3.2 GameFeature UI 注入

`UGameFeatureAction_AddWidgets::AddToWorld()` 对 `ALyraHUD` 注册 ModularGameplay extension handler；HUD 已存在时引擎 manager 会立即发 `ExtensionAdded`，HUD BeginPlay 后还会发 `GameActorReady`。处理器随后：

- Layout 条目通过 `UCommonUIExtensions::PushContentToLayer_ForPlayer()` 放入根布局某一 Layer；
- Widgets 条目通过 World 的 `UUIExtensionSubsystem::RegisterExtensionAsWidgetForContext(SlotID, LocalPlayer, Class)` 注册到插槽；
- GameFeature deactivation/receiver removal 时 deactivate layout 并注销所有 extension handle。

证据：`Source/LyraGame/GameFeatures/GameFeatureAction_AddWidget.cpp:91-188`；`Source/LyraGame/UI/LyraHUD.cpp:25-47`；UE `GameFrameworkComponentManager.cpp:358-398`。置信度 H。

### 3.3 UIExtension 匹配机制

- `UUIExtensionSubsystem` 是 `UWorldSubsystem`，因此同一进程不同 PIE World 不共享注册表（`UIExtensionSystem.h:191-251`）。置信度 H。
- Extension/Point 都以 GameplayTag 为 key；PartialMatch 沿 tag 父链匹配，ExactMatch 只处理原始 tag（`UIExtensionSystem.cpp:184-235`）。置信度 H。
- Contract 同时要求 ContextObject 相同和 DataClass 满足 `IsChildOf` 或 `ImplementsInterface`；Widget class 本身以 `UClass` 作为 Data 存储（`UIExtensionSystem.cpp:35-58,138-180`）。置信度 H。
- `UUIExtensionPointWidget` 同时注册无 context、LocalPlayer context，并在 PlayerState 就绪后再注册 PlayerState context；收到 Added 时创建 widget/data adapter，Removed 时移除对应 entry（`UIExtensionPointWidget.cpp:31-41,70-154`）。置信度 H。
- handle 不是纯标识：注销 handle 会触发 Removed 回调，从而驱动 UI 清理；Subsystem 用 `AddReferencedObjects` 为 shared struct 内 UObject 数据补 GC 引用（`UIExtensionSystem.cpp:15-31,63-84,237-304`）。置信度 H。

### 3.4 设置注册、展示、Apply/Cancel

```text
ULyraSettingScreen::CreateRegistry
  -> ULyraGameSettingRegistry::Initialize(LocalPlayer)
  -> OnInitialize creates Video/Audio/Gameplay/M&K/Gamepad collections
  -> UGameSettingRegistry::RegisterSetting recursively registers children and delegates
  -> UGameSettingScreen::GetOrCreateRegistry -> UGameSettingPanel::SetRegistry
  -> value changes -> Registry event -> ChangeTracker DirtySettings
  -> Apply: value.Apply + StoreInitial -> Registry::SaveChanges
  -> Lyra SaveChanges: Local settings ApplySettings + shared ApplySettings/SaveSettings
  -> Cancel: RestoreToInitial for every dirty value
```

证据：`LyraSettingScreen.cpp:22-31`；`LyraGameSettingRegistry.cpp:54-86`；`GameSettingRegistry.cpp:21-24,102-149`；`GameSettingScreen.cpp:33-70`；`GameSettingRegistryChangeTracker.cpp:22-100`。置信度 H。

本地/共享设置分层：`ULyraSettingsLocal` 是全机/设备侧 `UGameUserSettings`；`ULyraSettingsShared` 是按 LocalPlayer SaveGame，包含输入、字幕、文化等。桌面允许登录前同步读取 Shared；其他平台先建 temporary，登录后异步替换（`LyraLocalPlayer.cpp:97-144`；`LyraSettingsShared.cpp:52-114`）。置信度 H。

GameSettings 本身不硬编码 Lyra setting：它提供动态数据源、edit condition、registry/filter/widget 框架；Lyra 在多份 `LyraGameSettingRegistry_*.cpp` 里组合实际页面。以 Audio 为例，页面创建 `UGameSettingCollection` 后用 dynamic getter/setter 对接 Local/Shared settings（`LyraGameSettingRegistry_Audio.cpp:23+`）。置信度 H。

## 四、基础插件机制

### 4.1 GameplayMessageRouter

- 总线是 `UGameInstanceSubsystem`，消息只在本地进程/当前 GameInstance 内传递，不自带复制、可靠性或持久化（`GameplayMessageSubsystem.cpp:45-63`）。网络消息必须先经 RPC/复制到客户端，再由 Lyra GameState/PlayerState 转投本地总线。置信度 H。
- 广播沿 Channel 的 GameplayTag 父链向上遍历；原始 tag 无条件匹配，父 tag 只有 `PartialMatch` listener 才收到（65-121）。置信度 H。
- payload 用 `UScriptStruct` 描述；发送类型必须是 listener 期望类型的子 struct。回调列表先复制，允许回调中注销自己（90-118）。置信度 H。
- Blueprint async action 在广播期间临时保存裸 payload pointer，delegate 返回后立即清空；K2 节点在编译期生成强类型 payload 输出，因此 payload 不能跨回调保存为裸地址（`AsyncAction_ListenForGameplayMessage.cpp:71-107`；K2 node `:125-190`）。置信度 H。

### 4.2 ModularGameplayActors 与 UE ModularGameplay

- 每种 Modular Actor 基类在 `PreInitializeComponents` 调 `AddGameFrameworkComponentReceiver`，在 BeginPlay 发 `NAME_GameActorReady`，EndPlay remove receiver；GameState 还把 match-start 转发给注入的 `UGameStateComponent`（`ModularGameState.cpp:10-63`，其余 actor cpp 同构）。置信度 H。
- UE manager 的 `AddComponentRequest` 对 receiver class + component class 计数；首个请求会立即扫描当前 World 中已经 initialized 的实例并注入组件，handle 释放到计数 0 时销毁注入实例（UE `GameFrameworkComponentManager.cpp:251-355`）。置信度 H。
- `AddExtensionHandler` 同样会对现存 initialized actor 立即发 `ExtensionAdded`，之后 receiver 的显式 event 再驱动状态（358-398）。这解释了为何 GameFeature 可以晚于 actor 激活。置信度 H。
- 禁止以裸 `AActor` 为 receiver class，源码明确把它视为过宽且有性能风险（251-256,358-363）。置信度 H。

### 4.3 AsyncMixin

- `FAsyncMixin` 自身零成员状态；静态 `TMap<FAsyncMixin*, TSharedRef<FLoadingState>>` 只为活跃请求分配状态（`AsyncMixin.h:65-68,200-299`；`.cpp:11-42`）。置信度 H。
- 每个请求成为 `FAsyncStep`；资源可以并发开始，但 user callback 严格按提交顺序执行，之后才 `OnFinishedLoading`。未显式 Start 会通过 core ticker 下一帧自动启动（头文件 31-70；实现 `AsyncMixin.cpp:106+`）。置信度 H。
- 析构从静态 map 移除状态，取消 streaming handle/condition 回调；使用 `[this]` 的安全前提是宿主确实继承 mixin 且在析构前没有绕开其生命周期（`.cpp:17-24,51-57`）。置信度 H。
- ShooterCore 勋章 Widget 展示了真实消费：DataRegistry 行异步取得后，再用 mixin 加载 Sound/Icon，并用 sequence id 抵消加载完成乱序（`LyraAccoladeHostWidget.cpp:37-108`）。置信度 H。

### 4.4 PocketWorlds

- `UPocketLevelSubsystem::GetOrCreatePocketLevelFor` 以 `(LocalPlayer, PocketLevel)` 复用实例，并按已有 pocket bounds 的高度偏移避免相互重叠（`PocketLevelSystem.cpp:10-33`）。置信度 H。
- `UPocketLevelInstance` 用 `ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr` 加载远处小关卡，并从 Level 中寻找目标 actor/capture actor；LoadedVisible 后广播 ready（`PocketLevelInstance.cpp:19-74,93-131`）。置信度 H。
- `UPocketCapture` 动态注册 `USceneCaptureComponent2D`，只渲染 show-only actors；可分别捕获 diffuse 和材质替换后的 alpha mask。Subsystem 每帧管理 `bForceMipStreaming`，只为本帧/下一帧需要捕获的 primitive 强制驻留（`PocketCapture.cpp:24-35,145-306`；`PocketCaptureSubsystem.cpp:71-96`）。置信度 H。
- Effects capture 仍是 `ensure(false);//TODO`（`PocketCapture.cpp:310-317`），最终文档不得描述为已实现。置信度 H。

### 4.5 GameSubtitles

- `USubtitleDisplaySubsystem` 保存 `FSubtitleFormat` 并广播格式变化；Lyra SharedSettings apply 时设置字幕选项（`SubtitleDisplaySubsystem.cpp:16-39`；`LyraSettingsShared.cpp:102-114` 及 `ApplySubtitleOptions`）。置信度 H。
- `USubtitleDisplay` 是 UMG wrapper，重建为 `SSubtitleDisplay`；Slate widget 监听引擎 `FSubtitleManager::OnSetSubtitleText`，并尊重 `UGameplayStatics::AreSubtitlesEnabled()`（`SubtitleDisplay.cpp:51-115`；`SSubtitleDisplay.cpp:12-70`）。置信度 H。
- `UMediaSubtitlesPlayer` tick MediaPlayer 时间，从 `UOverlays` 取当前条目，再写入 `FSubtitleManager::SetMovieSubtitle`（`MediaSubtitlesPlayer.cpp:26-68`）。置信度 H。

## 五、GameFeature 示例如何消费 Lyra

### ShooterCore

- 它不是完整 game mode，而是被 Experience/GameFeature actions 组合进 Lyra 的 shooter 能力包。公开依赖 LyraGame/CommonGame/ModularGameplay，私有消费 GAS、消息、CommonUI、AsyncMixin 等（Build.cs 25-55）。置信度 H。
- `UAssistProcessor` 监听 Damage/Elimination verb message，按目标累计每个攻击者伤害；目标被击杀时为非击杀者广播 Assist message，并清除历史（`AssistProcessor.cpp:12-70`）。置信度 H。
- ElimChain/Streak processor 把原始 elimination 派生为更高层 GameplayTag 消息，体现 Message Processor 作为“事件投影器”而非权威状态（`ElimChainProcessor.cpp:16-54`；`ElimStreakProcessor.cpp:16+`）。置信度 H。
- `UTDM_PlayerSpawningManagmentComponent` 继承 Lyra spawning manager，结合 TeamSubsystem、PlayerArray、claim/occupancy，选择离敌方最远的可用出生点（`TDM_PlayerSpawningManagmentComponent.cpp:20-88`）。置信度 H。
- AimAssist 由 TargetComponent 提供 target options，Manager 做 overlap/filter/visibility，EnhancedInput modifier 根据输入类型、视图和目标计算 slowdown/pull；这是表现/输入修正，不应作为命中权威（`AimAssistInputModifier.cpp:454-799`；`AimAssistTargetManagerComponent.cpp:87-481`）。置信度 H。
- Accolade host 把 GameplayMessage、DataRegistry、AsyncMixin 与 BlueprintImplementable widget factory 串起来（`LyraAccoladeHostWidget.cpp:19-164`）。置信度 H。

### TopDownArena

- `UTopDownArenaAttributeSet` 复制 BombsRemaining/Capacity/Range/MovementSpeed，使用 `REPNOTIFY_Always`，并在 base/current value 变化前统一 clamp（`TopDownArenaAttributeSet.cpp:11-80`）。置信度 H。
- 自定义 movement component 在 Walking 时读取 ASC：MovementStopped tag 返回 0，否则读取 MovementSpeed attribute，回退 Super（`TopDownArenaMovementComponent.cpp:15-34`）。这是“GAS 数据驱动传统 ActorComponent 行为”的最小示例。置信度 H。
- CameraMode 与 pickup UI data 扩展 Lyra 的 camera/pickup 抽象；模块本身几乎无启动逻辑，玩法组合主要来自 GameFeature 内容资产。源码只能确认消费接口，无法仅凭 C++ 还原全部资产配置。置信度 M。

### ShooterTests

- 地图测试通过 CQTest `TEST_CLASS_WITH_FLAGS`、`FMapTestSpawner` 与 latent `StartWhen/Then/Until`，加载 `L_ShooterTest_Basic`，取得 LyraCharacter/ASC/HealthSet 后验证伤害与治疗（`ShooterTestsMapTests.cpp:18-121`）。置信度 H。
- 输入/动画 helper 把 EnhancedInput action 注入与 animation asset 查询封装起来；network test 在 server/client PIE 分派命令，验证某一方输入在所有客户端上的动画表现（`ShooterTestsActorNetworkTests.cpp:7-115`；`Utilities/ShooterTestsNetworkComponent.h`）。置信度 H。
- 网络测试被 `ENABLE_SHOOTERTESTS_NETWORK_TEST` 宏包住，是否实际编译取决于宏定义；最终文档必须区分“测试框架存在”和“本配置已启用”。置信度 H。

## 六、LyraEditor 与 LyraExtTool

### 6.1 LyraEditor 模块

- Startup 初始化编辑器 style、监听 GameplayAbilitiesEditor 模块加载、绑定 GameplayCue 默认类/接口/路径 delegate、注册 toolbar、PIE delegate 与 ContextEffectsLibrary asset actions（`LyraEditor.cpp:204-233,275-300`）。Shutdown 对称注销（247-270）。置信度 H。
- Toolbar 的 Check Content 调 `UEditorValidator::ValidateCheckedOutContent`；Common Maps 从 `ULyraDeveloperSettings::CommonEditorMaps` 生成快捷菜单，且 PIE 时不可用（103-199）。置信度 H。
- BeginPIE 通知 EngineSubsystem `ULyraExperienceManager::OnPlayInEditorBegun`（236-241）。`ULyraEditorEngine::PreCreatePIEInstances` 可由 map 的 WorldSettings 强制 Standalone，并通知开发/平台模拟设置（`LyraEditorEngine.cpp:56-82`）。置信度 H。
- 首 tick 强制 Content Browser 显示插件目录，反映 Lyra 的 GameFeature 内容工作方式（`LyraEditorEngine.cpp:42-53`）。置信度 H。

### 6.2 Validation 主链

```text
Toolbar / ContentValidationCommandlet
  -> collect changed/deleted packages (SourceControl or P4 arguments)
  -> add referencers of deleted packages / assets affected by changed headers
  -> preload assets and promote load warnings
  -> UEditorValidatorSubsystem::ValidateAssetsWithSettings
  -> project settings validation
  -> interactive dialog or commandlet return code
```

- `ValidateCheckedOutContent` 同步刷新 source-control opened 状态，收集 checked-out/added/deleted package；`.h` 变化会扩展到受影响资产（`EditorValidator.cpp:42-103`）。置信度 H。
- `ValidatePackages` 对删除包加入 referencer，预加载时捕获 warnings/errors 并把 load warnings 重发为 errors，然后交给 EditorValidatorSubsystem（`EditorValidator.cpp:161-180,248-299`）。置信度 H。
- Load validator 在 Editor/full-validation 中把内存包复制到 `/Temp`，必要时先编译非 data-only Blueprint，以 `LOAD_ForDiff`/`LOAD_DisableCompileOnLoad` 重载副本、等待 asset compilation、替换日志中的临时路径、清理对象与 GC；Commandlet 则不做此重复加载（`EditorValidator_Load.cpp:28-184`）。置信度 H。
- Blueprint validator 会检查 compile 状态并在 full validation 下追踪引用的非 data-only Blueprint；MaterialFunction validator递归跨 redirector 找 hard-referencing materials 并加载检查（`EditorValidator_Blueprints.cpp:22+`；`EditorValidator_MaterialFunctions.cpp:22-90`）。置信度 H。
- SourceControl validator 防止已受控资产依赖“已知但未加入源控”的资产（`EditorValidator_SourceControl.cpp:21-65`）。置信度 H。
- Commandlet 支持 `P4Filter`、`P4Changelist`、`P4Opened/P4Client`、`InPath`、`OfType`、`Packages`、`MaxPackagesToLoad`，最终调用同一 `ValidatePackages`（`ContentValidationCommandlet.cpp:51-168`）。置信度 H。
- 工具文件还注册 `CheckChaosMeshCollision`、`CreateRedirectorPackage`、`DiffCollectionReferenceSupport` console commands；应独立成“编辑器诊断命令”小节，而非混进 validator（各文件约 15-80 行）。置信度 H。

### 6.3 LyraExtTool

- Module Startup/Shutdown 为空，实际能力只有 `UBPFunctionLibrary::ChangeMeshMaterials`：对每个 StaticMesh 调 `Modify()`，遍历 StaticMaterials 替换引用并 `PostEditChange()`（`BPFunctionLibrary.cpp:11-25`）。置信度 H。
- 没有 null 检查、显式 `MarkPackageDirty`、事务对象范围说明或保存动作；它适合教学展示 Editor Blueprint Library，不应包装成完善的批处理管线。置信度 H。

## 七、文档划分建议（保持原始路径）

| 原路径 | 建议文档 | 拆分理由 |
|---|---|---|
| `Plugins/CommonGame/Source/` | `docs/learn/Plugins/CommonGame/README.md`、`ui-policy-and-root-layout.md`、`async-ui-actions.md`、`messaging.md` | GameInstance/LocalPlayer 生命周期与 UI policy/layer 是两条关键链；异步 action 可合并单篇 |
| `Plugins/CommonUser/Source/CommonUser/` | `README.md`、`user-initialization.md`、`online-context-and-privileges.md`、`session-host-find-join.md` | CommonUserSubsystem 与 CommonSessionSubsystem 都足够复杂，必须分篇；OSSv1/v2 作为横切小节 |
| `Plugins/CommonLoadingScreen/Source/CommonLoadingScreen/` | `README.md`、`loading-screen-decision-loop.md`、`loading-process-interface.md` | manager 决策树是关键文件级专题；task/interface 可合并 |
| `Plugins/CommonStartupLoadingScreen/Source/CommonStartupLoadingScreen/` | `README.md` 或单篇 `startup-preload-screen.md` | 文件少但生命周期特殊，需联读 UE PreLoadScreen |
| `Plugins/UIExtension/Source/` | `README.md`、`extension-contract-and-matching.md`、`extension-point-widget.md` | 数据注册表与 widget 消费端分篇 |
| `Plugins/GameSettings/Source/` | `README.md`、`setting-object-model.md`、`registry-filter-change-tracker.md`、`dynamic-data-sources.md`、`widgets-and-navigation.md`、`edit-conditions.md` | 60 个文件，按机制目录分而不是逐文件 |
| `Plugins/GameplayMessageRouter/Source/` | `README.md`、`runtime-router.md`、`blueprint-async-node.md` | Runtime 与 UncookedOnly 编译器节点必须隔离 |
| `Plugins/ModularGameplayActors/Source/` | `README.md` 或单篇 `receiver-lifecycle.md` | 各 actor cpp 高度同构，用矩阵覆盖，不应七篇重复 |
| `Plugins/AsyncMixin/Source/` | `README.md` 或单篇 `ordered-async-loading.md` | 单一核心机制，文件级讲解合适 |
| `Plugins/PocketWorlds/Source/` | `README.md`、`pocket-level-streaming.md`、`scene-capture.md` | 流式关卡与捕获渲染是两套机制 |
| `Plugins/GameSubtitles/Source/` | `README.md`、`subtitle-display-pipeline.md`、`media-subtitles.md` | 引擎字幕显示和 Media Overlay bridge 分篇 |
| `Plugins/GameFeatures/ShooterCore/Source/` | `README.md`、`message-processors.md`、`aim-assist.md`、`tdm-spawning.md`、`accolades.md`、`world-collectable.md` | 按独立玩法子系统；AimAssist 单独深挖 |
| `Plugins/GameFeatures/TopDownArena/Source/` | `README.md`、`gas-attributes-and-movement.md`、`camera-and-pickup-ui.md` | 小型玩法示例，突出 GAS→Movement 消费链 |
| `Plugins/GameFeatures/ShooterTests/Source/` | `README.md`、`cqtest-map-tests.md`、`input-animation-helpers.md`、`multiplayer-pie-tests.md` | 测试类型与运行模型不同 |
| `Source/LyraGame/UI/` | `README.md`、`frontend-flow.md`、`hud-and-gamefeature-ui.md`、`activatable-widget-and-input.md`、`ui-subsystems.md`，其余按 `Common/`、`Foundation/`、`IndicatorSystem/`、`PerformanceStats/`、`Weapons/` 聚合 | 关键调用链单篇，其余按原目录 |
| `Source/LyraGame/Settings/` | `README.md`、`registry-composition.md`、`local-vs-shared-settings.md`，并按 `CustomSettings/`、`Screens/`、`Widgets/` 聚合 | 区分框架注册链、持久化层和具体设置类型 |
| `Source/LyraEditor/` | `README.md`、`module-and-pie-hooks.md`、`Validation/README.md`、`Validation/load-validation.md`、`Commandlets/content-validation.md`、`Utilities/editor-console-commands.md`、`Private/context-effects-editor.md` | Validation 是最大子系统；命令/资产类型扩展分别聚合 |
| `Plugins/LyraExtTool/Source/` | `README.md` | 能力极少，一篇即可，并明确局限 |

## 八、每个插件的 5 个面试追问（含答题方向）

### AsyncMixin

1. 为什么资源完成顺序和业务回调顺序要解耦？答题方向：`FAsyncStep` 与 `CurrentAsyncStep`。
2. mixin 如何做到宿主类零额外成员开销？答题方向：静态 `Loading` map 与稀疏 `FLoadingState`。
3. `[this]` 捕获为什么在这里相对安全，边界是什么？答题方向：析构移除 state、GameThread 限制。
4. `AsyncPreloadPrimaryAssetsAndBundles` 为什么需要保留 handle？答题方向：bundle residency。
5. 忘记 `StartAsyncLoading` 会怎样，为什么仍建议显式调用？答题方向：ticker next-frame 与 UI 闪烁。

### CommonGame

1. UIManager、UIPolicy、PrimaryGameLayout 各自拥有哪层职责？答题方向：策略、玩家布局生命周期、tag→stack。
2. 为什么每个 LocalPlayer 都有 root layout？答题方向：split-screen、focus/input ownership。
3. Widget 异步 push 为什么要 suspend input？答题方向：加载窗口和焦点竞态。
4. `ShouldCreateSubsystem` 为什么检查派生实现？答题方向：项目覆写基类 subsystem。
5. requested session 为什么放在 GameInstance？答题方向：跨 world travel 的生命周期。

### CommonLoadingScreen

1. 它如何避免“地图已载入但客户端还不能玩”时过早隐藏？答题方向：GameState/PC/component interface 汇总。
2. 为什么 decision loop 每帧执行而不是靠单一完成事件？答题方向：多异步来源、动态加入/移除。
3. 加载屏为何修改 world rendering、PSO cache、streaming 与 heartbeat？答题方向：性能与误报控制。
4. external loading processor 的生命周期风险是什么？答题方向：忘记 unregister 会永久阻塞。
5. dedicated server 为什么不创建 subsystem？答题方向：无 viewport，纯客户端表现。

### CommonStartupLoadingScreen

1. 它与运行期 UMG loading screen 的生命周期分界是什么？答题方向：PreLoadScreen vs GameInstanceSubsystem。
2. 为什么使用 Slate 而不是 UUserWidget？答题方向：UObject/World 尚未就绪。
3. `EngineLoadingScreen` 类型如何参与引擎调度？答题方向：FPreLoadScreenManager。
4. 为什么 Editor 中不注册该 screen？答题方向：`!GIsEditor && CanEverRender`。
5. 与 startup movie 为什么互斥？答题方向：同一启动呈现管线和模块注释。

### CommonUser

1. PlatformUser、InputDevice、LocalPlayer、NetId 为什么不能视为同一个 ID？答题方向：映射与登录阶段。
2. local play 初始化和 online login 的 RequestedPrivilege 有何差异？答题方向：CanPlay/CanPlayOnline。
3. 当前为何实际走 OSSv1，OSSv2 源码存在意味着什么？答题方向：Build.cs 宏分支。
4. QuickPlay 为什么是 find-then-join-or-host？答题方向：统一用户操作与 session fallback。
5. Join 成功为何不等价于已进入游戏？答题方向：resolve URL、beacon、ClientTravel、地图加载。

### GameplayMessageRouter

1. 它为何不是网络消息系统？答题方向：GameInstanceSubsystem、本地回调，无 replication。
2. ExactMatch 与 PartialMatch 在父 tag 传播中如何工作？答题方向：广播循环的 `bOnInitialTag`。
3. payload 类型兼容规则为何允许发送子 struct？答题方向：`StructType->IsChildOf(listener type)`。
4. 为什么广播前复制 listener 数组？答题方向：回调内注销导致容器变更。
5. Blueprint payload pointer 为何只能在 delegate 期间使用？答题方向：临时裸指针与 K2 `GetPayload` 展开。

### GameSettings

1. Registry、Collection、Value、DataSource 分别解决什么问题？答题方向：树、叶、存取适配。
2. Apply 与 Save 为什么分开？答题方向：运行期生效、持久化、StoreInitial。
3. Cancel 如何只恢复脏设置？答题方向：ChangeTracker 的 object-key map。
4. 为什么 registry 要递归注册 inner settings？答题方向：统一事件、唯一 DevName、过滤。
5. Lyra 如何同时承载 device-local 与 player-shared settings？答题方向：UGameUserSettings vs LocalPlayerSaveGame。

### GameSubtitles

1. UMG、Slate、FSubtitleManager 各自在哪一层？答题方向：包装、渲染、全局字幕源。
2. Media subtitles 如何与普通游戏字幕汇流？答题方向：`SetMovieSubtitle`。
3. 字幕格式为何放 GameInstanceSubsystem？答题方向：跨 widget 统一配置和广播。
4. `AreSubtitlesEnabled` 在哪里最终生效？答题方向：Slate handler。
5. design-time/manual subtitles 为什么不监听全局 manager？答题方向：预览隔离。

### ModularGameplayActors

1. 为什么必须显式 AddReceiver/RemoveReceiver？答题方向：ComponentManager 无法自动发现任意 actor 生命周期。
2. late-activated GameFeature 如何影响已存在 actor？答题方向：AddComponentRequest/Handler 扫描 world。
3. `GameActorReady` 与 `ExtensionAdded` 有何不同？答题方向：receiver 生命周期状态 vs handler 注册即时通知。
4. handle 释放为什么能撤销注入组件？答题方向：request refcount。
5. 为什么禁止以 AActor 为 receiver class？答题方向：全世界扫描/注入的性能和边界。

### PocketWorlds

1. 为什么把展示对象放到远处 pocket level 而不是新 World？答题方向：共享 world/rendering 与隔离布局的权衡。
2. 如何保证多个 pocket instance 不重叠？答题方向：bounds Z offset。
3. SceneCapture 为什么使用 show-only list？答题方向：只渲染目标 actor。
4. 捕获 alpha mask 为何临时替换材质并恢复？答题方向：单通道遮罩输出。
5. mip streaming 强制驻留为何只维持有限帧？答题方向：捕获清晰度与常驻内存成本。

### UIExtension

1. UI layer 与 extension point 的区别是什么？答题方向：全屏 stack vs 布局内 slot。
2. 为什么匹配除了 GameplayTag 还需要 ContextObject？答题方向：同屏多玩家/PlayerState 隔离。
3. Widget class 为什么作为 UObject data 注册？答题方向：统一 widget/data contract。
4. WorldSubsystem 粒度如何帮助 PIE 与 travel？答题方向：World 隔离与自然清理。
5. GameFeature 卸载时 UI 为什么能自动消失？答题方向：FUIExtensionHandle::Unregister→Removed callback。

### ShooterCore

1. Assist processor 为什么监听消息而不是直接改伤害代码？答题方向：派生玩法规则与核心战斗解耦。
2. GameplayMessage 适合存累计击杀事实吗？答题方向：瞬时事件、processor 自己持有投影状态。
3. AimAssist 为什么不能决定服务器命中？答题方向：客户端输入修正与权威判定边界。
4. TDM 出生点选择如何处理 claimed 与 occupied？答题方向：best/fallback 两级候选。
5. Accolade 如何在异步资源乱序完成时保持显示顺序？答题方向：Allocated/NextDisplay sequence id。

### TopDownArena

1. AttributeSet 为什么同时实现 `PreAttributeBaseChange` 和 `PreAttributeChange`？答题方向：base/current 两条修改路径统一 clamp。
2. `REPNOTIFY_Always` 对 GAS attribute 有何意义？答题方向：预测/聚合器通知一致性。
3. MovementComponent 为什么从 ASC 读速度而不复制自有字段？答题方向：统一 buff/debuff 数据源。
4. tag 停止移动与速度 attribute 设 0 有何语义差异？答题方向：状态开关 vs 数值配置。
5. 仅看 C++ 为什么无法完整理解 TopDownArena？答题方向：GameFeatureData/Experience/资产配置在 Content。

### ShooterTests

1. CQTest 的 `Do/Then` 与 `StartWhen/Until` 有何执行差异？答题方向：单 tick vs 跨 tick latent predicate。
2. 为什么地图测试通常是 EditorContext？答题方向：未 cook 的蓝图资产动态查找。
3. 多人动画测试如何区分 server/client 命令执行？答题方向：NetworkComponent 转发。
4. 为什么“源码存在测试”不代表当前 build 会运行？答题方向：WITH_AUTOMATION_TESTS 与 ENABLE 宏。
5. 这种网络动画测试最容易产生什么假阳性/假阴性？答题方向：spawn 顺序、asset 名称、timeout、PIE 拓扑。

### LyraExtTool

1. `Modify()` 与 `PostEditChange()` 分别解决什么？答题方向：事务/undo 与编辑器刷新。
2. 代码为何可能因 null mesh 崩溃？答题方向：无输入校验。
3. 为什么批量改完不一定已经持久化？答题方向：package dirty/save 边界。
4. 这个函数适合 Runtime 吗？答题方向：Editor 模块、PostEditChange。
5. 如何把它升级成生产工具？答题方向：事务、null/filter、dirty/save、slow task、结果报告。

## 九、跨插件综合追问与待补证据

1. **加载屏为何要查询 GameState、PC 和组件，而 Frontend 又单独实现 interface？** 答题方向：manager 是汇总器，业务组件拥有真实 ready 条件。
2. **GameFeature 的 AddComponents 与 Lyra AddWidgets 有什么共同生命周期协议？** 答题方向：都依赖 GameFrameworkComponentManager receiver/handle 撤销；前者注入组件，后者监听 HUD 后操作 CommonUI/UIExtension。
3. **GameplayTag 在本区域承担了哪三种不同角色？** 答题方向：消息 channel、UI layer/slot、setting action/trait；不要混为同一注册表。
4. **WorldSubsystem、GameInstanceSubsystem、LocalPlayer 数据对象如何选择？** 答题方向：UIExtension 随 world；message/user/session/loading manager 跨 travel；shared settings 按玩家。
5. **当前研究不能从 C++ 单独确认什么？** GameFeatureData/Experience 中实际启用的 action、Widget 蓝图的 layer/slot tag、Shooter/TopDown 资产组合、网络测试宏的最终构建定义。需后续通过资产 inspect 或配置索引补证，不能把可能配置写成已执行事实。

## 来源清单

| # | 来源 | 层级 | 访问日期 | 用途 |
|---|---|---|---|---|
| 1 | 当前仓库 `Plugins/**/Source`（排除 ExtractionOps）及各 `.uplugin` | T0 | 2026-08-13 | 插件职责、模块依赖、关键实现 |
| 2 | 当前仓库 `Source/LyraGame/UI`、`Source/LyraGame/Settings`、`Source/LyraGame/GameFeatures/GameFeatureAction_AddWidget.*` | T0 | 2026-08-13 | Lyra 消费链、前端流程、设置组合、UI 注入 |
| 3 | 当前仓库 `Source/LyraEditor` | T0 | 2026-08-13 | 编辑器模块、校验器、Commandlet、工具命令 |
| 4 | `D:/Software/UE_5.8/Engine/Source/Runtime/PreLoadScreen` | T0 | 2026-08-13 | 启动加载屏基类与 manager 契约 |
| 5 | `D:/Software/UE_5.8/Engine/Plugins/Runtime/ModularGameplay` | T0 | 2026-08-13 | receiver、组件请求、extension handler 的底层行为 |
| 6 | `D:/Software/UE_5.8/Engine/Plugins/Runtime/CommonUI` | T0 | 2026-08-13 | ActivatableWidget、container、输入/焦点路由底层 |
| 7 | `D:/Software/UE_5.8/Engine/Plugins/Runtime/GameFeatures` | T0 | 2026-08-13 | GameFeatureAction_AddComponents 生命周期参照 |

## 矛盾报告

- **CommonStartupLoadingScreen 两份 Build.cs 依赖不同**：根 `Source/CommonStartupLoadingScreen.Build.cs` 声明 CommonLoadingScreen，模块目录内同名 Build.cs 未声明；插件级依赖声明启用 CommonLoadingScreen。判定：记录为仓库异常，最终教程引用插件依赖与实际编译日志时再裁决，不静默合并。
- **CommonUser 同时保留 OSSv1/v2 代码但当前只编译 v1**：不是机制矛盾，而是“代码能力”与“当前构建选择”的差异。最终文档以 `COMMONUSER_OSSV1=1` 为当前事实，同时把 v2 标为备用分支。
- **普通 ReturnToMainMenu 与 Frontend hard-disconnect 的用户重置策略不同**：前者重置 user+session，后者只在 hard disconnect 重置 user、始终清 session。二者调用场景不同，不应简化成单一规则。
- 其余关键源码事实未发现 T0 冲突。

## 面试追问链预判（本维度尚需主报告联动）

| # | 追问问题 | 答题方向 |
|---|---|---|
| 1 | 如果 Experience 在前端 flow 过程中被卸载，哪些 handle/delegate 能保证清理，哪些代码存在悬挂风险？ | 联读 GameFeature deactivation、ControlFlow cancellation、Frontend component EndPlay；当前 EndPlay 为空，需要主报告做生命周期审计 |
| 2 | GameplayMessage、UIExtension、ModularGameplay extension event 都是“解耦消息”，如何选择？ | 比较 payload 类型/作用域/持久性：业务事件、UI slot 注册、actor 生命周期扩展协议 |
| 3 | 多 LocalPlayer + seamless travel 时 root layout、UIExtension context、SharedSettings 各自如何迁移？ | 联读 LocalPlayer 生命周期、WorldSubsystem 重建、GameInstanceSubsystem 保留和 policy layout 重挂载 |

