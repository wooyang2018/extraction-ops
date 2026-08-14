# Pawn 初始化、所有权与网络边界

## 为什么 Lyra 需要初始化状态机

一个玩家 Pawn 的关键依赖不会同时到达：服务端先有 Controller/PlayerState，客户端可能先收到 Pawn、稍后复制 PlayerState/PawnData；GameFeature 还可能晚于 Actor 加载。Lyra 将这些不确定顺序归一为可重复检查的状态跃迁。

## 两个核心组件

### ULyraPawnExtensionComponent

它是 Pawn 初始化的协调者，持有复制的 `PawnData`，并负责把 PlayerState 上的 `ULyraAbilitySystemComponent` 绑定到当前 Pawn 作为 Avatar。

关键入口：

- `BeginPlay` 注册 Init State 并进入 Spawned：`LyraPawnExtensionComponent.cpp:56`
- `SetPawnData`：`:76`
- `InitializeAbilitySystem`：`:105`
- `UninitializeAbilitySystem`：`:152`
- `CanChangeInitState`：`:224`

### ULyraHeroComponent

它消费 PawnExtension 已建立的数据和 ASC，负责本地玩家输入、默认 CameraMode 以及 Hero 特有的初始化条件。`CanChangeInitState` 位于 `LyraHeroComponent.cpp:76`，输入初始化位于 `:225`。

## 状态条件

| 目标状态 | 典型条件 |
|---|---|
| Spawned | 组件已 BeginPlay |
| DataAvailable | PawnData 有效；需要的 Controller/PlayerState 已就绪；本地 Pawn 已有 InputComponent |
| DataInitialized | 其他 Feature 已到 DataAvailable；ASC ActorInfo、输入与数据已经绑定 |
| GameplayReady | 各 Feature 完成 DataInitialized，可开始正常玩法 |

每个 Feature 可以通过 `BindOnActorInitStateChanged` 观察其他 Feature，并在依赖变化时再次 `CheckDefaultInitialization`。跃迁函数必须幂等，因为复制、Possess 和组件事件都可能触发检查。

## ASC 的 Owner 与 Avatar

Lyra 的玩家 ASC 通常由 `ALyraPlayerState` 持有：

- OwnerActor：PlayerState，跨 Pawn 重生稳定并参与复制；
- AvatarActor：当前 Pawn，提供身体、移动和动画上下文。

`InitializeAbilitySystem` 会检测 ASC 是否仍指向旧 Avatar；若旧 Avatar 仍有 Authority 是不变量破坏，若是客户端的复制时序则先让旧 Pawn 解除绑定。`UninitializeAbilitySystem` 清除 Avatar、Input、GameplayCue 和组件引用，防止旧 Pawn 继续响应能力。

## 网络角色分工

| 行为 | Authority | 拥有客户端 | 模拟端 |
|---|---:|---:|---:|
| 选择 Experience/PawnData | 是 | 接收复制 | 接收复制 |
| 授予权威 AbilitySet | 是 | 接收/预测 Spec | 接收必要状态 |
| 绑定玩家输入 | 否 | 是 | 否 |
| 执行预测能力 | 校验 | 发起预测 | 通常只表现 |
| 生成/销毁权威装备 | 是 | 接收 FastArray | 接收 FastArray |

## UE 5.8 底层

`UGameFrameworkComponentManager` 的 Init State 不是 Actor 生命周期的替代品，而是建立在 Actor/Component BeginPlay 之上的跨 Feature 协议。ASC 的预测、AbilitySpec 与 ActorInfo 则由 `Engine/Plugins/Runtime/GameplayAbilities/.../AbilitySystemComponent.cpp` 提供。

## 面试追问

1. 为什么不能把 ASC 永久放在 Character 上？什么类型的 Pawn 反而适合这么做？
2. `OnRep_PlayerState`、PossessedBy、SetupPlayerInputComponent 顺序不一致时，哪个组件负责收敛？
3. 旧 Pawn 未清理 ASC Avatar 会产生哪些幽灵输入或 GameplayCue 问题？
4. Init State 的检查函数为什么不能带不可重复的副作用？

## 练习

在 Dedicated Server + 两客户端 PIE 中记录 PawnExtension/Hero 的每次状态跃迁，分别执行首次加入、死亡重生和断线重连；比较 Authority、拥有客户端和模拟端的事件顺序。

