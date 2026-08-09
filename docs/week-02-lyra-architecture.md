# 第 2 周：Lyra 架构阅读与 ExtractionOps 边界

## 本周目标

沿一条真实调用链读懂 Lyra，并创建项目自己的 `ExtractionOps` GameFeature Plugin。完成后应能解释 Experience 如何激活 Feature、输入如何到达 Pawn/GAS、服务器状态如何进入 HUD，同时拥有一个可加载、可停用、不会污染 Lyra 核心的最小插件。

## 前置条件与周门槛

- 第 1 周 Editor、Server 和双客户端基线已通过。
- `<RepoRoot>` 当前为 `D:\Document\AI\Codex\extraction-ops`；本周新增路径是 `<RepoRoot>\Plugins\ExtractionOps`。
- 开始前保存 `git status --short` 和可玩基线录屏；出现回归时必须能回到此基线。

第 1 周网络基线未通过时，不创建插件。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 预计时间 |
| --- | --- | ---: |
| 1 | 按调用链阅读 Lyra 并建立索引 | 4 小时 |
| 2 | 创建并编译 GameFeature Plugin | 3–4 小时 |
| 3 | 建立 Experience 激活与最小 Subsystem | 3–4 小时 |
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

## 工作单元 2：创建 ExtractionOps GameFeature Plugin

### 2.1 在 Editor 创建插件

1. 打开 `Edit > Plugins`；
2. 选择 `Add/新建插件`；
3. 选择 `Game Feature (C++)` 模板；
4. 名称填写 `ExtractionOps`；
5. 保持 Runtime 类型，创建后关闭 Editor。

计划新增结构：

```text
Plugins/ExtractionOps/
  ExtractionOps.uplugin
  Config/
  Content/
    Experiences/
    Input/
    UI/
  Source/ExtractionOps/
    ExtractionOps.Build.cs
    Public/
    Private/
```

若模板生成的模块名不同，以 `.uplugin` 中 Modules.Name 为准，但全项目只保留一个运行时模块。

### 2.2 设置最小依赖

`ExtractionOps.Build.cs` 只加入当前最小 Feature 所需模块：`Core`、`CoreUObject`、`Engine`、`GameplayTags`、`GameFeatures`、`ModularGameplay`，以及调用 Lyra 公共类型时需要的 `LyraGame`。不要提前加入 HTTP、SQLite、OnlineServices 或 Editor 模块。

重新生成项目文件并构建 `LyraEditor Win64 Development`。成功信号：插件模块编译并在 Plugins 面板中显示，没有循环依赖。

### 2.3 检查插件状态

在 Game Features 面板找到插件，分别执行 Load/Activate 与 Deactivate。验证停用后不会破坏 Lyra 默认 Experience。

## 工作单元 3：最小 Subsystem 与 Experience 激活

### 3.1 创建 Subsystem

在插件中新增 `UExtractionGameInstanceSubsystem`：

- 继承 `UGameInstanceSubsystem`；
- `Initialize`/`Deinitialize` 只记录结构化日志；
- 暂存当前本地项目阶段和后续 Backend Service 入口；
- 不保存生命、伤害、库存或服务器战局规则。

日志至少包括 `event=extraction_subsystem_initialized` 和当前 NetMode。关闭 PIE 时应看到 deinitialize 日志。

### 3.2 创建 Extraction Experience

在插件 Content 中复制最接近的 ShooterCore Experience 或创建引用它的组合资产，命名 `B_ExtractionExperience`。复用现有 PawnData、输入和武器，不复制整套 Lyra 资产。

创建对应 GameFeatureData，并添加最小 Action：

- 激活插件；
- 使用 `GameFeatureAction_AddWidget` 注入调试 UI；
- 后续周再增加 Ability/Input Action。

在测试地图 World Settings 或启动参数中选择 `B_ExtractionExperience`。成功信号：Experience 加载完成，Subsystem 日志出现，原 Shooter 行为仍可用。

## 工作单元 4：网络角色调试 HUD

### 4.1 定义只读调试数据

创建一个轻量 C++ Widget/ViewModel 基类或数据提供组件，向 Blueprint 暴露：

- `GetNetMode()`；
- `HasAuthority()`；
- `GetLocalRole()`/`GetRemoteRole()`；
- 本地 PlayerController 与 PlayerState 是否有效；
- 当前 Experience 名称。

不在 Widget 中缓存或修改游戏状态。

### 4.2 创建 Blueprint Widget

在 `Content/UI` 创建 `WBP_ExtractionDebugHUD`：

1. 用文本控件显示上述字段；
2. 通过事件或低频调试定时器刷新，禁止每帧重建 Widget；
3. 使用 GameFeature Action 注入 HUD Slot；
4. 插件停用时 Widget 必须移除。

验证矩阵：单机 PIE、Listen Server、独立 Server + 两 Client。拥有方应显示 AutonomousProxy，另一客户端看到的对方一般为 SimulatedProxy，Server 显示 Authority。

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
- [ ] `Plugins/ExtractionOps` 是可编译的 GameFeature Plugin；
- [ ] `UExtractionGameInstanceSubsystem` 正确初始化和释放；
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
