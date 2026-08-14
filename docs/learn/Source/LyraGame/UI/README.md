# Source/LyraGame/UI

## UI 分层

- `ULyraUIManagerSubsystem`：LocalPlayer UI policy/根布局生命周期。
- `ALyraHUD`、`ULyraHUDLayout`：进入玩法后的 HUD 宿主与布局。
- `ULyraActivatableWidget`：CommonUI 页面基类和输入模式。
- `ULyraUIMessaging`：确认/错误对话框。
- `Foundation/Common/Basic`：按钮、Tab、列表、进度条等基础组件。
- `Frontend`：ControlFlow 驱动的登录、主菜单、会话和前端性能流程。
- `IndicatorSystem`：世界目标到屏幕/Slate 图层的指示器投影。
- `PerformanceStats`、`Weapons`：消息/Subsystem 驱动的 HUD 表现。

## UI Policy 与 GameFeature Widget

宿主 UI 定义 layer/extension point；GameFeatureAction_AddWidget 在激活时推送布局或注册 Widget，停用时释放句柄。UIExtension 用 GameplayTag 作为语义插槽，让玩法插件不需要引用具体 HUD Widget 类。

CommonUI 的 Activatable Widget 栈处理页面前后关系、输入模式和 Back 行为；UIExtension 处理跨插件内容组合。两者不是替代关系。

## Frontend ControlFlow

`ULyraFrontendStateComponent` 把初始化步骤组织成可取消/可异步的 ControlFlow，例如用户初始化、Session 清理、主菜单显示。流程状态属于前端 World/GameState 组件，不应散落在 Widget Construct 回调。

## Indicator

IndicatorManager 持有 Descriptor；IndicatorLayer/SActorCanvas 将 Actor/SceneComponent 位置投影到屏幕并布局。Descriptor 是数据与目标引用，Widget 是表现；目标失效必须安全移除。

## 面试追问

1. CommonUI layer stack 与 UIExtension point 分别解决什么？
2. 为什么前端登录流程不应直接写在 Widget 蓝图里？
3. HUD/Widget 哪些只存在本地，为什么不能持有 Authority 状态？
4. 世界指示器怎样处理目标在镜头背后、屏幕外和被销毁？

## 练习

追踪 GameFeature Widget 从 Action 激活到 UIExtensionSubsystem 注册、扩展点匹配、Widget 创建，再到插件停用销毁的完整句柄链。

