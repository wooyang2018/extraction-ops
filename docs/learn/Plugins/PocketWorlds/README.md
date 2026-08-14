# Plugins/PocketWorlds

PocketWorlds 为 LocalPlayer 在主场景远处流式加载隔离的小关卡实例，并用 SceneCapture2D 生成角色/物品预览 RenderTarget。LevelSystem 分配空间，LevelInstance 管装载/卸载，PocketCapture 管展示 Actor、ShowOnly 列表、相机和捕获。

多个实例按 Bounds 做 Z 偏移避免重叠；捕获可临时替换材质生成 Alpha mask，并在结束后恢复。纹理 mip 只在有限帧强制驻留，平衡首帧清晰度与内存。

当前 Effects 捕获路径含 `ensure(false)` TODO，不能宣称完整实现。

## 面试追问

1. 为什么不用创建全新 UWorld 做每个预览？
2. ShowOnly 列表怎样避免捕获主世界？
3. 临时替换材质必须在哪些失败路径恢复？
4. 捕获任务与 LocalPlayer 生命周期怎样绑定？

