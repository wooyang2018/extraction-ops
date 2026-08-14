# Source/LyraGame/Settings

## 三层设置模型

1. `ULyraSettingsLocal`：设备/机器相关设置，如画质、分辨率、音频、性能统计；通常落入 GameUserSettings。
2. `ULyraSettingsShared`：随本地用户保存的偏好，如颜色、操作与无障碍设置。
3. `ULyraGameSettingRegistry`：将上述字段包装成 GameSettings 框架的页面、集合和 Setting 对象。

`CustomSettings` 为离散值、动态选项、键位、音频设备等提供适配；`Screens/Widgets` 是编辑器/界面表现层。Registry 的 Audio、Video、Gameplay、Gamepad、MouseAndKeyboard、PerfStats、DLC 分文件构建，避免一个巨型注册函数。

设置对象常通过 Getter/Setter、Dynamic Getter 和 EditCondition 反射访问真实数据。设置 UI 不是数据真相；应用/取消/恢复默认必须回到 Local/Shared Settings。

## 面试追问

1. Local 与 Shared 设置怎样按“设备”还是“用户”划分？
2. 为什么分辨率立即应用，而某些设置需确认或重启？
3. 键位重映射如何与 Enhanced Input UserSettings/Profile 对接？
4. Dedicated Server 为什么不应初始化完整视频设置页面？

## 练习

选一个画质设置和一个用户偏好，追踪 Registry→Setting→Getter/Setter→持久化→运行时应用，比较取消修改时的回滚路径。

