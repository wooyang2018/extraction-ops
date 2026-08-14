# Source/LyraGame/Animation

`ULyraAnimInstance` 是 Lyra Gameplay 状态到 AnimBP 的适配层。它用 GameplayTagBlueprintPropertyMap 把 ASC Tag 映射到动画属性，并从 `ULyraCharacterMovementComponent` 读取 GroundInfo。

ASC 可能晚于 AnimInstance 初始化，因此存在两条收敛路径：AnimInstance 主动从 Pawn 查 ASC；ASC 在 Avatar 切换完成后主动调用 `InitializeWithAbilitySystem`。初始化必须容忍重复调用与无 ASC 的短暂状态。

Character 对 simulated proxy 复制压缩加速度，动画端可恢复远端运动表现；这与本地预测角色直接读取 MovementComponent 的数据路径不同。

## 面试追问

1. 为什么动画状态优先消费 GameplayTag，而不是直接 Cast Ability 类？
2. ASC Avatar 切换时 AnimInstance 的旧 Tag delegate 如何避免悬挂？
3. GroundInfo 为什么缓存而不在 AnimGraph 多次 Trace？

## 练习

选一个移动/能力 Tag，从 Ability 激活追踪到 AnimBP 属性变化；分别观察拥有客户端和 simulated proxy 的数据来源。

