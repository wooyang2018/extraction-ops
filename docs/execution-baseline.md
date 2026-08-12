# Extraction Ops：12 周执行基线

本文是 `week-01` 到 `week-12` 的共同前置合同。每周开始前先读本文；若周文档与本文冲突，以本文为准，并先修正文档再实施。

## 唯一执行环境

- 当前 12 周 Vertical Slice 只使用官方 Installed Build：`D:\Software\UE_5.8`。
- 项目、脚本和验收命令只能把上述目录作为 `<UE_ROOT>`；不得自动搜索或回退到其他源码引擎目录。
- `LyraStarterGame.uproject`、Lyra 内容和插件必须来自兼容 UE 5.8 的同一资产基线。
- 每次周验收记录：仓库 commit、UE 完整版本、构建配置、Visual Studio/MSVC/Windows SDK 版本和 Lyra 资产来源。

## 受保护的 ue5-main 源码目录

`D:\Software\UE_5.8.1_Source` 由用户独立维护并保持在 `ue5-main`。Codex 和当前 12 周执行流程不得对该目录执行任何操作，包括读取、搜索、Git 查询、切换、拉取、构建、运行、生成工程文件、清理或修改。

ue5-main 是当前路线之外的未来迁移目标。迁移时必须由用户提供明确的里程碑 commit 和独立迁移规格；不得以“当日最新主干”作为可复现基线，也不得把未来迁移计入当前 12 周完成度。

## 构建与进程术语

当前 Installed Build 已知不支持 `LyraServer` Target。不要反复运行预期失败的 Server Target 构建，也不要把它列为当前路线硬门槛。

本路线统一使用以下术语：

- **Editor**：`UnrealEditor.exe <Project>`，用于资产编辑、PIE 和自动化测试。
- **Editor Client Process**：`UnrealEditor.exe <Project> <Address> -game`，是独立客户端窗口。
- **Editor Dedicated Process**：`UnrealEditor.exe <Project> <Map> -server -NullRHI`，无本地玩家、无客户端 UI，负责当前路线的服务器权威验证。
- **Packaged Dedicated Server**：由 `LyraServer` Target Cook/Stage/Package 得到的发布产物；当前路线未实现。

文档中简写“Server”时，若未另行说明，一律指 Editor Dedicated Process。作品集、录屏、性能报告和面试口述必须明确这一限制，不能把 Editor Dedicated Process 描述为已完成的发布级 Server 包。

## 标准启动方式

```powershell
$UE_ROOT = 'D:\Software\UE_5.8'
$PROJECT = '<RepoRoot>\LyraStarterGame.uproject'
$MAP = '/ShooterMaps/Maps/L_Convolution_Blockout'

& "$UE_ROOT\Engine\Binaries\Win64\UnrealEditor.exe" `
  $PROJECT "$MAP?NumBots=2" `
  -server -NullRHI -unattended -NoSound -log -port=7777

& "$UE_ROOT\Engine\Binaries\Win64\UnrealEditor.exe" `
  $PROJECT '127.0.0.1:7777' `
  -game -log -windowed -ResX=900 -ResY=600
```

第二个客户端使用相同地址、不同日志和窗口位置启动。脚本必须等待 Server 监听和两次 `Login/Join`，并且只关闭自己创建的进程。

## 版本控制与证据

- Lyra 原始 `Content/`、`Binaries/`、`Intermediate/` 和 `Saved/` 保持忽略。
- `Plugins/GameFeatures/ExtractionOps/Content/**` 是项目自研资产，必须进入版本控制。
- 除本文明确约定的开发基线路径外，不在脚本、配置或日志中提交额外的本机绝对路径；不提交访问令牌、EOS 凭据或 Saved 日志。
- 每周证据至少包含构建命令和退出码、自动化测试结果、关键日志摘要、Given/When/Then 验收记录及必要录屏。
- 未通过前置周门槛时，不通过增加内容掩盖问题；后续周顺延。

## 当前路线的明确非目标

- 不生成或发布 `LyraServer.exe`。
- 不对 Shipping Server 的 CPU、内存或带宽作最终承诺；Editor Dedicated Process 数据只作为开发基线。
- 不执行 ue5-main 迁移、引擎修改或跨版本兼容修复。
- 不把当前 Editor Dedicated Process 证据包装成 Steam 生产部署能力。
