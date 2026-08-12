# AGENTS.md

## 项目概览

这是一个基于 Unreal Engine 5.8 和 Lyra Starter Game 的多人 PvE Extraction
Vertical Slice。主要代码位于 `Plugins/GameFeatures/ExtractionOps/Source/`，项目资产位于
`Plugins/GameFeatures/ExtractionOps/Content/`，学习、设计和验收文档位于 `docs/`。

## 工作目录规则

- 使用 `D:/Software/UE_5.8` 作为本项目的 Unreal Engine 根目录；不要使用其他 UE 版本混合生成工程文件、编译或保存资产。
- 只提交源码、配置、脚本、文档和明确需要版本控制的 `Content/` 资产。
- 不要提交 `Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/`、`.vs/` 或 `.idea/` 中的生成物、缓存和本机用户配置。
- 保持 `LyraStarterGame.uproject`、`Plugins/GameFeatures/ExtractionOps/ExtractionOps.uplugin` 与模块依赖同步。

## Unreal 资产规则

- `.uasset`、`.umap`、`.ubulk` 和 `.pak` 在本仓库中使用普通 Git，不使用 Git LFS；远端仓库没有可用的 Git LFS 服务。
- 所有普通 Git 二进制文件必须小于 GitHub 的 100 MiB 硬限制；接近 50 MiB 时应先评估是否需要独立资源仓库或其他对象存储。
- 提交前运行 `git lfs ls-files`，结果必须为空；不要重新加入 `filter=lfs`、`diff=lfs` 或 `merge=lfs` 规则。
- 不要通过脚本拼接、伪造或直接修改 `.uasset`/`.umap` 二进制内容。资产创建和修改必须通过 Unreal Editor、Editor Utility、live Unreal MCP 或项目已有的 MCP JSON 流程完成。
- 修改资产后保存 Package，并用 MCP 或 Unreal Editor 检查类、父类、引用、脏状态和关键配置。
- 不要在 PIE 或运行中的多人测试期间修改资产；先停止 PIE，修改后保存，再重新启动测试。

## MCP 工作流

1. 确认 Unreal Editor 已打开 `LyraStarterGame.uproject`，并确认 MCP Server 正在监听。
2. 使用 `Scripts/Invoke-UnrealMcp.ps1` 调用 `Scripts/Mcp/` 下的可审查 JSON 请求。
3. 资产修改前先 Inspect，修改操作串行执行，修改后 Save，再执行 Verify/Inspect。
4. MCP 不可用时停止资产实施，不要退化为直接写二进制文件。
5. 资产验证应记录请求文件、Editor 日志、资产路径和最终验证结果。

## 编译与测试

- 首选使用 Unreal Editor/Rider 编译 `LyraEditor` Win64 Development；C++ 修改后先编译，再进行 PIE 或多人测试。
- 多人烟测优先使用 `Scripts/Start-Week01-Multiplayer.ps1`；Week 4–7 的端到端流程使用 `Scripts/Start-Week04-07-E2E.ps1`。
- 测试前关闭本轮测试创建的 UE Editor、Dedicated Process 和客户端进程；脚本只能清理自己创建的进程。
- C++ 规则测试、多人烟测、断线/延迟/丢包测试都应区分 Editor Dedicated Process 证据与真正 Packaged Server/Shipping 证据。
- 若项目包含 Go 后端改动，运行 `go test ./...`，必要时运行 `go test -race ./...`。

## Git 提交与推送

- 提交前检查：

  ```powershell
  git status --short
  git diff --check
  git lfs ls-files
  ```

- 确认没有 LFS 文件后再提交和推送：

  ```powershell
  git add -A
  git commit -m "<type>: <short description>"
  git push origin main
  ```

- 提交信息使用清晰的英文 Conventional Commit 风格，例如 `feat: add extraction validation flow`、`fix: preserve server authority` 或 `docs: update week acceptance evidence`。
- 不要使用 `git reset --hard`、`git checkout --` 或强制推送来覆盖用户改动；如需改写尚未推送的提交，先确认目标范围并保留工作区未提交内容。
- 推送前若出现 LFS 错误，先检查 `.gitattributes`、`git lfs ls-files` 和单文件大小；不要通过重复 `git push` 绕过问题。

## 文档要求

- 新增或改变工作流时同步更新相关 `docs/week-*.md`、`docs/implementation-status.md` 或 `docs/evidence/` 文档。
- 文档中的命令必须与仓库实际脚本、UE 版本和目录结构一致。
- 验收文档要明确区分“已执行并有证据”和“设计目标/待验证”，不要把计划写成已完成结果。
