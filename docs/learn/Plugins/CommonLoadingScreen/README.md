# Plugins/CommonLoadingScreen 与 CommonStartupLoadingScreen

## 三层加载屏

1. `CommonStartupLoadingScreen`：PreEarlyLoadingScreen 阶段注册 Slate `EngineLoadingScreen`；此时没有可依赖的 GameInstance/World/UMG。
2. `CommonLoadingScreen`：GameInstanceSubsystem 每帧汇总 World、Travel、GameState、PlayerController、组件与 external processor 的阻塞理由。
3. 业务阻塞者：ExperienceManager、FrontendStateComponent 等实现 `ILoadingProcessInterface`，报告自己的 Ready 条件。

运行期 Manager 不只是盖一个 Widget：显示时拦截输入、关闭 World rendering、调整 PSO cache/streaming 与 heartbeat；隐藏时恢复并做清理。显式 LoadingProcessTask 若忘记 Unregister，会永久阻塞。

## 仓库异常

`CommonStartupLoadingScreen` 存在两份同名 Build.cs，依赖列表不同；插件描述声明依赖 CommonLoadingScreen。学习时记录为构建布局异常，不把两份文件静默合并。

## 面试追问

1. 为什么启动加载屏必须用 Slate 而不是 UMG？
2. 为什么运行时 Loading 判断适合逐帧汇总而不是单一完成事件？
3. Experience Loaded 与玩家可操作之间还可能有哪些阻塞者？
4. External task 生命周期怎样避免永久黑屏？

