# Source/LyraGame/Messages

`FLyraVerbMessage` 是 Lyra 跨系统事件的通用信封：Verb Tag、Instigator、Target、数值、上下文 Tags。Helpers 在 GameplayCue 参数和 VerbMessage 间转换；ReplicationComponent 可将需要网络传播的消息从 Authority 转发；Processor 提供观察/聚合基础。

消息适合 Damage、Elimination、Inventory delta、QuickBar change 等瞬时通知，但不应替代需要迟到加入恢复的属性、FastArray 或 Actor 状态。消费方应能在漏掉旧消息时从权威复制状态重新构建 UI。

## 面试追问

1. GameplayMessageRouter 是 Event Bus，如何避免把它误用成状态仓库？
2. UObject 指针放入消息跨网络时有什么 relevancy/生命周期风险？
3. 哪类消息需要 ReplicationComponent，哪类只应本地广播？

## 练习

为 Damage、ActiveSlot 与 MatchPhase 分别判断应该使用消息、复制状态还是两者组合，并解释迟到加入行为。

