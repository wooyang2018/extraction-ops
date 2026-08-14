# Source/LyraGame/Weapons

## 开火主链

```text
InputTag
 -> ASC activates ULyraGameplayAbility_RangedWeapon
 -> local targeting / traces
 -> FLyraGameplayAbilityTargetData_SingleTargetHit(CartridgeID)
 -> prediction-key ServerSetReplicatedTargetData
 -> server target-data delegate
 -> validation hook
 -> CommitAbility / cost / spread
 -> GameplayEffect and DamageExecution
 -> HealthSet damage message
 -> hit marker / number pop feedback
```

`ULyraRangedWeaponInstance` 保存热量、散布、距离/材质衰减等武器实例状态；`ULyraWeaponStateComponent` 管理未确认命中批次与服务器确认；`ULyraGameplayAbility_RangedWeapon` 组织瞄准、TargetData 和提交。

## 预测不等于验证

PredictionKey 用于关联客户端预测与服务端确认/回滚，不证明 HitResult 合法。当前 `ValidateTargetDataOnServer` 无条件返回 true，是明确的扩展钩子；源码没有服务端重放 Trace、射速/弹药/视角/距离校验。面试或生产评审中必须把它称为安全缺口，而非“Lyra 已有反作弊”。

## 命中反馈

客户端可先显示未确认反馈，服务器通过 RPC 确认或替换命中。最终伤害仍由服务端 GameplayEffect Execution 计算；视觉命中标记不能反向成为伤害真相。

## 面试追问

1. 客户端 trace 的优点是什么，服务器至少要复核哪些约束？
2. CartridgeID 与 PredictionKey 分别解决什么关联问题？
3. 为什么 CommitAbility 应在服务端 TargetData 回调中完成？
4. 散布状态怎样避免客户端与服务端长期漂移？
5. SSR/lag compensation 应插在哪一层，当前源码是否已实现？

## 练习

为验证钩子设计一份不改源码的测试矩阵：伪造超距离 HitResult、背后射击、过高射速、无弹药与过期时间戳，记录当前哪些会被接受，并提出 Authority 校验方案。

