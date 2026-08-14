# Source/LyraGame/GameModes

## 目录职责

该目录把“一局游戏是什么”拆成选择、描述、装载和运行时状态四部分。GameMode 选择 Experience；ExperienceDefinition/ActionSet 描述内容；ExperienceManagerComponent 执行异步状态机；GameState 承载可复制的全局组件。

## 文件分组

| 文件组 | 作用 |
|---|---|
| `LyraGameMode.*` | Authority 规则入口、Experience 选择、玩家生成/重启 |
| `LyraGameState.*` | 全端可见状态，挂载 Experience、Phase 等组件 |
| `LyraExperienceDefinition.*` | Experience 配方：PawnData、插件和 Actions |
| `LyraExperienceActionSet.*` | 可复用 Action/插件组合 |
| `LyraExperienceManagerComponent.*` | 复制当前 Experience，装载资产、激活插件、执行/撤销 Action |
| `LyraExperienceManager.*` | 跨 Experience 实例协调插件激活引用 |
| `LyraUserFacingExperienceDefinition.*` | 前端可展示、可请求 Session 的体验描述 |
| `LyraWorldSettings.*` | 地图侧默认 Experience 等设置入口 |
| `AsyncAction_ExperienceReady.*` | 蓝图异步等待 Experience Ready |
| `LyraBotCreationComponent.*` | Experience 就绪后按规则创建 Bot |

## 主调用链

```text
ALyraGameMode::InitGame
  -> HandleMatchAssignmentIfNotExpectingOne
  -> OnMatchAssignmentGiven
  -> ULyraExperienceManagerComponent::SetCurrentExperience
  -> StartExperienceLoad
  -> OnExperienceLoadComplete
  -> UGameFeaturesSubsystem::LoadAndActivateGameFeaturePlugin
  -> OnExperienceFullLoadCompleted
  -> GameFeatureAction::OnGameFeatureActivating
  -> OnExperienceLoaded delegates
  -> ALyraGameMode::OnExperienceLoaded
  -> RestartPlayer
```

`HandleStartingNewPlayer_Implementation` 会在 Experience 尚未 Loaded 时暂缓 `Super`，防止使用尚未确定的 PawnClass/PawnData。Experience Loaded 回调会遍历尚无 Pawn 的 Controller 并重启玩家。

## PawnData 解析

`GetPawnDataForController` 的典型优先级是 PlayerState 已指定 PawnData，其次 Experience 默认 PawnData，最后 AssetManager 默认值。服务端生成 Pawn 时，`SpawnDefaultPawnAtTransform_Implementation` 在完成生成前向 PawnExtension 写入 PawnData，保证 BeginPlay/初始化状态能看到一致数据。

## Dedicated Server 分支

GameMode 还包含专服用户登录与 Session Host 流程。它说明 Experience 装载不只是地图逻辑：专服可能先完成 CommonUser 权限初始化，再选择 UserFacingExperience 并创建在线 Session。失败时是否允许无登录启动由配置决定。

## 设计取舍

- Experience 放在 GameState Component：服务端可决定、客户端可复制、组件可参与 LoadingScreen。
- UserFacingExperience 与 ExperienceDefinition 分开：菜单展示/Session 请求数据不等于进局后必须常驻的运行时配方。
- AsyncAction 只包装 Ready 信号：它不复制状态机，也不会重新触发装载。

## 面试追问

1. 新客户端在 Experience 已 Loaded 后加入，如何避免错过一次性 delegate？
   - `CallOrRegister` 先检查当前状态，已 Loaded 则立即执行。
2. 为什么 GameMode 不能成为客户端 Experience 状态源？
   - GameMode 不复制且客户端不存在。
3. PawnClass 已由 GameMode 返回，为什么仍要在生成时设置 PawnData？
   - PawnClass 只是实体类型，AbilitySet、InputConfig、CameraMode 等仍由 PawnData 驱动。
4. Experience 插件全部 Active 后，为什么还不能立刻 RestartPlayer？
   - Actions 及其初始化回调必须先执行完成并广播 Loaded。

## 练习

为每个 Experience 来源（URL、命令行、WorldSettings、默认值）设计一个 PIE/命令行用例，记录最终 `ExperienceIdSource`，验证优先级与回退行为。

