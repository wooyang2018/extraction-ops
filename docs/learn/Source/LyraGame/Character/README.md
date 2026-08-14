# Source/LyraGame/Character

## 目录职责

Character 目录定义 Pawn 的身体、移动、生命、玩家控制初始化和数据配方。它不独占玩家长期状态：玩家 ASC 通常在 PlayerState，输入语义在 InputConfig，镜头策略在 CameraMode。

## 类型关系

```text
APawn
└─ ALyraPawn
   └─ ALyraCharacter
      └─ ALyraCharacterWithAbilities (ASC 在自身，适合 AI/独立角色)

ULyraPawnExtensionComponent  -- 所有 Pawn 的数据/ASC/InitState 协调
ULyraHeroComponent           -- 玩家 Pawn 的输入与镜头初始化
ULyraHealthComponent         -- 将 HealthSet/ASC 事件翻译为生命与死亡状态
ULyraCharacterMovementComponent -- 移动 Tag、压缩标志与 Lyra 移动扩展
```

## PawnData 是组合根

`ULyraPawnData` 把 PawnClass、AbilitySets、TagRelationshipMapping、InputConfig 和默认 CameraMode 汇合。它是“这个可控实体具有什么能力”的数据入口，而不是运行时可变背包。

GameMode 在生成 Pawn 前设置 PawnData；PawnExtension 复制它并驱动初始化；HeroComponent 读取 InputConfig/CameraMode；PlayerState 据此授予 AbilitySets。

## PawnExtension 状态机

见[Pawn 初始化专题](../../../architecture/pawn-init-and-networking.md)。目录内最关键的不变量：

- 一个 Pawn 只能有一个 PawnExtension；
- PawnData 只设置一次；
- ASC 的 OwnerActor/AvatarActor 必须与当前 PlayerState/Pawn 匹配；
- 旧 Pawn 解除绑定后不能再持有输入或 GameplayCue。

## HeroComponent 输入链

`InitializePlayerInput`：

1. 取得 Pawn→PlayerController→LocalPlayer；
2. 获取 EnhancedInputLocalPlayerSubsystem；
3. 清理旧 Mapping，应用 PawnData.InputConfig；
4. 绑定 Native Actions；
5. 绑定 Ability Actions，将 Tag Press/Release 交给 ASC；
6. 向 Controller 和 Pawn 发送 `BindInputsNow`，允许 GameFeature 添加额外绑定。

移动和视角仍调用 Pawn/Controller 的标准接口；Gameplay Tag 是绑定表的稳定语义键，不直接取代 InputAction。

## HealthComponent 与死亡

HealthComponent 监听 ASC 的 HealthSet 属性变化和 OutOfHealth 事件，将“数值归零”提升为可观察的死亡流程。死亡开始/完成通常通过 Gameplay Tag 状态和 Gameplay Ability 驱动，使动画、输入阻断、消息与服务端重启可以解耦。

`LyraGameplayAbility_Death` 负责进入死亡状态；GameMode/PlayerController 负责最终重启边界。HealthComponent 不应擅自生成新 Pawn。

## CharacterWithAbilities

该类型把 ASC 和属性集直接放在 Character 上，展示另一种所有权策略。它适合不需要跨 Pawn 保留能力状态的角色；玩家长期 ASC 则更适合 PlayerState。

## 面试追问

1. PawnData 为什么是只读 DataAsset，而装备/背包是运行时实例？
2. CharacterWithAbilities 与 PlayerState-owned ASC 如何选择？
3. HeroComponent 为什么不在 OnRegister 直接绑定输入？
4. Health 归零、Death Ability 激活、Pawn 销毁分别属于哪一层？
5. 自定义移动标志怎样通过 SavedMove/网络预测保持客户端与服务端一致？

## 练习

沿 `SetPawnData → InitializeAbilitySystem → InitializePlayerInput` 记录一次玩家生成；再重生一次，确认 AbilitySet 是否重复授予、旧 Avatar 是否清除、额外输入是否重复绑定。

