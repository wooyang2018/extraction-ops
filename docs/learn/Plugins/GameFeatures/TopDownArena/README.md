# Plugins/GameFeatures/TopDownArena

TopDownArena 是对 Lyra 扩展性的反证测试：它不修改 LyraCharacter 核心，就提供顶视 CameraMode、自定义 AttributeSet、由属性/Tag 驱动的 MovementComponent 和 Pickup UI 数据。

AttributeSet 同时在 PreAttributeBaseChange/PreAttributeChange 做 Clamp，覆盖 base/current 两类修改；RepNotify Always 让 GAS 聚合/预测通知保持一致。Movement 从 ASC 读取速度等属性，Tag 负责状态开关，数值属性负责程度，两者语义不同。

具体 Experience/GameFeatureData/资产组合位于 Content，超出本次范围，所以只能证明 C++ 扩展点，不能断言实际资产配置。

## 面试追问

1. BaseValue 与 CurrentValue 的 Clamp 为什么需两条入口？
2. Movement 为什么消费 ASC 属性而不自建 replicated float？
3. StopMovement Tag 与速度为零有何差异？
4. 这个插件怎样证明 Lyra 宿主没有硬编码 Shooter？

