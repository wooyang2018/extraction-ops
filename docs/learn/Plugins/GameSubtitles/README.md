# Plugins/GameSubtitles

SubtitleDisplaySubsystem 保存跨 Widget 的格式选项并广播变化；SubtitleDisplay/SSubtitleDisplay 是 UMG→Slate 表现层；MediaSubtitlesPlayer 将媒体 Overlay 字幕桥接到 Engine `FSubtitleManager`，使媒体和普通游戏字幕共享显示策略。

字幕启用、字号、颜色、背景等是用户可访问性设置，不应绑定某一关卡 Widget。Design-time/manual subtitle 模式可与全局 Manager 隔离，便于预览。

## 面试追问

1. UMG、Slate、FSubtitleManager 分别处在哪层？
2. Media overlay 怎样进入统一字幕显示？
3. 为什么格式状态适合 GameInstanceSubsystem？
4. 分屏字幕应该共享还是按玩家区分？当前结构的边界是什么？

