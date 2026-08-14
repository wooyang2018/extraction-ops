# Experience 与 GameFeature 编排

## Experience 的数据模型

`ULyraExperienceDefinition` 是 Primary Data Asset 风格的玩法配方，核心字段包括默认 PawnData、要启用的 GameFeature 名称、直接 Actions 和 ActionSets。`ULyraExperienceActionSet` 让多个 Experience 复用一组插件与 Action，但自身不是独立运行时状态机。

## Experience 选择优先级

`ALyraGameMode::HandleMatchAssignmentIfNotExpectingOne` 会从 URL/options、命令行、WorldSettings、开发者设置和默认配置等来源决定 Experience。关键设计不是具体优先级，而是最终统一得到 `FPrimaryAssetId`，再只通过 `OnMatchAssignmentGiven` 交给 GameState 上的 ExperienceManagerComponent。

这样做有两个结果：

- Authority 是 Experience 选择的唯一真相源；
- 客户端通过复制 `CurrentExperience` 进入相同加载流程，而不是自行重新解释 URL 和配置。

## 加载状态机

`ULyraExperienceManagerComponent` 的核心状态为 `Unloaded → Loading → LoadingGameFeatures → ExecutingActions → Loaded`。主链位于：

- `SetCurrentExperience`：`LyraExperienceManagerComponent.cpp:56`
- `StartExperienceLoad`：`:123`
- `OnExperienceLoadComplete`：`:214`
- `LoadAndActivateGameFeaturePlugin`：`:269`
- `OnExperienceFullLoadCompleted`：`:289`
- Action 激活：`:336` 附近

首先用 AssetManager 加载 Experience 与 ActionSet 的 bundle，再收集插件 URL。所有插件回调完成后，创建 `FGameFeatureActivatingContext`，依次调用 Actions。最后广播高、普通、低三个优先级的 Loaded delegate。

这里有两个值得面试时说清的实现细节：`SetCurrentExperience` 为取得定义类会先 `TryLoad`，这一小步是同步的；随后 bundle 与插件装载才是主要异步阶段。另外当前 `OnGameFeaturePluginLoadComplete` 会推进完成计数，但没有根据失败 `FResult` 建立完整降级/中止策略。因此“回调都返回”不严格等于“所有插件都成功激活”，生产项目应定义失败策略。

优先级 delegate 是初始化排序工具，不是线程优先级：需要先建立基础状态的系统注册 High，普通消费者注册默认，UI 等后置消费者可注册 Low。

## Action 为什么必须可撤销

GameFeature 可能因 World 结束或插件停用而撤销。Action 不能只“Add”不记录句柄；它通常为每个 `FGameFeatureStateChangeContext` 保存 `FPerContextData`，其中包含组件请求句柄、扩展处理器句柄或已授予能力句柄。

以 `UGameFeatureAction_AddAbilities` 为例：

1. `AddToWorld` 注册目标 Actor 类的扩展处理器；
2. `HandleActorExtension` 响应 Actor Ready/Removed 或扩展事件；
3. `AddActorAbilities` 在 Authority 上向 ASC 授予 ability、attribute set、ability set；
4. `Reset`/`RemoveActorAbilities` 撤销授予并释放组件请求。

## UE 5.8 底层

- `UGameFeaturesSubsystem`：`Engine/Plugins/Runtime/GameFeatures/Source/GameFeatures/Private/GameFeaturesSubsystem.cpp`
- `UGameFrameworkComponentManager`：`Engine/Plugins/Runtime/ModularGameplay/Source/ModularGameplay/Public/Components/GameFrameworkComponentManager.h`

前者管理插件状态机和协议 URL；后者管理 Actor 类接收者、组件请求、扩展事件和 Init State。Lyra 的 Action 是这两个底层系统之间的适配层。

## 常见误解

- **“插件 Active 就等于 Experience Loaded”**：错误；所有插件完成后还要执行 Actions，并可能有测试延迟。
- **“客户端也可以设置 Experience”**：错误；`CurrentExperience` 由服务端设置并复制，客户端 `OnRep_CurrentExperience` 只启动本地装载。
- **“Action 只处理激活时已存在的 Actor”**：错误；组件管理器的扩展处理器覆盖后续 Actor。
- **“Experience 全程都是异步加载”**：不准确；取得定义类存在同步 `TryLoad`，bundle/插件阶段才构成主要异步链。

## 面试追问

1. 某个 GameFeature 插件加载失败时，LoadingScreen 为什么可能一直存在？应该在哪层决定降级？
2. 两个 Experience 同时引用同一插件时，为什么需要 `ULyraExperienceManager` 记录激活引用？
3. Action 异步撤销尚未完全结束时直接卸载插件会发生什么？
4. 为什么 `CurrentExperience` 复制的是定义对象，而不是复制全部 Action 执行结果？

## 练习

实现一个只记录日志的 GameFeatureAction：为目标 Pawn 注册扩展处理器，记录 Added/Removed/Ready，并确保停用后没有残留 delegate。重点验证第二次激活不会重复注册。
