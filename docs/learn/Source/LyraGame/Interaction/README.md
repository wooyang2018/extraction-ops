# Source/LyraGame/Interaction

## 发现与执行分离

`AbilityTask_GrantNearbyInteraction` 在 Authority 周期性 Overlap，为附近目标提供的交互 Ability 动态授予 Spec；`AbilityTask_WaitForInteractableTargets` 在本地/能力上下文中 Trace，调用 `IInteractableTarget::GatherInteractionOptions` 并用 `CanActivateAbility` 过滤；`ULyraGameplayAbility_Interact` 选择 Option 后以 GameplayEvent 触发目标 ASC 上的 AbilitySpec。

这个设计允许门、终端、拾取物等目标拥有自己的执行逻辑，交互者只负责查询与意图。

## 数据契约

- `FInteractionQuery`：谁在查询及其上下文。
- `FInteractionOption`：目标、Ability、Widget/文本等候选数据。
- `IInteractableTarget`：目标提供候选和自定义目标数据。
- `IInteractionInstigator`：交互发起者扩展接口。
- Duration Message：把持续时间旁路通知 UI，不承担 Authority 状态。

具体 Ability 的 NetExecutionPolicy 往往由资产类决定；只读 C++ 范围不能统一断言所有交互都采用相同预测策略。

## 面试追问

1. 为什么交互能力可能授予交互者，却在目标 ASC 上触发？
2. Nearby overlap 与视线 trace 分别解决什么？
3. Option 列表怎样避免显示实际不可激活的交互？
4. 持续交互中目标被销毁或距离超限时应由谁取消？

## 练习

画出一个有 2 秒持续时间的服务器权威交互：候选发现、UI 进度、开始、取消、完成以及目标销毁各由哪个对象持有状态。

