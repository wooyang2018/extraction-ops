# 第 7 周：地图交互、搜刮与撤离循环

## 本周目标

把战斗和背包串成一个完整的游戏循环：进入地图、搜刮资源、遭遇风险、到达撤离点、成功带出或死亡损失。本周结束后，即使不接后台，也应该能完成一局可复玩的对局。

## 验收目标

- 有一张中小型可导航地图；
- 地图中至少有三类可交互物：地面物品、容器、撤离点；
- 玩家能看到交互提示，并根据距离和视线限制操作；
- 撤离点有开放、倒计时、成功、取消、失败状态；
- 玩家死亡后掉落部分对局内物品；
- 一局可以走通“进入 -> 搜刮 -> 战斗 -> 撤离/死亡 -> 结算界面”；
- 交互由服务端权威判定，客户端不能凭空完成撤离或获得物品；
- 能录制一局 3-5 分钟的完整演示。

## 操作步骤

### 1. 先定义对局状态机

建议使用：

```text
Lobby
  -> Loading
  -> InRaid
  -> Extracting
  -> Extracted
  -> Dead
  -> Results
```

每个状态明确：

- 玩家能做什么；
- 服务器能接受哪些请求；
- 客户端显示什么；
- 哪些状态变化需要广播；
- 超时或断线如何处理。

### 2. 实现通用交互接口

让可交互物实现统一接口，例如：

```text
CanInteract(Interactor)
GetInteractionText(Interactor)
ServerInteract(Interactor, RequestId)
OnInteractionAccepted(Interactor)
OnInteractionRejected(ErrorCode)
```

这样容器、撤离点、开门和拾取不需要各自复制一套输入逻辑。

### 3. 实现容器搜刮

容器至少有：

- 容器 ID；
- 生成表或固定物品；
- 是否已打开；
- 是否已被搜刮；
- 物品实例列表；
- 是否允许多个玩家同时访问。

建议 MVP 规则是服务器为容器生成唯一实例，客户端打开时请求内容，玩家拿走后服务器立即更新容器状态。

### 4. 实现撤离点

撤离点状态可以是：

```text
Closed -> Available -> Countdown -> Extracted
                         -> Cancelled
                         -> Failed
```

验证条件包括：

- 玩家位于撤离区域；
- 玩家不是死亡状态；
- 服务器时间达到倒计时结束；
- 撤离点没有被禁用；
- 玩家仍然属于当前对局。

客户端可以展示倒计时，但最终结束时间由服务器计算。客户端时间只能用于平滑显示。

### 5. 实现死亡和掉落

明确物品损失规则：

- 已带入的对局外物品是否掉落；
- 对局中拾取物品是否全部掉落；
- 关键任务物品如何处理；
- 死亡后是否可以观察队友；
- 结算状态何时锁定。

先使用简单规则：玩家死亡后把部分对局内物品转移到一个死亡容器，服务器生成唯一容器 ID，其他玩家可搜刮。

## 实现原理

撤离射击的核心不是地图大小，而是风险和收益的状态循环。技术上，它把网络权威、物品状态、角色死亡、定时器和结算串到了一条链路。

所有倒计时、拾取、撤离和掉落都需要考虑客户端重复请求、延迟和并发。交互接口统一后，才能用同一套测试方法覆盖多个对象。

## 常见问题

### 客户端显示撤离成功但服务器拒绝

客户端可以先显示“正在撤离”，但只有服务器确认后才能进入 `Extracted`。把“交互开始”和“结果成功”分成两个状态。

### 撤离倒计时被客户端暂停

服务器保存开始时间或结束时间，客户端只根据服务器时间戳计算显示。不要使用客户端本地秒数作为最终判定。

### 死亡掉落重复生成

每次死亡都必须有唯一的 `death_event_id` 或 `drop_container_id`。服务器检测重复事件后返回已处理结果。

## 本周作品集产出

- 一局完整撤离视频；
- 对局状态机图；
- 通用交互接口设计；
- 撤离倒计时的服务器权威说明；
- 死亡掉落和物品唯一性的测试记录。

## 参考资料

- [Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine)
- [Timers and Delegates](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-timers-in-unreal-engine)
- [Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine)
- [Lyra Sample Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)

