# 第 1 周：环境基线与可重复构建

## 本周目标

把工程从“偶尔能打开”变成可重复验证的 UE 5.8 开发基线：能够初始化 Lyra 资源、生成项目文件、分别构建 Editor/Client/Server、启动独立 Dedicated Server，并让两个客户端进入同一局。本周不添加玩法。

## 与当前工程同步（先读）

当前已经完成 `LyraEditor Win64 Development` 构建，并通过 Unreal Editor MCP 运行了 ExtractionOps 自动化测试和 PIE 组件注入验证。不要重新创建工程或覆盖现有插件。

当前硬门槛是 `D:\Software\UE_5.8\Engine\Build\InstalledBuild.txt` 表明该引擎是 Installed Build，执行 `LyraServer Win64 Development` 会返回：

```text
Server targets are not currently supported from this engine distribution.
```

因此本周真正要完成的不是修改游戏代码，而是准备“支持 Server Target 的 UE 5.8 源码构建”，然后依次验证：

1. 用新引擎构建 `LyraEditor`，确认项目与插件没有版本偏差；
2. 构建 `LyraServer` 和 `LyraClient`；
3. Server 独立进程加载 Lyra 基线地图；
4. 两个 Client 直连同一 Server；
5. 保存三个进程的命令、日志和版本信息。

只有上述五项全部通过，才把第 1 周标记完成。当前事实与后续变化同步记录在 [实施状态](implementation-status.md)。

## 前置条件与路径约定

- Windows 11、Epic Games Launcher、Unreal Engine 5.8 和 Visual Studio 2022 已安装。
- Visual Studio Installer 中启用“使用 C++ 的游戏开发”、MSVC v143、Windows 10/11 SDK 和适用于 UE 的工具。
- `<RepoRoot>` 表示仓库根目录；当前示例为 `D:\Document\AI\Codex\extraction-ops`。
- `<UE_ROOT>` 表示 UE 5.8 安装目录，例如 `C:\Program Files\Epic Games\UE_5.8`。
- `<LyraSource>` 表示通过 Fab/Launcher 创建的原始 Lyra 5.8 工程。

开始前执行：

```powershell
Set-Location '<RepoRoot>'
git status --short
git branch --show-current
```

成功信号：位于预期分支，且能区分已有修改。不要删除或覆盖不属于本周的修改。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 预计时间 |
| --- | --- | ---: |
| 1 | 记录工具链并初始化 Lyra 资源 | 3–4 小时 |
| 2 | 生成项目文件并构建 Editor | 3–4 小时 |
| 3 | 建立单机可玩基线 | 2–3 小时 |
| 4 | 构建 Server/Client 并完成双客户端连接 | 4–5 小时 |
| 5 | Git、日志、复现脚本和周验收 | 3–4 小时 |

如果工作单元 4 未完成，顺延本周，不进入第 2 周。

## 先读什么

1. `LyraStarterGame.uproject`：确认 `EngineAssociation`、模块和插件。
2. `Source/LyraEditor.Target.cs`、`Source/LyraClient.Target.cs`、`Source/LyraServer.Target.cs`：理解三类构建目标。
3. `Source/LyraGame/LyraGame.Build.cs`：了解模块依赖。
4. `Scripts/Initialize-ExtractionOps.ps1` 和 `docs/asset-bootstrap.md`：了解资源初始化行为。
5. Epic 的 Lyra、项目文件生成和 Dedicated Server 文档。

阅读记录只回答：文件解决什么问题、在哪种 Target 中运行、失败时去哪看日志。

## 工作单元 1：记录环境并初始化资源

### 1.1 建立环境记录

在个人学习日志中记录：

```text
日期：
仓库 commit：
UE 版本与路径：
Visual Studio 版本：
MSVC/Windows SDK：
LyraSource：
资源模式：CopyAssets：
```

用以下命令获取可复制的信息：

```powershell
git rev-parse HEAD
& '<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor.exe' -Version
```

### 1.2 启用 Codex 的 MCP 工具（可选）

如果本周使用 Codex 调用本地开发工具，先启动 Model Context Protocol（MCP）Server，再刷新 Codex 的工具清单。MCP Server 负责提供工具端点；Codex 刷新后才能发现新增、删除或更新过的工具。刷新工具不会代替启动 Server，Server 启动也不代表 Codex 已经拿到最新工具定义。

在提供 `ModelContextProtocol` 命令的 IDE/宿主中，按以下顺序执行：

```text
ModelContextProtocol.StartServer
ModelContextProtocol.RefreshTools Codex
```

验证：

- [ ] `StartServer` 执行后没有启动失败或端口占用错误；
- [ ] `RefreshTools Codex` 完成后，Codex 能看到预期的 MCP 工具；
- [ ] 工具调用指向当前仓库和当前测试环境，没有误连生产环境；
- [ ] 环境记录中写下 Server 名称、工具清单版本和本次刷新时间。

常见问题：

- Codex 看不到工具：先重新执行 `ModelContextProtocol.StartServer`，确认 Server 仍在运行，再执行 `ModelContextProtocol.RefreshTools Codex`；
- 工具清单是旧的：配置或工具代码变更后重新刷新，必要时重启宿主；
- Server 启动失败：检查端口占用、工作目录、依赖和权限，不要把 Token、密钥或本机绝对路径写入仓库；
- 工具能看到但调用失败：区分“工具发现成功”和“工具运行成功”，保存调用参数、错误和对应 Server 日志。

### 1.3 初始化 Lyra 资源

关闭 Unreal Editor，再执行：

```powershell
Set-Location '<RepoRoot>'
.\Scripts\Initialize-ExtractionOps.ps1 `
  -LyraProject '<LyraSource>' `
  -EngineRoot '<UE_ROOT>'
```

当前初始化脚本使用复制模式，将 Lyra 根 `Content/` 和仓库中对应插件的 `Content/` 复制到当前工作区；脚本不创建 Junction，也没有 `-CopyAssets` 参数。复制得到的 Lyra 原始 `Content/` 继续由 `.gitignore` 排除，修改不会回写 `LyraSource`；项目自研的 `Plugins/GameFeatures/ExtractionOps/Content/**` 已被显式豁免并必须进入版本控制。

### 1.4 验证结果

- [ ] 脚本识别到 UE 5.8 与 Lyra 5.8。
- [ ] 根 `Content` 和需要的插件内容可访问。
- [ ] `LyraStarterGame.uproject` 未被替换成别的版本。
- [ ] `git status --short` 没有出现大批 `.uasset`、`Binaries` 或 `Intermediate`。

失败时按顺序检查：LyraSource 是否包含 `.uproject` → 版本是否一致 → Editor 是否占用目录 → PowerShell 执行策略 → 目标资源目录是否已存在且不是错误的重解析点。不要手工复制一半资源后继续。

## 工作单元 2：生成项目文件并构建 Editor

### 2.1 生成项目文件

优先再次运行初始化脚本的项目文件步骤。若需手动执行，使用：

```powershell
& '<UE_ROOT>\Engine\Build\BatchFiles\GenerateProjectFiles.bat' `
  -project='<RepoRoot>\LyraStarterGame.uproject' -game -engine
```

如果 UE 5.8 安装中没有该脚本，使用初始化脚本已经支持的 UnrealBuildTool 路径，不自行下载其他版本 UBT。

成功信号：生成 `.sln`/Rider 项目模型，输出最后没有 `ERROR`，`Source` 下所有 Target 可见。

### 2.2 命令行构建 Editor

```powershell
& '<UE_ROOT>\Engine\Build\BatchFiles\Build.bat' `
  LyraEditor Win64 Development `
  '-Project=<RepoRoot>\LyraStarterGame.uproject' -WaitMutex -FromMsBuild
```

成功信号：退出码为 0，末尾出现 `BUILD SUCCESSFUL` 或等价成功信息，并生成可加载的 LyraEditor 模块。

### 2.3 构建失败处理

只处理第一条真正的 `error C...`、`UnrealBuildTool Exception` 或缺失模块信息：

1. 确认命令使用 UE 5.8；
2. 确认 VS 工作负载完整；
3. 确认 `EngineAssociation` 为 `5.8`；
4. 重新生成项目文件后只重试一次；
5. 仍失败则把第一条错误、完整命令、环境版本写入学习日志。

不要通过修改 Lyra 源码来绕过工具链错误。

## 工作单元 3：建立 Editor 与单机可玩基线

### 3.1 启动 Editor

```powershell
& '<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor.exe' `
  '<RepoRoot>\LyraStarterGame.uproject' -log
```

等待 Shader 编译和 Asset Registry 扫描结束。打开 Lyra 自带的可玩 Experience/地图，不创建新地图。

### 3.2 执行固定操作序列

1. 使用 PIE 单玩家启动；
2. 进入默认 Shooter Experience；
3. 移动、跳跃、瞄准、射击、切换武器；
4. 观察生命、弹药、准星和死亡/重生；
5. 停止 PIE，保存 Output Log 中的第一条 Warning/Error 摘要。

记录现有资产路径：Experience、PawnData、InputConfig、武器 ItemDefinition、HUD Layout。第 3 周将复用这些资产。

通过标准：连续运行 5 分钟，无崩溃、缺失资产红字或无法进入玩法。

## 工作单元 4：独立 Server、Client 与双客户端连接

### 4.1 构建 Server 和 Client

分别执行：

```powershell
& '<UE_ROOT>\Engine\Build\BatchFiles\Build.bat' `
  LyraServer Win64 Development `
  '-Project=<RepoRoot>\LyraStarterGame.uproject' -WaitMutex

& '<UE_ROOT>\Engine\Build\BatchFiles\Build.bat' `
  LyraClient Win64 Development `
  '-Project=<RepoRoot>\LyraStarterGame.uproject' -WaitMutex
```

若 Development Client/Server 依赖已 Cook 内容，本周允许使用 UnrealEditor 的 `-server` 和 `-game` 进程完成网络基线，但必须把限制记录下来；第 9 周再完成独立打包进程。

### 4.2 启动专用服务器

先从 Editor 的目标地图复制准确包路径 `<MapPath>`，例如 `/Game/.../L_Convolution_Blockout`，再启动：

```powershell
& '<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor.exe' `
  '<RepoRoot>\LyraStarterGame.uproject' '<MapPath>' `
  -server -log -port=7777 -unattended -NoSound
```

成功信号：日志显示地图加载完成、NetDriver 创建成功并监听 `7777`；没有窗口渲染并不代表失败。

### 4.3 启动两个客户端

打开两个 PowerShell 窗口，各执行一次：

```powershell
& '<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor.exe' `
  '<RepoRoot>\LyraStarterGame.uproject' 127.0.0.1:7777 `
  -game -log -windowed -ResX=1280 -ResY=720
```

验证：

- [ ] Server 日志出现两个不同连接；
- [ ] 两个客户端进入同一地图；
- [ ] 双方能看到对方移动；
- [ ] 任一客户端退出后 Server 继续运行；
- [ ] Server 关闭后客户端得到明确断开表现，不是假死。

连接失败按顺序检查：Server 是否监听 → 地图包路径 → 端口占用/防火墙 → Client 与 Server 构建是否相同 → Experience 是否成功加载。

## 工作单元 5：Git、日志和可重复验收

### 5.1 检查仓库边界

```powershell
git status --short --ignored
git check-ignore Binaries Intermediate Saved Content
git check-ignore -v Plugins/GameFeatures/ExtractionOps/Content/ExtractionOps.uasset
```

确认生成目录和本地 Lyra 资产不会进入提交，同时确认最后一条命令命中 `.gitignore` 中的 `!Plugins/GameFeatures/ExtractionOps/Content/**` 否定规则，自研资产出现在 `git status`。不得提交账号、EOS 凭据、本机绝对引擎路径或 Saved 日志。

### 5.2 保存证据

- 30 秒 Editor 单机可玩录屏；
- 30 秒 Server 监听和两个连接的日志录屏；
- Editor/Client/Server 三次构建的命令、耗时和退出码；
- 一张 Target、模块和进程关系图；
- 一篇“为什么 Editor 能运行不等于 Server 能运行”的短文。

### 5.3 最终验收脚本

从关闭所有 UE 进程开始，按“生成项目文件 → 构建 Editor → 启动 Server → 启动两个 Client → 加入 → 退出”完整重做一次。任何一步依赖未记录的手工修复，本周都不算通过。

## 验收目标

- [ ] UE 5.8 能打开工程且无缺失模块/资产错误；
- [ ] `LyraEditor Win64 Development` 构建成功；
- [ ] `LyraClient` 与 `LyraServer` Target 可构建，或已明确记录 Cook 限制和 Editor 进程替代方案；
- [ ] 独立 Server 监听 7777，两个客户端能进入同一局；
- [ ] 原始 Lyra 战斗回路可以稳定运行；
- [ ] Git 不追踪生成目录、Lyra 原始 Content 和本机密钥，但追踪 ExtractionOps 自研 Content；
- [ ] 陌生人只看记录就能重复本周流程。

## 实现原理

Unreal 的 Editor、Client 和 Server 是不同 Target。Editor 成功不代表无渲染 Server 或独立 Client 一定成功。先建立可重复基线，后续每次故障才能区分是环境问题还是本周代码问题。

Dedicated Server 保存战局权威状态；客户端负责输入和表现。后续的伤害、拾取、撤离和结算都建立在这条边界上。

## 常见问题与停止条件

- 模块缺失：核对版本、Target、Build.cs 和项目文件，不删除源码。
- Server 启动后立即退出：查看第一条 Error，核对地图、Experience 和 Server 可用插件。
- 两客户端不能互相看到：先确认是否真的连接同一 Server，再检查地图和 Experience。
- 初次启动很慢：Shader/Derived Data 构建可等待；若日志 10 分钟无进展再诊断。

若 Editor、Server 或双客户端任一基线不稳定，停止增加功能，继续补课本周。

## 本周作品集产出

- 构建与运行记录；
- Client/Server 录屏；
- Target 与进程关系图；
- 环境复现笔记和已知问题清单。

## 参考资料

- [Lyra Sample Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)
- [Setting Up Dedicated Servers](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers-in-unreal-engine)
- [资源初始化说明](asset-bootstrap.md)
- [项目 Target](../Source/LyraServer.Target.cs)
