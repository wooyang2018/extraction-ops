# 第 2 周：Lyra 架构阅读与工程边界

## 本周目标

读懂 Lyra 的模块化方式，并为自己的玩法建立独立边界。本周的关键不是写很多代码，而是避免以后把所有逻辑塞进 `LyraGame` 或某个巨大 Blueprint。

## 验收目标

- 能画出从输入到角色行为、从服务器状态到 HUD 的调用链；
- 能解释 GameMode、GameState、PlayerState、PlayerController、Pawn/Character 的职责差异；
- 能指出 Lyra 中至少三个 GameFeature Plugin 的启用和加载位置；
- 创建一个名为 `ExtractionOps` 的独立 GameFeature Plugin 或等价插件边界；
- 新增一个最小 C++ 类和一个最小 Blueprint 资产，并能在 PIE 中加载；
- 形成一篇不少于 1000 字的 Lyra 源码阅读笔记。

## 操作步骤

### 1. 从入口开始读，不要随机点文件

按以下顺序阅读：

1. `LyraStarterGame.uproject`：插件和模块；
2. `Source/LyraGame/LyraGame.Build.cs`：模块依赖；
3. `Source/LyraGame/LyraGameModule.cpp`：模块启动；
4. `Source/LyraGame/AbilitySystem`：能力系统基础；
5. `Plugins/GameFeatures` 相关代码和配置；
6. `ShooterCore`、`ShooterMaps`、`CommonGame`、`CommonUI` 相关模块；
7. 默认地图和默认 GameMode 配置。

每读一个目录，回答三个问题：

- 它解决什么游戏问题？
- 它在 Client、Server 还是 Editor 中运行？
- 它通过什么方式被加载和引用？

### 2. 画出对象生命周期图

至少画清下面的对象：

```text
GameInstance
  -> World
    -> GameMode（仅服务器）
    -> GameState（复制给客户端）
    -> PlayerController（每个连接）
    -> PlayerState（玩家状态）
    -> Pawn/Character（可控制角色）
      -> Components
      -> AbilitySystemComponent
      -> Inventory/Equipment
```

特别标注“仅服务器”“仅本地客户端”“服务器和客户端都有”。这张图是后续排查 RPC 和状态错误的基础。

### 3. 创建 ExtractionOps 边界

优先使用 GameFeature Plugin；如果当前工程的插件创建流程不便于初学阶段操作，可以先建立清晰的 `Source/ExtractionOps` 模块，但要保留以后迁移成插件的可能。

建议目录：

```text
Plugins/ExtractionOps/
  ExtractionOps.uplugin
  Source/ExtractionOps/
    ExtractionOps.Build.cs
    Public/
    Private/
  Content/
    UI/
    Weapons/
    Items/
    Maps/
```

第一个类可以是 `UExtractionGameInstanceSubsystem`，只负责保存客户端侧的项目状态和服务初始化入口，不要在里面写战斗规则。

### 4. 做一个最小 Feature

实现一个可见但低风险的 Feature：进入 PIE 后在 HUD 中显示 `Extraction Ops Prototype`，并输出当前网络角色（Authority、AutonomousProxy 或 SimulatedProxy）。

这样可以同时验证：

- 插件能被加载；
- C++ 能编译；
- Blueprint 能找到 C++ 类；
- 客户端能读到网络角色；
- 你知道逻辑到底运行在哪一端。

### 5. 用代码搜索建立索引

在项目根目录使用 `rg` 搜索：

```text
rg "GameMode|GameState|PlayerState|PlayerController" Source Plugins
rg "GetLifetimeReplicatedProps|Server, Reliable|Client, Reliable" Source Plugins
rg "UGameFeature|GameFeatureAction|AbilitySystemComponent" Source Plugins
```

把搜索结果整理成“概念 -> 文件 -> 作用”表，而不是收藏一堆无法回忆的链接。

## 实现原理

Lyra 使用模块化和 GameFeature 思路，把玩法能力拆成可以启用、停用和复用的单元。这样做的价值不只是代码整洁：它可以减少多人项目中“修改一个基础类导致所有模式回归失败”的风险。

GameMode 只存在于服务器，因此不能把客户端 UI 或客户端输入状态放到 GameMode。GameState 适合表达一局游戏的公开状态；PlayerState 适合表达需要被其他客户端看到的玩家状态；PlayerController 更接近单个连接；Pawn/Character 负责当前被控制的实体。

## 常见问题

### 为什么 Blueprint 找不到新 C++ 类

确认模块已加入 `.uproject` 或插件，Build.cs 依赖完整，编辑器重新加载了模块，并且类带有正确的 `UCLASS` 宏。修改 UHT 相关头文件后，必要时重新生成项目文件和重启编辑器。

### 为什么在客户端调用 GameMode 会崩或为空

这是职责边界问题。客户端不应依赖 GameMode。把需要复制的状态放到 GameState 或 PlayerState，把客户端操作放到 PlayerController 或本地组件。

### 为什么不直接修改 Lyra 基础类

小范围实验可以，但作品项目尽量把新玩法放到独立模块。面试时你需要证明自己能理解并扩展框架，而不是把框架改到无法升级。

## 本周作品集产出

- Lyra 模块和插件关系图；
- 网络对象职责图；
- `ExtractionOps` 插件骨架；
- 一篇 Lyra 源码阅读文章；
- 30 秒展示网络角色和插件加载的视频。

## 参考资料

- [Lyra Sample Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)
- [Gameplay Feature Plugins](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-features-in-unreal-engine)
- [Unreal Engine Programming with C++](https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-with-cplusplus-in-unreal-engine)
- 项目内：[Source/LyraGame](../Source/LyraGame)

