# 第 3 周：客户端输入、角色、武器与 HUD

## 本周目标

复用 ShooterCore 已有 Experience、Pawn、GAS 武器、QuickBar 和 HUD，做出 ExtractionOps 的单机战斗回路：进入地图、移动、瞄准、射击、换弹、命中、受伤、死亡。本周只建立客户端体验基线；权威网络校验在第 4 周完成。

## 与总蓝图同步：两把武器和枪感

本周最终只保留两类主武器，建议用“稳定易控的突击步枪”和“近距离高爆发武器”形成明确差异。先复用 Lyra 武器链，只有项目差异进入 ExtractionOps。

每把武器建立一张调参表：

| 维度 | Rifle 基线 | 近战主武器基线 | 验证方式 |
| --- | --- | --- | --- |
| 有效距离 | 中距离 | 近距离 | 10/25/50 米靶位 |
| 射击方式 | 自动或点射 | 低射速高伤害 | 固定弹匣击杀时间 |
| 后坐与散布 | 连射逐步扩大 | 单发冲击明显 | 录制准星和弹着分布 |
| 命中反馈 | 稳定、清楚 | 更强音画冲击 | 无 HUD 时也能判断命中 |
| 换弹风险 | 中等 | 明显空窗 | AI 压迫下是否需要找掩体 |

实现顺序固定为：

1. 跑通原始 Lyra Rifle，记录输入到伤害的调用链；
2. 创建两个 Item/Equipment/Weapon 配置，只改数据，不复制整套 Ability；
3. 调整镜头 FOV、瞄准过渡、移动时散布、连续射击后坐；
4. 补齐枪口、弹道/命中、受击方向、音效和轻微镜头反馈；
5. 处理射击、换弹、切枪、治疗和死亡之间的动画/Ability 阻断；
6. 在相同靶场各录制三轮，依据可测数据调参，不凭一次手感判断。

完成门槛：两把武器在轮廓、距离、节奏和风险上能被测试者明确区分；连续战斗 5 分钟没有输入重复绑定、弹药负数、动画卡死或 HUD 状态滞留。

## 前置条件与周门槛

- 第 2 周 `ExtractionOps` Feature 能加载和停用，默认 Lyra 无回归。
- 本周新增内容全部放在 `Plugins/GameFeatures/ExtractionOps/Content` 或 `ExtractionOpsRuntime`；复制资产时保留对 ShooterCore 公共资产的引用。
- 不新建第二套角色移动、ASC、Inventory 或武器框架。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 跑通并记录 ShooterCore 战斗链 | 3 小时 |
| 2 | 配置 Extraction PawnData、输入和武器 | 4 小时 |
| 3 | 完成射击、换弹和死亡状态 | 4–5 小时 |
| 4 | 制作 Extraction HUD 与反馈 | 3–4 小时 |
| 5 | 单机测试、非法状态测试和录屏 | 2–4 小时 |

## 先读什么

- `Character/LyraHeroComponent.*` 与 `Input/LyraInputConfig.*`；
- `Equipment/LyraQuickBarComponent.*`、`LyraEquipmentManagerComponent.*`；
- `Weapons/LyraGameplayAbility_RangedWeapon.*`、`LyraRangedWeaponInstance.*`；
- `AbilitySystem/Attributes/LyraHealthSet.*`、`Character/LyraHealthComponent.*`；
- `UI/Weapons/LyraWeaponUserInterface.*` 与 ShooterCore 的实际 HUD 资产。

先在 Reference Viewer 中查看 ShooterCore Experience → PawnData → AbilitySet/InputConfig → HUD 的引用关系。

## 工作单元 1：建立 ShooterCore 行为基线

1. 用原 ShooterCore Experience 进入单机 PIE；
2. 录制移动、跳跃、瞄准、射击、自动/单发、换武器、受伤、死亡；
3. 在 Output Log 中为输入标签、Ability 名称、武器实例和弹药变化建立观察记录；
4. 找到实际使用的 Experience、PawnData、InputConfig、AbilitySet、武器 ItemDefinition 和 HUD Layout 资产路径；
5. 用 Reference Viewer 确认哪些资产来自 ShooterCore，哪些位于 LyraGame。

成功信号：能为一次射击写出 `InputTag -> Ability 激活 -> WeaponInstance -> TargetData -> Damage -> Health -> HUD` 链路。找不到链路时不要复制资产开始修改。

## 工作单元 2：配置 Extraction Pawn、输入和武器

### 2.1 创建最小派生资产

在插件中创建：

```text
Content/Experiences/B_ExtractionExperience
Content/Pawns/DA_ExtractionPawnData
Content/Input/DA_ExtractionInputConfig
Content/UI/W_ExtractionHUDLayout
Content/Weapons/DA_ExtractionRifle
Content/Weapons/DA_ExtractionShotgun
```

名称可依项目资产前缀规则调整，但文档和录屏中保持一致。PawnData 复用现有 Character/Pawn 类与动画，不复制模型和动画蓝图。

### 2.2 输入清单

确保 InputConfig/Mapping Context 中存在且只绑定一次：

- Move、Look、Jump；
- Aim、Fire；
- Reload；
- QuickBar Slot 或 Next/Previous Weapon；
- Interact（第 6–7 周使用，当前只确认不会冲突）。

逐个按键测试按下、持续、释放。一次点击触发多次时，检查 Input Trigger 与 Ability 输入绑定，不用额外布尔变量遮掩。

### 2.3 武器数据

从现有 Rifle ItemDefinition/EquipmentDefinition 派生两个最小配置：`DA_ExtractionRifle` 是中距离、稳定连射基线；`DA_ExtractionShotgun` 是近距离、低射速高爆发基线。分别记录射速、弹匣容量、散布/弹丸数、射程、伤害 Effect、准星、后坐和换弹时间。运行时状态继续由 Lyra WeaponInstance/Inventory 管理，不在 Data Asset 保存当前弹药。

两把武器都必须有对应 ItemDefinition、EquipmentDefinition、WeaponInstance 配置和弹药 Definition；复用同一 C++/Ability 框架，只通过数据、动画和表现形成差异，不复制两套射击逻辑。

通过标准：Extraction Experience 使用自己的 PawnData/InputConfig/HUD，但底层 Lyra 战斗能力仍正常。

## 工作单元 3：射击、换弹、受伤和死亡

### 3.1 明确状态与阻断

本周统一使用 Gameplay Tag 表达：

```text
State.Aiming
State.Reloading
State.Dead
Ability.Fire
Ability.Reload
```

标签先记录在 ExtractionOps 标签配置/原生标签入口；第 5 周系统化扩展。禁止在多个 Widget/Blueprint 各保存一份 `IsReloading`。

### 3.2 射击与弹药

复用 Lyra ranged weapon Ability：

1. 输入触发 Ability；
2. 本地播放枪口、音效和后坐；
3. WeaponInstance 提供散布和射击参数；
4. Item Tag Stack/现有成本系统扣除弹药；
5. TargetData 触发伤害；
6. HealthSet 变化驱动受伤/死亡。

日志加入本地 `shot_id`，记录 input、ability_activated、shot_fired、hit_feedback 四个事件，确认一次点击只有一个 shot_id。

### 3.3 换弹

在现有 Ability/AbilitySet 上配置换弹能力，至少覆盖：空弹匣、非满弹、满弹、无备用弹、换弹中开火、换弹中死亡。换弹完成前不直接把 UI 数字改成满弹；UI 读取真实库存/武器状态。

### 3.4 受伤与死亡

使用现有 HealthComponent 和 Death Ability。制作一个仅用于测试的伤害来源或 Lyra Bot，验证 Health 归零后：Fire/Reload 被阻断、死亡表现播放、HUD 状态更新。

本周可使用本地或 Listen Server 命中作为体验验证，但在笔记中明确最终命中必须由第 4 周 Server 确认。

## 工作单元 4：Extraction HUD 与表现

HUD 至少显示：武器名、当前/备用弹药、生命值、准星、命中反馈、Reloading/Dead、当前 NetMode。实现步骤：

1. 复用 Lyra HUD Layout 和 Weapon UI 扩展点；
2. C++/现有组件提供只读状态与变化委托；
3. Blueprint Widget 绑定变化事件并展示；
4. 关闭再打开 HUD，确认状态从数据源重建；
5. 禁止 Widget Tick 全量扫描 Inventory 或写回属性。

表现层依次增加枪口火焰、音效、后坐、准星扩散和命中标记。每加一项都测试关闭该表现不影响伤害与弹药逻辑。

## 工作单元 5：单机测试与证据

执行固定测试：

1. 正常：拾取/获得 Rifle → 射击 → 弹匣为空 → 换弹 → 击杀目标；
2. 非法：满弹换弹、零备用弹换弹、死亡后射击；
3. 输入：快速点按 20 次，shot_id 不重复且弹药变化一致；
4. UI：战斗中关闭并重建 HUD，数字恢复正确；
5. 回归：切回原 ShooterCore Experience，原玩法不受影响。

每项记录 Given/When/Then、日志事件和截图。录制 1 分钟不剪辑战斗视频。

## 验收目标

- [ ] Extraction Experience 支持移动、视角、跳跃、瞄准、射击和换弹；
- [ ] 武器数据集中在 Data Asset/现有 Lyra Definition；
- [ ] 一次输入、一条 shot_id、一次弹药成本；
- [ ] 换弹和死亡正确阻断射击；
- [ ] HUD 只消费状态，重建后显示正确；
- [ ] 单机可走通“获得武器 → 战斗 → 换弹 → 受伤 → 死亡”；
- [ ] 能解释客户端输入、预测表现和最终状态的区别。

## 实现原理

Lyra 已把输入标签、Ability、WeaponInstance、Inventory/Equipment 和 UI 扩展点连接起来。本周的价值是掌握并配置这条链，而不是重建一套 CombatComponent。输入表示意图，预测提供即时表现，状态组件保存事实，HUD 只是消费者。

## 常见问题与停止条件

- 一次输入多发：检查 Enhanced Input Trigger、Ability 输入和自动射击计时器。
- 换弹仍可射击：检查阻断标签和 Ability 激活条件。
- UI 不更新：先验证数据源，再验证委托绑定，禁止用 Tick 掩盖。
- 复制资产后引用丢失：用 Reference Viewer 修复依赖，不复制整个 ShooterCore。

战斗回路或原玩法回归未通过时，不进入网络改造。

## 本周作品集产出

- 1 分钟战斗视频；
- 输入到 HUD 数据流图；
- 武器数据配置截图；
- shot_id 调试记录；
- “客户端命中为何不可信”设计说明。

## 参考资料

- [Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)
- [Lyra Abilities](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)
- [Lyra 武器源码](../Source/LyraGame/Weapons)
