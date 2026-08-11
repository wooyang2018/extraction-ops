# Lyra 项目构建入口文件说明

本文总结当前项目中四类构建入口文件的作用、主要内容和工作原理：

1. [LyraStarterGame.uproject](../../LyraStarterGame.uproject)
2. [LyraEditor.Target.cs](../../Source/LyraEditor.Target.cs)
3. [LyraClient.Target.cs](../../Source/LyraClient.Target.cs)
4. [LyraServer.Target.cs](../../Source/LyraServer.Target.cs)
5. [LyraGame.Build.cs](../../Source/LyraGame/LyraGame.Build.cs)

建议阅读顺序：

~~~text
uproject
   ├── 项目模块、插件和引擎关联
   ▼
Target.cs
   ├── Editor、Client、Server 目标
   ├── 目标包含哪些模块
   ▼
Build.cs
   ├── 单个模块的依赖和编译规则
   ▼
UnrealBuildTool
   └── 生成可执行文件和模块 DLL
~~~

## 1. LyraStarterGame.uproject

### 1.1 文件定位

.uproject 是 Unreal 项目的描述文件，属于 JSON 配置，不负责实现 C++ 逻辑。Editor、UnrealBuildTool、项目启动器和插件系统都会读取它。

它主要回答四个问题：

- 项目关联哪套 Unreal Engine；
- 项目有哪些 C++ 模块；
- 项目启用了哪些插件；
- 哪些模块和插件应在什么阶段或平台加载。

### 1.2 EngineAssociation

当前字段是：

~~~json
"EngineAssociation": "5.8"
~~~

它表示项目默认关联 UE 5.8 系列。它不是源码 UE 的绝对路径，也不是某个具体 UnrealEditor.exe 的路径；实际引擎位置由 UnrealVersionSelector、注册信息或启动命令决定。

Launcher/源码 UE 并存时：

- 主项目不要随意修改 EngineAssociation；
- 源码测试 worktree 使用源码 UE 的 GenerateProjectFiles.bat；
- 启动时显式指定源码版 Editor 和 .uproject；
- 源码版完整验证通过后，才考虑正式切换项目关联。

### 1.3 Modules

当前项目声明两个模块：

~~~json
"Modules": [
  {
    "Name": "LyraGame",
    "Type": "Runtime",
    "LoadingPhase": "Default",
    "AdditionalDependencies": [
      "DeveloperSettings",
      "Engine"
    ]
  },
  {
    "Name": "LyraEditor",
    "Type": "Editor",
    "LoadingPhase": "Default"
  }
]
~~~

| 模块 | 类型 | 作用 |
|---|---|---|
| LyraGame | Runtime | 游戏运行时核心模块，Client、Server 和 Editor 都可能依赖 |
| LyraEditor | Editor | 仅编辑器使用的工具、资产操作和编辑器扩展 |

LoadingPhase: Default 表示默认加载阶段，不是 C++ 编译顺序。

AdditionalDependencies 是项目描述层的额外依赖提示；真正的 C++ 模块依赖仍由 LyraGame.Build.cs 的 DependencyModuleNames 决定，两者不能互相替代。

### 1.4 Plugins

当前插件可以按职责理解为：

- Gameplay Ability System：GameplayAbilities；
- Lyra/Shooter 基础：ShooterCore、ShooterMaps、ShooterTests；
- Game Feature 与模块化：GameFeatures、ModularGameplay、ModularGameplayActors；
- 输入与 UI：EnhancedInput、CommonUI、CommonInput、UIExtension；
- 网络与复制：ReplicationGraph、Online/Steam 相关插件；
- 资产和表现：Niagara、Water、Volumetrics、音频、动画；
- 自动化和调试：Gauntlet、FunctionalTestingEditor、GameplayInsights；
- Editor 专用：通过 TargetAllowList 限制目标类型；
- 平台专用：通过 SupportedTargetPlatforms 或 PlatformAllowList 限制平台。

常见字段：

- Enabled: true：项目声明启用插件；
- TargetAllowList：限制目标类型；
- SupportedTargetPlatforms：限制平台；
- PlatformAllowList：限制平台族或发行环境。

uproject 启用插件不等于每个 Target 都会编译它。UnrealBuildTool 还会结合 Target 类型、插件描述文件和 LyraGame.Target.cs 的规则做最终判断。

## 2. 三类 Target 文件

Target 文件位于 Source/ 根目录，继承 UnrealBuildTool 的 TargetRules。它们决定生成什么产品，而不是实现游戏玩法。

### 2.1 三类目标对比

| Target | Type | 包含模块 | 典型产物 | 编辑器功能 |
|---|---|---|---|---|
| LyraEditor | Editor | LyraGame、LyraEditor | Lyra 项目 Editor | 是 |
| LyraClient | Client | LyraGame | 独立 Client | 否 |
| LyraServer | Server | LyraGame | Dedicated Server | 否 |

核心原则：

- Editor 是开发和资产编辑环境；
- Client 是玩家运行时；
- Server 是服务器权威运行时；
- Client/Server 不应依赖 LyraEditor；
- 三者可以共享 LyraGame，但编译定义和可用插件可能不同。

### 2.2 LyraEditor.Target.cs

关键设置：

~~~csharp
DefaultBuildSettings = BuildSettingsVersion.V7;
Type = TargetType.Editor;
ExtraModuleNames.AddRange(new string[] { "LyraGame", "LyraEditor" });
~~~

作用：

1. 使用 UE 的 V7 默认构建设置；
2. 目标类型为 Editor；
3. 同时链接运行时 LyraGame 和编辑器模块 LyraEditor；
4. 非全量模块构建时限制部分原生指针成员行为；
5. 启用 RemoteSession，支持 Unreal Remote 等编辑器/触控场景。

LyraEditor 模块只应放编辑器工具、资产操作、编辑器 UI 和开发辅助逻辑。服务器必须运行的规则不能只放进去。

### 2.3 LyraClient.Target.cs

关键设置：

~~~csharp
Type = TargetType.Client;
ExtraModuleNames.AddRange(new string[] { "LyraGame" });
LyraGameTarget.ApplySharedLyraTargetSettings(this);
~~~

作用：

- 目标类型是 Client；
- 只加入 LyraGame；
- 不加入 LyraEditor；
- 使用 Lyra 的共享 Target 设置。

Client 负责输入、渲染、UI、本地预测和服务器连接。权威规则不能只存在 Client 中。

### 2.4 LyraServer.Target.cs

关键设置：

~~~csharp
[SupportedPlatforms(UnrealPlatformClass.Server)]
Type = TargetType.Server;
ExtraModuleNames.AddRange(new string[] { "LyraGame" });
bUseChecksInShipping = true;
~~~

作用：

- 声明 Server 平台目标；
- 类型为 Dedicated Server；
- 只加入 LyraGame；
- Shipping Server 保留更多检查，有助于发现服务器规则错误。

Dedicated Server 不应依赖编辑器模块，也不应把 UI、渲染或客户端表现当作核心逻辑。它是战局实时权威的一方。

### 2.5 共享 Target 设置

三个 Target 都调用 LyraGameTarget.ApplySharedLyraTargetSettings(this)。该函数实际定义在 [LyraGame.Target.cs](../../Source/LyraGame.Target.cs)。

它集中处理：

- 默认构建设置和 Include 顺序；
- Test、Shipping、Dedicated Server 的差异；
- 非 Editor 目标禁用不需要的插件；
- Game Feature 插件发现、启用和禁用；
- Shipping 的安全限制；
- Runtime AssetRegistry 和内存策略；
- Installed Engine 与源码/Unique BuildEnvironment 的差异。

这是“共享规则 + Target 个性化”的设计：公共策略放在 LyraGame.Target.cs，Editor/Client/Server 文件只表达目标类型和少量差异。

## 3. LyraGame.Build.cs

### 3.1 文件定位

Source/LyraGame/LyraGame.Build.cs 是 C# 编写的模块规则文件，继承 ModuleRules。

它回答：

- 编译 LyraGame 时需要哪些 UE 模块；
- 哪些依赖暴露给引用 LyraGame 的其他模块；
- 哪些依赖只在 LyraGame 内部使用；
- 需要哪些头文件路径、预编译头和编译宏；
- Shipping、Test、Installed Engine 是否改变依赖。

它不是运行时配置，也不是插件启用清单。修改后通常需要重新生成项目文件或重新编译模块。

### 3.2 预编译头和头文件路径

~~~csharp
PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

PublicIncludePaths.AddRange(new string[] {
    "LyraGame"
});
~~~

含义：

- 使用显式或共享 PCH，改善编译速度和一致性；
- 将 LyraGame 目录加入公开头文件搜索路径；
- PrivateIncludePaths 当前没有额外路径。

不要为了快速解决 include 错误而随意增加全局 IncludePath，优先使用正确的模块依赖和相对头文件引用。

### 3.3 PublicDependencyModuleNames

Public 依赖会影响 LyraGame 的公开头文件和使用 LyraGame 的模块。当前重要依赖包括：

- 基础运行时：Core、CoreUObject、Engine、ApplicationCore；
- 网络/平台基础：CoreOnline、NetCore；
- Gameplay：GameplayTags、GameplayTasks、GameplayAbilities；
- AI：AIModule；
- 模块化与 Game Feature：ModularGameplay、ModularGameplayActors、GameFeatures；
- 数据和复制：DataRegistry、ReplicationGraph、SignificanceManager；
- UI/启动流程：CommonLoadingScreen；
- 表现与工具：Niagara、ControlFlows、PropertyPath；
- 异步能力：AsyncMixin；
- 其他 Lyra 基础运行时：Hotfix。

判断规则：如果 LyraGame 的公开头文件包含某模块的类型，或者下游模块必须理解该类型，才考虑放入 Public 依赖。

### 3.4 PrivateDependencyModuleNames

Private 依赖只供 LyraGame 的实现文件使用，不自动向下游暴露。当前重要依赖包括：

- 输入和 UI：InputCore、EnhancedInput、Slate、SlateCore、UMG、CommonUI、CommonInput；
- 渲染和底层：RenderCore、RHI、PhysicsCore；
- 项目配置：DeveloperSettings、Projects、GameSettings；
- 网络和安全：DTLSHandlerComponent、Json；
- Lyra UI/用户：CommonGame、CommonUser、GameSubtitles、UIExtension；
- 测试与自动化：Gauntlet、ClientPilot；
- 消息和音频：GameplayMessageRuntime、AudioMixer、AudioModulation；
- 回放与平台：NetworkReplayStreaming、PlatformDLC；
- External RPC：ExternalRpcRegistry、HTTPServer。

如果依赖只出现在 .cpp 或私有实现中，优先放 Private，不要因为“能编译”就全部放 Public。

### 3.5 动态加载模块

当前 DynamicallyLoadedModuleNames 为空。

动态加载与静态依赖不同：

- 静态依赖参与编译和链接；
- 动态依赖通常在运行时按模块名加载；
- 动态加载不能替代公开头文件所需的编译依赖。

### 3.6 编译宏与安全规则

当前模块定义：

~~~csharp
PublicDefinitions.Add("SHIPPING_DRAW_DEBUG_ERROR=1");
~~~

作用是让 Shipping/Test 等构建在使用 DrawDebug 时尽早暴露问题，避免调试绘制代码意外进入发行逻辑。

External RPC 相关逻辑按构建类型切换：

- Shipping：关闭 RPC Registry、HTTP Server Listener 和 Automation Driver；
- 非 Shipping：源码引擎环境可启用 Automation Driver；
- Installed Engine：即使不是 Shipping，也可能关闭 Automation Driver；
- WITH_RPC_REGISTRY、WITH_HTTPSERVER_LISTENERS、WITH_AUTOMATION_DRIVER 通过 PublicDefinitions 控制条件编译。

这说明 Build.cs 不只是列依赖，也会根据 Target.Configuration 和 Target.bIsEngineInstalled 改变模块能力。

### 3.7 Gameplay Debugger 和 Iris

文件末尾调用 SetupGameplayDebuggerSupport(Target) 和 SetupIrisSupport(Target)。这两个辅助函数会根据目标配置设置 Gameplay Debugger 和 Iris 网络复制支持。

新增网络复制功能时，应先检查 Lyra 已有的 Iris/ReplicationGraph 集成，避免另起一套复制路径。

## 4. 从项目文件到可执行文件的原理

一次源码构建大致经过：

~~~text
读取 LyraStarterGame.uproject
        │
        ├── 读取 Modules
        ├── 读取 Plugins
        └── 确定项目和引擎范围
        │
        ▼
选择一个 Target.cs
        │
        ├── Editor：LyraGame + LyraEditor
        ├── Client：LyraGame
        └── Server：LyraGame
        │
        ▼
读取 LyraGame.Build.cs
        │
        ├── 解析 Public/Private 依赖
        ├── 解析 IncludePath 和 PCH
        ├── 应用编译宏
        └── 按配置裁剪能力
        │
        ▼
UnrealBuildTool 生成编译动作
        │
        ▼
编译模块并生成 Editor / Client / Server 产物
~~~

三个层次的职责：

| 层次 | 文件 | 主要决定 |
|---|---|---|
| 项目描述 | .uproject | 项目模块、插件、引擎关联 |
| 产品目标 | *.Target.cs | 生成什么目标、包含哪些模块 |
| 模块规则 | *.Build.cs | 单个模块如何编译、依赖什么 |

## 5. 常用验证命令

以下命令应显式使用源码 UE 和测试 worktree 的路径，不要依赖 PATH 中的另一套 UE：

~~~powershell
$Build = Join-Path $SourceUE_ROOT 'Engine\Build\BatchFiles\Build.bat'
$Project = Join-Path $SourceTestRoot 'LyraStarterGame.uproject'

& $Build LyraEditor Win64 Development "-Project=$Project"
& $Build LyraClient Win64 Development "-Project=$Project"
& $Build LyraServer Win64 Development "-Project=$Project"
~~~

预期结果：

- 三个目标都出现 Result: Succeeded；
- Editor 产物包含 LyraEditor；
- Client 和 Server 不包含 LyraEditor；
- Server 产物不依赖客户端 UI 和渲染逻辑；
- 生成物位于测试 worktree，而不是主 Launcher worktree。

## 6. 常见误区

### 误区一：修改 uproject 就能解决所有编译依赖

不能。uproject 负责项目模块和插件声明；C++ 模块之间的编译依赖必须在对应 Build.cs 中声明。

### 误区二：Client 和 Server 只是在启动参数上不同

不是。它们是不同 Target，编译时可用的插件、定义、模块和运行时能力都可能不同。

### 误区三：把 LyraEditor 加入 Client 或 Server

不应该。Editor 模块包含编辑器工具和编辑器专用依赖，加入运行时目标会增加体积并造成目标边界混乱。

### 误区四：所有依赖都放到 Public

不应该。Public 依赖会扩大下游编译范围和耦合。只有公开头文件或下游接口真正需要的依赖才放 Public。

### 误区五：Launcher 编译失败就修改 Target.cs

先确认实际调用的是哪套 UE。Launcher 安装版不支持某些 Client/Server 构建时，问题可能是引擎发行方式，而不是 Target.cs 写错。

## 7. 阅读和修改检查清单

- [ ] 确认 .uproject 的 EngineAssociation 和目标引擎；
- [ ] 确认项目模块是 Runtime 还是 Editor；
- [ ] 确认插件是否按平台和 Target 类型限制；
- [ ] 阅读目标对应的 Target.cs；
- [ ] 跟进 ApplySharedLyraTargetSettings；
- [ ] 再阅读模块的 Build.cs；
- [ ] 判断新增依赖应放 Public 还是 Private；
- [ ] 确认 Shipping、Installed Engine、Server 的条件分支；
- [ ] 使用源码 UE 的 Build.bat 显式构建；
- [ ] 检查生成物是否位于隔离的测试 worktree；
- [ ] 不把 Editor 专用代码加入 Client/Server；
- [ ] 不用修改主项目 EngineAssociation 来替代隔离测试。

