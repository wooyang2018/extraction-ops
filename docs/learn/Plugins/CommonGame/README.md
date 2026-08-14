# Plugins/CommonGame

CommonGame 提供跨项目宿主层：`UCommonGameInstance` 接入用户/Session 事件，`UCommonLocalPlayer` 持有用户上下文，`UGameUIManagerSubsystem` 选择 UI Policy，Policy 为每个 LocalPlayer 创建 `UPrimaryGameLayout`，Layout 将 GameplayTag 映射到 CommonUI Layer Stack。

```text
CreateLocalPlayer
 -> GameUIManagerSubsystem::NotifyPlayerAdded
 -> GameUIPolicy::NotifyPlayerAdded
 -> create PrimaryGameLayout
 -> RegisterLayer(Tag, ActivatableWidgetContainer)
 -> PushWidgetToLayerStack[Async]
```

异步 Widget Action 在流式加载期间 suspend CommonUI input，完成或取消后恢复，避免焦点落到半创建页面。MessagingSubsystem/Dialogs 提供确认、错误对话框；AsyncAction 封装蓝图异步生命周期并向 GameInstance 注册保活。

## 面试追问

1. UIManager、UIPolicy、PrimaryGameLayout 各自拥有哪层状态？
2. 为什么分屏中每个 LocalPlayer 必须有独立 root layout？
3. 异步 push Widget 时为什么要暂停输入？
4. Requested Session 为什么适合保存在 GameInstance？

