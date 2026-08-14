# Lyra 核心玩法系统：源码采集报告（Round 1）

## 研究边界与证据约定

- 范围：当前仓库 `Source/LyraGame/` 及与核心玩法直接相关的 Lyra 插件源码；必要处追踪 `D:/Software/UE_5.8/Engine/` 中 GAS、Enhanced Input、Modular Gameplay 的实现。
- 排除：`Plugins/GameFeatures/ExtractionOps/` 及其资产、代码和设计。
- 访问日期：2026-08-13。
- 来源层级：当前仓库和 UE 5.8 本地源码均记为 T0；本轮没有使用二手资料。
- 置信度：H=源码直接证明且调用两端均已核对；M=源码直接证明但依赖资产配置/蓝图才能闭合；L=存在 TODO、空实现或尚未核对资产。
- 行号均对应本次工作区快照；后续代码变化会造成漂移。

## 维度 1：GAS 初始化、授予与扩展数据链

### 发现 1：玩家 ASC 放在 PlayerState，Pawn 只是可替换 Avatar

- **内容**：`ALyraPlayerState` 持有长期存续的 ASC。PlayerState 在组件初始化时先用当前 Pawn（可能为空）建立 ActorInfo；Experience 加载后，服务器选择 `PawnData` 并授予其中 AbilitySets。Pawn 的模块化初始化达到 `DataInitialized` 时，HeroComponent 再把 PlayerState 设为 Owner、当前 Pawn 设为 Avatar。该结构使能力、属性等玩家状态跨死亡换 Pawn 保留，同时允许 Avatar 快速切换。
- **关键符号**：`ALyraPlayerState::PostInitializeComponents`、`ALyraPlayerState::OnExperienceLoaded`、`ALyraPlayerState::SetPawnData`、`ULyraHeroComponent::HandleChangeInitState`、`ULyraPawnExtensionComponent::InitializeAbilitySystem`。
- **代码证据**：
  - `Source/LyraGame/Player/LyraPlayerState.cpp:109-121`：Experience 完成后由 GameMode 选择 PawnData。
  - `Source/LyraGame/Player/LyraPlayerState.cpp:167-182`：ASC `InitAbilityActorInfo(this, GetPawn())`，且仅非客户端注册 Experience 回调。
  - `Source/LyraGame/Player/LyraPlayerState.cpp:185-213`：PawnData 仅权威端一次性设置，授予 AbilitySets，并发送 `NAME_LyraAbilityReady`。
  - `Source/LyraGame/Character/LyraHeroComponent.cpp:145-181`：DataAvailable→DataInitialized 时调用 PawnExtension 初始化 ASC、初始化本地输入、绑定相机模式委托。
  - `Source/LyraGame/Character/LyraPawnExtensionComponent.cpp:105-150`：处理旧 Avatar 驱逐，调用 `InitAbilityActorInfo(InOwnerActor, Pawn)` 并安装 TagRelationshipMapping。
- **调用链**：`Experience loaded → ALyraPlayerState::OnExperienceLoaded → SetPawnData → AbilitySet::GiveToAbilitySystem`；随后 `PawnExtension/Hero init-state convergence → HeroComponent::HandleChangeInitState → PawnExtension::InitializeAbilitySystem → LyraASC::InitAbilityActorInfo`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。
- **设计含义**：Owner 与 Avatar 分离是理解 Lyra GAS 的第一原则；把 ASC 误认为 Character 的组件，会误判死亡、重生和复制生命周期。

### 发现 2：AbilitySet 是只允许权威端执行的批量授予事务

- **内容**：AbilitySet 按“AttributeSet → AbilitySpec → GameplayEffect”顺序授予。AbilitySpec 的动态 SourceTag 存放 InputTag，SourceObject 可指向装备实例；可选的 `FLyraAbilitySet_GrantedHandles` 记录所有句柄，供装备卸下或 GameFeature 移除时成组回收。
- **关键符号**：`ULyraAbilitySet::GiveToAbilitySystem`、`FLyraAbilitySet_GrantedHandles::TakeFromAbilitySystem`。
- **代码证据**：`Source/LyraGame/AbilitySystem/LyraAbilitySet.cpp:32-66,73-145`。权威检查在 77-81；AttributeSet 在 83-100；AbilitySpec/InputTag/SourceObject 在 103-125；初始 GE 在 128-145。
- **引擎落点**：`D:/Software/UE_5.8/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp:292` 的 `UAbilitySystemComponent::GiveAbility` 是底层入口。
- **调用链**：`PawnData.AbilitySets` 或 `EquipmentDefinition.AbilitySetsToGrant` 或 GameFeature AddAbilities → GiveToAbilitySystem → AddAttributeSetSubobject/GiveAbility/ApplyGameplayEffectToSelf`。
- **来源**：本地 Lyra + UE GAS 源码（T0）。
- **置信度**：H。
- **边界**：PlayerState 的 PawnData AbilitySets 传入空 handles，因此是长期授予；Equipment 传入 handles，支持对称撤销。

### 发现 3：LyraASC 在 Avatar 切换后补做 Lyra 特有初始化

- **内容**：Lyra 覆盖 `InitAbilityActorInfo`，检测新 Pawn Avatar 后依次通知已实例化 Ability 的 `OnPawnAvatarSet`、注册 GlobalAbilitySystem、把 ASC 注入 AnimInstance、尝试激活 OnSpawn 能力。回放中 Ability 实例可能缺失，源码显式容忍。
- **代码证据**：`Source/LyraGame/AbilitySystem/LyraAbilitySystemComponent.cpp:39-82`；UE 基类入口为 `.../GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp:161`。
- **调用链**：`PawnExtension::InitializeAbilitySystem → LyraASC::InitAbilityActorInfo → Super → abilities.OnPawnAvatarSet → GlobalAbilitySystem.RegisterASC → AnimInstance.InitializeWithAbilitySystem → TryActivateAbilitiesOnSpawn`。
- **来源**：本地 Lyra + UE GAS 源码（T0）。
- **置信度**：H。

### 发现 4：TagRelationshipMapping 是数据驱动的“标签关系扩展器”

- **内容**：能力自身声明的 Block/Cancel/Required/Blocked 标签仍由 GAS 处理；Lyra 的 Mapping 根据 AbilityTags 追加关系。它并不代替 GAS 的标签校验，而是在调用 Super 前扩展集合。
- **关键符号**：`ULyraAbilityTagRelationshipMapping::GetAbilityTagsToBlockAndCancel`、`GetRequiredAndBlockedActivationTags`、`ULyraAbilitySystemComponent::ApplyAbilityBlockAndCancelTags`、`GetAdditionalActivationTagRequirements`。
- **代码证据**：`Source/LyraGame/AbilitySystem/LyraAbilityTagRelationshipMapping.cpp:8-46`；`Source/LyraGame/AbilitySystem/LyraAbilitySystemComponent.cpp:356-389`；Mapping 来源于 `PawnData`，安装于 `LyraPawnExtensionComponent.cpp:144-147`。
- **调用链**：`ability tags → LyraASC override → mapping append → GAS Super block/cancel or activation requirements`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。

### 发现 5：GlobalAbilitySystem 是 World 范围的“当前及未来 ASC”广播器

- **内容**：Subsystem 保存已注册 ASC；全局 Ability/Effect 既应用到当前全部 ASC，也记录在 map 中，使之后注册的 ASC 自动收到。移除时按 ASC 保存的句柄精确清理；ASC EndPlay 注销。
- **代码证据**：`Source/LyraGame/AbilitySystem/LyraGlobalAbilitySystem.cpp:9-76,82-146`；注册触发见 `LyraAbilitySystemComponent.cpp:70-74`，注销见同文件 `29-37`。
- **调用链**：`ApplyAbilityToAll/ApplyEffectToAll → map entry → each registered ASC`；`new Avatar → RegisterASC → replay every global list onto ASC`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。
- **设计取舍**：它不是静态单例，而是 WorldSubsystem，可自然隔离 PIE World/服务器 World。

### 发现 6：AdditionalCost 与 TargetData 通过服务器缓存耦合，实现“命中才扣费”

- **内容**：Lyra Ability 先执行 GAS 原生成本，再遍历 AdditionalCosts。标记 `bOnlyApplyCostOnHit` 的成本只在权威端从 ASC 的 replicated target-data cache 找到 HitResult 后执行。库存成本最终在服务器消费 Inventory。
- **代码证据**：`Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility.cpp:202-275`；缓存读取 `Source/LyraGame/AbilitySystem/LyraAbilitySystemComponent.cpp:520-527`；库存成本 `Source/LyraGame/AbilitySystem/Abilities/LyraAbilityCost_InventoryItem.cpp:14-50`；UE Commit 顺序入口 `.../GameplayAbilities/Private/Abilities/GameplayAbility.cpp:592-650,1115-1150`。
- **调用链**：`CommitAbility → GAS CheckCost + Lyra AdditionalCost::CheckCost → GAS ApplyCost → Lyra ApplyCost → server target-data cache hit test → AdditionalCost::ApplyCost → inventory consume`。
- **来源**：本地 Lyra + UE GAS 源码（T0）。
- **置信度**：H。
- **易错点**：客户端不是最终“命中扣费”判定者；若 target data 未及时进入服务器缓存，则 only-on-hit cost 不会应用。

### 发现 7：Lyra EffectContext 把武器/来源策略带入伤害执行，并兼容 Iris

- **内容**：`ULyraAbilitySystemGlobals::AllocGameplayEffectContext` 保证 GAS 创建 Lyra 派生 Context。Ability 的 `MakeEffectContext` 写入 `ILyraAbilitySourceInterface`、SourceLevel、Instigator、EffectCauser 和 SourceObject。Context 通过弱对象指针保存 AbilitySource；NetSerialize 先序列化基类，再序列化 AbilitySource 与 SourceLevel；UE 5.8 Iris 注册将其转发给基类 `FGameplayEffectContextNetSerializer`，注释明确若自定义序列化布局变化必须实现定制 Iris serializer。
- **代码证据**：`Source/LyraGame/AbilitySystem/LyraAbilitySystemGlobals.cpp:16-19`；`LyraGameplayAbility.cpp:278-300`；`LyraGameplayEffectContext.h:24-80`；`LyraGameplayEffectContext.cpp:16-57`；UE 分配入口 `.../GameplayAbilities/Private/AbilitySystemComponent.cpp:544-550` 与 `.../Private/Abilities/GameplayAbility.cpp:1907-1926`。
- **调用链**：`UGameplayAbility::MakeEffectContext → UAbilitySystemGlobals::AllocGameplayEffectContext (Lyra subclass) → LyraGameplayAbility fills source → GameplayEffectSpec → DamageExecution extracts typed context`。
- **来源**：本地 Lyra + UE GAS 源码（T0）。
- **置信度**：H。

### 发现 8：自定义 SingleTargetHit TargetData 增加 CartridgeID，并沿 GAS 预测 RPC 复制

- **内容**：Lyra TargetData 继承 GAS 单目标命中，额外序列化 `CartridgeID`；`AddTargetDataToContext` 把 cartridge id 写进 Lyra EffectContext。武器客户端本地 trace，创建一批 TargetData，共用随机 CartridgeID，经预测键调用服务器 target-data RPC；服务器的 GAS 缓存并触发 delegate。
- **代码证据**：`Source/LyraGame/AbilitySystem/LyraGameplayAbilityTargetData_SingleTargetHit.h:18-43`、`.cpp:13-29`；`Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.cpp:477-554,565-607`；UE RPC 接收与发送 `.../GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp:4007-4048,4279-4310`；预测窗口 `.../GameplayAbilities/Private/GameplayPrediction.cpp:387-435`。
- **调用链**：`StartRangedWeaponTargeting → PerformLocalTargeting → TargetData(CartridgeID) → OnTargetDataReadyCallback → FScopedPredictionWindow → CallServerSetReplicatedTargetData → ServerSetReplicatedTargetData_Implementation → AbilityTargetDataSetDelegate → CommitAbility/Blueprint effect application`。
- **来源**：本地 Lyra + UE GAS 源码（T0）。
- **置信度**：H（复制链）；L（反作弊有效性）。
- **关键疑点**：`ValidateTargetDataOnServer` 在 `LyraGameplayAbility_RangedWeapon.cpp:557-563` 无条件返回 true；当前源码没有服务端 trace 重放、射速/视角/距离校验。面试时必须称其为扩展钩子，不可称为已完成验证。

## 维度 2：Health、CombatSet、执行计算与死亡

### 发现 9：CombatSet 是攻击侧输入，HealthSet 的 Damage/Healing 是瞬时元属性

- **内容**：`ULyraCombatSet` 复制 `BaseDamage`/`BaseHeal`；DamageExecution 捕获 BaseDamage。HealthSet 接收 Damage/Healing 元属性，在 `PostGameplayEffectExecute` 转成 Health 改变后立刻清零，因此 Damage 不是长期状态。
- **代码证据**：`Source/LyraGame/AbilitySystem/Attributes/LyraCombatSet.h:19-52`、`.cpp:16-32`；`Source/LyraGame/AbilitySystem/Executions/LyraDamageExecution.cpp:14-34`；`Source/LyraGame/AbilitySystem/Attributes/LyraHealthSet.cpp:108-182`。
- **调用链**：`source CombatSet.BaseDamage capture → DamageExecution output modifier to target HealthSet.Damage → HealthSet::PostGameplayEffectExecute → Health -= Damage; Damage=0`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。

### 发现 10：最终伤害是服务器专属的多因子计算

- **内容**：DamageExecution 仅在 `WITH_SERVER_CODE` 下计算。它取 BaseDamage、命中角色与位置、队伍可伤害性、距离、物理材质衰减和 AbilitySource 距离衰减，最终输出非负 Damage 元属性。
- **公式**：`max(BaseDamage × DistanceAttenuation × PhysicalMaterialAttenuation × TeamAllowedMultiplier, 0)`。
- **代码证据**：`Source/LyraGame/AbilitySystem/Executions/LyraDamageExecution.cpp:36-138`，特别是团队判定 89-98、距离 100-114、衰减 116-128、最终公式 130-137。
- **调用链**：`GameplayEffect custom execution → typed EffectContext → team/distance/material policies → output HealthSet.Damage`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。

### 发现 11：HealthSet 在数值层统一处理免疫、作弊、消息与越界

- **内容**：PreExecute 对 DamageImmunity、GodMode、自毁例外做门控并保存旧值；PostExecute 广播标准 Damage verb message，将 Damage/Healing 折算为 Health，执行 clamp，广播 HealthChanged/OutOfHealth。UnlimitedHealth 将最低生命限定为 1，自毁绕过。
- **代码证据**：`Source/LyraGame/AbilitySystem/Attributes/LyraHealthSet.cpp:68-105,108-182,185-232`。
- **调用链**：`GE modifier → PreGameplayEffectExecute guard → modifier evaluation → PostGameplayEffectExecute → gameplay message + Health mutation → delegates`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。
- **设计取舍**：数值与死亡状态分离；HealthSet 不直接销毁 Pawn，只发 OutOfHealth。

### 发现 12：死亡由 OutOfHealth 事件触发 Ability，再由复制状态机表现

- **内容**：HealthComponent 订阅 HealthSet。权威端 OutOfHealth 构造 `GameplayEvent.Death`，在预测窗口中喂给 ASC，由 Death Ability 响应；同时发送 Elimination message。Death Ability 负责 `StartDeath`/`FinishDeath`，HealthComponent 将死亡阶段复制。客户端 OnRep 按顺序补跑缺失阶段，并拒绝从已预测更晚状态回退。
- **代码证据**：
  - 绑定：`Source/LyraGame/Character/LyraHealthComponent.cpp:52-90`。
  - 事件/消息：同文件 `148-187`。
  - OnRep 单调状态机：同文件 `190-233`。
  - 状态与死亡标签：同文件 `235-275`。
  - Ability：`Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility_Death.cpp:27-89`。
  - Character 回调最终关闭碰撞/销毁：`Source/LyraGame/Character/LyraCharacter.cpp:382-443`。
- **调用链**：`HealthSet.OnOutOfHealth → HealthComponent::HandleOutOfHealth(server) → ASC.HandleGameplayEvent(GameplayEvent.Death) → Death Ability Activate → HealthComponent.StartDeath → replicated DeathState → clients OnRep → Death Ability end → FinishDeath → Character death-finished teardown`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。
- **易错点**：血量到 0 不等同于立即 Destroy；死亡是可插入动画/交互的两阶段协议。

## 维度 3：Character、PawnExtension、Hero、Input、Camera、Animation

### 发现 13：Pawn 初始化不是 BeginPlay 一次性流程，而是跨复制依赖的标签状态机

- **内容**：PawnExtension 与 HeroComponent 都实现 Modular Gameplay init-state。共同链为 Spawned → DataAvailable → DataInitialized → GameplayReady。PawnExtension 要求 PawnData，并对权威/本地 Pawn 要求 Controller；进入 DataInitialized 前要求所有 feature 到 DataAvailable。Hero 还要求 Controller/PlayerState 配对，本地真人要求 InputComponent/LocalPlayer，并等待 PawnExtension 到 DataInitialized。
- **代码证据**：`Source/LyraGame/Character/LyraPawnExtensionComponent.cpp:213-289`；`Source/LyraGame/Character/LyraHeroComponent.cpp:77-143,186-215`；UE 状态链算法 `D:/Software/UE_5.8/Engine/Plugins/Runtime/ModularGameplay/Source/ModularGameplay/Private/Components/GameFrameworkInitStateInterface.cpp:101-145`；全 feature 检查 `.../GameFrameworkComponentManager.cpp:763-792`。
- **调用链**：`BeginPlay/OnRep_PawnData/OnRep_PlayerState/SetupPlayerInput/ControllerChanged → CheckDefaultInitialization → ContinueInitStateChain → CanChangeInitState → HandleChangeInitState`。
- **来源**：本地 Lyra + UE ModularGameplay 源码（T0）。
- **置信度**：H。
- **设计动机**：网络复制顺序不固定；状态机把“数据是否到齐”显式化，避免在某个生命周期回调里赌依赖顺序。

### 发现 14：Character 是适配层，PawnExtension 是编排中枢，Hero 是玩家控制特性

- **内容**：Character 构造组件、转发生命周期与 ASC 接口，ASC 初始化后再初始化 Health 和 tags。PawnExtension 管 PawnData、ASC Owner/Avatar 配对和 feature barrier。Hero 只负责玩家相关输入、相机选择和控制动作。因此非 Hero Pawn 可复用 Character/PawnExtension 而不需要本地玩家逻辑。
- **代码证据**：`Source/LyraGame/Character/LyraCharacter.cpp:40-80,182-210,212-260`；`LyraPawnExtensionComponent.cpp:76-221`；`LyraHeroComponent.cpp:145-302`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。

### 发现 15：输入链以 GameplayTag 解耦 InputAction 与 AbilitySpec

- **内容**：Hero 清空并安装 Enhanced Input mapping contexts；LyraInputComponent 从 InputConfig 将 Ability InputAction 绑定为 tag 回调。AbilitySet 已把同一个 InputTag 写入 AbilitySpec 动态 source tags。ASC 收到 tag 后扫描 spec、缓存 pressed/held/released handle；PlayerController 每 tick 调用 `ProcessAbilityInput`，按 ActivationPolicy 统一激活，避免 press+held 同帧重复触发。
- **代码证据**：
  - Mapping/binding：`Source/LyraGame/Character/LyraHeroComponent.cpp:225-302`。
  - tag 转发：同文件 `343-370`。
  - spec tag：`Source/LyraGame/AbilitySystem/LyraAbilitySet.cpp:114-120`。
  - 缓存与激活：`Source/LyraGame/AbilitySystem/LyraAbilitySystemComponent.cpp:150-310`。
  - tick 驱动：`Source/LyraGame/Player/LyraPlayerController.cpp:371-381`。
  - UE 映射底层：`D:/Software/UE_5.8/Engine/Plugins/EnhancedInput/Source/EnhancedInput/Private/EnhancedInputSubsystemInterface.cpp:251-283`，LocalPlayer 转发见 `EnhancedInputSubsystems.cpp:140-143`。
- **调用链**：`physical key → EnhancedInput mapping → InputAction trigger → LyraInputComponent tag callback → ASC AbilityInputTagPressed/Released → cached spec handles → PlayerController tick ProcessAbilityInput → TryActivateAbility`。
- **来源**：本地 Lyra + UE EnhancedInput 源码（T0）。
- **置信度**：H。

### 发现 16：CameraMode 是按帧求值的栈，Ability 可临时覆盖 Hero 默认模式

- **内容**：Hero 在初始化时把 `DetermineCameraMode` 委托交给 CameraComponent；CameraComponent 每帧询问并 Push 到 CameraModeStack。Hero 优先返回由 Ability 设置的 override，其次返回 PawnData 默认模式；Ability End/Clear 时释放覆盖。栈负责 blend，而不是切换 CameraActor。
- **代码证据**：`Source/LyraGame/Character/LyraHeroComponent.cpp:175-181,471-493`；`Source/LyraGame/Camera/LyraCameraComponent.cpp:20-99`；`Source/LyraGame/Camera/LyraCameraMode.cpp:313-489`；Ability 设置/清理 `Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility.cpp:520-542`。
- **调用链**：`CameraComponent::GetCameraView → UpdateCameraModes → Hero.DetermineCameraMode → CameraModeStack.PushCameraMode → EvaluateStack/Blend → desired view`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。

### 发现 17：动画层从 ASC 标签和压缩移动数据读取状态

- **内容**：LyraAnimInstance 用 GameplayTagBlueprintPropertyMap 将 ASC tag 映射成 AnimBP 属性，初始化既可由 `NativeInitializeAnimation` 主动查找 ASC，也可在 Avatar 切换时由 LyraASC 注入。每帧从 LyraCharacterMovementComponent 取 GroundInfo。Character 只向 simulated proxies 复制压缩 acceleration，AnimBP 可据此还原表现。
- **代码证据**：`Source/LyraGame/Animation/LyraAnimInstance.cpp:20-64`；ASC 注入 `LyraAbilitySystemComponent.cpp:76-79`；压缩加速度 `Source/LyraGame/Character/LyraCharacter.cpp:129-152,281-299`；GroundInfo `Source/LyraGame/Character/LyraCharacterMovementComponent.cpp:164-205`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。

## 维度 4：Equipment、Inventory、QuickBar、Weapon、Interaction

### 发现 18：Inventory 使用 FastArray 复制条目，ItemInstance 作为复制子对象

- **内容**：服务器创建 ItemInstance，运行 definition fragments 的 `OnInstanceCreated`，条目 `MarkItemDirty`。FastArray 的增删改回调在客户端计算 delta 并发 GameplayMessage；ItemInstance 通过 Registered SubObject List 或旧式 `ReplicateSubobjects` 复制。
- **代码证据**：`Source/LyraGame/Inventory/LyraInventoryManagerComponent.cpp:37-125,145-197,267-300`；FastArray 声明见 `.h:29-91`。
- **调用链**：`server AddItemDefinition → NewObject ItemInstance → fragments initialize → MarkItemDirty → FastArray replication → PostReplicatedAdd/Change/PreRemove → Inventory.StackChanged message → UI/listeners`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。
- **疑点**：`AddItemInstance` 调用的 `FLyraInventoryList::AddEntry(Instance)` 在 `.cpp:110-113` 为 `unimplemented()`；`GetTotalItemCountByDefinition` 在 222-238 对每个 entry 只加 1，没有累加 StackCount；当前类更接近“每实例一条”示例骨架。

### 发现 19：Equipment 是 Inventory 的运行时投影，并绑定能力生命周期

- **内容**：Inventory Item 通过 Equippable fragment 指向 EquipmentDefinition。服务器装备时创建 EquipmentInstance，把它作为 AbilitySet SourceObject，授予装备能力并生成附属 Actor；卸装反向撤销 handles 和附属 Actor。EquipmentList 同样用 FastArray，实例也作为 subobject 复制。
- **代码证据**：`Source/LyraGame/Inventory/InventoryFragment_EquippableItem.h:17-25`；`Source/LyraGame/Equipment/LyraEquipmentManagerComponent.cpp:68-128,133-195,221-238`；实例 actor 生命周期 `Source/LyraGame/Equipment/LyraEquipmentInstance.cpp:58-115`。
- **调用链**：`InventoryItem fragment → QuickBar active slot → EquipmentManager.EquipItem → EquipmentList.AddEntry → AbilitySet.GiveToASC(source=EquipmentInstance) + SpawnEquipmentActors → OnEquipped`；卸装严格逆序。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。

### 发现 20：QuickBar 是 Controller 上的权威选择器，消息只是观察层

- **内容**：Slots 和 ActiveSlotIndex 都复制。`SetActiveSlotIndex` 是 Server RPC；服务器先卸旧装备、改 index、装新装备，并手动调用 OnRep 向本机消息订阅者通知。客户端复制时 OnRep 广播 SlotsChanged/ActiveIndexChanged。QuickBar 自身不保存装备定义，只引用 InventoryItemInstance。
- **代码证据**：`Source/LyraGame/Equipment/LyraQuickBarComponent.h:25-79`；`.cpp:28-44,86-147,169-223`。
- **调用链**：`local input/UI → Server SetActiveSlotIndex RPC → Unequip old → set replicated index → Equip new → local OnRep broadcast → remote OnRep broadcast`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。
- **权威边界疑点**：`AddItemToSlot`/`RemoveItemFromSlot` 自身没有显式 authority check 或 RPC；调用者必须保证只在服务器执行。

### 发现 21：武器采用客户端本地瞄准 + GAS 预测传输 + 服务器效果提交

- **内容**：本地控制方 trace 并立即建立命中标记候选；TargetData 携带 UniqueId/CartridgeID。非权威本地端把数据用 activation prediction key 发给服务器。服务器确认/替换命中标记，再 CommitAbility，成功才加散布并交给蓝图应用效果。消费 replicated target data 避免缓存滞留。
- **代码证据**：`Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.cpp:352-438,440-554,565-607`；`Source/LyraGame/Weapons/LyraWeaponStateComponent.cpp:44-144`。
- **调用链**：`input activates weapon ability → StartRangedWeaponTargeting → local traces → unconfirmed marker batch → target-data RPC → server delegate → validation hook → client confirmation RPC → CommitAbility → effect application → damage message later confirms feedback`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H（数据流），L（安全性，验证钩子为空）。

### 发现 22：Interaction 把“发现候选”和“执行能力”拆开

- **内容**：权威端的 GrantNearbyInteraction task 定时球形 overlap，为附近目标声明的 interaction ability 动态 `GiveAbility` 并缓存句柄；WaitForInteractableTargets 在视线 trace 后让接口填充 options，并用 `CanActivateAbility` 过滤。Interact Ability 把首选 option 封装为 GameplayEvent，针对目标 ASC 的指定 spec 调 `TriggerAbilityFromGameplayEvent`。持续时间通过 GameplayMessage 发给 UI。
- **代码证据**：`Source/LyraGame/Interaction/Abilities/LyraGameplayAbility_Interact.cpp:21-121`；`Tasks/AbilityTask_GrantNearbyInteraction.cpp:23-97`；`Tasks/AbilityTask_WaitForInteractableTargets.cpp:19-176`；接口契约 `IInteractableTarget.h:18-84`；选项 `InteractionOption.h:20-65`。
- **调用链**：`server proximity scan → target.GatherInteractionOptions → optionally GiveAbility → local line trace/options → UI indicators → TriggerInteraction → target ASC TriggerAbilityFromGameplayEvent`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。
- **设计含义**：交互执行发生在目标 ASC，而不是强制发生在交互者 ASC；门、拾取物等可拥有自己的能力逻辑。

### 消息协作总表

| 生产者 | 消息 | 消费语义 | 证据 |
|---|---|---|---|
| HealthSet | Damage verb | 战斗日志、命中反馈等旁路观察 | `LyraHealthSet.cpp:128-145` |
| HealthComponent | Elimination verb | 计分/播报/模式规则 | `LyraHealthComponent.cpp:169-181` |
| Inventory FastArray | StackChanged | UI 增量刷新 | `LyraInventoryManagerComponent.cpp:37-77` |
| QuickBar OnRep | SlotsChanged / ActiveIndexChanged | HUD 与武器 UI | `LyraQuickBarComponent.cpp:205-223` |
| Interact Ability | Interaction duration | 进度 UI | `LyraGameplayAbility_Interact.cpp:41-76` |

结论：GameplayMessage 是跨系统观察总线，不负责权威状态；权威状态仍由 ASC、FastArray、复制属性和 RPC 承担。置信度 H。

## 维度 5：Feedback、Physics、Replay、Performance、Tests

### 发现 23：Context Effects 用 GameplayTag + PhysicalMaterial surface 解耦动画事件与声画资产

- **内容**：AnimNotify/接口产生 MotionEffect tag；ContextEffectComponent 收集当前 contexts，并从 hit 的 PhysicalMaterial 推导 surface context；WorldSubsystem 查询已加载 library，生成 sound/Niagara。Library 支持异步加载和按 Actor 引用管理。
- **代码证据**：`Source/LyraGame/Feedback/ContextEffects/AnimNotify_LyraContextEffects.cpp:45-199`；`LyraContextEffectComponent.cpp:28-180`；`LyraContextEffectsSubsystem.cpp:20-155`；`Source/LyraGame/Physics/PhysicalMaterialWithTags.h:13-24`。
- **调用链**：`animation notify → AnimMotionEffect interface/component → line trace physical surface + effect contexts → ContextEffectsSubsystem → library lookup → SpawnSoundAttached/Niagara`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。
- **与战斗链关联**：同一 PhysicalMaterial 还会把 tags 注入 GameplayEffectSpec（`LyraGameplayAbility.cpp:303-313`），并参与 DamageExecution 的材质衰减。

### 发现 24：NumberPop 与 WeaponState 是消息驱动的客户端反馈端点

- **内容**：HealthSet 发 Damage message；本地 PlayerController 的 NumberPop 组件可据此生成伤害数字。WeaponState 维护未确认命中批次，以服务器 Client RPC 修正成功/替换命中并更新时间。它们不改变伤害权威结果。
- **代码证据**：`Source/LyraGame/Feedback/NumberPops/LyraNumberPopComponent.h:17-48` 及具体 Mesh/Niagara 实现；`Source/LyraGame/Weapons/LyraWeaponStateComponent.cpp:44-144`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：M（部分订阅关系在资产/蓝图中配置）。

### 发现 25：Replay 明确受平台 trait 门控，并容忍能力实例缺失

- **内容**：ReplaySubsystem 先检查 `Platform.Trait.ReplaySupport`，再调用 GameInstance 的 StartRecordingReplay/PlayReplay；支持枚举后串行清理旧 replay 和 seek。ASC 在通知新 Avatar 时注明 replay 可能没有 Ability 实例，因此跳过空实例。
- **代码证据**：`Source/LyraGame/Replays/LyraReplaySubsystem.cpp:18-180`；`Source/LyraGame/AbilitySystem/LyraAbilitySystemComponent.cpp:51-67`。
- **调用链**：`platform trait → record/play → DemoNetDriver playback`；玩法对象通过普通复制路径被记录，Lyra 对回放的实例化差异做防御。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。

### 发现 26：Performance subsystem 同时采集帧、网络和输入延迟

- **内容**：每帧缓存 ClientFPS、线程/GPU frame time；从 GameState 取 ServerFPS，从 PlayerState/NetConnection 取 ping、packet loss/rate/size；可选接入 LatencyMarker，并向 CSV profiler 写自定义统计。它是 LocalPlayer subsystem，面向 HUD 与本地诊断。
- **代码证据**：`Source/LyraGame/Performance/LyraPerformanceStatSubsystem.cpp:17-177`，网络统计 48-73，延迟/CSV 76-113。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。

### 发现 27：测试体系把单机启动、HTTP 驱动和多人 PIE 自动化分层

- **内容**：Boot test 等待启动条件；GameplayRpcRegistrationComponent 暴露测试 HTTP 入口以执行 cheat、fire once、读取 vitals；ShooterTests 的网络测试把步骤显式分成 Then/Until Server 和 Client，并验证输入造成的动画能在所有客户端观察到。
- **代码证据**：`Source/LyraGame/Tests/LyraTestControllerBootTest.cpp:8-27`；`Source/LyraGame/Tests/LyraGameplayRpcRegistrationComponent.cpp:22-226`；`Plugins/GameFeatures/ShooterTests/Source/ShooterTestsRuntime/Private/ShooterTestsActorNetworkTests.cpp:14-114` 及 `Utilities/ShooterTestsNetworkComponent.h`。
- **调用链**：`automation controller / HTTP → local PC/pawn command → gameplay network path → server/client assertions`。
- **来源**：本地 Lyra 源码（T0）。
- **置信度**：H。
- **局限**：这些主要是 PIE/功能自动化证据，不能等同于 packaged dedicated server 或 shipping 网络证据。

## 跨维度主调用链

### 启动到可操作角色

`Experience loaded → PlayerState chooses PawnData → grants persistent AbilitySets → GameMode assigns PawnData to spawned Pawn → PawnExtension/Hero init-state barrier → ASC Owner=PlayerState, Avatar=Pawn → Health/Anim bind ASC → Enhanced Input contexts/actions bind → camera delegate bind → GameplayReady`

### 开火到伤害与死亡

`InputAction → InputTag → ASC cached handles → ProcessAbilityInput → ranged ability → local trace/TargetData → prediction-key RPC → server target-data delegate → CommitAbility/cost → effect spec with LyraEffectContext + physical tags → DamageExecution → HealthSet Damage→Health → Damage message → OutOfHealth → GameplayEvent.Death → Death ability → replicated two-stage death → Character teardown`

### 拾取到装备

`server pickup → Inventory AddEntry/FastArray → inventory message → QuickBar slot references item → server active-slot RPC → Equippable fragment → EquipmentDefinition → EquipmentInstance + equipment actors + temporary AbilitySets → Equipment FastArray/subobject replication → QuickBar messages/UI`

## 目录到学习主题的覆盖建议

| 原始路径 | 建议文档主题 | 划分粒度 |
|---|---|---|
| `Source/LyraGame/AbilitySystem/` | GAS 总览：ASC 生命周期、输入、激活组、标签关系、全局能力 | `LyraAbilitySystemComponent` 单篇；AbilitySet/Global/Mapping 可合篇 |
| `Source/LyraGame/AbilitySystem/Abilities/` | Lyra Ability 基类、激活策略、成本、相机覆盖、死亡/重置 | 基类单篇；Cost 合篇；Death 单篇 |
| `Source/LyraGame/AbilitySystem/Attributes/` + `Executions/` | 属性与执行计算：Combat→Damage→Health | 合成一条端到端主题，HealthSet 单独深挖 |
| `Source/LyraGame/Character/` | 模块化 Pawn 初始化与 ASC Owner/Avatar 模式 | PawnExtension+Hero 单篇主链；Character/Movement/Health 各专题 |
| `Source/LyraGame/Input/` | InputConfig、tag binding、用户映射与 modifiers | 目录合篇，配 Hero/ASC 交叉引用 |
| `Source/LyraGame/Camera/` | CameraMode stack、blend、第三人称穿透规避 | 目录总览 + CameraMode/Stack 关键代码篇 |
| `Source/LyraGame/Animation/` | ASC tag-property map 与运动数据 | 目录合篇；ShooterCore 动画资产逻辑另篇 |
| `Source/LyraGame/Inventory/` | Definition/Fragment/Instance/FastArray | 一篇主链，Manager 单独列缺口 |
| `Source/LyraGame/Equipment/` | 装备投影、能力生命周期、QuickBar | EquipmentManager 与 QuickBar 分两篇 |
| `Source/LyraGame/Weapons/` | 本地瞄准、TargetData 预测、命中确认、散布 | RangedWeapon Ability 单篇主链；WeaponInstance/State 合篇 |
| `Source/LyraGame/Interaction/` | 交互发现、动态授予、目标 ASC 执行 | Abilities+Tasks 合成端到端一篇 |
| `Source/LyraGame/Feedback/ContextEffects/` | 标签化环境反馈与 PhysicalMaterial | 目录合篇 |
| `Source/LyraGame/Feedback/NumberPops/` | 伤害数字表现策略 | 目录合篇 |
| `Source/LyraGame/Physics/` | Collision channel 与材质 tags | 短篇，并交叉引用 Weapon/Feedback/Damage |
| `Source/LyraGame/Replays/` | Replay 门控、查询、回放兼容性 | 目录合篇 |
| `Source/LyraGame/Performance/` | 本地性能/网络/延迟采样 | 目录合篇 |
| `Source/LyraGame/Tests/` | 测试控制器与 HTTP 驱动 | 目录合篇 |
| `Plugins/GameFeatures/ShooterTests/` | 多人 PIE 输入、动画、网络验证 | 插件单篇，明确证据边界 |
| UE `GameplayAbilities` 对应文件 | GAS 底层补充：ActorInfo、GiveAbility、Commit、TargetData RPC、PredictionKey | 不单独覆盖整个 Engine；作为 Lyra 文档中的“引擎下潜”框 |
| UE `EnhancedInput` / `ModularGameplay` | MappingContext 与 init-state 算法 | 同上，按调用链嵌入 |

## 疑点与下一轮定向核查

1. **服务器武器验证为空**（高优先级）：`ValidateTargetDataOnServer()` 永真。下一轮应追 ShooterCore 蓝图/GE 是否补了约束，并明确“示例设计”与生产反作弊差距。
2. **Inventory 示例未完成**（高优先级）：`AddEntry(instance)` 为 `unimplemented()`；总数函数忽略 StackCount；`CanAddItemDefinition` 恒 true。文档应按当前事实讲，不推断完整背包能力。
3. **QuickBar 调用者权威契约**（中优先级）：添加/移除 slot 没有内部 authority guard；需要反查所有调用点，确认是否始终由服务器能力/拾取逻辑调用。
4. **NumberPop 订阅闭环**（中优先级）：C++ 仅显示表现组件接口；具体 Damage message 监听可能在蓝图或 GameFeature action，因 Content 排除而只能标为资产依赖。
5. **Interaction 网络执行策略**（中优先级）：C++ 表明授予发生在服务器、目标 ability 通过事件触发；具体 ability NetExecutionPolicy 由资产类决定，需要在“只读源码”边界下注明不能统一断言预测策略。
6. **Replay 与 GAS prediction 的实际行为**（低优先级）：源码只有容错注释，尚未用 replay 自动化测试验证 TargetData/Ability 实例时序。
7. **Iris 与 legacy ReplicateSubobjects 双路径**（中优先级）：Inventory/Equipment 同时保留 RegisteredSubObjectList 和 ActorChannel fallback；需在 Engine ReplicationSystem 中核查项目实际默认路径。

## 矛盾报告

- **表面矛盾：武器“服务器验证”命名 vs 实际实现**。接口及日志称 server validation，但当前实现永真。判定：代码行为优先，文档必须写成“预留钩子”。
- **表面矛盾：Inventory 有 StackCount vs Count API 按 entry 计数**。条目确有 StackCount 且消息传播 delta，但 `GetTotalItemCountByDefinition` 没有累加它。判定：当前实现不完整，不能用 API 名推断行为。
- **并存设计：Registered subobject list vs ReplicateSubobjects**。不是互相排斥的事实冲突，而是 UE 迁移兼容双路径；运行时由 `IsUsingRegisteredSubObjectList()` 决定。

## 面试追问链预判（5 题）

| # | 追问问题 | 答题方向 |
|---|---|---|
| 1 | 为什么 Lyra 把 ASC 放在 PlayerState，而不是 Character？换 Pawn 时有哪些清理动作仍必须做？ | Owner/Avatar 生命周期；`PawnExtension::UninitializeAbilitySystem` 取消非 SurvivesDeath 能力、清输入、清 cue、解绑 Avatar；长期 ability/attribute 仍在 PlayerState。 |
| 2 | InputTag 如何从键盘输入一路找到具体 AbilitySpec？为什么 Lyra 不直接在输入回调里激活？ | InputConfig 映射、AbilitySet 动态 source tags、pressed/held/released 缓存、tick 集中处理；避免 press+held 双触发并支持激活策略。 |
| 3 | 客户端上报 HitResult 时服务器凭什么信任？PredictionKey 能防作弊吗？ | PredictionKey 解决预测关联/确认，不验证空间真实性；当前 `ValidateTargetDataOnServer` 永真，生产需重放 trace、射速/弹药/视角/延迟容差验证。 |
| 4 | 为什么伤害先写 Damage 元属性，再转成 Health，而不直接修改 Health？ | 集中免疫/作弊/消息/统计/Clamp；Execution 只产出结果，HealthSet 统一落实；Damage 清零避免持久状态污染。 |
| 5 | FastArray 已复制 Inventory/Equipment 条目，为什么还要复制 ItemInstance/EquipmentInstance 子对象？ | FastArray 复制“有哪些条目及结构字段”；UObject 实例自身的 replicated properties/RPC 需要 subobject replication；两层解决集合与对象状态的不同问题。 |

## 来源清单

| # | 来源 | 层级 | 访问日期 | 用途 |
|---|---|---|---|---|
| 1 | `Source/LyraGame/AbilitySystem/**` | T0 | 2026-08-13 | Lyra GAS、属性、执行、Context、TargetData |
| 2 | `Source/LyraGame/Character/**`、`Player/**`、`Input/**`、`Camera/**`、`Animation/**` | T0 | 2026-08-13 | Pawn/玩家初始化与数据流 |
| 3 | `Source/LyraGame/Inventory/**`、`Equipment/**`、`Weapons/**`、`Interaction/**` | T0 | 2026-08-13 | 权威、复制、预测、消息协作 |
| 4 | `Source/LyraGame/Feedback/**`、`Physics/**`、`Replays/**`、`Performance/**`、`Tests/**` | T0 | 2026-08-13 | 玩法外围设计 |
| 5 | `Plugins/GameFeatures/ShooterTests/Source/**` | T0 | 2026-08-13 | 多人 PIE 自动化 |
| 6 | `D:/Software/UE_5.8/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/**` | T0 | 2026-08-13 | GAS 底层调用链 |
| 7 | `D:/Software/UE_5.8/Engine/Plugins/EnhancedInput/Source/EnhancedInput/**` | T0 | 2026-08-13 | MappingContext 底层 |
| 8 | `D:/Software/UE_5.8/Engine/Plugins/Runtime/ModularGameplay/Source/ModularGameplay/**` | T0 | 2026-08-13 | InitState 状态机底层 |

