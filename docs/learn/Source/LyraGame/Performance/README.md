# Source/LyraGame/Performance

`ULyraPerformanceStatSubsystem` 是 LocalPlayer 诊断聚合器，每帧收集 Client FPS、Game/Render/GPU 时间、Server FPS、Ping、包丢失/速率/大小与可选 latency marker，并可写入 CSV profiler。Settings/Types 定义可展示指标和平台允许项；MemoryDebugCommands 提供开发期诊断。

指标来源不同：Server FPS 来自复制 GameState，Ping/包统计来自 PlayerState/NetConnection，本地帧时间来自 Engine counters。展示层必须处理某来源不可用，不能把零一概解释为性能优秀。

## 面试追问

1. Client FPS 与 GameThread ms 为什么不能互相简单换算？
2. UE Ping 的压缩/平滑可能怎样影响 UI 数值？
3. LocalPlayerSubsystem 为什么适合分屏下的网络统计展示？

