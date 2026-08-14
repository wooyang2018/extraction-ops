# Source/LyraGame/GameFeatures

## 目录职责

该目录把 UE GameFeatures 的通用生命周期转化为 Lyra 能直接使用的注入动作。所有 Action 都应满足：按 World/Context 隔离、处理现有与未来 Actor、可逆、重复激活不泄漏。

## Action 分类

| Action | 注入目标 | 核心机制 |
|---|---|---|
| `GameFeatureAction_AddAbilities` | Actor/ASC | 扩展处理器 + Authority 授予 Ability/Attribute/AbilitySet |
| `AddInputBinding` | Pawn/Hero | 等待 BindInputsNow 扩展事件，再添加 InputConfig |
| `AddInputContextMapping` | LocalPlayer | 向 EnhancedInput 子系统加入 Mapping Context/UserSettings 注册 |
| `AddWidget` | HUD/LocalPlayer UI | 注册 UIExtension、布局和 Widget |
| `AddGameplayCuePath` | GameplayCueManager | 动态增加 Cue 扫描路径并刷新库 |
| `SplitscreenConfig` | GameViewport | 激活期间覆盖分屏策略并在撤销时恢复 |
| `WorldActionBase` | World | 过滤合适 WorldContext，为派生 Action 提供遍历入口 |
| `LyraGameFeaturePolicy` | GameFeaturesSubsystem | 项目级装载策略与 Observer 配置 |

## Actor 扩展协议

Action 不应假定 Pawn 何时生成。典型模式：

```text
OnGameFeatureActivating
  -> AddToWorld
  -> AddExtensionHandler(TargetActorClass, HandleActorExtension)
  -> HandleActorExtension(existing/future actor, EventName)
  -> Add feature

OnGameFeatureDeactivating
  -> release handler/component request handles
  -> remove grants/widgets/input
```

`AddInputBinding` 特别依赖 HeroComponent 在完成基础输入绑定后发送的 `BindInputsNow` 事件；这避免 GameFeature Action 与 Pawn 生成/复制顺序耦合。

## AddAbilities 下钻

`UGameFeatureAction_AddAbilities::AddActorAbilities` 位于 `GameFeatureAction_AddAbilities.cpp:159`：

- 只允许 Authority 授予权威能力；
- 查找 ASC，必要时通过组件请求动态添加；
- 为每个 Actor 保存 AbilitySpecHandle、AttributeSet 和 AbilitySet grant handles；
- 移除时使用同一组句柄精确撤销。

如果找不到或不能增加 ASC，会记录错误而不是把半初始化 Actor 视为成功。

## AddWidget 下钻

Widget Action 通常包含两类注入：向 UI layer 推送布局、向 Gameplay Tag 标识的扩展点注册 Widget。前者改变页面栈，后者允许宿主 HUD 决定插槽，Feature 只声明“放到哪个语义位置”。

## 设计取舍

- Context 数据不能是单一全局数组：PIE 多 World 和插件多次激活需要隔离。
- Input Mapping 与 Input Binding 分成两个 Action：物理映射上下文属于 LocalPlayer，能力 Tag 绑定依赖具体 Pawn/Hero 就绪。
- 动态组件请求必须保存句柄：释放句柄才允许管理器按引用计数移除组件。

## 面试追问

1. 为什么授予 Ability 要检查 Authority，而添加本地 Input Mapping 不需要？
2. Feature 停用时如果不移除 UIExtension handle，会出现什么跨局残留？
3. 一个 Actor 同时被两个 Feature 请求同类组件，先停用一个时能否销毁组件？
4. 为什么 WorldActionBase 要筛选 WorldContext，而不是遍历 `GEngine->GetWorldContexts()` 后无条件执行？

## 练习

为现有 Action 建立“创建资源—持有句柄—撤销资源”表，逐项检查 `OnGameFeatureDeactivating` 是否对称。用两次连续激活/停用验证没有重复输入和 Widget。

