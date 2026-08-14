# 构建、Target 与启动链

## Target 不是运行时 GameMode

`Source/*.Target.cs` 决定生成哪种 UE Target、默认构建设置以及额外插件组合。Client、Server、Editor 和带 Steam/EOS 后缀的 Target 是编译/打包入口；它们不承担运行时玩法选择。

`Source/LyraGame/LyraGame.Build.cs` 则定义 LyraGame 模块的编译依赖。Public 依赖会进入使用者的编译接口，Private 依赖仅供实现使用。这里能直接看出 Lyra 的架构骨架：GameplayAbilities、GameplayTags、GameFeatures、ModularGameplay、ReplicationGraph、CommonLoadingScreen、AsyncMixin。

## 启动主链

1. `IMPLEMENT_PRIMARY_GAME_MODULE` 注册 `FLyraGameModule`。
2. 项目配置选择 `ULyraGameEngine`、`ULyraGameInstance` 和 `ULyraAssetManager`。
3. Engine 初始化 GameInstance；GameInstance 建立 CommonUser、Session 和全局消息/能力等 Subsystem 所需环境。
4. World 装载并由 `ALyraGameMode::InitGame` 在下一 Tick 决定 Experience。
5. Experience 完成前，LoadingScreen 接口持续报告阻塞原因。

延后一 Tick 很重要：`ALyraGameMode::InitGame` 在 `Source/LyraGame/GameModes/LyraGameMode.cpp:80` 调用 `SetTimerForNextTick`，避免在 World/组件尚未完成基础初始化时立刻触发资产和在线流程。

## Module 生命周期中的边界

`FLyraGameModule` 很薄是有意为之。全局业务若塞进模块 Startup/Shutdown，会绕过 GameInstance/Subsystem 的 World 与 PIE 生命周期。模块适合注册类型、控制台命令或进程级设施；游戏状态应进入 Subsystem、GameState Component 或 Actor。

## Shipping 条件编译

`LyraGame.Build.cs` 显式在 Shipping 关闭 External RPC/HTTP listeners 与 Automation Driver，并定义 `SHIPPING_DRAW_DEBUG_ERROR=1`。这展示了两层安全边界：

- 编译期移除不应进入 Shipping 的调试攻击面；
- 不依赖“运行时不开启”来保护测试接口。

## Engine 对照

- `UGameEngine::Init`/GameInstance 建立位于 `Engine/Source/Runtime/Engine`。
- Target 与 ModuleRules 由 `Engine/Source/Programs/UnrealBuildTool` 解释。
- GameMode 的 World 初始化入口最终建立在 `UWorld::InitializeActorsForPlay` 与 Gameplay Framework 的 Actor 生命周期上。

## 面试追问

1. `Target.cs`、`.Build.cs`、`.uplugin` 分别控制什么？
2. 为什么 LyraGame 的很多 UI/设置依赖是 Private？
3. 为什么不要在 Primary Game Module 中保存某个 PIE World 的状态？
4. Shipping 只靠 CVar 关闭调试服务是否足够？

## 练习

比较 `LyraGame.Target.cs`、`LyraClient.Target.cs`、`LyraServer.Target.cs` 和 Steam/EOS 变体，列出每个差异最终影响“编译进什么”还是“运行时选择什么”。

