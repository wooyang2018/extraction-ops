# 第 3 周：两把武器、输入、GAS 与 HUD

## 本周目标

在第 2 周唯一的 `B_ExtractionExperience` 上完成可测战斗闭环：进入现有测试地图、移动、瞄准、使用 Rifle/Shotgun、换弹、命中、受伤、死亡并从 HUD 看到真实状态。底层继续复用 Lyra 的 Pawn、ASC、Inventory、Equipment、QuickBar 和 Ranged Weapon 框架。

本周验证客户端体验和现有 Lyra 联机行为；第 4 周再系统完成服务器校验、延迟/丢包和非法请求实验。

## 执行基线

开始前完整阅读[12 周执行基线](execution-baseline.md)。只使用 `D:\Software\UE_5.8`；不得访问受保护的 ue5-main 源码目录。第 1 周构建/双客户端和第 2 周 Experience/GameFeature/HUD 骨架必须通过。

## 与总蓝图同步：两把武器和枪感

只保留两类主武器：

- Rifle：中距离、稳定、自动射击和持续输出；
- Shotgun：近距离、低射速、多弹丸和高爆发。

两者使用同一套 C++/GAS 射击框架，通过定义资产、数值、动画和反馈形成差异。不得复制两套射击组件，也不得在 Data Asset 或 Widget 中保存运行时弹药。

## 前置条件与周门槛

- `B_ExtractionExperience` 能自动激活 ShooterCore 与 ExtractionOps；
- MatchState/RunState 注入和 Debug HUD 已通过单机与双客户端验证；
- 默认 ShooterCore Experience 仍可玩；
- 已记录 Lyra Rifle/Shotgun 的实际 Item、Equipment、WeaponInstance、AbilitySet、Fire/Reload Ability、Damage Effect 和表现资产路径。

任一条件未满足时，先补第 2 周，不在未知引用链上复制资产。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 反向验证 Lyra 武器与输入链 | 2–3 小时 |
| 2 | 两套最小派生定义和默认装备 | 4–5 小时 |
| 3 | 输入、换弹和状态阻断 | 3–4 小时 |
| 4 | HUD 与枪感反馈 | 3–4 小时 |
| 5 | 固定距离测试、E2E 和回归 | 3–4 小时 |

## 先读什么

- `Character/LyraHeroComponent.*` 与 `Input/LyraInputConfig.*`；
- `Equipment/LyraQuickBarComponent.*` 与 `LyraEquipmentManagerComponent.*`；
- `Weapons/LyraGameplayAbility_RangedWeapon.*` 与 `LyraRangedWeaponInstance.*`；
- `AbilitySystem/Attributes/LyraHealthSet.*` 与 `Character/LyraHealthComponent.*`；
- ShooterCore Rifle、Shotgun、标准输入、HUD Layout 和 Weapon UI 资产。

先用 Asset Registry 和 Reference Viewer 写出：

```text
InputTag -> ASC Ability -> WeaponInstance -> TargetData
-> Damage Effect -> HealthSet -> Death -> HUD
```

找不到实际引用链时，不开始复制资产。

## 工作单元 1：建立可比较的 ShooterCore 基线

使用原始 ShooterCore Experience，在同一地图完成三轮：

1. Rifle 自动射击、打空弹匣、换弹；
2. Shotgun 单发、打空弹匣、换弹；
3. 切枪、受伤、死亡和 HUD 更新。

记录原始资产路径和以下参数：弹匣容量、备弹来源、射速、散布、弹丸数、射程/伤害衰减、Damage Effect、准星、后坐和换弹时间。该记录是派生前基线，不凭记忆调整资产。

## 工作单元 2：创建最小派生资产链

### 2.1 固定目录和职责

在 `/ExtractionOps/Weapons/Rifle` 与 `/ExtractionOps/Weapons/Shotgun` 下按职责创建：

| 职责 | Rifle | Shotgun |
| --- | --- | --- |
| Inventory Item | `ID_ExtractionRifle` | `ID_ExtractionShotgun` |
| Equipment Definition | `WID_ExtractionRifle` | `WID_ExtractionShotgun` |
| Weapon Instance | `B_WeaponInstance_ExtractionRifle` | `B_WeaponInstance_ExtractionShotgun` |
| Ability Set | `AbilitySet_ExtractionRifle` | `AbilitySet_ExtractionShotgun` |
| Fire 配置 | `GA_Weapon_Fire_ExtractionRifle` | `GA_Weapon_Fire_ExtractionShotgun` |
| Reload 配置 | `GA_Weapon_Reload_ExtractionRifle` | `GA_Weapon_Reload_ExtractionShotgun` |
| Damage Effect | `GE_Damage_ExtractionRifle` | `GE_Damage_ExtractionShotgun` |
| 装备表现 | `B_ExtractionRifle` | `B_ExtractionShotgun` |

表格描述职责，不强迫 Lyra 产生并不存在的层。实施前用 Asset Registry 确认 UE 5.8 中每个原型的真实类：

- 原型本身已能复用且无需项目差异时，ExtractionOps 资产直接引用原型；
- 某职责合并在另一个资产中时，在实现记录中写明映射，不创建空包装；
- 只有需要独立调参、授权或表现时才复制对应资产；
- 不复制 Character、ASC、Inventory、QuickBar 或通用 Ranged Weapon C++。

两把武器的最终资产清单必须按上述职责逐项标记为“项目派生”或“复用 Lyra 路径”，不得只留下含义模糊的单一 Rifle/Shotgun 聚合资产。

### 2.2 PawnData、默认装备和 QuickBar

- 延续第 2 周的 `DA_ExtractionPawnData`，不重新创建；
- 创建 `DA_ExtractionInputConfig` 并替换 PawnData 中第 2 周临时引用；
- 扩展第 2 周的 `W_ExtractionHUDLayout`，不创建第二个 HUD Layout；
- 通过 Lyra 现有 Experience/Equipment/Inventory Action 为测试 Pawn 授予 Rifle 和 Shotgun；
- 两把武器进入 QuickBar 固定槽位，可用槽位键或 Next/Previous 切换；
- 本周不依赖第 6–7 周才实现的地面拾取和背包。

### 2.3 弹药事实来源

先确认 Lyra 5.8 原型使用 Item Tag Stack、Cost 还是独立定义保存弹药。项目沿用该真实机制：

- 当前弹匣与备弹只有一个权威来源；
- Fire 每次合法提交只扣除一次成本；
- Reload 只在完成时转移真实库存；
- UI 不预测写入最终弹药；
- 若 Lyra 没有独立 Ammo Definition，不为满足文档名称新建空 Data Asset。

## 工作单元 3：输入、Gameplay Tag 与阻断

### 3.1 输入清单

`DA_ExtractionInputConfig` 中仅绑定一次：

- Move、Look、Jump；
- Aim、Fire；
- Reload；
- QuickBar Slot 或 Next/Previous Weapon；
- Interact。

逐项测试 Pressed/Held/Released。Interact 本周只验证键位和 Input Tag 不冲突，不实现终端或拾取逻辑。

### 3.2 标签规则

优先复用 Lyra 已有 Fire、Reload、Death、Ability Failure 和输入标签。只有 Lyra 没有项目所需语义时才新增标签，且必须位于 `ExtractionOps.*` 命名空间，例如：

```text
ExtractionOps.State.Aiming
ExtractionOps.State.Reloading
ExtractionOps.Ability.Fire
ExtractionOps.Ability.Reload
```

禁止新增无命名空间的 `State.*`、`Ability.*`，也禁止在 Widget 和 Blueprint 中各保存一份 `IsReloading`/`IsDead`。

### 3.3 Fire/Reload/Death 不变量

- Reloading 阻断 Fire；
- Dead 阻断 Aim、Fire 和 Reload；
- 满弹匣、零备弹时 Reload 拒绝激活且不消耗弹药；
- 空弹匣和部分弹匣可以按真实备弹量换弹；
- 换弹中死亡立即取消 Ability，不在延迟回调中补满弹匣；
- 一次合法射击只产生一次弹药成本和一次伤害提交；
- 本周不强制增加 `shot_id`。跨客户端请求/确认关联日志由第 4 周统一设计。

## 工作单元 4：HUD 与枪感表现

在第 2 周 `W_ExtractionHUDLayout` 上增加：

- 当前武器名；
- 弹匣/备弹；
- 生命值；
- 准星与散布表现；
- 命中反馈；
- Reloading/Dead；
- Debug HUD 中已有的 NetMode。

数据只来自 ASC、HealthComponent、Inventory、QuickBar 和 WeaponInstance 的只读查询/变化委托。禁止 Widget Tick 全量扫描 Inventory，禁止 UI 写回生命、弹药或 Ability 状态。关闭并重建 HUD 后，显示必须从真实数据源恢复。

表现层按枪口火焰、音效、准星扩散、后坐/镜头反馈、命中标记顺序接入。每项都要验证：关闭表现资源后，弹药、伤害和死亡规则仍然工作。

## 工作单元 5：枪感、E2E 与回归

### 5.1 固定距离枪感测试

使用同一地图、同一静止目标和相同开始姿态，在 10/25/50 米各测试三轮。记录：

- 弹匣容量与备弹；
- 射速；
- Rifle 连射散布；
- Shotgun 弹丸数和落点；
- 换弹时间；
- 固定目标击杀时间；
- 射击中断和换弹暴露窗口。

通过标准：测试者仅从有效距离、节奏、散布和换弹风险就能区分两把武器。连续战斗五分钟无负弹药、重复输入、Ability 卡死或 HUD 滞留。

### 5.2 正常与非法状态

用默认装备执行：

1. Rifle 射击到空仓、换弹、击杀目标；
2. 切换 Shotgun、近距离击杀目标；
3. 满弹换弹、零备弹换弹、换弹中开火；
4. 换弹中受致命伤、死亡后 Aim/Fire/Reload；
5. 快速点按 Fire 20 次，核对合法射击数、弹药成本和伤害提交数；
6. 战斗中销毁并重建 HUD，核对全部显示。

### 5.3 前三周端到端验收

在 Editor Dedicated Process + 两个 Editor Client Process 中：

1. 两个客户端完成 Join，自动加载 `B_ExtractionExperience`；
2. MatchState/RunState 组件存在，Debug HUD 网络角色正确；
3. 双方看到对方移动、持枪、瞄准、开火、切枪和死亡表现；
4. 两个客户端分别攻击服务器 Bot/测试目标，伤害和死亡结果在两端一致；
5. 关闭一个客户端，另一端和 Server 继续运行；
6. 关闭 Server，客户端明确断线；
7. 切回 ShooterCore Experience，确认原玩法无回归；
8. 从所有 UE 进程关闭状态完整重复一次。

该测试证明前三周资产和现有 Lyra 网络链能够端到端运行，不代替第 4 周的延迟、丢包、非法请求和安全校验。

## Unreal Editor MCP 操作合同

沿用第 2 周规范：先发现 Toolset/Blueprint Skill，串行调用，编辑前后保存，每个逻辑单元编译并检查结果；PIE 中只测试不改资产。MCP 不可用时停止 `.uasset` 实施，不用脚本伪造二进制资产。

## 验收目标

- [ ] 第 2 周唯一的 Experience、PawnData 和 HUD 被扩展，没有重复资产；
- [ ] Rifle/Shotgun 每项职责都有明确的项目资产或 Lyra 复用路径；
- [ ] 移动、观察、跳跃、瞄准、射击、换弹和切枪可用且输入不重复；
- [ ] 一次合法射击只有一次弹药成本和一次伤害提交；
- [ ] 满弹、零备弹、换弹中开火和死亡后输入被正确处理；
- [ ] HUD 只消费真实状态，重建后恢复正确；
- [ ] 10/25/50 米三轮数据能明确区分两把武器；
- [ ] 单客户端连续战斗五分钟稳定；
- [ ] Editor Dedicated Process + 两客户端 E2E 通过；
- [ ] ShooterCore 默认玩法无回归；
- [ ] 本周未访问或修改受保护源码目录。

## 本周作品集产出

- 一分钟未剪辑战斗视频；
- 双客户端前三周 E2E 视频和三个进程日志；
- 输入到 HUD 的数据流图；
- 两把武器职责/复用资产清单；
- 10/25/50 米三轮枪感数据；
- “客户端体验验证不等于服务器安全验证”的说明。

## 参考资料

- [12 周执行基线](execution-baseline.md)
- [Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)
- [Lyra Abilities](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)
- [Lyra 武器源码](../Source/LyraGame/Weapons)

## 2026-08-11 实施记录

- Rifle/Shotgun 八项职责资产均已创建、重接引用、编译和保存；Item → WID → WeaponInstance/AbilitySet/Actor → Fire/Reload/Damage 闭环已由 MCP 检查。
- 默认装备组件只在 Server 创建，将 Rifle/Shotgun 幂等加入 Inventory 与 QuickBar 0/1；最终双客户端日志对两名玩家均记录成功。
- 项目输入层已创建：`DA_ExtractionInputActionSet`、`DA_ExtractionInputAddOns`、`IMC_Extraction`、`IA_ExtractionInteract`。Interact 仅有一个 `F -> InputTag.Ability.Interact` 映射。
- Rifle：30/60、0.12 秒间隔、单弹丸、12 基础伤害、首发精准、28 米前无衰减。
- Shotgun：8/16、0.5 秒间隔、9 弹丸、每弹丸 12 基础伤害、6 米后衰减、较大散布。
- Extraction 与 ShooterCore 两套 Editor Dedicated Process 双客户端生命周期烟测均通过；5 项 ExtractionOps Automation 全部成功。
- 本轮按要求不使用 Windows 界面控制，因此 10/25/50 米真人主观枪感采样、五分钟人工操作和视频属于后续体验/作品集证据，不在无界面自动验收中伪造。
- 详细结果见 [Week 01–03 验收记录](evidence/week-01-03-acceptance.md)。
