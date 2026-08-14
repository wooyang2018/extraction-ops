# Source/LyraGame/Replays

ReplaySubsystem 在平台 Trait 允许时包装录制、播放、枚举、删除和 Seek；AsyncAction_QueryReplays 为蓝图提供异步查询。实际序列化由 DemoNetDriver/NetworkReplayStreaming 完成，Lyra 主要负责产品流程和平台门控。

回放不是重新执行客户端预测历史。ASC 源码显式容忍 Replay 中 Ability 实例缺失，说明表现代码必须能从复制状态恢复，而不是依赖当时的本地瞬时对象。

## 面试追问

1. Replay 与网络包录制有何差异？
2. 为什么平台 Trait 应在入口处门控？
3. Seek 后哪些瞬时 GameplayMessage 可能不会重放，UI 如何恢复？

