# UE 5.8.1 Windows 源码版构建与项目 Worktree 隔离手册

本手册用于在 Windows 上准备 UE 5.8.1 源码版，并在不污染 Launcher 版本项目的前提下构建 Lyra Editor、Client 和 Dedicated Server。

文档只使用路径占位符，不包含任何特定机器路径。请只在本地 PowerShell 会话中填写路径变量，不要把实际路径提交到项目文档或公开仓库。

## 1. 目标与隔离原则

最终验收目标：

~~~text
源码版 UnrealEditor.exe
源码版 LyraEditor
源码版 LyraClient
源码版 LyraServer
Windows Client/Server Cook 内容
一个 Dedicated Server + 两个 Client 的本地连接验证
~~~

Launcher UE 和源码 UE 必须视为两套不同的工具链。即使版本号相同，也可能存在不同的引擎二进制、头文件、Build ID、编译器 ABI 和生成规则。

以下目录是引擎相关生成物，不能在两套引擎之间复用：

- Binaries：模块 DLL、Editor、Client、Server 可执行文件；
- Intermediate：UHT、UBT、编译中间文件和自动生成代码；
- Saved：日志、Cook 结果、崩溃报告和本地会话数据；
- .sln、IDE 项目模型和 UnrealBuildTool 缓存；
- Shader、Cook 和 Pak 产物。

独立 Git worktree 为项目提供第二个物理目录。每个目录拥有自己的分支、索引、Binaries、Intermediate 和 Saved，因此源码 UE 的构建不会覆盖 Launcher 项目的生成物。Git 对象数据库仍可共享，不需要复制整个项目历史。

注意：worktree 不会自动隔离全局 Derived Data Cache。若要进行严格的 Shader 或性能对比，应另外为两套环境配置独立 DDC 目录。

## 2. 路径变量

将占位符替换成自己的路径；下面的值只是变量名，不是可直接使用的路径。

~~~powershell
$SourceUE_ROOT = '<SourceUE_ROOT>'       # 源码版 UE 5.8.1 根目录
$LauncherUE_ROOT = '<LauncherUE_ROOT>'   # Launcher 安装的 UE 根目录
$RepoRoot = '<RepoRoot>'                 # 当前 Extraction Ops Git 仓库
$SourceTestRoot = '<SourceTestRoot>'     # 源码 UE 专用项目 worktree
$LyraSourceRoot = '<LyraSourceRoot>'     # 原始 Lyra 项目或资源来源
~~~

约束：

1. SourceUE_ROOT 不要放进 RepoRoot 内部；
2. SourceTestRoot 不要与 RepoRoot 相同；
3. Launcher 项目继续使用 RepoRoot，源码 UE 项目只使用 SourceTestRoot；
4. 路径尽量短，避免超过 Windows 路径长度限制；
5. 源码、依赖、引擎编译和项目 Cook 所在卷预留至少 180–250 GB。

## 3. 阶段 0：环境预检

### 3.1 Git 和长路径

~~~powershell
$PSVersionTable.PSVersion
git --version
git config --global core.longpaths true
git config --global --get core.longpaths
~~~

成功标准：PowerShell 和 Git 可用，最后一条命令输出 true。失败时停止，先修复 Git 或长路径配置。

### 3.2 磁盘空间

~~~powershell
Get-PSDrive -PSProvider FileSystem
Get-Volume | Select-Object DriveLetter,SizeRemaining,Size
~~~

首次 Setup.bat 会下载大量依赖，完整源码构建还会产生中间文件、符号文件和 Cook 产物。空间不足时不要开始下载。

### 3.3 C++ 工具链

Visual Studio Installer 中确认已安装：

- Desktop development with C++；
- MSVC x64/x86 C++ build tools；
- Windows 10/11 SDK；
- C++ CMake tools（建议）；
- Visual Studio Tools for Unreal Engine（建议）。

检查安装实例：

~~~powershell
vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
~~~

成功标准：命令返回安装目录，并且该目录下存在 MSBuild\Current\Bin\MSBuild.exe。工具链“存在”不等于一定兼容；最终以实际 Build.bat 结果为准。

### 3.4 保留 Launcher 基线

~~~powershell
Test-Path (Join-Path $LauncherUE_ROOT 'Engine\Binaries\Win64\UnrealEditor.exe')
Test-Path (Join-Path $RepoRoot 'LyraStarterGame.uproject')
git -C $RepoRoot status --short
~~~

成功标准：Launcher Editor 和主项目都存在。记录主 worktree 当前未提交修改，后续不要把它们误认为测试 worktree 的内容。

## 4. 阶段 1：获取固定版本源码

UE 源码仓库需要完成 Epic 账号与 GitHub 账号关联并接受组织邀请。官方说明见 [Downloading Source Code in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/downloading-source-code-in-unreal-engine)。

### 4.1 验证权限和标签

~~~powershell
gh auth status
gh repo view EpicGames/UnrealEngine --json nameWithOwner,isPrivate,defaultBranchRef,url
gh api 'repos/EpicGames/UnrealEngine/git/matching-refs/tags/5.8.1-release' --jq '.[] | [.ref,.object.sha] | @tsv'
~~~

成功标准：仓库可读，并能看到 5.8.1-release 标签。若返回 404 或无权限，先处理 Epic/GitHub 关联、组织邀请和 gh auth login，不要改用第三方镜像。

### 4.2 克隆固定标签

确认目标目录不存在后执行：

~~~powershell
if (Test-Path $SourceUE_ROOT) { throw 'Source UE directory already exists. Inspect it before cloning.' }
gh repo clone EpicGames/UnrealEngine $SourceUE_ROOT -- --branch 5.8.1-release --single-branch --depth 1
~~~

验证：

~~~powershell
git -C $SourceUE_ROOT status --short
git -C $SourceUE_ROOT describe --tags --exact-match
git -C $SourceUE_ROOT rev-parse HEAD
Test-Path (Join-Path $SourceUE_ROOT 'Setup.bat')
Test-Path (Join-Path $SourceUE_ROOT 'GenerateProjectFiles.bat')
~~~

成功标准：工作区干净、精确标签为 5.8.1-release、两个批处理文件存在。标签检出后的 detached HEAD 是正常现象。

## 5. 阶段 2：下载源码依赖

关闭占用源码目录的 Editor、IDE、调试器和杀毒扫描进程：

~~~powershell
Set-Location $SourceUE_ROOT
& '.\Setup.bat'
~~~

成功标准：

- 命令退出码为 0；
- 没有 GitDependencies 哈希校验错误；
- Engine\Binaries\ThirdParty 等依赖目录已生成；
- git status --short 仍为空。

网络中断时保持源码目录不变，直接重新运行 Setup.bat。它会复用已完成的下载，不要删除依赖后从头开始。

停止条件：连续重试仍报告 CDN、代理或 SSL 错误时，先检查代理、防火墙、磁盘空间和杀毒软件隔离记录。

## 6. 阶段 3：生成源码引擎工程文件

~~~powershell
Set-Location $SourceUE_ROOT
& '.\GenerateProjectFiles.bat' -CurrentPlatform
~~~

成功标准：源码根目录出现 UE5.sln，Engine\Intermediate\ProjectFiles 已生成，输出没有 UnrealBuildTool Exception。

## 7. 阶段 4：构建源码版 UnrealEditor

推荐使用命令行以便保存完整日志：

~~~powershell
Set-Location $SourceUE_ROOT
& '.\Engine\Build\BatchFiles\Build.bat' UnrealEditor Win64 Development -WaitMutex
~~~

也可以打开 UE5.sln，选择 Development Editor | Win64，构建 UE5 或 UnrealEditor 目标。

成功标准：

~~~text
Result: Succeeded
~~~

并且：

~~~powershell
Test-Path (Join-Path $SourceUE_ROOT 'Engine\Binaries\Win64\UnrealEditor.exe')
~~~

首次构建可能耗时较长。失败时保存完整日志，优先处理第一条真正的编译错误。

首次启动验证：

~~~powershell
& (Join-Path $SourceUE_ROOT 'Engine\Binaries\Win64\UnrealEditor.exe') -log
~~~

确认版本和日志正常后关闭 Editor。不要先打开主项目，下一阶段先建立隔离 worktree。Epic 的构建说明见 [Building Unreal Engine from Source](https://dev.epicgames.com/documentation/unreal-engine/building-unreal-engine-from-source?lang=en-US)。

## 8. 阶段 5：创建项目 Git worktree

### 8.1 检查未提交修改

~~~powershell
git -C $RepoRoot status --short
git -C $RepoRoot branch --show-current
git -C $RepoRoot worktree list
~~~

重要规则：git worktree add 只从某个提交创建新工作树，不会带上主 worktree 的未提交修改。若测试 worktree 必须包含当前修改，先创建明确的检查点提交，或在创建后导入经过审查的补丁。不要直接复制整个主目录覆盖测试 worktree。

### 8.2 创建测试 worktree

确保 SourceTestRoot 不存在：

~~~powershell
if (Test-Path $SourceTestRoot) { throw 'Source test worktree already exists. Inspect it before continuing.' }
git -C $RepoRoot worktree add -b source-engine-test $SourceTestRoot HEAD
~~~

如果分支名已存在，换一个新的测试分支名，不要覆盖已有 worktree。

验证：

~~~powershell
git -C $RepoRoot worktree list
git -C $SourceTestRoot status --short
Test-Path (Join-Path $SourceTestRoot 'LyraStarterGame.uproject')
~~~

成功标准：主项目和测试项目是两个物理目录；测试 worktree Git 状态干净；主项目仍可由 Launcher 打开。

### 8.3 恢复 Lyra Content

Git 忽略的 Lyra Content 不会自动出现在新 worktree。现有初始化脚本会把 Lyra 根 `Content/` 以及仓库中对应插件的 `Content/` 复制到测试 worktree；脚本当前不创建 Junction，也没有切换资源映射方式的参数：

~~~powershell
Set-Location $SourceTestRoot
& '.\Scripts\Initialize-ExtractionOps.ps1' -LyraProject $LyraSourceRoot -EngineRoot $SourceUE_ROOT
~~~

该脚本使用复制模式，因此测试 worktree 中的资源可以独立修改，修改不会回写 `LyraSourceRoot`。复制出的资源仍由 `.gitignore` 排除，不应提交到 Git。

成功标准：

- 测试 worktree 能访问 Lyra Content 和 GameFeature Content；
- Git 状态没有大批资源被误加入；
- 测试 worktree 的资源目录是实际复制出来的目录，没有指向主项目的 Binaries、Intermediate 或 Saved。

## 9. 阶段 6：为测试 worktree 生成项目文件

不要依赖 uproject 中原有的 Launcher EngineAssociation，直接使用源码引擎和显式项目路径：

~~~powershell
Set-Location $SourceUE_ROOT
$TestUProject = Join-Path $SourceTestRoot 'LyraStarterGame.uproject'
& '.\GenerateProjectFiles.bat' "-project=$TestUProject" -game -engine
~~~

成功标准：测试 worktree 出现新的 IDE 项目模型，并能识别 LyraEditor、LyraClient 和 LyraServer Target。

不要在主 worktree 上执行 Switch Unreal Engine Version；那会改变主项目引擎关联并增加混用风险。

## 10. 阶段 7：构建项目 Editor、Client 和 Server

~~~powershell
$SourceBuild = Join-Path $SourceUE_ROOT 'Engine\Build\BatchFiles\Build.bat'
$TestUProject = Join-Path $SourceTestRoot 'LyraStarterGame.uproject'

& $SourceBuild LyraEditor Win64 Development "-Project=$TestUProject" -WaitMutex
& $SourceBuild LyraClient Win64 Development "-Project=$TestUProject" -WaitMutex
& $SourceBuild LyraServer Win64 Development "-Project=$TestUProject" -WaitMutex
~~~

每条命令都必须出现 Result: Succeeded。随后检查：

~~~powershell
Get-ChildItem (Join-Path $SourceTestRoot 'Binaries\Win64') | Where-Object Name -Match 'Lyra(Client|Server)|UnrealEditor-Lyra'
~~~

成功标准：出现源码版生成的 LyraClient.exe、LyraServer.exe 和项目 Editor 模块，不再出现 Launcher installed distribution 不支持 Client/Server 的提示。

## 11. 阶段 8：Cook Client 与 Server

使用源码版 Editor 打开测试 worktree：

~~~powershell
$SourceEditor = Join-Path $SourceUE_ROOT 'Engine\Binaries\Win64\UnrealEditor.exe'
$TestUProject = Join-Path $SourceTestRoot 'LyraStarterGame.uproject'
& $SourceEditor $TestUProject -log
~~~

在 Editor 中分别执行：

### Server Cook

1. Platforms > Windows > Build Target > Server；
2. Binary Configuration > Development；
3. Platforms > Windows > Content Management > Cook；
4. 等待 Output Log 出现成功信号；
5. 检查 Saved\Cooked\WindowsServer。

### Client Cook

1. 将 Build Target 改为 Client；
2. 保持 Development；
3. 再次执行 Cook；
4. 检查 Saved\Cooked\WindowsClient。

成功标准：两个 Cook 目录存在，日志没有 missing shader、missing package 或未处理资产错误。

## 12. 阶段 9：Dedicated Server + 两个 Client

先确认端口未占用：

~~~powershell
Get-NetTCPConnection -LocalPort 7777 -ErrorAction SilentlyContinue
~~~

启动 Server：

~~~powershell
Set-Location $SourceTestRoot
$ServerProcess = Start-Process -FilePath (Join-Path $SourceTestRoot 'Binaries\Win64\LyraServer.exe') -ArgumentList @('/ShooterMaps/Maps/L_Convolution_Blockout?NumBots=0','-log','-port=7777') -PassThru
$ServerProcess.Id
~~~

等待日志出现：

~~~text
IpNetDriver listening on port 7777
~~~

启动两个 Client：

~~~powershell
$Client1 = Start-Process -FilePath (Join-Path $SourceTestRoot 'Binaries\Win64\LyraClient.exe') -ArgumentList @('127.0.0.1:7777','-WINDOWED','-ResX=900','-ResY=600','-WinX=0','-WinY=0') -PassThru
$Client2 = Start-Process -FilePath (Join-Path $SourceTestRoot 'Binaries\Win64\LyraClient.exe') -ArgumentList @('127.0.0.1:7777','-WINDOWED','-ResX=900','-ResY=600','-WinX=920','-WinY=0') -PassThru
~~~

验证：

- [ ] Server 是 LyraServer.exe，不是 UnrealEditor.exe -server；
- [ ] 两个 Client 都成功 Login/Join；
- [ ] 两个窗口进入同一地图；
- [ ] 一个 Client 退出后，Server 和另一个 Client 继续运行；
- [ ] Server 关闭后，Client 得到明确断开状态；
- [ ] 日志来自 SourceTestRoot\Saved\Logs，没有写入主 worktree。

只终止本次记录的 PID：

~~~powershell
Stop-Process -Id $Client1.Id -ErrorAction SilentlyContinue
Stop-Process -Id $Client2.Id -ErrorAction SilentlyContinue
Stop-Process -Id $ServerProcess.Id -ErrorAction SilentlyContinue
~~~

不要按进程名批量结束所有 UE 进程，以免关闭仍在使用 Launcher 项目的 Editor。

## 13. 两套环境之间的切换规则

日常开发继续使用主 worktree 和 Launcher UE。源码 UE 只在测试 worktree 中执行编译、Cook 和 Dedicated Server 验证。

切换前检查：

~~~powershell
Get-Process UnrealEditor,LyraClient,LyraServer -ErrorAction SilentlyContinue
git -C $RepoRoot status --short
git -C $SourceTestRoot status --short
~~~

必须先关闭上一套环境的进程，再启动另一套环境。不要把测试 worktree 的 Binaries、Intermediate、Saved 复制回主 worktree。

只有在以下条件全部满足后，才考虑把源码 UE 设为主项目引擎：

1. 源码 UnrealEditor 构建并启动成功；
2. LyraEditor、LyraClient、LyraServer 全部构建成功；
3. Client/Server Cook 成功；
4. Dedicated Server 加两个 Client 验证成功；
5. 主项目代码、文档和回退方式已经提交并记录；
6. 已保存原 Launcher EngineAssociation 和恢复步骤。

## 14. 删除或暂存测试 worktree

删除前关闭源码 Editor、Client、Server 和 IDE，并确认没有需要保留的未跟踪文件：

~~~powershell
git -C $SourceTestRoot status --short
git -C $RepoRoot worktree list
~~~

正常删除：

~~~powershell
git -C $RepoRoot worktree remove $SourceTestRoot
git -C $RepoRoot worktree prune
~~~

只有确认所有测试生成物都不再需要时才使用 --force。删除前应备份日志、崩溃报告和性能数据；不要删除源码 UE 根目录来解决项目生成物问题。

## 15. 常见问题与定位顺序

### GitHub 仓库 404 或无权限

检查 Epic/GitHub 关联、组织邀请和 gh auth status；确认后重新查询 5.8.1-release，不要使用第三方镜像。

### Setup.bat 下载失败

保持源码目录不变，重新运行 Setup.bat。检查代理、磁盘、杀毒软件隔离和 Git 长路径；不要删除已完成的依赖后从头开始。

### GenerateProjectFiles.bat 失败

确认 Setup.bat 成功、源码标签正确、长路径已启用，并关闭占用源码目录的 IDE 和 Editor。

### 编译器版本提示偏好值不同

newer than preferred 只是警告时可继续观察。只有出现实际 C++、标准库或链接错误，才安装或选择项目要求的 MSVC 工具集。

### Client/Server 仍提示 installed distribution 不支持

确认 Build.bat 来自 SourceUE_ROOT，Project 指向 SourceTestRoot，并且运行的是测试 worktree 的可执行文件。

### 源码项目提示模块不兼容

关闭所有 UE 进程，确认当前目录是测试 worktree，删除测试 worktree 中的 Binaries 和 Intermediate 后重新生成项目文件并完整构建。不要删除主 worktree 的生成物。

### EXE 存在但缺少 Shader 或 Content

回到 Cook 阶段，确认 WindowsClient 和 WindowsServer 都已成功生成。重复 C++ 编译不能替代 Cook。

### Worktree 看不到 Lyra Content

这是正常现象：Git 忽略资源不会被 worktree checkout。重新运行 `Initialize-ExtractionOps.ps1`，确认 Lyra 资源已复制到测试 worktree，并检查目标目录不是指向主项目的 Junction。

## 16. 最终验收清单

- [ ] GitHub 官方仓库授权正常；
- [ ] 固定检出 5.8.1-release；
- [ ] Setup.bat 成功且依赖完整；
- [ ] GenerateProjectFiles.bat 成功；
- [ ] 源码 UnrealEditor 构建并启动；
- [ ] Launcher UE 和源码 UE 位于不同引擎目录；
- [ ] 主项目和源码测试项目位于不同 worktree；
- [ ] 测试 worktree 的 Lyra Content 映射正确；
- [ ] LyraEditor、LyraClient、LyraServer 全部构建成功；
- [ ] Windows Client/Server Cook 成功；
- [ ] LyraServer.exe 监听端口 7777；
- [ ] 两个 LyraClient.exe 成功加入并互相观察；
- [ ] 主 worktree 没有被源码版生成物污染；
- [ ] 已保存构建、Cook、Server/Client 日志和回退步骤；
- [ ] 没有公开 Unreal Engine 私有源码、私有仓库内容或受授权限制的资产。

## 17. 官方资料

- [Downloading Source Code in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/downloading-source-code-in-unreal-engine)
- [Building Unreal Engine from Source](https://dev.epicgames.com/documentation/unreal-engine/building-unreal-engine-from-source?lang=en-US)
- [How to Generate Unreal Engine Project Files for Your IDE](https://dev.epicgames.com/documentation/unreal-engine/how-to-generate-unreal-engine-project-files-for-your-ide?lang=en-US)
- [Setting Up Dedicated Servers](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers-in-unreal-engine)
- [EpicGames/UnrealEngine](https://github.com/EpicGames/UnrealEngine)（完成 Epic 授权后访问）
