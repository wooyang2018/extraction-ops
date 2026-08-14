# Source/LyraGame/System

## 系统层地图

| 文件组 | 职责 |
|---|---|
| `LyraGameEngine` | Engine 生命周期与项目级故障入口 |
| `LyraGameInstance` / `LyraGameSession` | 跨 World 用户、Session、旅行和网络加密 |
| `LyraAssetManager` / `StartupJob` / `LyraGameData` | Primary Asset、启动任务、全局软引用数据 |
| `LyraReplicationGraph*` | Actor 复制路由、连接分组与频率策略 |
| `LyraSignificanceManager` | 重要性计算与表现/更新预算 |
| `GameplayTagStack` | Tag→Count 的可复制/可查询栈结构 |
| `LyraActorUtilities` / `SystemStatics` / `DevelopmentStatics` | 边界清晰的工具函数 |

## AssetManager 启动

AssetManager 是 Lyra 数据驱动架构的根：它装载 GameData、默认 PawnData、GameplayCue 等启动依赖，并用 StartupJob 记录进度。同步 getter 通常假定启动依赖已经完成；在过早生命周期调用会触发检查或同步加载代价。

## ReplicationGraph

ReplicationGraph 不改变 Authority，只改变服务器如何为每条连接筛选与调度 Actor。Lyra 自定义节点/频率桶、PlayerState limiter 和 Team/连接相关策略，目标是让多人项目的复制成本不随 Actor 数量简单线性增长。阅读时先标出 AlwaysRelevant、空间化、连接专属和动态频率四类。

## TagStack

TagStack 表达带计数语义的状态/资源，区别于只判断存在性的 TagContainer。调用方必须决定零计数是否删除、修改是否只在 Authority、是否需要复制；它不是 GAS OwnedTags 的自动替代。

## 面试追问

1. AssetManager PrimaryAsset 与普通 SoftObjectPtr 的关系是什么？
2. ReplicationGraph 优化的是带宽、CPU 还是一致性语义？
3. Significance 为什么不应决定权威 Actor 是否存在？
4. GameInstance 为什么适合 Session，却不适合保存某局 GameState？

## 练习

选择一个 Pawn、PlayerState、Pickup 和瞬时效果 Actor，说明它们应进入哪种 ReplicationGraph 节点以及迟到加入行为。

