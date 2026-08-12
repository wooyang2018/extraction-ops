# Unreal Editor Agent Skill 与 Codex Skill：两层技能系统的边界与项目实践

## 1. 为什么需要区分两种 Skill

ExtractionOps 当前同时使用了两种名称相似、所属层次不同的 Skill：

- Unreal Editor Agent Skill：保存在项目 Content 中，由 Unreal Editor 内的 Agent Skill Registry 发现；
- Codex Skill：保存在 Codex 工具环境中的 `SKILL.md`，由 Codex 在执行相关任务前读取。

它们都用于向 AI 提供可复用的工作约束，但并不是同一种资产，也不由同一套系统加载。简单地说：

> Codex Skill 指导外层开发代理如何工作；Unreal Editor Agent Skill 指导已经连接到 Editor 的代理如何在当前 Unreal 项目中工作。

当前项目注册的三项 ExtractionOps Skill 属于第一类，也就是“Editor 内的项目资产”；它们既不是 Codex 本地 Skill，也不是新增的 Gameplay 系统。

## 2. 核心区别

| 维度 | Unreal Editor Agent Skill | Codex Skill |
| --- | --- | --- |
| 所属系统 | Unreal MCP / Agent Skill Registry | Codex 技能系统 |
| 当前项目中的载体 | `UAgentSkill` 类型的 `.uasset` | 文件系统或插件提供的 `SKILL.md` |
| 保存位置 | GameFeature 的 `Content/Skills` | Codex 用户目录、系统目录或插件缓存 |
| 发现方式 | Editor 中调用 `ListSkills` | Codex 根据会话提供的技能目录和触发规则选择 |
| 内容结构 | `Description` 与 `Instructions` | YAML 元数据、触发说明和完整工作指令 |
| 典型作用域 | 当前 Unreal 项目的资产操作、权威规则和 Editor 验收约束 | 完整开发任务，可覆盖源码、Shell、文档、网络和 Editor MCP |
| 修改方式 | 通过 live Editor 创建、更新和保存，遵守 Unreal 序列化 | 通过文本文件或技能插件维护 |
| 版本控制形态 | 二进制 `.uasset`，另以注册脚本保留可审查的文本定义 | 可直接审查的文本文件 |
| 是否提供新工具 | 否，只提供工作知识和约束 | 通常也不直接提供能力；可以规定如何使用现有工具与脚本 |

两者的共同点是：Skill 本身主要是上下文和操作规范，不会因为存在就自动完成资产创建、运行测试或获得额外权限。实际能力仍由当前可调用的工具、Editor 状态和项目代码决定。

## 3. Unreal Editor Agent Skill 的数据模型

项目专属 Unreal Skill 是保存在 Content Browser 中的 `UAgentSkill` 资产，主要包含两个字段。

### 3.1 Description

`Description` 是发现阶段读取的简短说明。它应回答两个问题：

- 这项 Skill 解决什么问题；
- 什么任务应该加载它。

Description 不适合塞入完整步骤，因为代理在决定是否激活 Skill 时需要快速判断相关性。

### 3.2 Instructions

`Instructions` 是 Skill 激活后载入的完整约束。它应记录工具无法从资产反射中自行推断、但对项目正确性非常重要的知识，例如：

- 哪些 Lyra 系统必须复用；
- Client 与 Server 的权威边界；
- 项目状态机中不允许出现的转换；
- 多人验收必须采用什么进程模型；
- 哪个 EngineRoot 允许使用，哪个目录绝对禁止访问。

Skill 不应大量硬编码容易随插件升级变化的工具函数名称。代理可以在运行时发现 Toolset，Skill 更适合保存稳定的项目原则和验收不变量。

## 4. ExtractionOps 为什么使用 UAsset Skill

Unreal Agent Skill 支持两条常见实现路径：

- Python `UAgentSkill` 子类：适合属于通用代码插件、需要随插件自动注册的 Skill；
- Content Browser 中的 `UAgentSkill` 资产：适合只服务于单个项目、不需要新增代码的 Skill。

ExtractionOps 的规则高度依赖当前项目，包括唯一 Experience、Raid 状态机、UE 5.8 Installed Build 路径和 Editor Dedicated Process 验收口径。因此使用项目 Content 中的 UAsset 比放进通用 MCP 插件更合适。

这些资产位于：

```text
Plugins/GameFeatures/ExtractionOps/Content/Skills/
  ExtractionOpsAssetAuthoring.uasset
  ExtractionOpsAuthorityWorkflow.uasset
  ExtractionOpsValidationWorkflow.uasset
```

它们对应的 Unreal 包路径是：

```text
/ExtractionOps/Skills/ExtractionOpsAssetAuthoring
/ExtractionOps/Skills/ExtractionOpsAuthorityWorkflow
/ExtractionOps/Skills/ExtractionOpsValidationWorkflow
```

Agent Skill Registry 返回的生成类对象路径还会包含对象名和 `_C` 后缀，例如：

```text
/ExtractionOps/Skills/ExtractionOpsAssetAuthoring.ExtractionOpsAssetAuthoring_C
```

包路径、磁盘上的 `.uasset` 文件和 Registry 返回的类对象路径描述的是同一项资产的不同寻址层次，编写验证脚本时不能混用。

## 5. 三项项目 Skill 的职责

### 5.1 ExtractionOpsAssetAuthoring

路径：`/ExtractionOps/Skills/ExtractionOpsAssetAuthoring`

用途：约束 ExtractionOps 资产的创建、修改和验证过程。

它固定了以下原则：

- 保持唯一的 `B_ExtractionExperience` 和 ExtractionOps GameFeatureData；
- 优先复用 Lyra 的 Character、ASC、移动、动画、Inventory、Equipment、QuickBar 和 Ranged TargetData 流程；
- C++ 负责规则、服务器权威和复制，Blueprint/DataAsset 负责配置与表现；
- 资产操作前先通过反射确认真实类型、父类和属性；
- 资产修改必须串行执行，不能在 PIE 中进行；
- Blueprint 需要完成编译、保存，并反查父类、引用和 dirty 状态；
- 禁止在 Editor 外生成、拼接或修补 `.uasset`。

这项 Skill 解决的是“如何安全地改 Unreal 资产”，而不是决定玩法规则。

### 5.2 ExtractionOpsAuthorityWorkflow

路径：`/ExtractionOps/Skills/ExtractionOpsAuthorityWorkflow`

用途：维护服务器权威边界、Raid 状态机和物品唯一性不变量。

它固定了以下原则：

- Client 只产生输入和预测表现；
- Server 决定弹药提交、TargetData 校验、伤害、终端、Threat 波次、背包命令、死亡掉落、撤离和结果快照；
- Match、Run、Terminal 与 Extraction 必须遵守项目规定的状态转换；
- 交互 Ability 使用所属 PlayerState 的 ASC；
- 每个 Threat 波次只能调度一次；
- 每个物品实例使用稳定 GUID，并且同一时刻只能位于世界、容器、背包、已撤离或已丢失状态之一；
- 旧背包版本和重复 `request_id` 必须被确定性拒绝。

这项 Skill 不实现状态机。它确保代理在修改 C++、Blueprint 或测试资产时，不会破坏状态机已经承诺的权威边界。

### 5.3 ExtractionOpsValidationWorkflow

路径：`/ExtractionOps/Skills/ExtractionOpsValidationWorkflow`

用途：固定项目构建、自动化和双客户端 E2E 的证据标准。

它固定了以下原则：

- 从所有 Unreal 进程关闭的状态开始；
- 只使用 `D:/Software/UE_5.8` 构建 `LyraEditor Win64 Development`；
- 运行全部 `ExtractionOps.*` 自动化测试；
- 使用一个 `-server -NullRHI` Editor Dedicated Process 和两个独立 `-game` 客户端；
- 在 `L_ExtractionTest`、`B_ExtractionExperience`、`NumBots=0` 的统一条件下采集证据；
- 检查两次 Join、Experience 与 GameFeature 激活、Raid 启动、角色复制、单客户端断线隔离和 Server 退出后的明确断线；
- 额外覆盖 100 ms 延迟以及 100 ms 延迟加 5% 丢包；
- 明确把结果称为 Editor Dedicated Process 证据，不能包装成独立 Server Target 或 Shipping 性能数据；
- 禁止访问 `D:/Software/UE_5.8.1_Source`。

这项 Skill 解决的是“什么证据才算完成”，避免不同周或不同代理采用互不相容的验收口径。

## 6. 为什么拆成三项，而不是写成一个大 Skill

三项 Skill 分别对应三类任务：

```text
资产创作任务
  -> ExtractionOpsAssetAuthoring

网络权威与玩法规则任务
  -> ExtractionOpsAuthorityWorkflow

构建、自动化与多人验收任务
  -> ExtractionOpsValidationWorkflow
```

拆分的好处是按需加载：修改 Widget 时不必加载完整 E2E 参数，运行网络验收时也不必携带所有资产创作细节。这样既减少上下文成本，也能让每项 Description 更准确地触发对应工作流。

当一个任务横跨多个领域时，可以同时读取多项 Skill。例如实现并验收终端交互时，资产配置需要 AssetAuthoring，权威状态转换需要 AuthorityWorkflow，最终多人测试需要 ValidationWorkflow。

## 7. 三项 Skill 是如何注册的

可审查的注册定义保存在：

```text
Scripts/Mcp/Register-ExtractionOpsSkills.json
```

该脚本不是在文件系统中手工制造 `.uasset`，而是通过 live Unreal Editor MCP 调用 Editor 内的 Agent Skill Toolset。注册流程为：

```text
1. ListSkills
   读取当前 Registry，避免创建重复 Skill

2. CreateSkill 或 UpdateSkill
   不存在时创建；已存在时按完整生成类路径更新

3. SaveAssets
   保存三个 /ExtractionOps/Skills 包

4. GetSkills
   回读 Description 和 Instructions，验证 Registry 看到的最终内容
```

重复执行脚本会更新现有 Skill，而不是不断生成同名资产。因此它同时承担可重复注册和文本化审查入口的职责。

验证脚本 `Scripts/Mcp/Verify-Week04-07Assets.json` 还会检查：

- 三个包路径都存在；
- Registry 中 `/ExtractionOps/Skills/` 前缀下恰好有三项 Skill；
- 所有相关资产均已保存，不存在 dirty package。

项目验收记录进一步确认：Editor 重启后的新会话仍可通过 `ListSkills/GetSkills` 发现并读取三项 Skill。这证明它们不是只存在于某次 MCP 会话内的临时注册结果。

## 8. 创建和修改 UAsset Skill 时的正确流程

项目后续若需要调整这三项 Skill，应继续遵守以下顺序：

1. 在 live Editor 中先列举当前 Skill；
2. 读取目标 Skill 的 Description 与 Instructions；
3. 判断应该更新现有职责，还是确实存在新的独立任务域；
4. 通过 Editor Toolset 更新或创建，不从外部直接写 `.uasset`；
5. 保存包；
6. 通过 Registry 回读最终内容；
7. 检查 dirty 状态；
8. 必要时重启 Editor，再验证持久化与自动发现。

新增 Skill 不应只是为了记录普通文档知识。只有当某条知识会反复影响代理的项目操作，而且不能仅靠 Editor 工具即时发现时，才值得占用 Skill 上下文。

## 9. 版本控制与可审查性

UAsset Skill 的最终持久化载体是二进制文件，代码审查无法像 Markdown 一样直接阅读其字段差异。因此 ExtractionOps 同时保留两层证据：

- `.uasset` 是 Editor 实际加载和注册的结果；
- `Register-ExtractionOpsSkills.json` 保存 Description 与 Instructions 的文本定义；
- `Verify-Week04-07Assets.json` 保存结构和 dirty 状态断言；
- `docs/evidence/week-04-07-acceptance.md` 保存重启后验证结论。

修改 Skill 时应同步更新并验证这些层次，避免出现“脚本文本已经变更，但 UAsset 没有重新保存”或“UAsset 已修改，但没有可审查定义”的漂移。

## 10. 使用边界

需要特别注意以下几点：

- Unreal Agent Skill 不是 Gameplay Ability，不参与玩家运行时逻辑；
- Skill 不会自动执行注册脚本、构建或 E2E 测试；
- Skill 不会绕过 Editor MCP 的连接、Toolset 可用性或 Unreal 权限；
- Skill 中写着“Server 权威”并不等于代码已经安全，仍需要源码审查和多人测试；
- 三项 UAsset 的存在不等于 Codex 会自动看到它们，外层代理仍需连接 Editor，并通过 Registry 发现和读取；
- Codex Skill 可以指导如何连接和调用 Unreal MCP，但它不能替代项目内 Skill 保存的 ExtractionOps 专属不变量。

因此，两层 Skill 最合理的协作方式是：

```text
Codex Skill
  指导代理如何发现和使用 Unreal MCP
        |
        v
Unreal Editor Agent Skill
  提供 ExtractionOps 项目专属约束
        |
        v
Editor Toolsets + 项目源码/资产
  执行实际修改与验证
```

## 11. 对应实现索引

- `Plugins/GameFeatures/ExtractionOps/Content/Skills/ExtractionOpsAssetAuthoring.uasset`
- `Plugins/GameFeatures/ExtractionOps/Content/Skills/ExtractionOpsAuthorityWorkflow.uasset`
- `Plugins/GameFeatures/ExtractionOps/Content/Skills/ExtractionOpsValidationWorkflow.uasset`
- `Scripts/Mcp/Register-ExtractionOpsSkills.json`
- `Scripts/Mcp/Verify-Week04-07Assets.json`
- `docs/evidence/week-04-07-acceptance.md`
- `docs/implementation-status.md`
