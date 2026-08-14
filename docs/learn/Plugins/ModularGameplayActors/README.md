# Plugins/ModularGameplayActors

该插件为 GameMode、GameState、Controller、PlayerState、Pawn、Character、AIController 提供同构基类：在合适 Actor 生命周期调用 `UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver`，结束时 RemoveReceiver，并发送 GameActorReady 等扩展事件。

这层桥接使晚激活 GameFeature 能扫描已存在 receiver，也能影响未来 Actor。动态组件请求用 handle/引用计数撤销；以过宽的 AActor 作为 receiver class 会造成全世界扫描与边界失控。

## 面试追问

1. 为什么 Manager 不能自动可靠发现任意 Actor？
2. ExtensionAdded 与 GameActorReady 有何差异？
3. 两个 Feature 请求同一组件时如何决定销毁？
4. RemoveReceiver 遗漏会产生哪些 World teardown 问题？

