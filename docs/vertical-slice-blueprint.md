# Extraction Ops：20 周 Vertical Slice 执行蓝图

本文是项目范围、阶段门槛与验收的单一事实来源。`week-01` 至 `week-12` 文档保留为技术专题手册，不再代表交付排期。

## 核心承诺

- 第三人称、1–2 人合作 PvE，单局 12–15 分钟。
- 玩家启动“信号终端”换取物资与撤离情报，同时提高 Threat Level。
- 每次扫描都必须制造可感知的“继续贪还是立即撤”选择。
- 免费基础套装确保连续失败后仍可测试。
- 内部保留 Lyra Team 敌对测试开关验证 PvP 权限，但 v1 不承诺 PvP 平衡。

## 游戏与内容合同

一张近未来工业区地图包含 3 条主要路线、3 个信号终端、4 个物资区、2 个撤离区和 1 个高风险地标。服务器每局选择一个有效撤离区；至少启动一个终端才会显示并解锁撤离。

| 终端进度 | 情报与奖励 | 压力变化 |
| --- | --- | --- |
| Level 1 | 显示撤离区与基础缓存 | 小规模 AI 响应 |
| Level 2 | 显示高价值缓存 | 增援并开始侧翼搜索 |
| Level 3 | 最终奖励倍率提高 | 精英猎手生成，撤离压力强化 |

内容上限固定为 2 类主武器、1 个治疗物品、8 种三档价值战利品、12 格背包，以及巡逻射手、侧翼突击者、精英猎手 3 类 AI。`UExtractionArmorSet` 是必做的 GAS 战斗层：玩家出生时 `Armor=MaxArmor=30`，子弹伤害先扣 Armor 再扣 Health。v1 不做护甲物品、修甲、自动回复或多部位护甲；治疗物品只恢复 Health。死亡丢失本局携带物；成功撤离才形成结算快照并写入仓库。

## 权威边界与状态

| 边界 | 职责 |
| --- | --- |
| Client | 输入、预测表现、HUD、基于服务器时钟平滑显示倒计时 |
| Dedicated Server | 伤害、AI、终端、Threat、物品转移、死亡、撤离、结算快照 |
| Go Backend | 身份、双人房间、Join Ticket、永久仓库、幂等结算 |

- Match：`Loading -> InRaid -> Completed`
- Run：`InRaid -> Extracting -> Extracted | Dead | Abandoned`
- Terminal：`Idle -> Activating -> Activated`
- Extraction：`Locked -> Available -> Countdown -> Extracted | Cancelled`

公共标识统一使用 `player_id`、`room_id`、`match_id`、`run_id`、`server_instance_id`、`request_id` 和 `idempotency_key`；Gameplay DTO 与 HTTP DTO 分离。

后台最小接口为登录、玩家资料、双人房间 create/join/ready、match 与 join-ticket、服务器注册/心跳、幂等 settlement。后台不得早于核心玩法验证。

## 20 周路线与硬门槛

| 阶段 | 周次 | 交付与门槛 |
| --- | --- | --- |
| 重构与基线 | 1–2 | 源码引擎、Client/Server 构建、双客户端连接；记录 Lyra 扩展边界 |
| 打与压迫感 | 3–5 | 两把武器、命中反馈、三类 AI、一个终端、灰盒战斗区；脱离后台能反复玩 5 分钟并出现风险选择 |
| 联网撤离闭环 | 6–9 | 全图、三档 Threat、搜刮、背包、掉落、服务端撤离、结果页；Dedicated Server 支持两客户端 |
| 后台纵深 | 10–13 | 登录、双人房间、Ticket、仓库、幂等结算和 90 秒重连 |
| 体验与性能 | 14–17 | 统一资产风格、音频、动画、UI、引导；完成网络、AI、CPU、内存、带宽分析 |
| 外测与包装 | 18–20 | 封闭测试、Windows 构建、3 分钟演示、架构/时序图、Lyra 对比、文章与面试追问材料 |

阶段门槛未通过就顺延，不用增加内容掩盖核心问题。Steam 扩展顺序固定为：封闭测试通过 → 玩家入侵/2v2 原型 → Steam 身份与大厅 → 商店页/公开 Demo。

## 技术证据链

1. 3C：移动、镜头、武器、命中反馈与动画衔接。
2. GAS：射击、换弹、Armor/Health 伤害、治疗、死亡和状态限制；`UExtractionArmorSet` 必须具备复制、Clamp 和自动化测试。
3. AI：感知、行为树或 StateTree、EQS 侧翼、Threat 驱动调度。
4. 网络：Replication/RPC、服务器校验、延迟丢包实验、重连恢复。
5. 性能：Unreal Insights、Network Profiler、AI/复制热点优化前后对比。

不为展示技术提前实现自定义 Replication Graph、复杂预测框架或引擎修改；只有 Profiling 证明现有方案阻塞目标时才升级。

## 最终验收

- 至少 8 名外部测试者，每人至少 3 局。
- 第二局起至少 70% 无需口头提示即可说明终端、Threat 与撤离关系。
- 至少 60% 能复述一次真实的“继续扫描还是撤离”决策；乐趣与紧张感中位数达到 4/5。
- Dedicated Server 连续完成 10 局双客户端流程，无复制物品、错误终态或必须重启恢复的问题。
- 100 ms 延迟、5% 丢包下射击、终端、拾取、撤离与结算最终一致，客户端不能伪造关键结果。
- 重复死亡、拾取、结算和响应丢失不会重复生成物品或奖励。
- 目标机器 1080p 客户端 60 FPS、服务器 30 Hz；未达标时保留 Insights 证据和优化前后数据。
- 交付可运行构建、真实双客户端视频、自动化/故障测试、性能报告、个人新增代码索引，以及明确的 Lyra 基线说明。

## 非目标

v1 不做开放世界、完整交易经济、制作、赛季、付费、公开匹配、大规模扩容、生产级反作弊或完整 PvP 平衡。首发平台为 Windows 键鼠；核心文本通过 String Table 管理，为中英文支持留接口。
