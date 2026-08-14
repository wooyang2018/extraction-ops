# Plugins 源码地图

Lyra 把可复用基础设施和具体玩法拆成插件。基础插件提供用户、UI、设置、消息、模块化 Actor 等稳定能力；GameFeatures 下的插件消费这些接口形成可启停玩法。无源码插件也会通过 `.uplugin` 声明内容包与依赖，因此纳入覆盖索引，但不分析 Content。

| 插件 | 学习重点 |
|---|---|
| CommonGame | GameInstance/LocalPlayer 与每玩家 UI Policy |
| CommonUser | 用户、权限、Session 与 Travel |
| CommonLoadingScreen / Startup | 运行时与引擎预加载屏 |
| GameplayMessageRouter | Tag 分频道的本地类型化消息总线 |
| GameSettings | 设置对象模型、Registry、Apply/Restore |
| UIExtension | Feature 向宿主 UI 插槽注入内容 |
| ModularGameplayActors | Actor receiver 生命周期桥接 |
| AsyncMixin | 有序异步装载链 |
| PocketWorlds | 口袋关卡与 SceneCapture |
| GameSubtitles | SubtitleManager/Media 字幕到 UMG/Slate |
| ShooterCore / TopDownArena | 两种 Feature 消费示例 |
| ShooterTests | CQTest 与多人 PIE 测试 |

