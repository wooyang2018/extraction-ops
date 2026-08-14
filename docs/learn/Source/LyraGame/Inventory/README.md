# Source/LyraGame/Inventory

## Definition、Fragment、Instance、Entry

- `ULyraInventoryItemDefinition`：静态物品类型，组合多个 Fragment。
- `ULyraInventoryItemFragment`：可扩展静态能力片段，如初始 Stat、图标、可装备定义。
- `ULyraInventoryItemInstance`：运行时 UObject，持有 Definition 与 GameplayTagStack。
- `FLyraInventoryEntry/List`：服务端集合与 FastArray 复制协议。
- `ULyraInventoryManagerComponent`：Authority API、查询和子对象复制。

服务端 AddItemDefinition 创建 Instance，依次调用 Fragment `OnInstanceCreated`，加入 FastArray 并 `MarkItemDirty`。客户端的 FastArray add/change/remove 回调计算变化并广播 GameplayMessage；消息是 UI 增量通知，真实状态仍是复制集合。

## 子对象复制

FastArray 只复制 Entry 的结构字段；ItemInstance 自身是 UObject，需要 Registered SubObject List 或旧 `ReplicateSubobjects` 路径复制其属性。只实现 FastArray 而不注册 Instance，会得到“列表里有指针语义，但对象状态不完整”的系统。

## 当前源码的示例级缺口

- `FLyraInventoryList::AddEntry(ULyraInventoryItemInstance*)` 当前调用 `unimplemented()`。
- `GetTotalItemCountByDefinition` 按 Entry 加一，没有累计 `StackCount`。
- `CanAddItemDefinition` 基本是扩展钩子，未形成重量、容量或互斥规则。

因此当前实现更适合作为 FastArray/Fragment/Instance 架构样板，不能直接声称是完整生产背包。

## 面试追问

1. FastArray 与复制 ItemInstance 子对象分别解决什么？
2. Fragment 为什么适合静态能力组合，却不适合保存每件物品的可变耐久？
3. StackCount 与 TagStack 的语义怎样区分？
4. 丢包/迟到加入时 GameplayMessage 能否替代当前 Inventory 状态？

## 练习

补一个只读测试：服务端加入两种物品，验证客户端 FastArray、ItemInstance Definition 和 Stat TagStack；再让迟到客户端加入，确认它不依赖历史消息恢复状态。

