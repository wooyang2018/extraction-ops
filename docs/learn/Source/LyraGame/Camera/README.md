# Source/LyraGame/Camera

## CameraMode 栈

`ULyraCameraComponent::GetCameraView` 每帧更新 CameraMode 并求值栈。HeroComponent 提供模式选择委托：Ability 临时覆盖优先，否则使用 PawnData 默认模式。`ULyraCameraModeStack` 复用模式实例、维护权重并从栈顶向下混合视角。

CameraMode 输出的不只是位置旋转，还包括 FOV、控制旋转等 Camera View 数据。它不是生成/切换多个 CameraActor。

## 第三人称模式

`ULyraCameraMode_ThirdPerson` 依据目标 Actor、蹲伏偏移和曲线计算期望位置，再用 penetration feelers 做墙体穿透规避。Feeler 可配置射线角度、权重、追踪间隔和忽略 Actor，使中心射线严格、外围射线更便宜。

## UI Camera

`ULyraUICameraManagerComponent` 允许 UI 场景暂时接管视图目标；`ALyraPlayerCameraManager` 是标准 PlayerCameraManager 扩展入口。Gameplay Camera 与 UI Camera 的切换应保持明确所有者，避免 Ability CameraMode 和前端镜头互相覆盖。

## 面试追问

1. CameraMode 栈与 `SetViewTargetWithBlend` 的适用边界有何不同？
2. Ability 结束时忘记 ClearCameraMode 会怎样？
3. 为什么穿透规避用多根带权 Feeler，而不只做一次 SphereTrace？
4. Camera 逻辑应该在哪些网络端运行？

## 练习

实现一个短时 ADS CameraMode，由 Ability 激活/结束压栈和清理；在角色贴墙、蹲伏和死亡取消时验证 blend 与残留状态。

