# 第 3 周：客户端输入、角色、武器与 HUD

## 本周目标

做出第一条有操作感的客户端战斗回路：进入地图、移动、瞄准、射击、换弹、命中反馈、显示弹药和生命值。本周先把“能玩”做出来，下一周再把网络边界做严谨。

## 验收目标

- 能使用 Enhanced Input 完成移动、视角、跳跃、瞄准、射击、换弹；
- 武器、弹药、准星、生命值和命中反馈可见；
- 武器配置使用 Data Asset 或数据驱动方式，不把数值散落在 Blueprint 节点中；
- 核心行为用 C++ 编写，表现层用 Blueprint/动画/特效组合；
- PIE 单机模式下完成“拾枪 -> 射击 -> 换弹 -> 受伤 -> 死亡”的流程；
- 能解释本地输入、角色移动、武器逻辑和 HUD 的数据流。

## 操作步骤

### 1. 先复用 Lyra 的输入和角色

先确认 Lyra 现有输入映射、输入标签和角色类。不要同时重写移动系统、动画系统和武器系统。先找到可扩展点，再替换最小行为。

记录：

- Input Action 资产；
- Input Mapping Context；
- 角色绑定输入的入口；
- 武器激活和切换入口；
- HUD 的创建和数据绑定方式。

### 2. 定义客户端领域对象

建议建立以下最小类型，具体命名可以按现有项目规范调整：

- `UExtractionWeaponData`：射速、弹匣容量、换弹时间、伤害类型等配置；
- `UExtractionWeaponInstance`：当前武器实例和运行时状态；
- `UExtractionCombatComponent`：当前装备、射击输入、换弹状态；
- `UExtractionHUDViewModel`：把游戏状态转换为 UI 可消费的数据；
- `WBP_ExtractionHUD`：只负责展示，不直接修改服务器状态。

### 3. 实现输入状态机

至少明确以下状态：

```text
Idle -> Aiming -> Firing
Idle -> Reloading -> Idle
Firing -> Reloading
任何状态 -> Dead
```

禁止在多个 Blueprint 里分别维护“是否换弹”“是否射击”“是否死亡”。这些状态要集中管理，并通过 Gameplay Tag、枚举或明确的组件状态表达。

### 4. 实现命中反馈

先实现本地表现：

- 准星扩散；
- 枪口火焰；
- 后坐力；
- 命中音效；
- 受击数字或方向提示；
- 弹药减少；
- 换弹动画。

本周可以暂时使用本地射线检测，但要在代码中明确标记：真实版本必须由服务器确认命中和伤害。不要把本地命中结果直接当成可信结果。

### 5. 做基础 HUD

HUD 至少显示：

- 当前武器；
- 当前弹匣/备用弹药；
- 当前生命值；
- 当前网络模式；
- 调试状态：本地角色和服务器角色。

把 HUD 当作状态的消费者，而不是游戏逻辑的持有者。UI 关闭或重建时，游戏状态不能丢失。

## 实现原理

客户端玩法通常由输入、预测、表现和最终状态四层组成。输入层说明玩家意图；预测层让操作即时响应；表现层播放动画和特效；服务器确认后再修正状态。把这四层混在一起，会导致单机看起来正确、联机后出现双发、重复扣弹或状态回滚。

数据驱动的意义是把“武器是什么”与“武器怎么运行”分开。调整武器参数时不需要复制一套 Blueprint；以后接后台配置或活动武器也更容易。

## 常见问题

### 射击按一下却触发多次

检查 Input Action 的触发事件、自动射击计时器和 Ability/Component 状态是否重复响应。给每次射击增加本地 shot id，日志中打印输入、触发和发射三个事件。

### 换弹过程中仍然能射击

把 `Reloading` 作为明确的阻断状态，而不是在多个地方通过布尔值临时判断。Gameplay Tags 或集中式状态机更容易维护。

### UI 显示不更新

不要在 Tick 中无条件刷新整个 HUD。使用事件、属性变化回调或轻量 ViewModel。先确认数据源变化，再确认 UI 绑定是否有效。

## 本周作品集产出

- 一段 1 分钟可操作战斗视频；
- 输入到 HUD 的数据流图；
- 武器数据驱动示例；
- “客户端命中不是最终可信结果”的设计说明；
- 至少一个可复用的 C++ Combat Component。

## 参考资料

- [Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)
- [Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine)
- [Lyra Sample Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)
- [Common UI](https://dev.epicgames.com/documentation/en-us/unreal-engine/common-ui-plugin-for-advanced-user-interfaces)

