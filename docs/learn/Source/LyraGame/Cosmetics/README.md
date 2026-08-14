# Source/LyraGame/Cosmetics

Cosmetics 将“玩家选择了哪些角色部件”和“Pawn 上实际生成哪些部件”分离。ControllerComponent 保存面向玩家/控制器的部件选择，PawnComponent 在具体 Pawn 上生成 ChildActor/部件并形成合并后的 GameplayTag 表现；Pawn 重生时选择可以重新投影。

`FLyraCharacterPart`/handle 描述部件与来源，`FLyraCosmeticAnimationTypes` 根据合并 Tag 选择动画层或身体类型。DeveloperSettings/Cheats 提供开发期覆盖。

关键网络判断：Authority 决定需要复制的部件选择；客户端生成表现 Actor。纯本地第一人称或平台特定装饰可按观察者策略过滤，但不能影响权威碰撞/伤害。

## 面试追问

1. 为什么部件选择放 ControllerComponent，而生成放 PawnComponent？
2. Cosmetics Tag 怎样参与动画选择而不污染权威 Gameplay Tags？
3. ChildActor 表现和网络复制的边界在哪里？

## 练习

模拟玩家重生，验证 Controller 选择如何重新应用到新 Pawn，并检查旧 Pawn 的部件实例是否全部销毁。

