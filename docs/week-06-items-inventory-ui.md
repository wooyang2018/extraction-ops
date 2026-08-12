# 第 6 周：物品、背包与客户端状态

## 本周目标

基于 Lyra Inventory/Equipment 实现服务器权威的简单槽位背包：稳定物品定义、唯一实例、拾取、移动、堆叠、拆分、装备、使用、丢弃和 UI。对局外永久仓库仍留给第 8–10 周 Backend。

## 执行基线

开始前完整阅读[12 周执行基线](execution-baseline.md)。本周只使用 `D:\Software\UE_5.8` 和 Editor Dedicated Process；不得访问受保护的 ue5-main 源码目录。武器资产必须引用第 3 周按职责确定的清单，不再使用模糊的聚合名称。

## 与总蓝图同步：战利品与 12 格背包

本 Slice 的内容合同是 12 格本局背包、2 类主武器、1 个治疗物品和 8 种三档价值战利品。第一版不要实现重量、嵌套容器、俄罗斯方块占格、制作或交易。

建议把物品分成三类：

```text
Equipment：两把主武器及其必要弹药/装备引用
Consumable：一个治疗物品，通过 GAS Ability 使用
Valuable：8 种战利品，只包含 tier、base_value、图标和显示文本
```

实现路径：

1. 复用 Lyra ItemDefinition、ItemInstance、InventoryManager、Equipment/QuickBar；
2. 只新增 Extraction 的价值、可掉落、结算分类等 Fragment；
3. 用服务器生成的 `item_instance_id` 标识运行时物品，不用资产路径充当实例 ID；
4. 地面 → 背包、背包 → 装备、背包 → 丢弃都必须是“一个来源删除、一个目标增加”的服务器原子变换；
5. Owner Only 复制完整背包，其他玩家只接收手中武器和地面物品表现；
6. 治疗物品的扣除与治疗 Ability 使用同一服务器提交点，Ability 失败不得消耗物品；
7. 撤离成功生成带出清单，死亡生成丢失/掉落清单，但永久仓库仍不在本周写入。

每个命令都记录 `request_id`、`item_instance_id`、`expected_version` 和结果码。重点验证两人争抢同一物品、响应乱序、背包满、治疗中死亡、重复丢弃和重复拾取。

## 前置条件与周门槛

- 第 5 周 GAS 属性和死亡状态在两客户端一致。
- 所有背包核心代码进入 ExtractionOps；UI 只发 Command 并消费状态。
- 本周采用固定数量槽位，不做俄罗斯方块背包、重量与槽位双重限制或持久化仓库。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 数据模型和 Lyra Inventory 阅读 | 3 小时 |
| 2 | 物品定义、实例与地面拾取 | 4 小时 |
| 3 | 背包命令、版本与服务器校验 | 4–5 小时 |
| 4 | 背包/装备 UI | 3–4 小时 |
| 5 | 双人争抢、非法操作和恢复测试 | 2–4 小时 |

## 先读什么

- `Inventory/LyraInventoryItemDefinition.*`、`LyraInventoryItemInstance.*`、`LyraInventoryManagerComponent.*`；
- `Inventory/IPickupable.*` 与 Inventory Fragment；
- `Equipment/LyraEquipmentDefinition.*`、`LyraEquipmentManagerComponent.*`、`LyraQuickBarComponent.*`；
- Fast Array/复制集合相关实现；
- CommonUI 的 Activatable Widget 和输入路由。

## 工作单元 1：锁定数据模型和职责

在文档中先固定：

```text
ItemDefinition：类型数据；不含运行时数量
ItemInstance：instance_id、definition_id、quantity、durability、附加状态
InventoryState：玩家本局槽位、版本、容量
WorldPickup：世界容器中的 ItemInstance
PersistentStash：Backend 持久化对象，本周不实现
```

`instance_id` 由 Server 创建，使用 UUID/Guid 字符串；相同 Definition 的两个物品仍有不同 ID。堆叠时保留一个实例并调整数量，拆分时由 Server 生成新实例 ID。

固定 12 个槽位、同类物品最大堆叠取 Definition 配置。容量只按槽位判定。资产清单固定如下，后续不得用“先创建三个”省略正式内容：

```text
Weapons（复用第 3 周）
  Rifle: ID_ExtractionRifle -> WID_ExtractionRifle -> B_WeaponInstance_ExtractionRifle
  Shotgun: ID_ExtractionShotgun -> WID_ExtractionShotgun -> B_WeaponInstance_ExtractionShotgun
  AbilitySet/Fire/Reload/Damage/表现使用第 3 周记录的项目资产或 Lyra 复用路径

Support（不计入 8 种价值战利品）
  DA_Ammo_Rifle
  DA_Ammo_Shells
  DA_Item_Medkit

Valuable Tier 1
  DA_Valuable_ScrapMetal
  DA_Valuable_PowerCell
  DA_Valuable_CircuitBoard

Valuable Tier 2
  DA_Valuable_OpticModule
  DA_Valuable_ServoMotor
  DA_Valuable_EncryptedDrive

Valuable Tier 3
  DA_Valuable_ReactorCore
  DA_Valuable_PrototypeChip
```

第 3 周武器职责链是唯一命名来源，本周不得再改名或创建单一 Rifle/Shotgun 聚合资产。弹药是武器运行资源，不算作八种可结算 Valuable；若 Lyra 5.8 使用 Tag Stack/Cost 而不是独立 Ammo Definition，则 `DA_Ammo_*` 改为对应真实 Definition/Tag 映射，不创建空包装资产。

## 工作单元 2：物品资产与地面拾取

### 2.1 创建定义

按上面的完整清单创建或复用 ItemDefinition：两把武器引用第 3 周资产；新增两类弹药、一个 Medkit 和八个 Valuable。使用 Fragment 配置图标、显示名、最大堆叠、装备定义、价值档位或使用 Ability。不要为八种 Valuable 创建八套 C++ 类，也不要把显示名/图标复制到 Pickup Blueprint。

### 2.2 创建 WorldPickup

新增 Extraction Pickup Actor/组件，复制 `item_instance_id`、Definition、数量和是否可用；Server 创建实例，客户端只显示。交互请求只携带 `item_instance_id` 和 `request_id`。

Server 按顺序校验：玩家/Pawn 有效 → 非 Dead → 距离在阈值内 → Pickup 仍可用 → Inventory 有容量 → 原子地从世界移入背包。成功后销毁/禁用 Pickup 并复制结果。

错误码固定为：`ItemNotFound`、`InventoryFull`、`OutOfRange`、`AlreadyConsumed`、`InvalidState`。

## 工作单元 3：命令、版本和复制

统一从 Inventory Component 暴露意图接口：

```text
MoveItem(source_slot, target_slot, expected_version)
SplitStack(item_instance_id, quantity, target_slot, expected_version)
EquipItem(item_instance_id, expected_version)
UseItem(item_instance_id, expected_version)
DropItem(item_instance_id, quantity, expected_version)
```

每个请求经过拥有客户端 → Server 权威入口，验证 Ownership、槽位、数量、Definition 能力、当前状态和 `expected_version`。成功完成一次事务性内存变换后 `inventory_version += 1`；失败不修改状态，并返回错误码和当前版本。

实现顺序：Add/Pickup → Move/Swap → Merge → Split → Equip/Unequip → Medkit Use → Drop。每完成一个命令先写最小测试，再进入下一个。

复制策略：Owner 收到完整槽位和数量；其他客户端只收到外观所需的装备/地面 Pickup，不复制别人的完整背包。旧版本响应不得覆盖新版本状态。

## 工作单元 4：背包与装备 UI

创建 `W_ExtractionInventoryScreen`、槽位 Widget、装备槽、物品详情、容量文本、拾取提示和错误提示。

1. 打开界面时从 Inventory State 构建 ViewModel；
2. 拖拽只产生 Move/Equip Command，不修改数组；
3. 请求处理中显示 pending，但不假定成功；
4. Server 接受后用新版本状态更新；
5. 拒绝时撤销 pending 并显示对应中文信息；
6. 关闭/重开、角色重生后重新订阅状态。

输入路由必须阻止背包打开时同时开火；关闭后恢复 Gameplay 输入。Dedicated Server 不加载 Widget。

## 工作单元 5：多人、非法请求与恢复

执行固定矩阵：

1. A 正常拾取三类物品、移动、合并、拆分、装备、治疗、丢弃；
2. A/B 同时拾取同一个实例，仅一个成功，另一方收到 `AlreadyConsumed`；
3. 满背包、超距离、Dead、错误槽位、数量为 0/超量均被拒绝；
4. 用旧 `expected_version` 提交命令，返回当前版本且不覆盖新状态；
5. B 只能看到 A 的装备变化，看不到 A 完整背包；
6. A 关闭 HUD/背包再打开，状态完整恢复；
7. 100 ms 延迟和 5% 丢包下重复点击不会复制物品。

保存 Server 的 instance_id 转移日志，证明物品从 World 容器移动到 Inventory，再移动回 World，而不是复制出第二份。

## 验收目标

- [ ] Rifle、Ammo、Medkit 使用稳定 Definition；
- [ ] 每个 ItemInstance 有 Server 生成的唯一 ID；
- [ ] 支持拾取、移动、堆叠、拆分、装备、使用和丢弃；
- [ ] 16 槽容量限制生效；
- [ ] 命令统一带 expected_version 并返回确定错误码；
- [ ] 双人争抢只产生一个胜者；
- [ ] Owner-only 背包与公开装备边界正确；
- [ ] UI 重建后状态正确且不直接写数据。

## 实现原理

背包是唯一实例在容器之间的权威状态转移，不是 UI 数组操作。Definition 描述类型，Instance 描述具体对象，InventoryState 描述本局容器，PersistentStash 属于战局外 Backend。实例 ID、版本号和单一 Server 入口是后续重连与幂等结算的基础。

## 常见问题与停止条件

- 两人都拿到物品：Server 检查与转移不是同一个权威操作。
- UI 已移动但 Server 拒绝：UI 不得先改真实数组，只显示 pending。
- Fast Array 不更新：检查项脏标记、复制拥有者和回调。
- 重建 HUD 为空：状态被错误保存在 Widget，而非 Inventory Component。

出现复制物品、丢失实例 ID或版本回退时，不进入撤离循环。

## 本周作品集产出

- 物品/容器数据模型图；
- 双人争抢视频；
- Command 与错误码表；
- Owner-only 复制说明；
- UI 不直接改状态的技术笔记。

## 参考资料

- [Lyra Inventory and Equipment](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-inventory-and-equipment-in-unreal-engine)
- [Data Assets](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-assets-in-unreal-engine)
- [项目 Inventory 源码](../Source/LyraGame/Inventory)
