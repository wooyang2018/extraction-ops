# Extraction Ops 资源初始化

公开仓库只保存源码、配置、构建脚本和文档，不分发 Lyra 的 `Content/` 资产。这样其他人可以查看和复用项目代码，但需要先通过 Epic Games Launcher/Fab 获取与项目匹配的 Lyra Starter Game。

Epic 官方 Lyra 文档说明，Lyra 应从 Fab/Launcher 获取；不同引擎版本应使用对应版本的 Lyra 项目。请使用 Unreal Engine 5.8 和同版本 Lyra 资源，并遵守你账号适用的 Epic/Fab 授权条款。

## 快速初始化

先在 Epic Games Launcher 中创建 Lyra Starter Game，例如：

```text
C:\Samples\LyraStarterGame
```

然后在 Extraction Ops 仓库根目录执行：

```powershell
.\Scripts\Initialize-ExtractionOps.ps1 -LyraProject 'C:\Samples\LyraStarterGame'
```

也可以直接双击 `Scripts\Initialize-ExtractionOps.ps1` 进入交互式初始化。脚本会提示项目目录和引擎目录，并确认是否编译和启动编辑器；资源会自动复制到当前工作区。双击后如果 Windows 阻止脚本执行，请使用右键“使用 PowerShell 运行”。

脚本会：

1. 检查当前工程、Lyra 资源和 Unreal Engine 版本；
2. 将根 `Content/` 以及仓库中对应插件的 `Content/` 复制到当前工作区；
3. 优先使用 UnrealVersionSelector，其次使用 GenerateProjectFiles.bat；新版 UE 如果没有这两个文件，则使用引擎自带的 UnrealBuildTool 和 .NET 运行时生成 Rider 项目文件。

复制的资源仍被 `.gitignore` 排除，不会进入公开仓库。后续对工作区资源的修改不会回写 Lyra 源项目。

## 编译和启动

```powershell
.\Scripts\Initialize-ExtractionOps.ps1 `
  -LyraProject 'C:\Samples\LyraStarterGame' `
  -BuildEditor `
  -LaunchEditor
```

如果脚本没有自动找到引擎，可以显式指定：

```powershell
.\Scripts\Initialize-ExtractionOps.ps1 `
  -LyraProject 'C:\Samples\LyraStarterGame' `
  -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
```
