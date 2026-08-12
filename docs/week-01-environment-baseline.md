# 第 1 周：Installed Build 环境与可重复多人基线

## 本周目标

把工程从“能够打开”变成可以重复构建和验证的 UE 5.8 基线：构建 Editor、运行原始 Lyra 战斗、启动一个 Editor Dedicated Process，并让两个 Editor Client Process 进入同一局。本周不增加玩法。

## 执行基线与当前事实

开始前完整阅读[12 周执行基线](execution-baseline.md)。本周唯一引擎是 `D:\Software\UE_5.8`，不得访问基线中声明的受保护 ue5-main 源码目录。

当前已经验证：

- `LyraEditor Win64 Development` 曾经构建成功；
- ExtractionOps 状态规则测试和 PIE 组件注入曾经通过；
- `D:\Software\UE_5.8` 是 Installed Build；
- 该 Installed Build 不支持 `LyraServer Win64 Development`。

因此本周不准备源码引擎，不重复执行预期失败的 Server Target 构建。多人运行验收使用 `UnrealEditor.exe -server -NullRHI` 和两个 `-game` 进程。这个结果证明独立无本地玩家的网络闭环，不代表已经完成可发布的 `LyraServer.exe`。

## 前置条件与路径

- `<RepoRoot>`：`D:\Document\AI\Codex\extraction-ops`。
- `<UE_ROOT>`：固定为 `D:\Software\UE_5.8`。
- `<Project>`：`<RepoRoot>\LyraStarterGame.uproject`。
- `<Map>`：`/ShooterMaps/Maps/L_Convolution_Blockout`。
- Visual Studio 2022 已安装 C++ 游戏开发负载、MSVC v143 和匹配 Windows SDK。
- Lyra 内容已经复制到当前工作区。本周只读验证，不重新运行可能覆盖 Content 的初始化流程。

开始前执行：

```powershell
Set-Location '<RepoRoot>'
git status --short
git branch --show-current
git rev-parse HEAD
```

若工作区已有修改，先记录其所有者和范围；不得清理、覆盖或夹带无关改动。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 环境、资产与仓库边界核验 | 2–3 小时 |
| 2 | Editor 构建和单机 Lyra 基线 | 3–4 小时 |
| 3 | Editor Dedicated Process 与双客户端 | 4–5 小时 |
| 4 | 启动脚本、日志断言和故障测试 | 3–4 小时 |
| 5 | 从零复验和证据整理 | 3–4 小时 |

## 先读什么

1. [12 周执行基线](execution-baseline.md)：理解允许的引擎和进程边界。
2. `LyraStarterGame.uproject`：确认 `EngineAssociation`、模块和插件。
3. `Source/LyraEditor.Target.cs`、`Source/LyraClient.Target.cs`、`Source/LyraServer.Target.cs`：理解 Target 与运行进程不是同一个概念。
4. `Scripts/Start-Week01-LocalMatch.ps1` 和 `Scripts/Start-Week01-Multiplayer.ps1`：理解端口、日志、进程所有权和清理策略。
5. `Plugins/GameFeatures/ExtractionOps/ExtractionOps.uplugin`：确认自研插件边界。

阅读产出必须能解释：为什么 Editor 构建成功不等于 Server Target 可构建，以及 Editor Dedicated Process 能证明什么、不能证明什么。

## 工作单元 1：环境与资产只读核验

### 1.1 固定环境记录

记录：

```text
date:
repository_commit:
engine_root: D:\Software\UE_5.8
engine_version:
build_configuration: Win64 Development
visual_studio:
msvc:
windows_sdk:
lyra_asset_source:
```

使用 Installed Build 的 `UnrealEditor.exe -Version` 或 Editor About 信息记录完整版本。不得从受保护源码目录读取版本或 commit。

### 1.2 检查资产和版本控制边界

- 根 `Content/`、ShooterCore、ShooterMaps 和 ExtractionOps 可访问；
- `LyraStarterGame.uproject` 仍指向预期 UE 5.8 基线；
- 不重新执行 `Initialize-ExtractionOps.ps1`；
- `Plugins/GameFeatures/ExtractionOps/Content/**` 被 Git 跟踪；
- Lyra 原始 Content、Binaries、Intermediate 和 Saved 被忽略。

```powershell
git status --short --ignored
git check-ignore Binaries Intermediate Saved Content
git check-ignore -v Plugins/GameFeatures/ExtractionOps/Content/ExtractionOps.uasset
```

最后一条应命中 `.gitignore` 中针对 ExtractionOps Content 的否定规则，使自研资产出现在 `git status`，而不是被忽略。

## 工作单元 2：构建 Editor 与单机基线

### 2.1 生成项目文件

```powershell
& '<UE_ROOT>\Engine\Build\BatchFiles\GenerateProjectFiles.bat' `
  -project='<Project>' -game -engine
```

若 Installed Build 不提供该批处理入口，使用同一引擎内的 UnrealBuildTool 项目文件生成入口；不得下载其他版本 UBT。

### 2.2 构建 Editor

```powershell
& '<UE_ROOT>\Engine\Build\BatchFiles\Build.bat' `
  LyraEditor Win64 Development `
  '-Project=<Project>' -WaitMutex
```

硬门槛：退出码为 0，ExtractionOpsRuntime 可加载，没有缺失模块或循环依赖。

`LyraClient Win64 Development` 可以作为补充实验；若 Installed Build 或未 Cook 内容阻止运行，记录第一条真实错误即可，不把它作为本周失败。禁止执行已知不支持的 `LyraServer` Target。

### 2.3 单机操作序列

使用 `Scripts/Start-Week01-LocalMatch.ps1` 或等价命令进入原始 Shooter Experience：

1. 移动、跳跃和观察；
2. 瞄准、射击、换弹和切枪；
3. 观察生命、弹药、准星、死亡和重生；
4. 连续运行五分钟；
5. 停止后记录日志中的第一条 Warning/Error，并判断是否影响验收。

同时记录实际 Experience、PawnData、InputConfig、AbilitySet、武器 ItemDefinition 和 HUD Layout 路径，供第 2–3 周使用。

## 工作单元 3：Editor Dedicated Process 与双客户端

### 3.1 启动 Server

优先使用 `Scripts/Start-Week01-Multiplayer.ps1`。等价核心命令为：

```powershell
& '<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor.exe' `
  '<Project>' '/ShooterMaps/Maps/L_Convolution_Blockout?NumBots=2' `
  -server -NullRHI -unattended -NoSound -NoSplash `
  -port=7777 -log
```

成功信号：目标地图加载完成、NetDriver 创建成功、日志出现 `IpNetDriver listening on port 7777`。没有渲染窗口是预期行为。

### 3.2 启动两个客户端

```powershell
& '<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor.exe' `
  '<Project>' '127.0.0.1:7777' `
  -game -log -windowed -ResX=900 -ResY=600
```

第二个客户端使用不同日志和窗口位置。必须证明：

- Server 日志出现两个独立 Login/Join；
- 两个客户端进入同一地图和 NetDriver；
- 双方能看到对方移动；
- Server 没有本地 PlayerController、Pawn 或客户端 Widget；
- 关闭一个客户端后另一个客户端和 Server 继续运行；
- 关闭 Server 后客户端得到明确断线表现，不是假死。

## 工作单元 4：脚本与故障验证

启动脚本必须：

- 只接受 `D:\Software\UE_5.8` 或调用者显式传入且等价的 UE 5.8 Installed Build；
- 不搜索、读取或回退到受保护源码目录；
- 启动前验证端口未占用；
- 为 Server、Client1、Client2 写入独立时间戳日志；
- 等待监听和两次 Join，超时后输出日志尾部；
- 只清理脚本自己创建的 PID；
- 支持只验证参数而不启动进程的模式。

固定故障实验：端口已占用、错误地图、客户端早于 Server、关闭一个客户端、关闭 Server。每项记录可观察错误和恢复动作。

## 工作单元 5：从零复验与证据

关闭所有 UE 进程后，完整执行一次：

```text
记录版本 -> 生成项目文件 -> 构建 Editor -> 单机五分钟
-> 启动 Server -> 启动两个 Client -> 两次 Join
-> 移动复制 -> 退出一个 Client -> 关闭 Server
```

任何步骤依赖未记录的手工修复，都不算通过。

保存：

- Editor 构建命令、耗时和退出码；
- Server 与两个 Client 的完整启动参数和日志；
- 30 秒单机视频和 30 秒双客户端视频；
- Target、进程和权威边界图；
- “Editor Dedicated Process 不等于 Packaged Server”的限制说明。

## 验收目标

- [ ] 使用 `D:\Software\UE_5.8` 构建 `LyraEditor Win64 Development` 成功；
- [ ] 原始 ShooterCore 连续运行五分钟，无崩溃或缺失资产；
- [ ] Editor Dedicated Process 监听 7777，且没有本地玩家或客户端 UI；
- [ ] 两个 Editor Client Process 完成独立 Login/Join 并进入同一 NetDriver；
- [ ] 双方看到移动复制，退出与断线行为正确；
- [ ] 三个进程的命令、日志和退出结果可复现；
- [ ] Git 忽略 Lyra 原始内容和生成目录，但跟踪 ExtractionOps 自研 Content；
- [ ] 本周未访问或修改受保护源码目录；
- [ ] 记录明确说明尚未完成 `LyraServer.exe`、Server Cook 或发布包。

## 停止条件

- Editor 构建失败：只处理第一条真实编译/模块错误；不通过修改 Lyra 核心绕过工具链问题。
- Server 未监听：依次检查地图包路径、端口、Experience 和日志第一条 Error。
- 两客户端不能互见：先确认两次 Join 和相同 NetDriver，再检查 Pawn 复制。
- 任一基线不稳定：停止进入第 2 周，不增加新资产掩盖问题。

## 本周作品集产出

- 可重复构建与启动记录；
- Editor Dedicated Process + 双客户端视频；
- Target/进程/权威边界图；
- 环境清单、故障矩阵和已知限制。

## 参考资料

- [12 周执行基线](execution-baseline.md)
- [Lyra Sample Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)
- [Setting Up Dedicated Servers](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers-in-unreal-engine)
- [项目 Target](../Source/LyraServer.Target.cs)

## 2026-08-11 实施记录

- `D:\Software\UE_5.8` 下 `LyraEditor Win64 Development` 构建成功。
- `Start-Week01-Multiplayer.ps1` 已支持无界面自动烟测、指定 Experience、双 Join、单客户端退出存活和 Server 断线断言。
- Extraction 最终通过日志：`Saved/Logs/Week01/Multiplayer-*-20260811-233349.log`。
- ShooterCore 回归通过日志：`Saved/Logs/Week01/Multiplayer-*-20260811-232538.log`。
- 详细证据见 [Week 01–03 验收记录](evidence/week-01-03-acceptance.md)。
