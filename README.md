# Extraction Ops

基于 Unreal Engine 5.8 与 Lyra Starter Game 的多人撤离射击 Vertical Slice 学习与实践项目。

## 项目定位

项目围绕“登录 → 大厅 → 会话 → 角色与武器 → 搜刮 → 背包 → 撤离/淘汰 → 服务端结算 → Go 后端持久化 → 返回大厅”这条可运行、可解释、可演示的闭环展开。

技术重点包括：

- Unreal Engine 5.8、C++、Lyra、Game Features、GAS
- 多人联机、复制、会话、专用服务器与重连
- 背包、战利品、撤离点、结算与幂等
- Go 后端服务、数据持久化、测试、性能与可观测性

## 目录

- `docs/`：学习路线、技术设计、验收标准与面试材料
- `Plugins/ExtractionOps/`：项目新增功能的主要落点
- `Source/`、`Config/`：Unreal 项目代码与配置
- `Build/`：构建和自动化脚本

## 开始使用

1. 使用 Unreal Engine 5.8 打开 `LyraStarterGame.uproject`。
2. 安装项目要求的 Visual Studio C++ 工具链。
3. 通过 Unreal Editor 或 Rider 编译并运行项目。

当前仓库基线只纳入源码、配置、构建脚本和文档。`Content/` 资产以及 `Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/` 等生成内容保留在本地，后续如需分发再单独规划资源仓库或 Git LFS。

## 计划与文档

详细路线见 [`docs/00-roadmap.md`](docs/00-roadmap.md)，项目总览见 [`docs/README.md`](docs/README.md)。
