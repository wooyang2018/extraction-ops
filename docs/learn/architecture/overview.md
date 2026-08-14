# Lyra 架构总览：从静态模块到运行时体验

## 核心结论

Lyra 的核心架构选择是把“一个游戏模式需要什么”从硬编码类层次移到数据驱动的 Experience，再用 GameFeature Action 把功能动态注入世界。它因此同时解决了三个问题：不同玩法共享宿主框架、功能可按体验启停、Actor 初始化可以等待异步依赖和网络复制。

不要把以下概念混为一谈：

| 概念 | 所有者 | 生命周期 | 主要职责 |
|---|---|---|---|
| Module | 进程 | DLL/进程级 | 编译与启动注册 |
| GameInstance | 进程中的游戏实例 | 跨 World | 用户、Session、全局 Subsystem |
| World/GameMode | World/服务端 | 地图级 | 规则、玩家生成、Experience 选择 |
| Experience | GameState 组件复制其定义 | 一局/一次 World 体验 | 描述需要加载的功能组合 |
| GameFeature plugin | GameFeaturesSubsystem | 可跨 World 激活 | 提供可启停功能包 |
| GameFeatureAction | Experience/GameFeature 激活上下文 | 激活区间 | 注入能力、组件、输入、UI 等 |
| PawnData | Pawn 初始化链 | Pawn/玩家配置级 | Pawn 类、AbilitySet、InputConfig、CameraMode |

## 三个控制平面

### 1. 资产控制平面

`ULyraAssetManager` 负责启动作业、GameData 和 Primary Asset；ExperienceManager 使用 Asset Bundle 收集 Experience、ActionSet 与其依赖。软引用让宿主代码不必静态依赖所有玩法内容。

### 2. 功能控制平面

`ULyraExperienceManagerComponent` 将插件 URL 交给 `UGameFeaturesSubsystem::LoadAndActivateGameFeaturePlugin`。插件进入 Active 后，其 Action 才能修改 World。卸载时必须按相反顺序撤销 Action 和插件引用。

### 3. Actor 初始化控制平面

`ULyraPawnExtensionComponent` 与 `ULyraHeroComponent` 不假设 `BeginPlay` 时所有依赖已存在，而是加入 `UGameFrameworkComponentManager` 的 Init State 链：

```text
InitState.Spawned
  -> InitState.DataAvailable
  -> InitState.DataInitialized
  -> InitState.GameplayReady
```

状态跃迁同时受 Controller、PlayerState、PawnData、ASC、InputComponent 和本地玩家子系统约束。这是 Lyra 处理“服务端生成顺序、客户端复制顺序、GameFeature 注入顺序可能不同”的关键。

## 依赖方向

理想依赖方向是插件能力指向稳定宿主接口，而不是宿主枚举具体玩法：

```text
ShooterCore / TopDownArena
        ↓ GameFeature actions and data
LyraGame runtime framework
        ↓ public plugin APIs
CommonGame / CommonUser / GameSettings / UIExtension / GameplayMessageRouter
        ↓
UE Engine modules and Engine plugins
```

如果在 `LyraGame` 中直接引用某个具体 Shooter 类，通常意味着模块边界被反转。Lyra 更偏好 Gameplay Tag、接口、消息或 GameFeature Action。

## 面试追问

1. **为什么 Experience 不直接等同于 GameMode？**
   - GameMode 仅存在于 Authority，且类选择发生得早；Experience 是可复制、可异步装载、可组合 ActionSet/GameFeature 的玩法描述。
2. **GameFeature 已激活后才生成的 Actor 怎样获得组件？**
   - Action 向 `UGameFrameworkComponentManager` 注册扩展处理器；管理器既处理现有 Actor，也监听后续 Actor 的扩展事件。
3. **为什么初始化状态优于在 BeginPlay 中反复判空？**
   - 它显式表达依赖和幂等跃迁，并能被其他 Feature 监听；判空重试没有统一所有者，也难以处理撤销。
4. **PlayerState-owned ASC 有什么网络收益和代价？**
   - Pawn 重生时能力状态可保留，Owner 在服务端和拥有客户端稳定；代价是必须在 Pawn 切换时可靠更新 AvatarActor 并清理旧 Pawn。

## 练习

从 `ALyraGameMode::InitGame` 开始，只用符号跳转画出“地图启动到本地玩家输入绑定完成”的调用图。每个节点标注运行端、状态所有者和可能异步的边。

