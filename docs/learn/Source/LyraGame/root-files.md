# Source/LyraGame 根文件

- `LyraGame.Build.cs`：模块依赖、Shipping 条件编译、GameplayDebugger/Iris 支持。
- `LyraGameModule.cpp`：Primary Game Module，保持薄启动层。
- `LyraGameplayTags.*`：集中声明原生 Gameplay Tags，包含 InitState 等跨系统协议。
- `LyraLogChannels.*`：按系统划分日志通道，支持运行时过滤。

原生 Tag 是 C++ 代码引用的稳定协议；数据驱动 Tag 可来自配置/资产。重命名跨系统 Tag 会影响初始化、输入、能力、消息和 UI，应像 API 变更一样审查。

## 面试追问

1. Native Gameplay Tag 相比运行时 RequestGameplayTag 有什么初始化/重构优势？
2. 为什么主模块越薄越有利于 PIE 和自动化？
3. PublicDependency 与 PrivateDependency 的错误划分会造成什么编译耦合？

