# 实施状态

更新时间：2026-08-09

本文只记录已经由本机命令或 Unreal Editor MCP 验证的事实，不以计划项代替完成项。

## 已完成

- 创建 `ExtractionOps` GameFeature Runtime 模块，并在项目中启用。
- GameFeatureData 激活时向 `LyraGameState` 注入服务器/客户端复制的 MatchState 组件，向 `LyraPlayerState` 注入 RunState 组件。
- 实现 Match、Run、Terminal、Extraction Zone 与 Threat 的权威状态模型。
- 实现信号终端的服务器激活、唯一终端计数、三档 Threat 和奖励倍率。
- 实现撤离区的服务器重叠校验、基于 GameState 服务器时钟的玩家倒计时、离开取消与成功撤离。
- 创建可配置的 `B_SignalTerminal` 与 `B_ExtractionZone` Blueprint 资产。
- `LyraEditor Win64 Development` 构建通过；状态规则自动化测试通过；PIE 验证 GameFeature 组件注入。

## 当前硬门槛

当前引擎 `D:\Software\UE_5.8` 是 Installed Build，存在 `Engine/Build/InstalledBuild.txt`。执行 `LyraServer Win64 Development` 时 UnrealBuildTool 返回：

```text
Server targets are not currently supported from this engine distribution.
```

因此“Client/Server 构建、双客户端连接”尚未完成，不能进入完成态。需要切换到支持 Server Target 的 UE 5.8 源码构建，或重新生成包含 Server 的 Installed Build，再重复构建和双客户端验证。

## 下一可执行切片

1. 建立支持 Server Target 的源码引擎基线并完成两客户端连接。
2. 创建专属 Extraction Experience，让 Experience 负责激活 `ExtractionOps`，不依赖手工 MCP 激活。
3. 建立一个 5 分钟灰盒战斗区，将一个终端、一个有效撤离区和第一类 AI 串成可玩回路。
4. 再开始两把武器的 3C/命中反馈与 Threat 驱动 AI 调度；后台继续延后。
