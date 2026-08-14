# Plugins/LyraExtTool

该 Editor 插件很薄：Blueprint Function Library 批量替换 StaticMesh 的 StaticMaterials，调用 `Modify()` 支持事务/Undo，再 `PostEditChange()` 刷新编辑器。

当前实现没有完整 null 检查、SlowTask、结果报告、显式保存或批量失败恢复，适合作为 Editor Blueprint Library 示例，不是生产级资产管线。`Modify` 不等于 Package 已持久化保存。

## 面试追问

1. Modify 与 PostEditChange 分别解决什么？
2. MarkPackageDirty/SavePackage 边界在哪里？
3. 如何为大批量操作加入事务、进度和部分失败报告？

