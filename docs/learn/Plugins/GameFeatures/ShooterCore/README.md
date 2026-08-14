# Plugins/GameFeatures/ShooterCore

ShooterCore 展示具体 Shooter Feature 如何只依赖 Lyra 稳定接口：

- Assist/ElimChain/ElimStreak Processor 监听 VerbMessage 并维护玩法投影；
- AimAssist Target/Manager/InputModifier 只修正本地输入，不决定 Authority 命中；
- TDM Spawning Component 扩展出生点策略；
- Accolade Definition/HostWidget 将消息异步排队展示；
- WorldCollectable 通过 Interaction/Inventory 接口消费核心框架。

消息处理器适合派生计分/播报，不应把历史事实只存在瞬时消息里；需要迟到加入的比分仍须复制状态。AimAssist 与武器服务器验证是两条不同安全边界。

## 面试追问

1. Assist 为什么可由 MessageProcessor 实现而不侵入 DamageExecution？
2. AimAssist 为何不能证明服务器 HitResult 合法？
3. Accolade 异步资源乱序完成时如何保持展示顺序？
4. TDM 出生点如何区分最佳候选和 fallback？

