# Plugins/CommonUser

## 用户身份不是一个 ID

PlatformUser、InputDevice、LocalPlayer、Online NetId 处在不同层。CommonUserSubsystem 负责映射、登录状态、Guest、Privilege 与在线上下文；CommonSessionSubsystem 负责 Host、Find、QuickPlay、Join、Destroy 和 Travel。

当前 `CommonUser.Build.cs` 将 `bUseOnlineSubsystemV1=true`，所以实际编译 `COMMONUSER_OSSV1=1`。源码中的 OSSv2 是备用分支，不代表当前运行路径。

## 初始化链

Frontend ControlFlow 请求本地/在线初始化；Subsystem 校验玩家顺序、设备占用、最大本地人数和权限，执行登录，必要时创建 LocalPlayer，再在下一 Tick 广播完成，避免在 Online delegate 深层栈中修改玩家集合。

## Session 链

- Host：UserFacingExperience 生成 Request → CreateSession → Start → ServerTravel。
- QuickPlay：Find → 有结果 Join 第一项，无结果 Host；当前“best”没有复杂择优。
- Join：JoinSession → 可选 Beacon reservation → resolve connect string → PreClientTravel hook → ClientTravel。
- Invite：GameInstance 暂存 requested session，Frontend Flow 尝试加入。

普通 ReturnToMainMenu 与 hard disconnect 的 user/session reset 策略不同，应按调用场景阅读，不能概括成单一重置规则。

## 面试追问

1. 登录成功为何不等于已经进入 Session？
2. QuickPlay 当前为何不能称为最佳延迟匹配？
3. PlatformUser 与 NetId 何时才建立对应？
4. OSSv2 分支存在为何不代表项目正在使用 OSSv2？

