# Source/LyraGame/Teams

Team 系统把队伍身份、公开/私有信息、显示资产和查询服务分开。`ILyraTeamAgentInterface` 让 Controller、PlayerState、Pawn 等暴露 TeamId 并通知变化；`ULyraTeamSubsystem` 是 World 查询与关系判断入口；TeamCreationComponent 在 Experience 就绪后由 Authority 创建 TeamInfo Actor。

`ALyraTeamPublicInfo` 面向所有相关客户端复制；`ALyraTeamPrivateInfo` 用于受限信息。`ULyraTeamDisplayAsset` 提供颜色/材质等表现配置。AsyncAction 只观察 Team/颜色变化，不拥有状态。

DamageExecution 通过 TeamSubsystem 判断伤害是否允许，因此友伤规则最终仍必须在 Authority 数值链校验，UI 颜色不能成为规则依据。

## 面试追问

1. TeamId 为什么不只存在 Pawn？
2. Public/Private TeamInfo 分离解决什么复制问题？
3. 队伍关系为何应由 WorldSubsystem 查询，而不是比较整数？
4. 客户端显示敌我颜色错误会不会改变伤害结果？

## 练习

追踪一次 PlayerState Team 变化到 Pawn/Controller observer、UI 颜色和 DamageExecution 的传播，标出复制与本地 delegate 边界。

