# Extraction Ops

基于 Unreal Engine 5.8 与 Lyra Starter Game 的多人撤离射击 Vertical Slice 学习与实践项目。

## 项目定位

项目定位为第三人称、1–2 人合作 PvE、12–15 分钟短局的撤离射击 Vertical Slice。核心选择是：启动信号终端可获得物资与撤离情报，但会同步提高 Threat Level；玩家必须不断判断“继续扫描，还是带着现有收益立即撤离”。

实现顺序先证明枪感、AI 压迫、信息博弈和 Dedicated Server 权威闭环，再接入 Go 后台。完整范围、20 周阶段门槛和最终验收见 [`docs/vertical-slice-blueprint.md`](docs/vertical-slice-blueprint.md)。

技术重点包括：

- Unreal Engine 5.8、C++、Lyra、Game Features、GAS
- 多人联机、复制、会话、专用服务器与重连
- 背包、战利品、撤离点、结算与幂等
- Go 后端服务、数据持久化、测试、性能与可观测性

## 目录

- `docs/`：学习路线、技术设计、验收标准与面试材料
- `Plugins/GameFeatures/ExtractionOps/`：项目新增玩法、复制状态与本地资产的主要落点
- `Source/`、`Config/`：Unreal 项目代码与配置
- `Build/`：构建和自动化脚本
- `Scripts/`：本地资源初始化和工程准备脚本

## 开始使用

1. 安装 Unreal Engine 5.8 和项目要求的 Visual Studio C++ 工具链。
2. 通过 Epic Games Launcher/Fab 创建同版本的 Lyra Starter Game。
3. 执行资源初始化脚本：

   ```powershell
   .\Scripts\Initialize-ExtractionOps.ps1 -LyraProject 'C:\Samples\LyraStarterGame'
   ```

   也可以直接双击 `Scripts\Initialize-ExtractionOps.ps1`。脚本会依次提示 Lyra 项目目录、Unreal Engine 目录，以及是否编译和启动编辑器；资源会自动复制到当前工作区。

4. 用 Rider 或 Unreal Editor 编译并运行项目。

脚本会将 Lyra 资源复制到当前工作区，并保留在 `.gitignore` 排除范围内。脚本支持 UnrealVersionSelector、GenerateProjectFiles.bat 以及新版 UE 自带的 UnrealBuildTool。完整说明见 [`docs/asset-bootstrap.md`](docs/asset-bootstrap.md)。

Lyra 原始 `Content/` 仍保留在本地；仅 `Plugins/GameFeatures/ExtractionOps/Content/` 中少量自研 Slice 资产纳入版本控制。`Binaries/`、`Intermediate/`、`Saved/` 和 `DerivedDataCache/` 等生成内容继续忽略。

## 计划与文档

完整路线、项目总览和统一工作规则见 [`docs/README.md`](docs/README.md)。
