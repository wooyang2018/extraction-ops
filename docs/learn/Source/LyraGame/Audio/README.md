# Source/LyraGame/Audio

`ULyraAudioMixEffectsSubsystem` 根据前端/玩法状态和用户设置管理 Control Bus、Mix 与 HDR/音量效果；`ULyraAudioSettings` 提供软引用配置。Subsystem 负责运行时应用，Settings 负责数据入口，UI 只修改设置。

音频资源使用软引用意味着初始化可能异步；Dedicated Server 和无音频设备平台应安全跳过。Experience Loaded 后设置系统可重新应用 Mix，避免资产尚未就绪时访问。

## 面试追问

1. SoundClass、Submix、ControlBus/Mix 各负责哪类控制？
2. 为什么音量设置不能只改 Widget 显示值？
3. World/LocalPlayer/Engine Subsystem 中哪个更适合音频混合，为什么？

