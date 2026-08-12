# 第 4 周：多人联机与复制验证

## 实现状态（2026-08-12）

服务器 TargetData 校验已真实接入 Rifle/Shotgun Fire Ability，Baseline、100 ms、100 ms + 5% 丢包三组 Editor Dedicated Process 双客户端会话均通过。规则自动化与日志证据见 [Week 04–07 验收记录](evidence/week-04-07-acceptance.md)。逐发射击与主观延迟表现仍需真人操作录制，不能由无界面 Join smoke 代替。

## 本周目标

把第 3 周战斗回路放进 Editor Dedicated Process + 两个 Editor Client Process，明确每个动作的发起者、执行者和观察者。伤害、死亡和弹药结果由服务器确认；客户端本地篡改不能改变最终状态。

## 执行基线

开始前完整阅读[12 周执行基线](execution-baseline.md)。本周只使用 `D:\Software\UE_5.8`；Editor Dedicated Process 是当前权威测试进程，但不等价于 Packaged Dedicated Server。不得访问受保护的 ue5-main 源码目录。

## 与总蓝图同步：Dedicated Server 双客户端证据

这一周验证实时玩法网络边界；第 9 周再接 Backend 房间、Ticket、注册和心跳。当前 Installed Build 不支持 Server Target，因此沿用第 1 周已验证的 Editor Dedicated Process，不等待源码引擎。

固定启动拓扑：

```text
Editor Dedicated Process（独立进程，无本地玩家、NullRHI）
  <- Client A（拥有自己的 Pawn/PlayerController）
  <- Client B（拥有自己的 Pawn/PlayerController）
```

按以下顺序建立证据：

1. 构建 `LyraEditor Win64 Development`，保存完整命令与 commit，并复用第 1 周三个独立进程启动方式；
2. Server 显式加载灰盒地图和 Extraction Experience；
3. 两个 Client 分别连接，日志记录唯一连接与 PlayerState；
4. A/B 互相观察移动、装备、射击、伤害、死亡；
5. 分别加入 100 ms 延迟、5% 丢包，重复固定操作序列；
6. 尝试客户端伪造命中、弹药和死亡结果，记录服务器拒绝原因；
7. 用 Network Profiler 记录空闲、交战和高频射击三个片段。

验收报告至少包含：进程启动命令、三端日志时间线、Authority/Ownership 表、一次非法请求、一次延迟丢包实验，以及最终状态是否收敛。只在 Editor 多人 PIE 中通过不能替代独立 Server 证据。

## 前置条件与周门槛

- 第 3 周固定战斗序列在单机模式稳定通过。
- 第 1 周双客户端启动命令仍可复现。
- 本周不新增背包或撤离功能，只处理网络边界和实验记录。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 网络角色与复制字段清单 | 3 小时 |
| 2 | 服务器权威射击和校验 | 4–5 小时 |
| 3 | 伤害、死亡、弹药和 UI 一致性 | 3–4 小时 |
| 4 | 延迟、丢包、非法请求实验 | 3–4 小时 |
| 5 | 回归、带宽初测和报告 | 2–4 小时 |

## 先读什么

- `Weapons/LyraGameplayAbility_RangedWeapon.*` 和 `LyraWeaponStateComponent.*`；
- `AbilitySystem/LyraGameplayAbilityTargetData_SingleTargetHit.*`；
- `Character/LyraHealthComponent.*` 与 PlayerState ASC 初始化；
- `System/LyraReplicationGraph.*`；
- UE Ownership、RPC、Replicated Property、Relevancy 和网络模拟文档。

## 工作单元 1：网络角色和状态清单

### 1.1 扩展调试 HUD/日志

为本地 Pawn、瞄准目标和武器显示：Authority、LocalRole、RemoteRole、Owner Controller、PlayerState、NetMode。Dedicated Server 只写日志，不创建 Widget。

### 1.2 给状态分类

建立并评审表格：

| 状态 | 权威位置 | 接收者 | 机制 |
| --- | --- | --- | --- |
| 输入意图 | Owning Client 发起 | Server | Ability/Server RPC |
| 生命、死亡 | Server | 相关客户端 | GAS/复制属性 |
| 弹药 | Server | Owner 为主 | Inventory/复制 |
| 枪口与后坐 | 本地预测 | 本地/观察者表现 | 本地 + 必要 Cue |
| 命中确认 | Server | 射手 | Client 消息/复制结果 |

没有明确用途的字段不得标记 Replicated；不得用 Multicast 同步永久状态。

## 工作单元 2：服务器权威射击

沿用 Lyra Ranged Weapon Ability 的 TargetData/预测流程，不额外创建无 Ownership 的 RPC Actor。对每个射击请求验证：

1. 请求来自拥有该 Pawn/ASC 的连接；
2. 武器实例确实装备在该玩家；
3. 玩家不是 Dead/Reloading 等阻断状态；
4. Server 记录的弹药大于 0；
5. Server 时间满足射速；
6. 起点、方向、射程和客户端时间在容许范围；
7. 命中目标在服务器世界中有效。

拒绝日志统一包含：

```text
event=shot_rejected player_id=local-dev shot_id=... reason=fire_rate_limited
```

不得记录为模糊的 `invalid request`。客户端收到拒绝后取消错误命中反馈，并由真实弹药/状态覆盖预测显示。

用两个 Client 验证：A 射击 B，只有 Server 应用 Damage Effect；B 的 Health 与死亡状态复制到双方。

## 工作单元 3：最终状态一致性

执行并观察：

1. A 连续射击 B，记录 Server、A、B 三端 Health；
2. B 死亡后继续发送 Fire，Server 拒绝；
3. A 弹匣为空时连续点击，Server 弹药不为负；
4. A 换弹时 B 击杀 A，换弹取消且不补发弹药；
5. A 重建 HUD，生命和弹药从复制状态恢复。

所有最终状态以 Server 日志为基准。允许枪口、动画短暂不同步，不允许生命、死亡和弹药永久分叉。

加入每次测试的关联字段：`player_id` 暂用本地稳定开发 ID，`match_id` 暂用本次进程生成值，`shot_id` 由拥有客户端生成但由 Server 与玩家身份共同校验。

## 工作单元 4：网络与非法请求实验

### 4.1 固定三组网络条件

使用 PIE Network Emulation 或 `NetEmulation`/项目适用控制台命令，记录实际生效值：

- A：RTT 近似 0、无丢包；
- B：单向延迟约 100 ms；
- C：单向延迟约 100 ms、5% 丢包。

每组执行相同的 10 发射击、一次换弹、一次死亡，并记录预测表现、Server 接收、最终一致性和恢复时间。

### 4.2 非法请求

仅在开发构建加入受控测试入口，依次提交：过快射击、零弹射击、不属于自己的武器、超长射线、Dead 状态射击、重复 shot_id。验证 Server 返回确定错误码且最终状态不变。测试后保留入口但限制为非 Shipping，或移除临时作弊节点。

### 4.3 本地篡改

在客户端调试器/开发命令中临时修改显示生命或弹药，确认下一次权威更新会纠正且 Server 值不变。不要通过真正制作作弊程序来完成本项。

## 工作单元 5：复制成本与周验收

使用 `stat net`、网络调试命令或 Network Profiler 记录空闲与战斗时：Actor 数、RPC 数、发送/接收速率。此周只建立基线，不做无数据优化。

最终从关闭所有进程开始：启动 Server、两个 Client，完成加入、互相观察、A 击杀 B、延迟/丢包射击、非法请求、退出。保存三端日志和 1 分钟录屏。

## 验收目标

- [ ] 独立 Server 与两个 Client 稳定进入同一局；
- [ ] 客户端只提交射击意图，Server 应用伤害与死亡；
- [ ] Server 校验 Ownership、武器、状态、射速、弹药和方向；
- [ ] 本地修改生命/弹药不能改变 Server 最终值；
- [ ] 延迟和丢包下没有重复扣血、负弹药或永久分叉；
- [ ] 非法请求有结构化拒绝日志；
- [ ] 有正常、延迟、丢包三组可复现实验记录；
- [ ] 有复制带宽初始基线。

## 实现原理

RPC/Ability TargetData 传递事件或意图，复制属性/GAS 传播状态。Ownership 决定客户端是否能向 Server 发起调用，Relevancy 决定谁需要收到 Actor。客户端预测追求即时手感，Server 权威保证最终可信；两者不能混成“客户端先写最终值再广播”。

## 常见问题与停止条件

- Server RPC 不执行：查调用对象 Ownership、复制建立时机、调用端和参数可复制性。
- 双方结果不同：先区分表现差异与最终状态差异，再查 Server 是否真正修改权威数据。
- 延迟下双发：用 shot_id 对齐输入、预测和确认，不增加无依据的延时。
- Server 被请求淹没：做频率/状态校验，Reliable 不应承载高频可丢事件。

最终状态在任一网络条件下不能收敛时，不进入 GAS 扩展。

## 本周作品集产出

- 双 Client + Dedicated Server 视频；
- 射击权威时序图；
- 三组网络实验报告；
- 非法请求拒绝日志；
- RPC 与 Replicated Property 取舍说明。

## 参考资料

- [Networking Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-overview-for-unreal-engine)
- [RPC](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-procedure-calls-in-unreal-engine)
- [Replication Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/replication-graph-in-unreal-engine)
