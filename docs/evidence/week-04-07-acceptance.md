# Week 04–07 实现与验收记录

## 结论

Week 04–07 的源码、项目资产、专属 Skill 和无界面多人闭环已落地。最终冷态构建成功，`ExtractionOps.*` 自动化测试 9/9 成功；Editor Dedicated Process 在 Baseline、100 ms、100 ms + 5% 丢包三组配置下均完成两次 Join、Raid 激活、单客户端退出隔离和 Server 断开通知。

本记录只声明可由源码、Unreal Editor MCP、资产反查或日志证明的事实。它不等价于 Packaged Dedicated Server，也不声明已经完成真人枪感、AI 压力可感知性或 12–15 分钟节奏验证。

## 构建与自动化

- Engine：仅使用 `D:\Software\UE_5.8` Installed Build。
- Target：`LyraEditor Win64 Development`。
- 最终构建成功；ExtractionOpsRuntime 与窄范围 Lyra 导出/TargetData 扩展钩子均完成链接。
- 最终日志：`Saved/Logs/Week04-07-FinalAutomation.log`。
- 成功测试：ThreatSchedule、DebugSnapshot.InvalidContext、ArmorDamageSplit、Inventory.CommandRules、Network.ShotValidation、Match、Run、Threat、Zone。

## Week 04：网络权威

- `ULyraGameplayAbility_RangedWeapon` 增加默认放行的服务器 TargetData 校验扩展点；这是对 Lyra 基线的明确、可索引改动，不包装成 GameFeature 自研。
- Rifle/Shotgun 的 Fire Blueprint 都重新设父类为 `UExtractionGameplayAbility_RangedWeapon`。
- Server 在扣弹和执行伤害图之前校验 Authority、来源 Controller、武器实例、Dead/Reloading/背包打开状态、弹丸数、Trace 起点误差、方向、射程、命中 Actor 和 Cartridge 重复提交。
- 弹药与冷却仍由 Lyra `CommitAbility` 权威提交；伤害只从服务器 Effect 图进入 `UExtractionDamageExecution`。
- `FExtractionNetworkValidation` 提供稳定拒绝原因与纯规则测试；不使用无 Owner 的平行 RPC Actor。

## Week 05：GAS、Armor 与终端

- `UExtractionArmorSet` 复制 `Armor/MaxArmor`，出生值为 30/30，并由 `AbilitySet_ExtractionCore` 随 PawnData 授予。
- `UExtractionDamageExecution` 先扣 Armor、再扣 Health，并保留 Lyra Team/Damage Source、距离衰减和物理材质衰减路径。
- Rifle/Shotgun Damage Effect 均引用该 Execution；自动化覆盖护甲足够、溢出和无护甲三类不变量。
- `UExtractionGameplayAbility_InteractTarget` 是终端、世界 Pickup 和 Loot Container 的统一 Server 交互入口。
- 终端状态为 `Idle -> Activating -> Activated`；结束时再次检查距离、视线、RunState 和 MatchState，失败返回 Idle，成功只由 MatchState 增加 Threat。

## Week 06：物品、背包与 UI

- Raid loot ledger 为 PlayerState 上 Owner-only 复制的 12 格背包，使用稳定 `FGuid`、`expected_version` 和 `request_id`。
- 已实现原子拾取、移动/交换、堆叠、拆分、使用和丢弃；背包满、旧版本、重复请求、错误数量、Dead/Extracted 状态均有确定结果。
- 武器装备状态不在 raid ledger 复制第二份：两把主武器继续由 Lyra Inventory/Equipment/QuickBar 唯一管理，默认由 Server 授予 QuickBar 0/1。
- 内容资产为两类弹药、一个 Medkit、八种三档 Valuable；Medkit 只在存活且 Health 未满时由 Server 应用 Effect，失败不扣数量。
- `I -> InputTag.Ability.Inventory -> UExtractionGameplayAbility_ToggleInventory` 只有一条映射；Widget 订阅 Inventory 事件与本地 ASC Tag 显隐，不保存第二份背包。
- 背包打开时两个项目 Fire Ability 被 `ExtractionOps.Inventory.Open` 阻断。

## Week 07：地图、Threat AI 与撤离结果

- `L_ExtractionTest` 的 Editor 资产包含 3 个终端、2 个撤离区、4 个普通物资区和 1 个高风险地标；MCP 反查数量为 3/2/4/1，所有 Actor 保存后无 dirty package。
- World Partition 的外部 Actor 在无本地玩家的 Editor Dedicated Process 启动早期没有进入运行世界。Director 因而先枚举运行世界，只在数量为零时生成同一 3/2/5 合同；日志记录 `event=world_contract_ready`。已有 Actor 时不会重复生成。
- Match 开始时随机选择有效撤离区，复制唯一 `match_id` 和 15 分钟 `raid_end_server_time`；到期后仍在 Raid/Extracting 的 Run 进入 Abandoned。
- 撤离由 Zone Server overlap、10 秒服务器时间和 RunState 决定；离开、死亡或 Match 结束会取消。
- Threat Director 的 Low/High/Critical 波次各有一次性 bit fence、全局 AI 上限和离玩家最小导航距离。
- 三个项目 AI Controller 分别引用三个项目 BT；Flanker BT 的移动查询只引用一个 `EQS_ExtractionFlank`。Flanker 速度更高，Elite 为 250 Health 并有独立视觉尺度。
- 死亡回调先进入 Dead，再把 raid ledger 可掉落物原子转入唯一死亡容器；终态快照 Owner-only 复制。退出时活动 Run 进入 Abandoned。

## Unreal Editor MCP 与项目 Skill 证据

MCP 验证结果：24 个必需资产存在，两个 Fire Ability 父类正确，GameFeatureData 注入 6 个组件，Flanker EQS 引用数为 1，所有检查资产均无未保存修改。

项目注册并保存了三个 Editor Agent Skill：

- `/ExtractionOps/Skills/ExtractionOpsAssetAuthoring`
- `/ExtractionOps/Skills/ExtractionOpsAuthorityWorkflow`
- `/ExtractionOps/Skills/ExtractionOpsValidationWorkflow`

`AgentSkillToolset.ListSkills/GetSkills` 在 Editor 重启后的会话中可以发现并读取三项 Skill。重复注册脚本会更新既有 Skill 并保存，而不是创建重复资产。

## 多人 E2E 证据

脚本：`Scripts/Start-Week04-07-E2E.ps1`。它复用通用多人启动器，只结束自己创建的进程。

| 配置 | Server 日志时间戳 | 结果 |
| --- | --- | --- |
| Baseline | `20260812-234200` | 两次 Join、Raid started、Client 1 退出后 Server/Client 2 存活、Server 退出后 Client 2 明确断线 |
| 100 ms | `20260812-234402` | 同上 |
| 100 ms + 5% loss | `20260812-234614` | 同上 |

三个最终 Server 日志均包含 `event=world_contract_ready`、`event=raid_started match_id=...`、两次背包/生命周期 ready 和两次 `Join succeeded`；均未出现项目级 `raid_start_failed`、缺失 Gameplay Tag、assert 或 fatal 模式。

## 尚需真人/可视化验证

- 10/25/50 米三轮 Rifle/Shotgun 枪感与 TTK 记录。
- 终端完整操作、拾取/拆分/Medkit/丢弃、死亡容器和撤离的未剪辑操作视频。
- 玩家是否能感知 Patrol/Flanker/Elite 差异，以及是否因 Threat 压力改变扫描/撤离决策。
- 12–15 分钟真实路线、导航、音频线索和节奏；当前灰盒布局只是技术 Slice。
- Network Profiler/Insights 的带宽、CPU 和 AI 热点数据留到 Week 11。

这些项目不影响源码、资产、自动化和无界面多人会话已经完成的事实，但在真人验收前不得对外宣称“玩法已打磨完成”。
