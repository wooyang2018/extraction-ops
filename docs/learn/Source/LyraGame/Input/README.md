# Source/LyraGame/Input

## 四层输入模型

1. Enhanced Input Mapping Context：物理键/手柄轴到 InputAction。
2. `ULyraInputConfig`：InputAction 到 GameplayTag，分 NativeAction 与 AbilityAction。
3. `ULyraInputComponent`：按 Tag 查找 Action 并绑定回调。
4. `ULyraAbilitySystemComponent`：Tag 匹配 AbilitySpec，按激活策略处理 Pressed/Held/Released。

HeroComponent 是组装者，PlayerController Tick 是 Ability 输入消费时钟。GameFeature 的 AddInputContextMapping 与 AddInputBinding 分别扩展第 1 层和第 2/3 层。

## 文件分组

- `LyraInputConfig.*`：数据映射和按 Tag 查找。
- `LyraInputComponent.*`：模板绑定 Native/Ability Action。
- `LyraPlayerInput.*`、`LyraInputUserSettings.*`、`LyraPlayerMappableKeyProfile.*`：玩家重映射、Profile 与持久化。
- `LyraInputModifiers.*`：设置驱动的灵敏度、死区、瞄准反转等值变换。
- `LyraAimSensitivityData.*`：离散灵敏度等级到标量。

## 关键边界

- GameplayTag 是稳定的玩法语义，不是物理按键。
- InputAction 是 Enhanced Input 资产身份，不应被 Ability 直接硬编码。
- 重映射属于 LocalPlayer/UserSettings，Dedicated Server 不参与。
- Ability 激活最终仍受 Authority/Prediction、Owner Tag、Cost 和 Cooldown 校验；绑定成功不等于可激活。

## 面试追问

1. 为什么用 Tag 再间接匹配 AbilitySpec？
2. Mapping Context 动态增删时怎样避免重复绑定？
3. 输入 Modifier 读取 SharedSettings 时如何处理本地玩家尚未初始化？
4. Pressed 与 Held 同帧为何不能各自直接 TryActivate？

## 练习

加入一个新的 Ability Input Tag，沿 InputConfig、AbilitySet、HeroComponent、ASC 四层验证；然后运行时由 GameFeature 增删绑定，检查停用后是否残留。

