# 第 2 周：Lyra 架构阅读与 ExtractionOps 边界

## 本周目标

沿一条真实调用链读懂 Lyra，理解并完成项目自己的 `ExtractionOps` GameFeature Plugin。完成后应能解释 Experience 如何激活 Feature、输入如何到达 Pawn/GAS、服务器状态如何进入 HUD，同时拥有一个可加载、可停用、不会污染 Lyra 核心的最小插件。

## 执行基线

开始前完整阅读[12 周执行基线](execution-baseline.md)。本周只使用 `D:\Software\UE_5.8` 和 Editor/Editor Dedicated Process；不得访问受保护的 ue5-main 源码目录。第 1 周构建、单机和双客户端基线必须已经通过。

## 与当前实现同步（不要重复创建）

GameFeature Runtime 底座已经存在于：

```text
Plugins/GameFeatures/ExtractionOps/
  ExtractionOps.uplugin
  Config/Tags/ExtractionOpsTags.ini
  Content/ExtractionOps.uasset
  Source/ExtractionOpsRuntime/
```

当前代码已经提供：

- `UExtractionMatchStateComponent`：注入 `LyraGameState`，复制 Match、Threat、终端数量、有效撤离区和奖励倍率；
- `UExtractionRunStateComponent`：注入 `LyraPlayerState`，复制 `run_id`、玩家终态和撤离结束服务器时间；
- `AExtractionSignalTerminal` 与 `AExtractionZone`：终端和撤离区的服务器权威 Actor；
- `FExtractionStateRules`：Match、Run、Terminal、Zone 的纯状态转换规则；
- `/ExtractionOps/ExtractionOps`：GameFeatureData，通过 `GameFeatureAction_AddComponents` 注入上述两个组件。

本周应先沿这些文件反向理解实现，再创建专属 Extraction Experience。不要再创建一个平行 Subsystem 保存同一份 Match/Run 状态，否则会产生双事实来源。

### 本周要补的 Extraction Experience

资产结构固定为：

```text
/ExtractionOps/Experiences/B_ExtractionExperience
/ExtractionOps/Experiences/DA_ExtractionActionSet
/ExtractionOps/Pawns/DA_ExtractionPawnData
/ExtractionOps/UI/W_ExtractionHUDLayout
/ExtractionOps/UI/WBP_ExtractionDebugHUD
```

实施顺序：

1. 复制最接近第三人称射击的 Lyra Experience，先记录所有原始引用；
2. 改名并放入 ExtractionOps，不复制 Character 类、ASC 或动画系统；
3. 在 Experience 的 GameFeature 列表中加入 `ExtractionOps` 和必需的 `ShooterCore`；GameFeatureData 继续负责组件和 Widget Action；
4. PawnData 第一版继续引用 Lyra 可工作的 Pawn、AbilitySet、InputConfig 和武器；第 3 周再替换项目专属输入与武器链；
5. HUD Layout 本周建立骨架，Debug HUD 显示 Experience、NetMode、MatchState、Threat 和 RunState；第 3 周在同一 Layout 上增加武器 HUD；
6. 用该 Experience 启动 PIE，不再手工调用 MCP 激活 GameFeature；
7. 验证退出 Experience 后 Feature 能正确停用，不残留组件或委托。

完成信号是“加载 Extraction Experience 自动激活插件并注入组件”，而不是仅在 MCP 中看到插件可手工变为 `Active`。

## 前置条件与周门槛

- 第 1 周 Editor、Editor Dedicated Process 和双客户端基线已通过。
- `<RepoRoot>` 当前为 `D:\Document\AI\Codex\extraction-ops`；本周插件路径是 `<RepoRoot>\Plugins\GameFeatures\ExtractionOps`。
- 开始前保存 `git status --short` 和可玩基线录屏；出现回归时必须能回到此基线。

第 1 周网络基线未通过时，可以阅读已有插件和运行 Editor 测试，但不把 Experience 的 Editor Dedicated Process 验收标为完成。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 预计时间 |
| --- | --- | ---: |
| 1 | 按调用链阅读 Lyra 并建立索引 | 4 小时 |
| 2 | 创建并编译 GameFeature Plugin | 3–4 小时 |
| 3 | 理解现有权威组件并建立 Experience 激活 | 3–4 小时 |
| 4 | 添加网络角色调试 HUD | 3–4 小时 |
| 5 | 双客户端验证、架构图和阅读笔记 | 2–4 小时 |

## 先读什么

按顺序读，不随机浏览：

1. `LyraStarterGame.uproject`、`Source/LyraGame/LyraGame.Build.cs`、`LyraGameModule.cpp`；
2. `GameModes/LyraExperienceDefinition.*`、`LyraExperienceManagerComponent.*`；
3. `GameFeatures/GameFeatureAction_AddAbilities.*`、`GameFeatureAction_AddInputBinding.*`、`GameFeatureAction_AddWidget.*`；
4. `Character/LyraPawnExtensionComponent.*`、`LyraHeroComponent.*`；
5. `Player/LyraPlayerState.*`、`GameModes/LyraGameState.*`；
6. `UI/LyraHUD.*` 和 ShooterCore 中实际使用的 HUD/Experience 资产。

每项只记录：职责、生命周期、Client/Server 所在端、被谁创建、向下一环传什么数据。

## 工作单元 1：建立可查询的 Lyra 调用链

### 1.1 用搜索定位入口

```powershell
Set-Location '<RepoRoot>'
rg "ULyraExperienceDefinition|OnExperienceLoaded|GameFeature" Source Plugins
rg "BindNativeAction|BindAbilityActions|InputTag" Source Plugins
rg "GetAbilitySystemComponent|InitAbilityActorInfo" Source Plugins
rg "GameFeatureAction_AddWidget|RegisterExtension" Source Plugins
```

不要复制全部结果。建立四张表：启动与 Experience、输入与 Pawn、GAS 与 PlayerState、状态与 UI；每张表最多记录 10 个关键符号。

### 1.2 跑一遍对象生命周期

在 PIE 单机和双客户端各观察一次：

```text
GameInstance
  -> World
  -> GameMode（仅 Server）
  -> GameState（Server 创建并复制）
  -> PlayerController（连接与本地输入）
  -> PlayerState（稳定玩家状态与 ASC）
  -> Pawn（可更换控制实体）
  -> PawnExtension/HeroComponent
```

在笔记中给每个对象标记 `Server Only`、`Owning Client`、`All Peers`，并写一个错误示例，例如“客户端不能获取 GameMode”。

通过标准：能从 Experience 资产沿代码和 GameFeature Action 解释 Pawn、Ability、输入和 HUD 是如何装配的。

## 工作单元 2：复核 ExtractionOps GameFeature Plugin

### 2.1 理解创建步骤

当前插件已经创建。通过 `.uplugin`、Build.cs 和模块入口反向理解标准步骤；只有在全新练习分支中才使用 Editor 的 `Game Feature (C++)` 模板重做，当前工作区不要创建同名插件。

计划新增结构：

```text
Plugins/GameFeatures/ExtractionOps/
  ExtractionOps.uplugin
  Config/
  Content/
    Experiences/
    Input/
    UI/
  Source/ExtractionOpsRuntime/
    ExtractionOpsRuntime.Build.cs
    Public/
    Private/
```

若模板生成的模块名不同，以 `.uplugin` 中 Modules.Name 为准，但全项目只保留一个运行时模块。

### 2.2 设置最小依赖

`ExtractionOpsRuntime.Build.cs` 当前只包含玩法底座所需模块。新增依赖前先确认实际 include/API 需要它；不要提前加入 HTTP、SQLite、OnlineServices 或 Editor 模块。

重新生成项目文件并构建 `LyraEditor Win64 Development`。成功信号：插件模块编译并在 Plugins 面板中显示，没有循环依赖。

### 2.3 检查插件状态

在 Game Features 面板找到插件，验证 Load/Activate。停用测试必须先退出使用 ExtractionOps 资产的 Experience、停止 PIE 并切回默认 Lyra Experience，再执行 Deactivate；禁止在活动 World 仍引用插件资产时强制卸载。验证停用后不会破坏 Lyra 默认 Experience。

## 工作单元 3：现有权威组件与 Experience 激活

### 3.1 理解现有权威组件

当前不创建重复 Subsystem。分别阅读 MatchStateComponent、RunStateComponent 及 GameFeatureData 的组件注入配置，记录每个字段由谁写入、复制给谁、UI 如何订阅 `OnSnapshotChanged`。如果未来确实需要跨 World 的非战局服务，再单独论证 GameInstanceSubsystem；本局实时状态不得搬入它。

### 3.2 创建 Extraction Experience

从 `/ShooterCore/Experiences/B_ShooterGame_Elimination` 及其标准 ActionSet/HUD 提取最小组合，创建唯一的 `B_ExtractionExperience`。复用现有 Pawn、ASC、移动、动画、AbilitySet、输入和武器，不复制整套 Lyra 资产。

复用现有 `/ExtractionOps/ExtractionOps` GameFeatureData，不再创建第二个同名事实来源。职责固定为：

- `B_ExtractionExperience` 请求激活 `ShooterCore` 与 `ExtractionOps`；
- GameFeatureData 使用 `GameFeatureAction_AddComponents` 注入 MatchState/RunState；
- GameFeatureData 使用 `GameFeatureAction_AddWidgets` 注入调试 UI；
- 第 3 周在已有 Experience/PawnData/HUD 上增加 Input 和武器配置，不重新创建它们。

在测试地图 World Settings 或启动参数中选择 `B_ExtractionExperience`。成功信号：Experience 加载完成，MatchState/RunState 组件出现，原 Shooter 行为仍可用。

## 工作单元 4：网络角色调试 HUD

### 4.1 定义只读调试数据

定义唯一只读结构 `FExtractionNetworkDebugSnapshot`，并通过 Blueprint 可调用的 `UExtractionDebugDataLibrary::GetNetworkDebugSnapshot` 生成快照。结构字段固定为：

- NetMode；
- HasAuthority；
- LocalRole/RemoteRole；
- 本地 PlayerController 与 PlayerState 是否有效；
- 当前 Experience Primary Asset ID；
- MatchState、Threat Level 和本地 RunState 摘要。

接口只读取 GameState、PlayerState、Pawn 和 Experience Manager；不保存第二份玩法状态。Widget 可以缓存上一次用于比较的显示快照，但不得修改游戏状态。

### 4.2 创建 Blueprint Widget

在 `Content/UI` 创建 `WBP_ExtractionDebugHUD`：

1. 用文本控件显示上述字段；
2. 优先订阅已有状态委托；网络角色等无委托字段使用不高于 2 Hz 的调试定时器，禁止 Tick 全量扫描或每帧重建 Widget；
3. 使用 GameFeature Action 注入 HUD Slot；
4. 插件停用时 Widget 必须移除。

验证矩阵：单机 PIE、Listen Server、Editor Dedicated Process + 两个 Editor Client Process。拥有方应显示 AutonomousProxy，另一客户端看到的对方一般为 SimulatedProxy；Server 只写 Authority 日志，不创建 Widget。

### 4.3 Unreal Editor MCP 操作合同

创建或修改 `.uasset` 时固定执行：

1. 先发现可用 Toolset 和项目 Blueprint Agent Skill；
2. 所有 MCP 调用串行执行并逐项检查明确返回值；
3. 编辑前保存当前资产，完成一个逻辑单元后编译并再次保存；
4. 等待 Blueprint 编译完成，检查 Compiler Results 和 Output Log；
5. PIE 运行期间只观察和测试，不修改资产；
6. MCP 断开时停止资产实施，不使用二进制文件脚本伪造 `.uasset`。

## 工作单元 5：架构验收与阅读产出

### 5.1 回归测试

- [ ] 默认 Lyra Experience 仍可玩；
- [ ] Extraction Experience 可加载并显示调试 HUD；
- [ ] 插件停用后调试 HUD 消失；
- [ ] 两客户端显示正确而不同的网络角色；
- [ ] Dedicated Server 不创建客户端 UI；
- [ ] 插件日志没有依赖缺失或重复初始化。

非法路径测试：在客户端 Blueprint 中尝试读取 GameMode，只观察它为 `None`，随后删除实验节点；笔记中解释为什么应使用 GameState/PlayerState。

### 5.2 形成架构证据

提交前完成：

- Lyra Module/GameFeature/Experience 关系图；
- Gameplay Framework 网络对象职责图；
- 输入 → HeroComponent → ASC/Ability 调用链；
- GameState/PlayerState → UI 数据流；
- 不少于 1000 字源码阅读笔记；
- 30 秒双客户端网络角色视频。

## 验收目标

- [ ] 能解释 GameMode、GameState、PlayerState、Controller 和 Pawn 边界；
- [ ] 能指出至少三个现有 GameFeature 的数据资产和加载入口；
- [ ] `Plugins/GameFeatures/ExtractionOps` 是可编译的 GameFeature Plugin；
- [ ] MatchState/RunState 通过 GameFeature Action 正确注入和释放；
- [ ] `B_ExtractionExperience` 能激活 Feature；
- [ ] 调试 HUD 正确显示网络角色且不会在 Server 创建；
- [ ] Lyra 默认玩法没有回归。

## 实现原理

Lyra 的 Experience 负责描述一局需要装配哪些 Feature、PawnData、Ability 和 UI。GameFeature 让玩法可以加载和停用，减少修改核心模块造成的升级与回归风险。GameMode 只在服务器存在；公开战局状态使用 GameState，玩家稳定状态使用 PlayerState，本地连接和输入属于 PlayerController，Pawn 是可替换的受控实体。

## 常见问题与停止条件

- Blueprint 找不到 C++ 类：先检查模块导出宏、Build.cs、UCLASS 和编译日志，再重启 Editor。
- Feature 加载但 Widget 不出现：检查 Experience 是否真的激活、Action 的 HUD Slot 和 Widget 类。
- 插件依赖 Lyra 私有头：改用公共接口，不把 Lyra Private 路径加入 include。
- 客户端 GameMode 为空：这是预期，不通过复制 GameMode 修复。

若插件不能独立激活/停用或默认 Lyra 回归，停止进入第 3 周。

## 本周作品集产出

- 插件骨架与最小 Feature；
- 两张架构图和两条数据流；
- 源码阅读笔记；
- 网络角色调试视频和回归记录。

## 参考资料

- [Game Features and Modular Gameplay](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-features-and-modular-gameplay-in-unreal-engine)
- [Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine)
- [LyraGame 源码说明](../Source/LyraGame/README.md)

## 2026-08-11 实施记录

- 已创建并保存唯一的 `B_ExtractionExperience`、`DA_ExtractionActionSet`、`DA_ExtractionPawnData`、`W_ExtractionHUDLayout` 与 `WBP_ExtractionDebugHUD`。
- Experience 自动请求 `ShooterCore` 与 `ExtractionOps`，不依赖手工激活 Feature。
- `FExtractionNetworkDebugSnapshot` 与 `UExtractionDebugDataLibrary` 已实现；Widget 通过 `UExtractionDebugHUDWidget` 每 0.5 秒（2 Hz）只读刷新，不使用 Tick 扫描。
- GameFeatureData 注入 MatchState、RunState 与 Server-only 默认装备组件；Debug HUD 由项目 HUD ActionSet 注入。
- Blueprint/Widget Blueprint 编译成功，核心资产检查 Missing 0、Dirty 0。
- 详细资产链和运行证据见 [Week 01–03 验收记录](evidence/week-01-03-acceptance.md)。
