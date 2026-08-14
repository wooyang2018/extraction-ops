# Plugins/GameplayMessageRouter

GameplayMessageSubsystem 是 GameInstance 范围、GameplayTag 分频道、UScriptStruct 类型检查的本地消息总线。广播会沿 Tag 父链通知 PartialMatch listener；ExactMatch 只命中原 Tag。广播前复制 listener 列表，允许回调内注销。

它不自带网络复制。Lyra 若需要网络转发，必须由 Actor/Component/RPC 显式桥接。消息 payload 指针只在回调期间有效，蓝图 AsyncAction/K2Node 通过动态 pin 和临时变量安全暴露结构体。

`GameplayMessageRuntime` 可进 Runtime；`GameplayMessageNodes` 依赖 BlueprintGraph/UnrealEd，是 UncookedOnly 编译器支持模块。

## 面试追问

1. 消息 Tag 父级监听怎样工作？
2. 为什么 payload 类型允许子 struct，却不允许无关 struct？
3. 广播中注销 listener 为什么不会破坏遍历？
4. 为什么它不能替代 replicated property/FastArray？

