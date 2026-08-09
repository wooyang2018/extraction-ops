# 第 5 周：GAS 战斗能力与数据驱动

## 本周目标

在第 3–4 周可用网络战斗链上，沿用 `ULyraAbilitySystemComponent`、HealthSet、CombatSet 和武器 Ability，增加 ExtractionOps 自己的 Gameplay Tags、一个专属 AttributeSet，以及可预测/可阻断的战斗能力。重点是掌握 Ability、Attribute、Effect、Cue 和普通组件的边界。

## 前置条件与周门槛

- 两客户端在延迟/丢包下的生命、死亡和弹药能收敛。
- 不创建第二个 ASC；继续使用 Lyra PlayerState/Pawn 初始化链。
- 新 C++ 位于 `Plugins/ExtractionOps/Source/ExtractionOps`，新资产位于插件 Content。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 阅读和调试现有 GAS 链 | 3 小时 |
| 2 | 标签规范与专属 AttributeSet | 4 小时 |
| 3 | Gameplay Effect 与伤害/护甲规则 | 3–4 小时 |
| 4 | Fire/Reload 扩展、预测和 Cue | 4–5 小时 |
| 5 | 双客户端测试与设计说明 | 2–4 小时 |

## 先读什么

- `AbilitySystem/LyraAbilitySystemComponent.*`、`LyraAbilitySet.*`；
- `Attributes/LyraHealthSet.*`、`LyraCombatSet.*`；
- `Executions/LyraDamageExecution.*`；
- `Abilities/LyraGameplayAbility.*`、`LyraAbilityCost_*`；
- `LyraGameplayTags.*`、Gameplay Cue Manager 与 ShooterCore 的 AbilitySet/Effect 资产。

先用 GAS 调试命令观察已有 Ability、Effect、Tag 和 Attribute，再开始添加类型。

## 工作单元 1：画出现有 GAS 数据流

对一次射击和死亡逐步记录：ASC 在哪里、Avatar/Owner Actor 是谁、Ability 从哪个 AbilitySet 授予、成本如何扣除、TargetData 如何到 Server、DamageExecution 如何修改 Health、Death Ability 如何激活。

日志统一包含 `ability`、`player_id`、`prediction_key`、`tags`、`result/reason` 和属性前后值。不得只写“ability failed”。

通过标准：能解释现有逻辑放在 GAS 的原因，以及 Inventory/Backend 为什么不属于 GAS。

## 工作单元 2：标签与专属 AttributeSet

### 2.1 固定标签表

在插件的原生标签或配置中只建立本项目需要的集合：

```text
State.Aiming
State.Reloading
State.Dead
State.Extracting
Ability.Fire
Ability.Reload
Weapon.Rifle
Weapon.Pistol
Damage.Bullet
Damage.Environment
```

若 Lyra 已存在语义完全相同的标签，复用现有标签并在表中记录映射，不创建近义重复项。

### 2.2 创建 `UExtractionArmorSet`

计划新增 `AbilitySystem/Attributes/ExtractionArmorSet.h/.cpp`，包含：

- `Armor`：当前护甲；
- `MaxArmor`：上限；
- 属性访问器与复制回调；
- Server 权威修改和 Owner/相关观察者需要的复制策略；
- Clamp：`0 <= Armor <= MaxArmor`。

把 ArmorSet 加入 Extraction Pawn 使用的 AbilitySet，而不是在 Character 构造函数中硬编码第二套 ASC。

验证：出生时 Health、MaxHealth、Armor、MaxArmor 均可在 GAS Debugger 中看到；两个客户端读到一致值；客户端直接修改只影响本地临时显示并会被 Server 修正。

## 工作单元 3：Gameplay Effect 与伤害规则

创建数据资产：初始护甲 Effect、Bullet Damage Effect/Execution 配置、Reloading 状态 Effect、必要的受击 Cue。固定 MVP 规则：伤害先扣 Armor，剩余值再进入 Health；Armor 不为负；Health 归零触发 Dead，Dead 为终止性阻断标签。

对每个 Effect 记录：Instant/Duration/Infinite、修改属性、Granted Tags、Stack 规则、由谁应用、是否允许预测。

测试数值：Armor=30、Health=100，依次施加 20 和 25 伤害，预期第一次 Armor=10/Health=100，第二次 Armor=0/Health=85。再施加 100 伤害，预期进入 Dead 一次。

环境伤害与 Bullet 使用不同 Damage Tag，但共用权威属性修改链。

## 工作单元 4：Ability、预测和 Gameplay Cue

### 4.1 Fire/Reload 扩展

复用 Lyra Ranged Weapon Ability；只在需要项目标签、护甲或失败原因时建立 Extraction 派生类/配置。Reload Ability 必须检查：武器存在、未满弹、有备用弹、非 Dead、非已 Reloading；激活时授予 `State.Reloading`，结束/取消/死亡时必定移除。

### 4.2 预测边界

允许预测：枪口、音效、后坐、换弹动画开始。等待 Server：伤害、Armor/Health、弹药最终值、死亡。为 Server 拒绝准备回滚：停止错误动画/命中标记，UI 回到复制值并显示明确失败原因。

### 4.3 Cue

Gameplay Cue 只播放表现，不修改属性或库存。至少配置 BulletHit/ArmorHit/Death 中两种，并验证 Dedicated Server 无渲染时不会依赖 Cue 才完成规则。

## 工作单元 5：测试与验收

执行：

1. 正常射击消耗护甲再消耗生命；
2. Reload 成功、满弹拒绝、无备用弹拒绝；
3. Reload 中死亡，状态和动画被取消；
4. 100 ms 延迟下本地表现即时，属性最终与 Server 一致；
5. 伪造客户端 Armor、重复 Damage Event、Dead 后再次伤害；
6. Dedicated Server 关闭表现资源后规则仍工作。

为每项保存 prediction_key/Ability/Effect/Attribute 日志和双客户端截图。

## 验收目标

- [ ] 只使用 Lyra 的 ASC 初始化链；
- [ ] 标签表无重复语义；
- [ ] `UExtractionArmorSet` 正确授予、复制和 Clamp；
- [ ] Damage Effect 按规则消耗 Armor/Health；
- [ ] Fire/Reload 至少两项以 Ability 表达；
- [ ] Reload 的完成、取消和死亡清理正确；
- [ ] Cue 仅负责表现；
- [ ] 延迟下预测与最终属性可以分离且最终一致；
- [ ] 能解释每项逻辑为何属于 Ability/Effect/AttributeSet/普通组件。

## 实现原理

Ability 表达行为与生命周期，AttributeSet 保存参与计算并复制的数值，Effect 描述修改，Tag 表达可组合事实，Cue 播放表现。GAS 适合战斗预测和阻断，但账号、仓库、结算事务不属于它。

## 常见问题与停止条件

- Attribute 不出现：检查 AttributeSet 是否通过 AbilitySet 注册、ASC Owner/Avatar 初始化顺序。
- Effect 无效：检查目标 ASC、Effect Spec、Tag 要求和 Execution 输出。
- Reload 标签残留：所有 End/Cancel/Death 路径必须清理，不能只处理成功回调。
- 预测闪回：只预测可回滚表现，最终资源等待 Server。

双客户端属性不一致或 Dead 能重复触发时，不进入背包系统。

## 本周作品集产出

- GAS 组件关系图；
- Attribute/Effect/Tag 表；
- 射击 Ability 时序图；
- 换弹中断与护甲伤害视频；
- GAS 边界技术笔记。

## 参考资料

- [Gameplay Ability System](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine)
- [Gameplay Attributes](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-attributes-and-attribute-sets-for-the-gameplay-ability-system)
- [Lyra AbilitySystem 源码](../Source/LyraGame/AbilitySystem)
