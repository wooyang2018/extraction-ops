# Source/LyraGame/Hotfix

`ULyraHotfixManager` 扩展 Engine Hotfix 流程；`ULyraRuntimeOptions` 暴露可热更新/命令行覆盖的运行参数；`ULyraTextHotfixConfig` 处理文本修补配置。

热修复适合数据和配置，不应假定可以改变已编译 C++ ABI。应用顺序、版本匹配、失败回退和多人一致性比“能下载文件”更重要；客户端与服务器若得到不同规则会产生 Authority 判定分歧。

## 面试追问

1. Config Hotfix 与代码补丁的能力边界是什么？
2. 热更失败后应继续使用旧配置还是阻止进入 Session？
3. 文本热修复为何要与 Gameplay 权威配置分开评估？

