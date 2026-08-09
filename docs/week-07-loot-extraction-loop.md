# 第 7 周：地图交互、搜刮与撤离循环

## 本周目标

把战斗和背包串成一局完整 Vertical Slice：进入测试地图、搜刮地面物品/容器、战斗、撤离或死亡、查看本局结果。交互、倒计时、掉落和结果全部由 Dedicated Server 决定。

## 前置条件与周门槛

- 第 6 周不存在物品复制、实例 ID 丢失或背包版本回退。
- 本周地图为中小型测试场，不做开放世界、World Partition 调优或美术精修。
- 对局状态存在于 Dedicated Server；永久结算第 8–10 周接入。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 对局状态机与测试地图 | 3–4 小时 |
| 2 | 通用交互与战利品容器 | 4 小时 |
| 3 | 撤离点与服务器倒计时 | 4 小时 |
| 4 | 死亡掉落与结果界面 | 3–4 小时 |
| 5 | 完整对局、并发和断网测试 | 2–4 小时 |

## 先读什么

- `Interaction/IInteractableTarget.h`、`InteractionOption.h`、Interact Ability 与 WaitForInteractableTasks；
- `AbilitySystem/Phases/LyraGamePhaseSubsystem.*`；
- `GameModes/LyraGameState.*` 与 Experience/地图绑定；
- UE Server Timer、GameState Server World Time 与 Overlap 权威判定。

## 工作单元 1：对局状态机与测试地图

固定状态：

```text
Lobby -> Loading -> InRaid -> Results
玩家 Run：InRaid -> Extracting -> Extracted
              |          -> InRaid（取消）
              -> Dead
```

`match_id` 标识一局，`run_id` 标识一名玩家在该局中的运行。MatchState 放 GameState/权威组件，玩家 RunState 放 PlayerState/专属组件；不要把每个玩家的 Extracting 当作全局 MatchState。

创建 `L_ExtractionTest`：一个出生区、三处 Loot、一个容器区、一个交战区、一个撤离区。使用现有 Lyra 几何和导航资产，保证两个玩家 3–5 分钟能走完。

配置 Extraction Experience 和 Server 启动路径。通过标准：独立 Server + 两 Client 稳定加载地图，生成唯一 match_id/run_id。

## 工作单元 2：统一交互与 Loot Container

复用 Lyra `IInteractableTarget` 与 Interaction Ability，ExtractionOps 只增加项目校验和请求关联。统一行为包含：CanInteract、交互文本、持续时间、Server 接受/拒绝、`request_id` 与错误码。

创建 `AExtractionLootContainer`：Server 生成 `container_id` 和物品实例列表；首次生成后固定，不因第二个客户端打开而重新随机。MVP 允许多人查看，但每个物品仍通过第 6 周原子转移领取。

测试：超距离、无视线、Dead、已取空容器、两人同时取同一实例。失败码至少覆盖 `OutOfRange`、`LineOfSightBlocked`、`InvalidState`、`ItemNotFound`、`AlreadyConsumed`。

## 工作单元 3：服务器权威撤离点

创建 `AExtractionZone`，状态为：

```text
Closed -> Available -> Countdown -> Extracted
                         -> Cancelled
                         -> Failed
```

Server 保存每位玩家的 `extraction_end_server_time`，固定倒计时 10 秒。开始时校验：RunState=InRaid、Pawn 存活、处于区域、Zone Available、match_id/run_id 匹配。倒计时中每次离开区域、死亡或对局结束都取消。

客户端用 GameState Server World Time 平滑显示剩余时间；本地计时归零只显示等待确认，不能自行写 Extracted。Server 成功后锁定玩家输入/伤害参与并生成本局结果快照。

测试客户端改系统时间、暂停 UI、重复 BeginExtraction、最后 0.1 秒离开区域；只有 Server Timer 决定结果。

## 工作单元 4：死亡掉落与结果界面

固定 MVP 规则：死亡时本局背包内所有可掉落物转移到一个 `death_container_id`；当前装备也按 Definition 的 droppable 配置处理。死亡事件生成唯一 `death_event_id`，重复回调只返回已处理。

Server 在一个权威流程中：RunState `InRaid/Extracting -> Dead` → 取消撤离 → 创建死亡容器 → 转移实例 → 锁定本玩家结果。其他玩家可以按通用交互搜刮。

结果界面只展示 Server 快照：result、带出/丢失物品、结束时间、match_id/run_id。本周暂存内存；Backend 不可用概念从第 8 周加入。

## 工作单元 5：完整对局与故障实验

从关闭进程开始执行：

1. Server 加载测试地图，A/B 加入；
2. A/B 搜刮不同 Loot，再争抢同一容器物品；
3. B 击杀 A，A 物品进入唯一死亡容器；
4. B 搜刮并进入撤离区；
5. 先离开一次验证 Cancelled，再重新进入并成功 Extracted；
6. 双方进入 Results，实例总数守恒。

异常测试：100 ms 延迟、5% 丢包、撤离中客户端退出、重复死亡回调、重复撤离请求、客户端伪造 Extracted。当前周断线玩家按未恢复状态保留在 Server，完整重连第 10 周实现；但不得凭断线直接获得成功。

## 验收目标

- [ ] 中小型地图可在 3–5 分钟完成一局；
- [ ] 地面物品、容器、撤离点共用 Lyra 交互入口；
- [ ] 容器物品只生成一次且并发领取唯一；
- [ ] Server 决定撤离开始、取消和成功；
- [ ] 死亡只生成一个死亡容器，物品实例总数守恒；
- [ ] 一名玩家死亡、一名玩家撤离后都进入正确结果；
- [ ] 客户端不能伪造倒计时、掉落或结果；
- [ ] 完整对局在延迟/丢包下最终一致。

## 实现原理

撤离循环是多个权威状态机的组合：全局 Match、每个玩家 Run、交互对象和物品容器。客户端展示 Server 时间和快照，但不决定成功。唯一事件 ID 和实例转移让重复回调、网络重试不会创造额外战利品。

## 常见问题与停止条件

- 客户端成功而 Server 拒绝：区分“正在撤离”和“已撤离”，只展示权威终态。
- 倒计时被本地暂停：改用 Server 结束时间，不累计客户端 DeltaSeconds。
- 死亡掉落重复：用 death_event_id/终态转换保护整个流程。
- 容器每次打开重置：Loot 只能由 Server 首次创建并保存在容器状态。

完整一局不能稳定重复三次时，不接 Backend。

## 本周作品集产出

- 3–5 分钟完整对局视频；
- Match/Run/Extraction 状态图；
- 通用交互设计；
- 服务器倒计时说明；
- 物品守恒和重复死亡测试记录。

## 参考资料

- [Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine)
- [Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine)
- [项目 Interaction 源码](../Source/LyraGame/Interaction)
