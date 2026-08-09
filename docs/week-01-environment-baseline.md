# 第 1 周：环境基线与可重复构建

## 本周目标

把 Lyra 工程从“能打开”变成“能稳定编译、能启动客户端和专用服务器、能重复验证”。本周不做新玩法，先建立以后每周都能依赖的工程基线。

## 验收目标

### Given/When/Then

- Given 工程位于 `D:\Document\AI\Codex\LyraStarterGame`，When 用 UE 5.8 打开 `LyraStarterGame.uproject`，Then 不出现缺失模块、插件或资产加载错误。
- Given Visual Studio 和 UE 5.8 已安装，When 构建 `LyraGameEditor Win64 Development`，Then 构建成功且没有新增编译错误。
- Given工程已经生成项目文件，When 启动编辑器并打开一个 Lyra 地图，Then 能够 PIE 运行并控制角色。
- Given启用了 `LyraServer` Target，When构建并启动专用服务器，Then服务器能监听端口并输出启动日志。
- Given Git 仓库存在，When提交本周变更，Then DerivedDataCache、Binaries、Intermediate、Saved 等生成目录不会进入提交。

## 操作步骤

### 1. 记录工程和环境

先记录以下信息，写入本周学习笔记：

- UE 版本和安装位置；
- Visual Studio 版本、Windows SDK、MSVC 工具链；
- 当前 Git commit；
- 项目是否能打开；
- 当前 `LyraStarterGame.uproject` 中的 EngineAssociation；
- 当前可用的 Target 文件。

检查项目文件时重点关注：

- `EngineAssociation` 是否为 `5.8`；
- `Source/LyraGame` 是否存在；
- `Source/LyraServer.Target.cs` 是否存在；
- `Plugins` 中是否包含本工程启用的额外插件。

### 2. 生成项目文件并编译编辑器

在项目根目录执行与你安装路径匹配的 GenerateProjectFiles 操作，或者在资源管理器中右键 `.uproject` 选择生成 Visual Studio 项目文件。

然后在 Visual Studio 中：

1. 选择 `Development Editor`；
2. 选择 `Win64`；
3. 选择 `LyraEditor` 或对应编辑器目标；
4. 执行 Build；
5. 记录第一个构建成功时间和构建失败信息。

如果是源码工程，优先使用项目提供的 Target 文件，不要自行创建第二套 Target。工程已经提供 `LyraClient` 和 `LyraServer`，说明后续学习可以沿用 Client/Server 分离结构。

### 3. 启动编辑器并确认最小可玩状态

进入编辑器后完成以下动作：

1. 打开 Lyra 默认入口地图；
2. 使用 PIE 运行；
3. 移动角色、跳跃、切换武器、射击；
4. 打开或关闭 HUD；
5. 停止 PIE 后观察 Output Log；
6. 记录角色、输入、武器和 HUD 的现有资产路径。

不要一开始修改 Lyra 的默认角色。先建立“原始行为基线”，后续每次改动都能与基线对比。

### 4. 生成并启动专用服务器

先确认 `LyraServer.Target.cs` 和 `LyraGameServer` 相关模块能够编译。启动时使用一个单独的测试端口，例如 `7777`，并将日志写入 `Saved/Logs`。

验证以下内容：

- 服务器进程能够启动；
- 启动日志中没有关键插件加载失败；
- 服务器不会因为缺少编辑器资产而直接退出；
- 客户端能够通过 IP 或工程已有的在线服务方式加入；
- 服务器关闭时能正常退出而不是卡死。

### 5. 建立 Git 与文档基线

本周提交只包含工程必须的源文件和配置。不要提交：

- `Binaries/`；
- `Intermediate/`；
- `DerivedDataCache/`；
- `Saved/` 中的临时日志和缓存；
- 本机绝对路径配置；
- 账号、密钥和 EOS 凭据。

在 `docs` 中记录：

```text
本周 commit：
UE 版本：
编辑器构建结果：
客户端运行结果：
专服运行结果：
已知问题：
```

## 实现原理

Unreal 工程不是只有 `.uproject`。编译时还会根据 Target、Module、Build.cs 和插件依赖生成不同的目标。编辑器目标能运行，不代表 Client 或 Server 目标一定能运行；因此必须分别验证。

专用服务器的核心意义是让游戏规则在没有渲染窗口的进程中运行。客户端负责输入、预测和表现，服务器负责最终状态。后续的射击命中、物品掉落和撤离结算都应以这个边界为基础。

## 常见问题

### 编译提示找不到模块

检查：

- EngineAssociation 是否指向正确版本；
- 插件是否放在工程的 `Plugins` 或引擎插件目录；
- Target 的模块名称是否和目录中的模块一致；
- 是否误用了其他 UE 版本生成的中间文件。

### 编辑器能开但 Server 启动失败

优先看 Server 的第一条 `Error`，不要只看最后一条退出信息。常见原因是：服务端依赖编辑器专用插件、地图或资产不适合无渲染环境、目标配置不一致。

### 打开工程很慢

第一次编译和着色器准备较慢是正常现象。不要为了加速随意删除 `Config` 或插件配置；先记录耗时，后续再优化。

## 本周作品集产出

- 一张工程目标和模块关系图；
- 一段 30 秒的 Client 启动视频；
- 一段 30 秒的 Server 启动日志视频；
- 一篇“为什么客户端和专服必须分别验证”的笔记。

## 参考资料

- [Lyra Sample Game - Epic Games](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)
- [Upgrading Lyra to the latest engine release](https://dev.epicgames.com/documentation/en-us/unreal-engine/upgrading-the-lyra-starter-game-to-the-latest-engine-release)
- 项目内：[LyraStarterGame.uproject](../LyraStarterGame.uproject)
- 项目内：[Source/LyraGame/README.md](../Source/LyraGame/README.md)

