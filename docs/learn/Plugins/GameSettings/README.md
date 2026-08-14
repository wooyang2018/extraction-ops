# Plugins/GameSettings

GameSettings 是通用设置框架：Registry 管树和事件；Collection 分组；Value 是可编辑叶子；DataSource 通过 PropertyPath/动态 Getter Setter 连接真实存储；EditCondition/Filter 决定可见、可编辑和平台适用性；Widget 层负责展示。

ChangeTracker 记录脏 Setting：Apply 先让 Value 生效并 StoreInitial，再由 Registry 保存；Cancel 对脏项 RestoreToInitial。Apply 与 Save 分离，因为有些设置能运行时预览，但持久化属于另一步。

LyraSettingsLocal/Shared 是消费者，不属于插件核心；插件本身不硬编码具体视频或音频选项。

## 面试追问

1. Registry、Collection、Value、DataSource 如何分工？
2. Apply、Save、StoreInitial、RestoreToInitial 各是什么阶段？
3. Dynamic DataSource 的反射路径失效时如何报告？
4. Filter 与 EditCondition 有何区别？

