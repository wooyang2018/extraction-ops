# Lyra 依赖的 UE 5.8 底层机制

本篇不是泛读 Engine，而是给 Lyra 调用链建立“向下一层”的索引。

## Gameplay Framework

Lyra 仍遵守 UE 的标准所有权：GameMode 仅服务端、GameState 全端复制、PlayerController 只在服务端与拥有客户端存在、PlayerState 面向所有相关客户端复制、Pawn 是可替换 Avatar。Lyra 的 Experience 和 Modular Gameplay 没有改变这些事实，只是在其上增加延迟编排。

重点追踪：

- `Engine/Source/Runtime/Engine/Classes/GameFramework/GameModeBase.h`
- `Engine/Source/Runtime/Engine/Classes/GameFramework/GameStateBase.h`
- `Engine/Source/Runtime/Engine/Classes/GameFramework/PlayerController.h`
- `Engine/Source/Runtime/Engine/Classes/GameFramework/PlayerState.h`
- `Engine/Source/Runtime/Engine/Classes/GameFramework/Pawn.h`

## AssetManager 与 StreamableManager

Experience/PawnData/GameData 使用 Primary Asset ID 和软引用。AssetManager 负责规则、扫描和 bundle；StreamableManager 负责异步装载句柄。理解重点是“对象引用存在”与“依赖 bundle 已加载”不是一回事。

重点追踪：`Engine/Source/Runtime/Engine/Classes/Engine/AssetManager.h` 与 `StreamableManager.h`。

## GameFeatures 状态机

`UGameFeaturesSubsystem` 把插件从 Installed/Registered/Loaded 推进到 Active，并管理依赖和协议 URL。Lyra ExperienceManager 不复制这套状态机，而是组织多个插件完成后何时执行 Experience Actions。

源码：`Engine/Plugins/Runtime/GameFeatures/Source/GameFeatures/Private/GameFeaturesSubsystem.cpp`。

## ModularGameplay

`UGameFrameworkComponentManager` 提供三类能力：

1. 向某 Actor 类请求动态组件；
2. 注册扩展处理器并接收 Actor 生命周期/自定义事件；
3. 为同一 Actor 上的多个 Feature 管理 Init State。

源码：`Engine/Plugins/Runtime/ModularGameplay/Source/ModularGameplay/Public/Components/GameFrameworkComponentManager.h`。

## Gameplay Ability System

Lyra 在 GAS 上增加 AbilitySet、输入 Tag、激活组、TagRelationship、成本、GamePhase 和定制 EffectContext，但预测键、AbilitySpec、属性聚合、GameplayEffect、GameplayCue 和 RPC 仍属于 Engine GAS。

源码根：`Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/`。

## Enhanced Input

Input Mapping Context 决定“哪些物理输入产生哪些 InputAction”；`ULyraInputConfig` 再把 InputAction 映射到 Gameplay Tag；HeroComponent 把 Native Tag 绑定到移动/视角函数，把 Ability Tag 转交 ASC。三层不要混淆。

源码根：`Engine/Plugins/EnhancedInput/Source/EnhancedInput/`。

## 网络复制

Lyra 的 Inventory、Equipment、Team 与消息复制大量依赖：

- Actor/Component replicated properties 与 RPC；
- `FFastArraySerializer` 增量复制；
- ReplicationGraph 的连接分组与空间节点；
- GAS 自带的 AbilitySpec/ActiveGameplayEffect 复制与预测。

判断一段代码时先问：状态真相放在哪里、谁能写、谁需要收到、迟到加入者是否需要历史状态。GameplayMessage 更适合瞬时事件，不应替代需要 late join 一致性的复制属性。

## CommonUI 与 Activatable Widget

CommonUI 使用可激活 Widget 栈、InputConfig 和 Action Router 管理页面层级及输入模式。Lyra 的 UIExtension 解决“功能插件把 Widget 放到宿主定义的扩展点”，二者分别回答页面导航和跨插件组合。

## 面试追问

1. SoftObjectPtr 已非空，为什么仍可能不能立即使用对象？
2. FastArray 相比普通 replicated TArray 多解决了什么，仍未解决什么？
3. GameFeature 插件 Loaded 与 Active 的语义差异是什么？
4. GameplayMessage 为什么不能天然保证迟到加入客户端看到旧事件？
5. Enhanced Input 的 Mapping Context、InputAction、Gameplay Tag 各处于哪一层？

