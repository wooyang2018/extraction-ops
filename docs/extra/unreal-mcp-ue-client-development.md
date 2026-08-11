# Unreal MCP、UE 与游戏客户端开发入门

本文面向第一次接触 Unreal Engine、Gameplay Ability System（GAS）、GameFeature 和 MCP 的开发者，目标不是背工具名称，而是建立一张可以用于实际开发的地图：

```text
Codex
  ↓ MCP 协议
Unreal MCP Server
  ↓ ToolsetRegistry
UE Editor 工具集
  ↓
资产、插件、GAS、UI、特效、测试和日志
```

本文以当前项目和 UE 5.8 引擎中的实现为准。当前项目在 [`LyraStarterGame.uproject`](../../LyraStarterGame.uproject) 中启用了：

- `ModelContextProtocol`：在 Unreal Editor 内提供 MCP HTTP Server；
- `ToolsetRegistry`：管理可被 AI 调用的 Toolset；
- `AllToolsets`：聚合引擎提供的 Editor Toolset；
- Lyra 使用的 `GameplayAbilities`、`GameFeatures`、`CommonUI`、`EnhancedInput`、`OnlineServices` 等能力。

## 1. 先建立三个基本概念

### 1.1 Unreal MCP 是什么

MCP（Model Context Protocol）是一种让 AI 客户端调用外部工具的协议。对本项目来说，Codex 不会直接操作 Unreal 的 C++ 内存，而是向 Unreal MCP Server 发送结构化请求：

```text
Codex：请查询当前打开的地图和选中的 Actor
  ↓
MCP：调用某个 Tool，并传入 JSON 参数
  ↓
UE Editor：读取当前编辑器状态
  ↓
MCP：返回结构化结果
  ↓
Codex：解释结果，决定下一步操作
```

Unreal MCP Server 的默认地址是：

```text
http://127.0.0.1:8000/mcp
```

在 Unreal Editor 的控制台中可以按以下顺序启动和刷新：

```text
ModelContextProtocol.StartServer
ModelContextProtocol.RefreshTools Codex
```

`ModelContextProtocol.StartServer` 会启动 UE 内的 MCP HTTP Server；也可以带端口参数，例如 `ModelContextProtocol.StartServer 8000`。`ModelContextProtocol.RefreshTools Codex` 的 `Codex` 参数在当前 UE 5.8 实现中不会参与筛选，实际效果等同于 `ModelContextProtocol.RefreshTools`：清空当前已注册的全部工具，广播刷新事件，让 Toolset 重新注册，并通知已连接的 MCP 会话工具列表发生变化。

刷新期间工具列表可能短暂为空，正在执行的工具调用也可能失败；如果某个 Toolset 重新注册失败，它可能暂时从工具列表消失。因此应在没有重要 MCP 调用执行时刷新。

Codex 端的连接配置位于用户级配置中，例如：

```toml
[mcp_servers.unreal-mcp]
url = "http://127.0.0.1:8000/mcp"
```

MCP Server 和 Toolset 不是一回事：

| 概念 | 作用 |
| --- | --- |
| MCP Server | 提供 HTTP/MCP 协议入口，接收工具调用 |
| Toolset | 一组围绕某个 UE 领域组织的工具 |
| Tool | 一个具体操作，例如查询 Gameplay Tag 或修改 Widget |
| Codex | 发现工具、组合工具、解释结果并决定下一步 |

因此，“连接成功”只说明 Codex 能访问 Server，不代表所有 UE 工具都已经加载，也不代表 Codex 能自动完成任何游戏开发任务。

### 1.2 当前工具搜索模式

当前 UE 5.8 的 MCP 设置默认启用工具搜索模式：

```text
bEnableToolSearch = true
```

在这种模式下，Codex 通常先看到少量元工具：

- `list_toolsets`：列出当前可用的 Toolset；
- `describe_toolset`：查看某个 Toolset 的工具和参数；
- `call_tool`：按 Toolset 名称和工具名执行具体操作。

这是一种按需发现方式。它避免在每次请求中把几百个工具的完整 Schema 一次性塞给模型，也降低上下文消耗。看到的工具少，不等于 UE 只能做三件事。

### 1.3 Toolset 是如何进入 UE 的

一个 UE Toolset 通常继承 `UToolsetDefinition`，并把允许 AI 调用的函数声明为带有 `AICallable` 元数据的 `UFUNCTION`。Toolset 模块启动时向 `ToolsetRegistry` 注册自己的类，MCP Editor 适配器再把它们转换成 MCP Tool。

```text
UToolsetDefinition
  ├── UFUNCTION(meta=(AICallable)) ValidateSomething()
  ├── UFUNCTION(meta=(AICallable)) FindSomething()
  └── UFUNCTION(meta=(AICallable)) ModifySomething()
```

这意味着能力不是由模型“想象”出来的，而是由当前进程中已注册的函数决定。没有注册的函数，Codex 不能凭空调用。

## 2. 当前可覆盖的 UE 能力

下面按学习顺序介绍能力。每一节都说明四件事：UE 概念是什么、MCP 能做什么、对客户端开发有什么用、目前的边界是什么。

### 2.1 Editor 状态、资产和日志

#### UE 概念

Unreal Editor 是一个长期运行的工具进程。它维护当前打开的地图、选中的 Actor、Content Browser 资产、World、Play-in-Editor（PIE）会话、Output Log 和各种 Editor Subsystem。

游戏客户端开发的大量时间并不在写 C++，而在确认：

- 当前打开的到底是哪张地图；
- 当前 Experience、PawnData、InputConfig、Weapon 和 HUD 是什么；
- 某个资产引用了哪些依赖；
- 某个错误来自资产、插件、模块还是运行时逻辑；
- 当前 PIE 是 Client、Listen Server 还是 Dedicated Server 模式。

#### MCP 能做什么

Editor 类 Toolset 可以让 AI 查询或操作编辑器状态，例如：

- 获取当前地图、当前项目和选中对象；
- 查询资产、类、组件和引用；
- 读取 Output Log 或 Editor 日志；
- 检查某个对象的属性和类型；
- 触发部分 Editor 操作；
- 在修改后重新查询结果进行验证。

#### 对客户端开发的价值

它可以把“打开编辑器到处点”变成“先查询事实，再决定修改”。例如：

```text
查询当前 Experience
  → 找到 PawnData 和 InputConfig
  → 找到默认 AbilitySet
  → 找到武器 ItemDefinition
  → 记录真实资产路径
```

这一步很重要。Lyra 中同一个功能往往由多个资产、GameFeature 和 C++ 类共同组成。没有先建立引用链，直接复制资产容易得到一套看似能打开、实际运行时缺依赖的半成品。

#### 边界

Editor 查询工具不等于完整的源码分析器，也不等于 Git 工具。需要修改任意 C++ 文件、运行 PowerShell、提交 Git 或访问 Go Backend 时，仍需要对应的文件、Shell、Git 或网络工具。

### 2.2 GameFeature：把玩法做成可装卸模块

#### GameFeature 是什么

GameFeature 是 UE 用于模块化游戏功能的机制。它的核心思想是：不要把所有玩法都硬编码进 Lyra 核心，而是把一项功能封装成可独立激活和停用的插件。

可以把它理解成“游戏运行时的功能包”：

```text
GameFeature Plugin
  ├── Content/
  ├── Source/
  ├── Plugin 配置
  └── Activation Actions
```

GameFeature 的典型生命周期是：

```text
Installed → Registered → Loaded → Activated → Deactivated → Unloaded
```

激活动作可以向游戏注入：

- Experience；
- Pawn、Input、Ability 或 UI；
- Component；
- Gameplay Tag；
- DataRegistry 或其他资产；
- 特定地图或模式的规则。

#### MCP 能做什么

GameFeatures Toolset 可以帮助：

- 查询已安装和已注册的 GameFeature；
- 加载、激活、停用 Feature；
- 检查 Feature 的依赖和状态；
- 观察激活后出现的资产、组件和能力；
- 在开发循环中快速验证一个 Feature 是否真正生效。

#### 对 ExtractionOps 的意义

当前项目的新玩法应优先放到：

```text
Plugins/GameFeatures/ExtractionOps/
```

例如：

- `ExtractionExperience`：定义本项目的游戏体验；
- `ExtractionPawnData`：定义玩家 Pawn 和输入/能力集合；
- `ExtractionLootFeature`：注入战利品和搜刮规则；
- `ExtractionThreatFeature`：注入 Threat 计量和 AI 反应；
- `ExtractionUIFeature`：注入 HUD、背包和撤离提示。

MCP 可以帮助验证 Feature 的生命周期，但不能替你决定哪些状态必须由 Server 权威维护。Feature 是模块边界，不是网络权威边界。

#### 初学者要记住的区别

```text
GameFeature = 功能如何装配和启用
Dedicated Server = 游戏状态由谁最终说了算
MCP = AI 如何检查和操作 Editor
```

三者解决的是不同问题。

### 2.3 Gameplay Tags：游戏状态的可组合标签

#### Gameplay Tag 是什么

Gameplay Tag 是层级化、可组合的字符串标识，例如：

```text
InputTag.Ability.Fire
Ability.Weapon.Reload
State.Dead
State.Extraction.InProgress
Event.Loot.PickedUp
GameplayCue.Damage
```

它不是普通的 C++ 枚举，也不是最终状态本身。Tag 更像“语义索引”：不同系统可以用同一个标签协作，而不用互相依赖具体类。

#### Tag 的常见用途

- 输入映射：某个按键触发哪个 Ability；
- Ability 激活条件：拥有或缺少哪些标签；
- Ability 阻断：换弹时阻止射击；
- 状态表达：死亡、眩晕、撤离中；
- 事件通知：拾取、命中、开火、结算；
- Gameplay Effect：给目标添加或移除状态；
- UI 过滤：根据状态显示按钮、准星或提示。

#### MCP 能做什么

Gameplay Tags Toolset 可以帮助：

- 搜索现有 Tag，避免创建重复语义；
- 查看 Tag 层级和来源；
- 检查资产是否引用某个 Tag；
- 检查输入、Ability、Effect 和 UI 是否使用一致的 Tag；
- 在工具支持写入时创建或修改 Tag 配置。

#### 初学者常见错误

不要把所有东西都设计成 Tag：

```text
Tag：State.Dead
事实：Health == 0、DeathTimestamp、KillerPlayerId
```

Tag 适合表达“我处于什么语义状态”，不应替代完整业务数据。Tag 也不应该成为绕过 Server 校验的客户端写入入口。

### 2.4 GAS：Lyra 中的能力、属性和效果

GAS 是 Lyra 战斗和交互系统的核心之一。初学者可以先把它理解成一套“数据驱动的能力与状态框架”。

#### 2.4.1 Ability System Component

`AbilitySystemComponent`（ASC）是 GAS 的运行时中枢，负责管理：

- 已授予的 Gameplay Ability；
- 当前属性和 Gameplay Effect；
- Tag；
- Ability 激活、取消和结束；
- 预测和复制相关状态。

Lyra 通常把 ASC 放在 PlayerState 或与 PlayerState 生命周期关联的对象上，让玩家死亡、换 Pawn 或重生时仍能保持合适的能力状态。

#### 2.4.2 Gameplay Ability

Gameplay Ability 表示一个可激活的行为，例如：

- 开火；
- 换弹；
- 跳跃；
- 交互；
- 使用医疗包；
- 启动撤离终端。

Ability 通常包含：

```text
输入触发
  → CanActivate 检查
  → Commit 消耗资源/冷却
  → 执行行为
  → 等待动画、TargetData 或事件
  → 结束或取消
```

#### 2.4.3 AttributeSet

AttributeSet 保存可被 GAS 管理的数值属性，例如：

```text
Health / MaxHealth
Armor / MaxArmor
Stamina
Ammo
```

属性是事实的一部分，尤其是生命、弹药和护甲，不能由客户端直接决定最终值。

#### 2.4.4 Gameplay Effect

Gameplay Effect 表示对属性或状态的修改，例如：

- 造成伤害；
- 恢复生命；
- 添加护甲；
- 施加减速；
- 添加持续时间状态；
- 修改冷却或资源消耗。

一条典型伤害链可以表示为：

```text
客户端提交开火意图
  → Server 验证拥有者、武器、射速、弹药和方向
  → Server 计算命中
  → Server 应用 Damage GameplayEffect
  → HealthSet 更新
  → 死亡 Tag / UI / 复制状态更新
```

#### 2.4.5 Gameplay Cue

Gameplay Cue 更偏向表现层，例如：

- 命中特效；
- 受击音效；
- 枪口火焰；
- 死亡表现；
- UI 反馈。

Cue 不应该成为真正的伤害事实来源。表现可以预测，最终生命和死亡必须由 Server 确认。

#### 2.4.6 MCP 能做什么

GAS Toolset 可以帮助：

- 查找 Ability、Effect、Cue 和 Attribute；
- 查看 Ability 的输入 Tag、激活条件和阻断 Tag；
- 追踪某个角色当前授予的能力；
- 检查某个 Gameplay Effect 修改了哪些属性；
- 找到 Tag、Ability、Weapon 和 UI 之间的配置关系；
- 辅助验证“输入 → Ability → Effect → 状态 → UI”的链路。

#### MCP 不能替代的部分

MCP 可以读写部分 Editor 资产，但它不会自动替你设计正确的网络模型。以下决策仍需要开发者明确：

- 哪些数据是客户端预测，哪些数据只能 Server 写入；
- 哪些 RPC 或 TargetData 必须做校验；
- Ability 被取消时资源是否退回；
- 断线重连后哪些状态从 Server 恢复；
- 同一请求重复到达时如何保证幂等。

### 2.5 DataRegistry、配置和数据驱动客户端

#### DataRegistry 是什么

DataRegistry 提供按类型查询游戏数据的机制。相比在代码里写大量硬编码，它可以让武器、物品、敌人或任务数据由资产驱动。

例如：

```text
Weapon.Rifle.M4
  ├── Damage
  ├── FireRate
  ├── MagazineSize
  ├── ReloadDuration
  └── GameplayTags
```

#### MCP 能做什么

DataRegistry Toolset 和 Config Toolset 可以帮助：

- 查询数据项和来源资产；
- 对比不同武器、物品或配置；
- 找到缺失字段和默认值；
- 检查数据是否被正确加载；
- 批量验证数据规则。

#### 对客户端开发的价值

客户端不是“把所有规则写死在 UI 中”。更稳的分层是：

```text
数据资产/Registry：描述配置
Gameplay Ability：执行行为
Server：确认权威结果
UI：展示当前状态
```

MCP 最适合帮助你检查这四层之间的连接是否完整。

### 2.6 UMG、Slate 和客户端 UI

#### UMG 与 Slate 的关系

- UMG 是面向设计和蓝图的 UI 层，适合制作 Widget、布局和交互；
- Slate 是 UE 更底层的 C++ UI 框架，编辑器和很多运行时控件最终都建立在它之上。

玩家看到的 HUD、背包、提示、撤离进度和伤害反馈，通常是 UMG Widget 消费游戏状态的结果。

#### MCP 能做什么

UMG Toolset 可以帮助：

- 创建 Widget 和子控件；
- 查询 Widget 层级；
- 修改布局、文本、颜色和可见性；
- 检查绑定和组件关系；
- 查找 Widget 使用的资产。

Slate Inspector 可以帮助检查运行中的 UI 树：

```text
Viewport
  → HUD Root
    → Crosshair
    → Health Bar
    → Ammo Counter
    → Extraction Prompt
```

#### UI 的网络边界

UI 只应该消费状态和发送意图：

```text
正确：玩家点击撤离按钮 → 请求交互 → Server 校验 → UI 显示进度
错误：玩家点击撤离按钮 → UI 直接把撤离状态改成成功
```

MCP 可以帮你修改 Widget，但不应该让 UI 成为绕过游戏规则的入口。

### 2.7 Niagara、PCG、Dataflow 和 Physics

这些 Toolset 主要面向表现、环境和资产生产：

- Niagara：粒子、枪口火焰、命中特效、烟雾、环境效果；
- PCG：程序化布置资源、植被、掩体、拾取点或环境装饰；
- Dataflow：节点图、变量、连接和数据处理图；
- Physics：Physics Asset、骨骼碰撞、物理约束；
- StateTree / WorldConditions：状态驱动的行为和世界条件；
- Conversation：对话和交互流程；
- Animation Toolset：动画资产和辅助操作。

客户端开发中，这些工具的价值不只是“帮 AI 画东西”，而是让表现和玩法数据形成可验证的管线。例如：

```text
武器数据改变
  → Niagara 枪口效果参数同步
  → HUD 弹药状态同步
  → 命中 Cue 使用同一伤害语义
  → 自动化测试确认资产没有断引用
```

### 2.8 Automation、日志和 Semantic Search

#### Automation

Automation Toolset 可以帮助发现和执行测试。它适合验证：

- 插件是否能加载；
- GameFeature 激活后是否有预期对象；
- Gameplay Tag、DataRegistry 和资产引用是否完整；
- 基础客户端行为是否回归。

#### Semantic Search

语义搜索可以帮助从“我想找负责换弹的代码/资产”开始，而不是必须知道完整文件名。它适合用于：

- 从行为反查代码和资产；
- 找相似的 Lyra 实现；
- 建立 Experience、Pawn、Ability、Weapon 和 UI 的证据链；
- 在大型插件树中定位扩展点。

语义搜索的结果仍需要回到源码、资产和日志核对，不能把相似名称当成真实调用关系。

### 2.9 MCPClientToolset：让 UE 连接其他 MCP Server

UE 自己还可以通过 `MCPClientToolset` 连接外部 MCP Server，把外部能力注册进 UE 的 ToolsetRegistry。例如理论上可以连接：

- 项目文档检索 Server；
- 数据库只读查询 Server；
- 版本控制或构建系统 Server；
- 外部资产管理 Server。

这会形成双向结构：

```text
Codex → Unreal MCP → UE Toolset
                         ↓
                   External MCP Server
```

但应严格控制权限。不要让 UE Editor 默认连接生产数据库、生产部署系统或能执行任意命令的外部 Server。

## 3. 当前项目中“现成能力”和“需要开发”的区别

### 3.1 当前可以直接利用的能力

当前项目已经具备或启用了：

- UE Editor MCP Server；
- ToolsetRegistry；
- AllToolsets 聚合插件；
- GameFeature、Gameplay Tags、GAS、DataRegistry、UMG、Niagara、Automation 等相关 UE 能力；
- Codex 通过 `unreal-mcp` 访问 `http://127.0.0.1:8000/mcp`。

### 3.2 不能假设已经存在的能力

以下是 ExtractionOps 业务语义，不是 UE 通用 Toolset 自动提供的：

- `ValidateExtractionExperience`；
- `InspectLootTable`；
- `CheckInventoryCapacity`；
- `ValidateThreatState`；
- `SimulateExtractionSettlement`；
- `ReplayReconnectScenario`；
- `CheckSettlementIdempotency`。

如果希望 Codex 直接理解这些概念，需要自行实现 `ExtractionOpsToolset`，而不是只打开 `ModelContextProtocol` 插件。

## 4. 可以实现的高级工作流

MCP 的高级价值不在于单独执行一个工具，而在于让 Codex 把多个工具串成“查询 → 修改 → 验证 → 记录”的闭环。

### 4.1 工作流一：从玩家行为反查 Lyra 调用链

目标：理解“按下 Fire 后到底发生了什么”。

```text
1. 查询当前 Experience
2. 找到 PawnData、InputConfig 和 AbilitySet
3. 查找 Fire InputTag
4. 定位对应 Gameplay Ability
5. 检查 WeaponInstance 和 TargetData
6. 检查 Damage GameplayEffect 和 HealthSet
7. 检查 GameplayCue 和 HUD 消费者
8. 输出带资产路径和源码位置的调用链
```

最终得到：

```text
InputTag
  → Ability
  → WeaponInstance
  → TargetData
  → Server 验证
  → Damage Effect
  → Health
  → Cue / HUD
```

这是最适合初学者的第一个 MCP 项目，因为它把抽象的 Lyra 结构连接到了一个可观察的玩家动作。

### 4.2 工作流二：创建并验证一个 GameFeature

目标：创建一个最小的 ExtractionOps Feature，而不是直接修改 Lyra 核心。

```text
1. 检查现有 GameFeature 插件和命名约定
2. 创建或准备 ExtractionOps Feature
3. 配置 Feature 依赖
4. 添加 Experience 或组件注入动作
5. 添加输入、Tag、Ability 或 UI 资产
6. 激活 Feature
7. 检查运行时是否出现预期对象
8. 停用 Feature
9. 检查对象、输入和 UI 是否正确清理
10. 运行回归测试
```

验收重点不是“插件能激活”，而是：

- 激活前默认 Lyra 不受影响；
- 激活后 ExtractionOps 行为出现；
- 停用后没有残留输入、组件或 UI；
- Dedicated Server 和 Client 的激活结果符合目标；
- 资产依赖不会偷偷指向错误 worktree。

### 4.3 工作流三：设计一个 GAS 交互能力

以“启动撤离终端”为例：

```text
1. 定义输入 Tag：InputTag.Interact
2. 定义状态 Tag：State.Extraction.InProgress
3. 查询现有交互 Ability，复用合适的基类
4. 配置 Ability 激活条件和阻断条件
5. 配置交互距离、持续时间和取消条件
6. 配置客户端表现和 GameplayCue
7. 在 Server 侧添加终端、玩家身份和状态校验
8. 运行正常、离开范围、死亡、重复请求四组测试
```

这里 MCP 可以帮助检查资产和配置，但“终端是否属于当前 Run”“玩家是否有权撤离”“重复请求是否幂等”必须由游戏代码和 Server 规则决定。

### 4.4 工作流四：建立客户端 UI 数据链

以撤离 HUD 为例：

```text
Server 权威状态
  → 复制属性 / Gameplay Tag / 事件
  → Client 状态组件
  → ViewModel 或 Widget Controller
  → UMG Widget
  → Slate Inspector 验证实际显示树
```

Codex 可以按以下步骤工作：

1. 查找当前 HUD Layout；
2. 找到 Health、Ammo、Extraction 状态来源；
3. 检查 Widget 是否直接修改了游戏事实；
4. 修改布局或绑定；
5. 启动 PIE，使用 Slate Inspector 检查 Widget 是否存在；
6. 让状态改变，确认 UI 只消费真实状态；
7. 记录两客户端表现差异和 Server 最终值。

### 4.5 工作流五：资产依赖和引用完整性检查

目标：避免“Editor 能打开，但 Client/Server Cook 失败”。

```text
1. 找到目标 Experience
2. 遍历 PawnData、InputConfig、AbilitySet、Weapon、HUD 和地图引用
3. 检查每个引用资产是否存在
4. 检查插件依赖和 GameFeature 激活条件
5. 检查 Client/Server 是否需要不同资源
6. 运行资产验证或 Cook 前检查
7. 输出缺失引用、循环引用和版本不一致项
```

这类工作流能把 Cook 错误提前到资产编辑阶段发现。

### 4.6 工作流六：测试失败后的自动诊断闭环

```text
运行 Automation Test
  → 获取失败测试名
  → 读取 Output Log
  → 关联资产、模块和 GameFeature
  → 判断是环境、资源、代码还是网络问题
  → 提出最小修复
  → 重跑同一测试
  → 保存前后证据
```

注意不要让 AI 看到测试失败就盲目改代码。先记录：

- 构建配置；
- 地图和 Experience；
- Client/Server 数量；
- 网络延迟和丢包；
- 首个真正错误；
- 修复前后的最终状态。

### 4.7 工作流七：为 ExtractionOps 建立领域 Toolset

如果通用 Toolset 无法表达项目业务，可以添加一个专用 Toolset：

```cpp
UCLASS()
class UExtractionOpsToolset : public UToolsetDefinition
{
    GENERATED_BODY()

public:
    UFUNCTION(meta=(AICallable), Category="ExtractionOps.Validation")
    FExtractionValidationResult ValidateExperience(FName ExperienceId);

    UFUNCTION(meta=(AICallable), Category="ExtractionOps.Loot")
    FLootValidationResult ValidateLootTable(FName LootTableId);
};
```

推荐先实现只读工具：

- 查询当前 Run 状态；
- 检查 Loot/Inventory 数据；
- 验证 Experience；
- 读取测试结果；
- 检查结算幂等记录。

再逐步增加写入工具，并为写入工具提供：

- 参数校验；
- Preview/Dry Run；
- 权限边界；
- 事务或回滚；
- 明确的错误码；
- 操作日志；
- 不允许跨项目或跨环境写入。

## 5. 一套适合小白的实际学习顺序

### 阶段一：只查询，不修改

先学会回答：

- 当前打开了哪张地图？
- 当前选中了什么 Actor？
- 资产的真实路径是什么？
- 当前 Experience、PawnData 和 InputConfig 是什么？
- 某个错误出现在哪个模块？

目标是建立事实感，不急着让 AI 修改资产。

### 阶段二：理解 GameFeature 和资产组合

选择一个最小 Feature，观察：

```text
插件文件
  → 依赖
  → 激活动作
  → Experience
  → Pawn / Input / Ability / UI
```

每次只改一个环节，激活、停用并记录结果。

### 阶段三：理解 GAS 的一条链

只追踪开火或交互中的一条链：

```text
输入
  → Ability
  → Effect
  → Attribute
  → Cue
  → UI
```

同时标记每一步是 Client 预测、Server 权威还是纯表现。

### 阶段四：建立可复现闭环

最后才让 Codex 执行多步骤工作流：

```text
查询 → 修改 → 激活 → 运行 → 测试 → 读日志 → 修复 → 重测
```

每一步都要有成功标准。没有成功标准的自动化只是“看起来很智能”，无法用于游戏开发。

## 6. 安全和工程边界

Unreal MCP 连接的是正在运行的 Editor，因此它拥有的权限可能比普通静态代码工具更大。建议遵守以下规则：

- 不把 MCP Server 暴露到公网；
- 默认只监听 `127.0.0.1`；
- 不让工具直接访问生产数据库或生产服务器；
- 写入资产前先做 Preview 或备份；
- 不暴露“执行任意命令”的通用工具；
- 不让客户端工具直接修改 Server 权威状态；
- 不把 Token、密钥、绝对路径或本地日志提交到 Git；
- 修改后运行资产验证、编译或测试；
- 记录工具调用参数和最终结果。

尤其要区分：

```text
MCP 能调用 UE 工具
≠ MCP 自动理解游戏规则
≠ MCP 自动保证网络安全
≠ MCP 自动保证资产修改可回滚
```

## 7. 常见误区

### 误区一：看到 `unreal-mcp` 就以为所有 UE API 都能调用

实际只能调用已注册的 Tool。没有 Toolset 的 C++ 函数不会自动暴露给 Codex。

### 误区二：Editor 中能运行就代表 Client/Server 没问题

PIE 和 Editor 可能使用未 Cook 的资源、Editor-only 模块或不同的启动路径。真正的 Client/Server 仍需单独 Build、Cook 和连接验证。

### 误区三：AI 改了资产就代表功能完成

资产修改只是配置变化。还必须验证：

- Feature 是否激活；
- Client 和 Server 是否都加载；
- 复制和权威边界是否正确；
- 失败和重复请求是否有明确行为；
- Cook 和独立运行是否通过。

### 误区四：把 Tag 当成数据库

Tag 表达语义，不保存完整的玩家库存、Run、结算或权限事实。

### 误区五：把 UI 当成权威逻辑

UI 可以请求、显示和预测，不能决定生命、弹药、物品归属或撤离结果。

## 8. 最小实践任务

建议按以下任务熟悉系统：

1. 让 Codex 列出当前可用 Toolset；
2. 查询当前地图和当前 Experience；
3. 找到一个 Fire 或 Interact Ability；
4. 输出它使用的 InputTag、GameplayTag 和 GameplayEffect；
5. 找到对应 HUD Widget；
6. 启动 PIE，检查 UI 层级；
7. 运行一次已有自动化测试；
8. 记录一个失败日志并让 Codex 给出证据链；
9. 只修改一个无风险的 Widget 文本或颜色；
10. 重新运行验证并检查 Git 状态。

完成这十步后，再考虑让 Codex 创建 GameFeature 或修改 GAS 资产。

## 9. 参考实现位置

当前项目和引擎中可继续阅读：

- 项目插件启用情况：[`LyraStarterGame.uproject`](../../LyraStarterGame.uproject)
- 第 1 周环境和 MCP 初始化：[week-01-environment-baseline.md](../week-01-environment-baseline.md)
- Lyra 构建入口：[lyra-build-entry-files.md](lyra-build-entry-files.md)
- Lyra 能力研究：[lyra-enabled-capabilities-research.md](lyra-enabled-capabilities-research.md)
- Unreal MCP 插件：`<UE_ROOT>/Engine/Plugins/Experimental/ModelContextProtocol/`
- UE Toolset 聚合插件：`<UE_ROOT>/Engine/Plugins/Experimental/Toolsets/AllToolsets/`
- MCP 设置类：`ModelContextProtocolSettings.h`
- Toolset 适配器：`ModelContextProtocolToolsetRegistryAdapter.h`

本文的核心判断标准是：先理解 UE 的状态、资产和网络边界，再使用 MCP 加速查询、修改和验证；不要让工具调用替代对游戏客户端结构的理解。
