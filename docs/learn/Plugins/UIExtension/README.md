# Plugins/UIExtension

UIExtensionSubsystem 是 WorldSubsystem。Feature 注册 Extension（内容），宿主 Widget 注册 ExtensionPoint（插槽）；两者按 GameplayTag、ContextObject 和 DataClass/Interface contract 匹配。PartialMatch 可沿父 Tag 匹配，ExactMatch 只接受原 Tag。

ExtensionPointWidget 可为无 Context、LocalPlayer、PlayerState 分别注册插槽，适应分屏和玩家状态晚到。Handle 注销会发 Removed 回调并删除 Widget；Subsystem 还为 shared struct 内 UObject 数据补充 GC 引用。

UI layer 解决全屏页面栈，extension point 解决布局内插件插槽；Lyra 的 AddWidget Action 同时消费两者。

## 面试追问

1. 为什么 Tag 匹配之外还需要 ContextObject？
2. WorldSubsystem 粒度如何帮助 PIE 与 Travel？
3. Widget class 为什么也可作为统一 Data contract 注册？
4. 插件卸载时 UI 自动消失依赖哪个 handle 语义？

