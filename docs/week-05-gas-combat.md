# 第 5 周：GAS 战斗能力与数据驱动

## 本周目标

把第 3、4 周的射击、换弹、受伤和死亡从分散的客户端逻辑整理成 Gameplay Ability System（GAS）能力。目标不是把所有代码都强行塞进 GAS，而是理解 Ability、Attribute、Gameplay Effect、Gameplay Tag 和 Gameplay Cue 的边界。

## 验收目标

- 玩家拥有一个可工作的 Ability System Component；
- 有生命值、护甲、弹药或体力等至少两类 Attribute；
- 射击、换弹、受伤、死亡至少有两项通过 Gameplay Ability 表达；
- 伤害和状态变化通过 Gameplay Effect 应用；
- 使用 Gameplay Tag 表达 `State.Dead`、`State.Reloading`、`Weapon.Rifle` 等状态；
- 客户端预测的表现和服务器最终属性可以分离；
- 两客户端联机时，生命和死亡状态最终一致；
- 能解释为什么某个逻辑放在 Ability、Effect、AttributeSet 或普通 Component 中。

## 操作步骤

### 1. 先阅读项目已有 GAS 代码

重点查看：

- `Source/LyraGame/AbilitySystem/LyraAbilitySystemComponent.*`；
- `LyraAbilitySet.*`；
- `LyraGameplayAbility.*`；
- `LyraAttributeSet.*`；
- `Abilities/` 目录下的死亡、跳跃和失败处理；
- Gameplay Tags 配置和 Gameplay Cue 管理。

不要先照着教程复制一个独立 Ability System。先回答：Lyra 已经替你解决了什么，ExtractionOps 只需要扩展什么。

### 2. 建立项目标签表

建议先建立一个小而稳定的标签集合：

```text
State.Dead
State.Reloading
State.Aiming
State.Extracting
Weapon.Rifle
Weapon.Pistol
Damage.Bullet
Damage.Environment
Ability.Fire
Ability.Reload
```

标签应当表达可组合的事实或状态，不要把临时调试文字也做成 Gameplay Tag。

### 3. 实现 AttributeSet

最小集合可以是：

- `Health`：当前生命；
- `MaxHealth`：生命上限；
- `Armor`：护甲；
- `Stamina`：跑动或冲刺资源；
- `AmmoInMagazine`：当前弹匣子弹数。

持久化库存、货币和账号经验不要放在角色的 AttributeSet 中。它们属于对局外或后台持久化数据。

### 4. 实现 Gameplay Effect

至少实现：

- 子弹伤害 Effect；
- 护甲吸收或减伤 Effect；
- 换弹状态 Effect；
- 死亡状态 Effect；
- 短暂的受击反馈 Effect。

要明确每个 Effect 是瞬时、持续还是周期性。伤害一般是瞬时修改；换弹阻断可以是带持续时间的状态；恢复或中毒可以是周期性 Effect。

### 5. 实现射击和换弹 Ability

射击 Ability 的流程建议是：

```text
检查标签与资源
  -> 消耗弹药/体力
  -> 播放本地动画和枪口表现
  -> 生成服务器请求
  -> 服务器校验并命中检测
  -> 对目标应用 Damage Effect
  -> 通过属性变化驱动受击和 HUD
```

换弹 Ability 要处理：

- 没有备用弹药；
- 已经满弹；
- 换弹中被打断；
- 换弹期间死亡；
- 客户端预测后服务器拒绝。

### 6. 做 GAS 调试

每个关键事件打印：

- Ability 名称；
- 激活者；
- 当前角色；
- 预测 Key 或请求序号；
- Ability Tags；
- 失败原因；
- Attribute 修改前后值。

不要只打印“射击失败”，要打印“因为 `State.Reloading` 阻断”或“因为服务器确认弹匣为 0”。

## 实现原理

GAS 把“行动如何发生”和“状态如何被修改”拆开。Ability 表达可被触发的行为，AttributeSet 保存可被计算的属性，Gameplay Effect 表达对属性和标签的修改，Gameplay Cue 负责表现层反馈。

多人场景下，GAS 的价值在于能够配合复制、预测、成本和阻断规则。但 GAS 不是万能容器。登录、背包持久化、服务端结算和数据库事务不应放在 Ability 里。

## 常见问题

### Ability 激活但属性没有改变

检查 Effect 是否应用到了正确的 Ability System Component，AttributeSet 是否注册，Effect 是否被正确授予，以及是否在客户端误把本地表现当成最终属性。

### 预测行为在服务器确认后闪回

把可以安全预测的表现与不可预测的最终状态分开。弹道、枪口音效可以先表现；伤害、物品和奖励必须等待服务端确认。

### Gameplay Tag 过多难以维护

先建立标签命名规范：`State.*` 表示状态，`Ability.*` 表示能力，`Weapon.*` 表示类别，`Damage.*` 表示伤害类型。新增标签前先确认是否真的需要组合和查询。

## 本周作品集产出

- GAS 组件关系图；
- 射击 Ability 时序图；
- Attribute 和 Gameplay Effect 表；
- 一段展示换弹被打断、受伤和死亡的视频；
- 一篇“为什么不能把所有游戏逻辑写进 GAS”的技术笔记。

## 参考资料

- [Gameplay Ability System](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine)
- [Understanding Gameplay Ability System](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-the-gameplay-ability-system)
- [Gameplay Attributes and Attribute Sets](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-attributes-and-attribute-sets-for-the-gameplay-ability-system)
- 项目内：[Source/LyraGame/AbilitySystem](../Source/LyraGame/AbilitySystem)

