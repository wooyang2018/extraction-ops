# Source/LyraGame/AbilitySystem

## 目录职责

该目录是在 UE GAS 上建立的 Lyra 玩法语言：AbilitySet 负责批量授予，LyraASC 负责 Tag 输入与激活组，LyraGameplayAbility 定义激活策略/镜头/失败消息，AttributeSet 与 Execution 定义数值，GamePhase 用能力生命周期表达全局阶段。

## AbilitySet：授予事务

`ULyraAbilitySet` 将 GameplayAbility、GameplayEffect 和 AttributeSet 组合为数据资产。`GiveToAbilitySystem` 只应由 Authority 调用，并把产生的 AbilitySpecHandle、ActiveEffectHandle、AttributeSet 指针写入 `FLyraAbilitySet_GrantedHandles`。撤销时由同一句柄集合 `TakeFromAbilitySystem`，形成对称事务。

它与 GameFeatureAction_AddAbilities 的区别：AbilitySet 描述“授予什么”，Action 决定“何时、给哪类 Actor 授予”。

## 输入处理

`ULyraAbilitySystemComponent::AbilityInputTagPressed/Released` 不立即无条件激活，而是更新三个 SpecHandle 集合：Pressed、Released、Held。`ProcessAbilityInput`（`LyraAbilitySystemComponent.cpp:216`）按激活策略处理：

- `OnInputTriggered`：本帧按下时尝试激活；
- `WhileInputActive`：保持按下时尝试激活；
- 已激活 Ability 收到 replicated InputPressed/InputReleased 事件；
- `Gameplay.AbilityInputBlocked` 存在时清空输入。

先收集再激活可以避免遍历 AbilityList 时结构变化，并把多种输入策略放在同一帧边界处理。

## 激活组

| 组 | 语义 |
|---|---|
| Independent | 可与其他能力并行 |
| Exclusive_Replaceable | 独占，但可被新的独占能力取消 |
| Exclusive_Blocking | 独占，并阻止其他独占能力进入 |

ASC 用计数维护运行组，并在激活/结束通知中加减。激活组不是 Gameplay Tag block/cancel 的替代：前者表达并发策略，后者表达能力类别关系。

## TagRelationshipMapping

映射表允许数据定义某类 Ability Tag 额外阻断/取消哪些 Tag，以及需要/禁止哪些 Owner Tag。LyraASC 覆盖 GAS 查询入口，把映射结果合并进 Ability 自身声明，避免每个 Ability 重复硬编码关系。

## LyraGameplayAbility

基类增加：

- ActivationPolicy 与 ActivationGroup；
- OnSpawn 自动激活；
- 从 ActorInfo 获取 Lyra Character/Controller/PlayerState/ASC；
- AbilitySource 与 EffectContext 扩展；
- CameraMode 设置与清理；
- 失败 Tag 到本地化消息/Montage 的映射；
- AbilityCost 的可扩展检查和扣除。

Cost 分成 InventoryItem、ItemTagStack、PlayerTagStack，展示“能力成本不等于 GameplayEffect 数值消耗”：它可以消费外部库存或栈结构，但必须分别实现 CheckCost/ApplyCost，并考虑预测与权威校验。

## 属性与 Execution

`ULyraHealthSet` 持有 Health/MaxHealth/Healing/Shield/Damage 等聚合入口；`ULyraCombatSet` 持有 BaseDamage/BaseHeal。Damage/Heal Execution 从 Spec、Source/Target Tags 和 SetByCaller/捕获属性计算最终 Modifier。

原则：Execution 负责数值变换，HealthComponent 负责生命状态语义，Death Ability 负责死亡流程。不要在一个 Execution 中直接销毁 Pawn。

## EffectContext 与 TargetData

`FLyraGameplayEffectContext` 扩展标准 EffectContext，携带 AbilitySource 等 Lyra 信息，并实现复制/重复所需协议。`FLyraGameplayAbilityTargetData_SingleTargetHit` 扩展命中目标数据。自定义 struct 若遗漏 NetSerialize、WithNetSerializer traits 或 Duplicate 深拷贝，会导致预测端和服务端看到不同上下文。

## GamePhase

`ULyraGamePhaseSubsystem` 在 Authority 上通过激活 `ULyraGamePhaseAbility` 启动阶段，以 Gameplay Tag 的层级匹配决定观察者与互斥。阶段结束与 Ability End 绑定，因此取消、异常结束也能统一清理。

Subsystem 的 `ActivePhaseMap` 本身不是直接复制状态。客户端若需要阶段表现，依赖 Ability/GAS 状态、Gameplay Tags、消息或玩法另行复制的数据；不要把服务端 WorldSubsystem 容器误认为全端共享状态。

## GlobalAbilitySystem 与 GameplayCue

GlobalAbilitySystem 维护已注册 ASC，可向所有目标批量应用 Ability/Effect；ASC Begin/EndPlay 必须正确注册注销。LyraGameplayCueManager 定制 Cue 库装载与预加载策略，GameFeature Action 可动态添加 Cue 路径。

## UE 5.8 对照

GAS 底层位于 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/`。重点阅读 `AbilitySystemComponent.cpp` 的 ActorInfo、GiveAbility/TryActivateAbility、预测键和 replicated event，再回看 Lyra 覆盖点；否则容易把 Engine 能力误认为 Lyra 自创。

## 面试追问

1. AbilitySpec 是定义、实例还是授权记录？
2. 为什么输入按下事件对已激活 Ability 仍要通过 replicated event 传播？
3. AbilitySet 撤销时为何必须保存句柄而不能按类搜索删除？
4. Exclusive_Blocking 与 BlockTags 的差异是什么？
5. 自定义 EffectContext 在网络下必须实现哪些协议？
6. Cost 消耗库存时如何避免客户端预测造成永久错扣？

## 练习

实现一个 `WhileInputActive` 能力：按住期间激活、松开结束；加入 Exclusive_Replaceable 组，再用 Blocking 能力抢占。分别记录预测端与 Authority 的 Spec 状态和 replicated input event。
