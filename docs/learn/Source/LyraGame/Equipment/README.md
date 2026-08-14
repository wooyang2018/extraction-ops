# Source/LyraGame/Equipment

## Inventory 到 Equipment

Inventory 表示“拥有”，Equipment 表示“当前装备并生效”。`InventoryFragment_EquippableItem` 从 ItemDefinition 指向 `ULyraEquipmentDefinition`；EquipmentManager 在 Authority 创建 `ULyraEquipmentInstance`，授予临时 AbilitySets、生成附属 Actor，并通过 FastArray/子对象复制到客户端。

卸装必须按授予句柄撤销 AbilitySets、销毁附属 Actor、执行 OnUnequipped 并移除复制子对象。EquipmentInstance 作为 AbilitySpec SourceObject，使 Ability 能反查是哪件装备授予自己。

## QuickBar

QuickBar 是 Controller 上的选择器：Slots 引用 InventoryItemInstance，ActiveSlotIndex 复制；切换槽位通过 Server RPC，服务端先卸旧装备再装新装备，OnRep/手动通知向本地 UI 广播消息。

源码中的 `AddItemToSlot`/`RemoveItemFromSlot` 没有完整 RPC/Authority 防护，契约依赖调用者在服务端执行。生产代码应在边界处显式 `HasAuthority`/Server RPC 校验。

## 面试追问

1. 为什么 Equipment AbilitySet 必须保存 GrantedHandles，而 PawnData 的长期 AbilitySet 可以不保存？
2. EquipmentInstance 为何既要进 FastArray又要作为 replicated subobject？
3. QuickBar 消息为什么不能成为 ActiveSlot 的真相源？
4. 装备 Actor 应由客户端预测生成还是等待 Authority 复制？如何隐藏延迟？

## 练习

连续执行装备、切换、卸装、死亡重生，检查 AbilitySpec、附属 Actor 和 FastArray Entry 数量，确保每条创建路径都有对称销毁。

