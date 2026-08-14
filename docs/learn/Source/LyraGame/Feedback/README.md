# Source/LyraGame/Feedback

## Context Effects

动画 Notify 或接口产生 MotionEffect Tag；ContextEffectComponent 聚合角色 Context，并从 Trace 命中的 PhysicalMaterial 推导 Surface；WorldSubsystem 查询已加载 Library，生成 Sound/Niagara。它用 Tag+材质把动画事件与具体资产解耦。

## Number Pops

NumberPopComponent 是客户端表现接口，MeshText/NiagaraText 是不同策略；DamagePopStyle 决定颜色、合并等视觉规则。它消费伤害事件但不修改 Health。具体消息订阅可能位于资产/蓝图，因本研究排除 Content，不能断言所有闭环都在 C++ 完成。

## 面试追问

1. 为什么脚步声同时需要动作 Tag、角色 Context 和物理表面？
2. ContextEffectsSubsystem 为何是 WorldSubsystem？
3. 伤害数字为什么不能作为命中/伤害权威证据？

## 练习

从 AnimNotify 开始画出脚步效果查询键，并设计 Library 尚未异步加载完成时的降级行为。

