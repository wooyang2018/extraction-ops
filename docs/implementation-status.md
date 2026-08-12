# 实施状态

更新时间：2026-08-11

本文只记录已经由本机命令、Unreal Editor MCP、资产检查或运行日志验证的事实，不以计划项代替完成项。

## 当前结论

前三周的工程实现已落地，并通过可重复的无界面构建、资产契约检查、自动化测试和 Editor Dedicated Process 双客户端烟测。

当前环境始终为官方 Installed Build：`D:\Software\UE_5.8`。本轮没有读取、查询、构建、运行或修改 `D:\Software\UE_5.8.1_Source`。

当前结果仍是 Editor Dedicated Process 验证，不等价于 `LyraServer.exe`、Server Cook/Stage/Package 或 Steam 发布级 Dedicated Server。

## Week 01：环境与多人基线

- `LyraEditor Win64 Development` 最终构建：`Result: Succeeded`。
- `Scripts/Start-Week01-Multiplayer.ps1` 新增可选 `Experience` 和 `AutomatedSmokeSeconds`，可以无界面启动一个 Editor Dedicated Process 与两个 `-game -NullRHI` 客户端。
- 自动验收包含：端口监听、两次独立 Login/Join、指定 Experience 加载、单客户端退出后剩余进程存活、Server 退出后的客户端明确断线，以及只清理脚本自己创建的 PID。
- Extraction 最终通过日志：`Saved/Logs/Week01/Multiplayer-Server-20260811-233349.log`、两个同时间戳 Client 日志。
- ShooterCore 回归通过日志：`Saved/Logs/Week01/Multiplayer-*-20260811-232538.log`。
- 最终三份 Extraction 日志未出现 `Fatal error`、`LogBlueprint: Error`、`LogWindows: Error`、`ensure condition failed` 或 `LogExtractionLoadout: Error`。

## Week 02：Experience、边界与 Debug HUD

- 唯一 Experience：`/ExtractionOps/Experiences/B_ExtractionExperience`。
- Experience 请求 `ShooterCore` 与 `ExtractionOps`，引用项目 PawnData、输入 ActionSet、StandardComponents 与项目 HUD ActionSet。
- `ExtractionOps` GameFeatureData 注入：
  - `LyraGameState -> UExtractionMatchStateComponent`；
  - `LyraPlayerState -> UExtractionRunStateComponent`；
  - `LyraPlayerController -> UExtractionDefaultLoadoutComponent`，仅 Server 创建。
- 新增 `FExtractionNetworkDebugSnapshot` 与只读 `UExtractionDebugDataLibrary`。
- 新增 `UExtractionDebugHUDWidget`：0.5 秒（2 Hz）定时读取快照，不使用 Widget Tick 扫描，不写回玩法状态。
- `WBP_ExtractionDebugHUD` 由 Unreal UMG ToolSet 创建、编译并保存，绑定 `SnapshotText`；`W_ExtractionHUDLayout` 与 `DA_ExtractionActionSet` 负责 HUD 注入。

## Week 03：输入与两把武器

- PawnData 使用 `/ExtractionOps/Pawns/DA_ExtractionInputConfig`。
- 项目输入层包含 `DA_ExtractionInputActionSet`、`DA_ExtractionInputAddOns`、`IMC_Extraction` 与 `IA_ExtractionInteract`。
- Interact 只有一个映射：`F -> InputTag.Ability.Interact`；本周只建立映射，不实现终端交互行为。
- Rifle 与 Shotgun 的 Item、WID、WeaponInstance、AbilitySet、Fire、Reload、Damage Effect 和装备表现均为真实项目资产；Item 到 WID、WID 到 Instance/AbilitySet/Actor、Fire 到 Damage Effect 的引用已重接并验证。
- Server-only 默认装备组件幂等地加入 Rifle/Shotgun 到 Inventory 和 QuickBar 0/1，并激活 0 号槽；最终服务器日志对两名玩家均记录授予成功。
- Rifle 基线：30/60 弹药、0.12 秒射击间隔、单弹丸、12 基础伤害、10000 cm Trace、28 m 前无衰减、允许首发精准。
- Shotgun 基线：8/16 弹药、0.5 秒射击间隔、9 弹丸、每弹丸 12 基础伤害、1000 cm Trace、6 m 后开始衰减、较大初始散布。

## 自动化与资产验收

- Unreal Automation 发现 5 项 `ExtractionOps.*` 测试并全部通过：DebugSnapshot InvalidContext、Match、Run、Threat、Zone；最终冷态命令行日志为 `Saved/Logs/Week03-FinalAutomation.log`，退出码 0。
- MCP 资产契约检查：核心清单 23 项全部存在、0 缺失、0 Dirty；输入专项资产 5 项全部保存且引用正确。
- Blueprint/Widget Blueprint 在修改后编译并保存；最终 E2E 能由 URL 自动识别并完成加载 `LyraExperienceDefinition:B_ExtractionExperience`。
- 可重复 MCP 请求位于 `Scripts/Mcp/`；调用入口为 `Scripts/Invoke-UnrealMcp.ps1`。
- 详细证据见 [Week 01–03 验收记录](evidence/week-01-03-acceptance.md)。

## Week 04–05：网络权威与 GAS

- Rifle/Shotgun Fire Ability 已进入项目服务器 TargetData 校验路径；Lyra 基线只增加一个默认放行的 virtual hook 和跨模块导出标记，具体规则位于 ExtractionOpsRuntime。
- 校验覆盖来源、状态、弹丸数、起点、方向、射程和重复 Cartridge；Lyra `CommitAbility` 继续权威处理弹药/冷却。
- `UExtractionArmorSet` 为 30/30，`UExtractionDamageExecution` 先 Armor 后 Health；两把武器 Damage Effect 已重接。
- 终端、Pickup 和 Loot Container 共用 `UExtractionGameplayAbility_InteractTarget`，由 Server 二次校验并提交。

## Week 06：战利品背包

- PlayerState 注入 Owner-only 12 格 raid ledger；稳定 GUID、版本号和 request ID 贯穿拾取、移动、堆叠、拆分、使用、丢弃与死亡转移。
- 两类弹药、一个 Medkit、八种三档 Valuable 已保存；满血、死亡或非 InRaid 使用不会消耗 Medkit。
- `I -> InputTag.Ability.Inventory` 唯一映射和本地 Toggle Ability 已接线；Widget 事件驱动显隐，打开时阻断项目 Fire Ability。
- 两把主武器装备仍由 Lyra Inventory/Equipment/QuickBar 唯一管理，不在 raid ledger 复制平行装备状态。

## Week 07：Raid、AI 与结果

- `L_ExtractionTest` 的 Editor 资产为 3 终端、2 撤离区、4 普通物资区、1 高风险地标。
- 无本地玩家的 Editor Dedicated Process 启动早期未流送 World Partition 外部 Actor；Threat Director 会先枚举世界并仅在零实例时生成同一运行时合同，避免 Raid 因无 Zone 无法启动。
- Match 复制唯一 ID、随机撤离区和 15 分钟 Server 截止时间；超时/断线进入 Abandoned。
- Low/High/Critical 波次有一次性 fence、全局上限和最小导航距离；Patrol/Flanker/Elite 使用三个项目 Controller/BT，Flanker 使用项目 EQS。
- 死亡物品只转入一个死亡容器，结果快照 Owner-only 复制。

## Week 04–07 自动化、资产与多人验收

- 最终 `LyraEditor Win64 Development` 冷态构建成功。
- `Saved/Logs/Week04-07-FinalAutomation.log` 中 9 项 `ExtractionOps.*` 测试全部成功。
- MCP 检查 24 个必需资产、6 个 GameFeature 组件、两个 Fire 父类和 Flanker EQS；无 dirty package。
- Baseline、100 ms、100 ms + 5% loss 三组 Editor Dedicated Process + 两客户端 E2E 全部通过。
- 项目已注册并保存 `ExtractionOpsAssetAuthoring`、`ExtractionOpsAuthorityWorkflow`、`ExtractionOpsValidationWorkflow` 三个 Editor Agent Skill。
- 详细证据见 [Week 04–07 验收记录](evidence/week-04-07-acceptance.md)。

## 未由无界面脚本声称完成的主观证据

本轮按要求不使用 Windows 界面控制，因此没有伪造以下人工体验证据：10/25/50 米三轮主观枪感记录、连续五分钟人工战斗观察、未剪辑操作视频。实现与参数链已经就绪，这些属于后续真人体验采样和作品集录制，不影响源码、资产与自动 E2E 的完成状态。

## 下一阶段

进入 Week 08 前，先由真人补录终端、背包、死亡容器、撤离和三类 AI 的完整操作证据；随后按 Week 08–10 契约接入 Go 身份、双人房、Ticket、仓库与幂等结算。当前不扩展 Packaged Server Target，也不触碰受保护的源码引擎目录。
