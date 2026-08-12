# Lyra 跨模块射击校验扩展：从硬编码放行到最小服务器钩子

## 1. 背景与目标

ExtractionOps 的射击能力复用了 Lyra 的完整 Ranged Weapon 流程，包括本地瞄准、预测、TargetData 复制、弹药 Cost、命中反馈和 Gameplay Effect。项目需要在服务器接受客户端 TargetData、扣除弹药并执行伤害之前，增加自己的合法性校验，同时满足两个约束：

- 不复制一整套 Lyra 射击框架；
- 不改变 ShooterCore 等既有 Experience 的默认行为。

最终采用的是一个很窄的 Lyra 基线扩展：在 `ULyraGameplayAbility_RangedWeapon` 中加入默认放行的 virtual 校验函数，再由 ExtractionOpsRuntime 的子类实现严格规则。

这里修改的是项目仓库中的 `Source/LyraGame`，不是 Unreal Engine 安装目录或引擎源码。

## 2. Lyra 原始流程为什么无法直接扩展

Lyra 在 `OnTargetDataReadyCallback()` 中完成 TargetData 的接收与后续提交。原始代码将合法性直接写死为：

```cpp
const bool bIsTargetDataValid = true;
```

随后进入：

```cpp
if (bIsTargetDataValid && CommitAbility(...))
{
    WeaponData->AddSpread();
    OnRangedWeaponTargetDataReady(LocalTargetDataHandle);
}
```

这意味着 `CommitAbility()` 会在 Blueprint 的 `OnRangedWeaponTargetDataReady` 之前发生。对于 Lyra 武器，它通常包含弹药 Cost 与射击提交，因此在 Blueprint 事件或 Damage Execution 中拒绝非法请求都已经太晚：

- 在 Blueprint 事件中拒绝，只能阻止后续伤害，不能撤销已经提交的弹药和 Ability 状态；
- 在 Damage Execution 中拒绝，只能令伤害为零，TargetData 和命中确认路径仍然已经被接受；
- 使用额外 Server RPC 会产生第二条射击协议，破坏 Lyra 已有的预测键、Ability Ownership 和 TargetData 生命周期。

原类还存在两个扩展限制：

- `OnTargetDataReadyCallback()` 不是 virtual；
- TargetData Delegate Handle 是 private，插件子类不能替换回调绑定。

因此，GameFeature 子类无法在正确的提交点插入服务器校验。

## 3. 最小扩展缝

### 3.1 在 Lyra Ranged Ability 中声明钩子

在 `LyraGameplayAbility_RangedWeapon.h` 中增加：

```cpp
virtual bool ValidateTargetDataOnServer(
    const FGameplayAbilityTargetDataHandle& TargetData,
    FGameplayTag ApplicationTag,
    FString& OutFailureReason);
```

基类实现始终返回 `true`：

```cpp
bool ULyraGameplayAbility_RangedWeapon::ValidateTargetDataOnServer(
    const FGameplayAbilityTargetDataHandle& TargetData,
    FGameplayTag ApplicationTag,
    FString& OutFailureReason)
{
    return true;
}
```

默认放行非常重要。它保证不使用 ExtractionOps 子类的 ShooterCore 武器维持原有语义，而不需要为所有既有资产重新配置规则。

### 3.2 在提交前调用钩子

原来的硬编码放行改为：

```cpp
FString TargetDataFailureReason;
const bool bIsTargetDataValid = !CurrentActorInfo->IsNetAuthority()
    || ValidateTargetDataOnServer(
        LocalTargetDataHandle,
        ApplicationTag,
        TargetDataFailureReason);
```

逻辑边界如下：

- Client 继续运行 Lyra 原有的本地预测与表现路径；
- Server 收到复制的 TargetData 后调用项目校验；
- 只有校验通过，才执行 `CommitAbility()`、扩散累积和后续伤害图；
- 校验失败时结束 Ability，并记录稳定的拒绝原因。

这个调用点位于 TargetData 到达之后、Ability Cost 与伤害提交之前，是本次修改最重要的设计选择。

## 4. 为什么要增加 `LYRAGAME_API`

ExtractionOpsRuntime 是独立的 GameFeature Runtime 模块，其 DLL 与 `LyraGame` 模块不同。项目子类的继承链是：

```text
UExtractionGameplayAbility_RangedWeapon
  -> ULyraGameplayAbility_RangedWeapon
    -> ULyraGameplayAbility_FromEquipment
      -> ULyraGameplayAbility
```

当一个类需要被另一个模块继承或直接调用非内联实现时，Windows DLL 构建通常需要通过模块 API 宏导出相关符号。因此增加了：

```cpp
class LYRAGAME_API ULyraGameplayAbility_RangedWeapon
```

以及：

```cpp
class LYRAGAME_API ULyraGameplayAbility_FromEquipment
```

第一次只导出 Ranged Weapon 类时，链接器仍报告父类构造、析构、虚表和 `IsDataValid()` 等未解析符号。这是因为子类 DLL 不只依赖最外层类名，也依赖整条继承链中实际定义于 LyraGame DLL 的符号。为 `FromEquipment` 增加导出宏属于模块可见性修正，不改变它的 Gameplay 行为。

典型链接错误包括：

```text
LNK2001 / LNK2019 unresolved external symbol
ULyraGameplayAbility_FromEquipment::IsDataValid(...)
ULyraGameplayAbility_RangedWeapon constructor/destructor/vtable
```

需要区分两类问题：

- 编译错误通常表示头文件、类型或访问权限不正确；
- 此处的代码能够编译、但 DLL 链接失败，说明声明可见而实现符号没有正确导出。

## 5. ExtractionOps 如何使用该钩子

项目实现位于：

```text
Plugins/GameFeatures/ExtractionOps/Source/ExtractionOpsRuntime/
  Public/Abilities/ExtractionGameplayAbility_RangedWeapon.h
  Private/Abilities/ExtractionGameplayAbility_RangedWeapon.cpp
```

Rifle 和 Shotgun 的 Fire Blueprint 都以 `UExtractionGameplayAbility_RangedWeapon` 为父类。项目子类只负责服务器校验，没有复制 Lyra 的预测、Trace、TargetData 发送、Cost 或命中反馈代码。

当前校验覆盖：

- 当前执行端必须是 Authority；
- Avatar、Controller、ASC 和 WeaponInstance 必须有效；
- 玩家不能处于 Dead、Reloading 或背包打开状态；
- TargetData 数量不得超过该武器的每发弹丸数；
- TargetData 必须是 Lyra 的 SingleTargetHit 类型；
- 同一发中的弹丸必须共享 Cartridge ID；
- Trace 起点必须在角色附近；
- Trace 方向不能明显背离服务器所知的瞄准方向；
- Trace 距离不能超过武器射程与容差；
- 命中 Actor 必须有效且不能是射手自己；
- 同一次 Ability 激活中的 Cartridge ID 不能重复处理。

弹药数量和 Ability Cost 仍由 Lyra `CommitAbility()` 作为最终权威入口，项目校验没有维护第二份弹药状态。

## 6. 调用时序

```text
Owning Client
  1. 本地输入触发 Fire Ability
  2. 执行本地 Trace，构造 Lyra TargetData
  3. 使用 PredictionKey 发送 TargetData
  4. 保留本地预测表现

Dedicated Server
  5. 收到 TargetData Delegate 回调
  6. ExtractionOps 校验 TargetData
  7a. 失败：拒绝提交、记录 reason、结束 Ability
  7b. 成功：CommitAbility 扣除权威 Cost
  8. 执行 Damage Effect / Damage Execution
  9. 复制 Health、Armor、Death 等最终状态
```

核心原则是：客户端可以预测“我开枪了”，但不能决定“这份 TargetData 合法”“弹药已正确扣除”或“目标受到了伤害”。

## 7. 为什么没有复制整套射击 Ability

复制 Lyra 的 `ULyraGameplayAbility_RangedWeapon` 虽然能够做到 Lyra 基线零修改，但会引入较高维护成本：

- 复制数百行预测、Trace、TargetData、HitMarker 和 Delegate 生命周期代码；
- 后续 Lyra 更新时需要人工比较两套实现；
- 容易遗漏 PredictionKey、TargetData 消费或 Delegate 解绑细节；
- 面试中很难证明复制版本与原版的行为仍完全一致。

相比之下，默认放行的 virtual hook 具有更清晰的变化边界：

- Lyra 基线只知道“这里允许项目做服务器校验”；
- 具体 Extraction 规则全部留在 GameFeature；
- 原有 ShooterCore 行为不变；
- 扩展点可以通过一个小 diff 审查和回归测试。

这是一种“最小侵入式扩展”，不是“零侵入式扩展”。

## 8. 可选方案与取舍

| 方案 | 优点 | 主要问题 |
| --- | --- | --- |
| 当前 virtual hook | 校验时点正确、复用完整 Lyra 流程、差异很小 | 需要维护少量 Lyra 基线 patch |
| 插件复制完整 Ranged Ability | Lyra 基线零修改 | 大量重复代码，升级和预测正确性风险高 |
| Blueprint 事件中校验 | 资产配置直观 | `CommitAbility()` 已执行，无法完整拒绝 |
| Damage Execution 中校验 | 伤害始终由 Server 计算 | 不能撤销已接受的 TargetData、弹药和命中路径 |
| 新建平行 Server RPC | 规则完全自定义 | 形成第二套网络协议，绕开 GAS PredictionKey/Ownership |
| 修改引擎网络层 | 控制能力最强 | 对当前 Slice 明显过度设计，维护成本最高 |

若未来升级 Lyra 或要求上游基线完全干净，可以将这三个文件的差异维护为独立 patch，并在每次升级时执行以下检查：

1. Lyra 是否已经提供等价的服务器验证扩展点；
2. TargetData 回调、CommitAbility 和 HitMarker 的先后顺序是否变化；
3. 模块 API 宏是否仍有必要；
4. ExtractionOps 子类是否仍只覆盖校验、不复制流程；
5. ShooterCore 回归与 ExtractionOps 非法请求测试是否都通过。

## 9. 安全边界与当前限制

这套校验提升了服务器权威性，但不能等同于生产级反作弊或服务器回溯命中系统。

当前服务器主要验证客户端 TargetData 的结构与合理范围，并依赖服务器世界中的 Actor 和角色状态。它尚未实现：

- 基于服务器历史快照的 lag compensation / rewind；
- 对客户端 Trace 的完整服务器重新射线；
- 行为统计、反自动瞄准或内核级反作弊；
- 面向大规模公开 PvP 的对抗性验证。

对于当前 1–2 人合作 PvE Vertical Slice，这个边界足以展示 GAS、预测、模块化与服务器校验的技术理解；如果未来进入玩家入侵或 2v2，应重新评估服务器重算、回溯与作弊模型。

## 10. 作品集应如何披露

作品集不能把这三处文件描述成“完全未修改 Lyra”。推荐表述：

> ExtractionOps 复用了 Lyra 的预测射击管线，并在项目 LyraGame 模块增加一个默认放行的服务器 TargetData virtual hook。具体校验规则位于 GameFeature Runtime；ShooterCore 保持默认行为。这样避免复制整套射击框架，并让非法 TargetData 在 Ability Cost 与伤害提交前被拒绝。

建议同时提供：

- 三个 Lyra 基线文件的最小 diff；
- ExtractionOps 子类实现索引；
- 合法与非法 TargetData 自动化测试；
- ShooterCore 回归结果；
- 对“当前不是生产级反作弊”的明确说明。

这比声称“所有代码都在插件内、完全没有修改 Lyra”更可信，也更能体现对 Unreal 模块边界、GAS 提交流程和维护成本的理解。

## 11. 对应实现索引

- `Source/LyraGame/Equipment/LyraGameplayAbility_FromEquipment.h`
- `Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.h`
- `Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.cpp`
- `Plugins/GameFeatures/ExtractionOps/Source/ExtractionOpsRuntime/Public/Abilities/ExtractionGameplayAbility_RangedWeapon.h`
- `Plugins/GameFeatures/ExtractionOps/Source/ExtractionOpsRuntime/Private/Abilities/ExtractionGameplayAbility_RangedWeapon.cpp`
- `Plugins/GameFeatures/ExtractionOps/Source/ExtractionOpsRuntime/Public/Network/ExtractionNetworkValidation.h`
- `Plugins/GameFeatures/ExtractionOps/Source/ExtractionOpsRuntime/Private/Network/ExtractionNetworkValidation.cpp`
- `Plugins/GameFeatures/ExtractionOps/Source/ExtractionOpsRuntime/Private/Tests/ExtractionOpsStateRulesTests.cpp`

