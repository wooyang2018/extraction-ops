# 第 6 周：物品、背包与客户端状态

## 本周目标

做出撤离射击中最能体现“游戏业务理解”的系统：物品定义、拾取、背包、装备、丢弃和 HUD 展示。本周先完成客户端和对局内服务端状态，持久化到 Go 后台放到第 8～10 周。

## 验收目标

- 至少有三类物品：武器、弹药、医疗物品；
- 物品由稳定的 Item Definition 描述，实例有唯一 Item Instance ID；
- 玩家能拾取、放入背包、装备、使用和丢弃物品；
- 背包容量、重量或格子限制至少实现一种；
- 两客户端看到的地面物品和装备状态最终一致；
- 非法拾取、重复拾取、超距离拾取会被服务器拒绝；
- UI 与游戏状态解耦，重建 HUD 后仍能显示正确背包；
- 能解释物品定义、物品实例、对局状态和持久化仓库的区别。

## 操作步骤

### 1. 设计数据模型

建议区分四个概念：

```text
ItemDefinition
  描述物品类型：名称、图标、重量、最大堆叠、装备槽、使用效果

ItemInstance
  描述某个具体物品：instance_id、definition_id、耐久、数量、附加属性

InventoryState
  描述玩家当前对局背包：槽位、物品实例、容量、版本号

PersistentStash
  描述玩家对局外仓库：由后台数据库负责保存
```

不要把图标、名称和数值直接复制到每个拾取物 Blueprint。使用 Data Asset、Data Registry 或项目已有的数据驱动方式。

### 2. 实现地面拾取

地面物品至少包含：

- `item_instance_id`；
- `item_definition_id`；
- 世界位置；
- 是否已被拾取；
- 是否属于某个玩家；
- 是否已经过期。

客户端只发送“我想拾取哪个实例”的请求。服务器检查距离、可见状态、玩家容量和实例是否仍存在，然后把实例从世界状态转移到背包。

### 3. 实现背包操作

先做简单槽位背包，不要一开始做复杂俄罗斯方块背包。支持：

- 添加物品；
- 移动槽位；
- 合并堆叠；
- 拆分堆叠；
- 装备和卸下；
- 使用医疗品；
- 丢弃到世界。

所有操作都通过一个明确的 Command 或函数入口处理，例如：

```text
MoveItem(source_slot, target_slot)
EquipItem(item_instance_id)
UseItem(item_instance_id)
DropItem(item_instance_id, quantity)
```

避免在 UI 的拖拽事件里直接修改数组。UI 只创建操作请求，状态组件负责校验和执行。

### 4. 设计状态版本

给 `InventoryState` 加一个递增版本号或操作序号。客户端收到较旧状态时不要覆盖较新状态；服务器拒绝客户端操作时，要返回当前正确状态或明确的错误码。

建议错误码：

- `ItemNotFound`；
- `InventoryFull`；
- `InvalidSlot`；
- `NotOwner`；
- `OutOfRange`；
- `AlreadyConsumed`；
- `InvalidState`。

### 5. 做 UI 状态消费

UI 至少包含：

- 背包面板；
- 装备槽；
- 物品详情；
- 拾取提示；
- 操作失败提示；
- 当前重量或容量。

测试 HUD 被关闭、重新打开、地图切换和角色死亡后是否仍能正确渲染。

## 实现原理

背包是一个典型的“状态转移”系统。拾取不是复制一个物品，而是把一个唯一实例从世界容器移动到玩家容器。装备、使用和丢弃也应该是对状态的合法变换，而不是 UI 事件直接改字段。

实例 ID 对后续后台持久化和幂等很重要。只有 `definition_id` 无法区分两个相同类型但耐久不同的物品，也无法阻止同一物品被重复消费。

## 常见问题

### 两个客户端都拾取到了同一件物品

服务器在一个临界区内检查和转移物品，并在成功后立即标记已拾取。客户端的本地隐藏只是表现，不能作为权威锁。

### UI 显示了已经被其他玩家拿走的物品

这是正常的短暂网络延迟，但最终要收到服务器状态并移除。不要为避免闪烁而允许客户端永久保留一个不存在的物品。

### 重连后背包变空

本周可以先恢复对局内服务器状态。对局外仓库和结算持久化留到第 8～10 周；文档中必须明确两种状态的边界。

## 本周作品集产出

- 物品数据模型图；
- 一段多人同时争抢同一物品的演示；
- 背包命令和错误码表；
- 一篇“为什么 UI 不能直接改背包状态”的文章；
- 一张客户端、Dedicated Server 和后台之间的物品状态边界图。

## 参考资料

- [Data Assets](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-assets-in-unreal-engine)
- [Data Registry](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-registries-in-unreal-engine)
- [Common UI Plugin](https://dev.epicgames.com/documentation/en-us/unreal-engine/common-ui-plugin-for-advanced-user-interfaces)
- 项目内：[Source/LyraGame/Equipment](../Source/LyraGame)

