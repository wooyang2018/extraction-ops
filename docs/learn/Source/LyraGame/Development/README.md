# Source/LyraGame/Development

DeveloperSettings 汇总仅开发期或可配置的体验覆盖、自动登录、Bot/平台模拟等选项；PlatformEmulationSettings 用 Trait/设备 Profile 模拟平台能力；BotCheats 提供调试入口。

这些设施必须区分 Editor/Development 与 Shipping。开发覆盖不应成为生产规则真相，平台模拟也不等于真实设备验证。

## 面试追问

1. DeveloperSettings 与普通 GameUserSettings 生命周期有何不同？
2. 平台 Trait 模拟能验证哪些逻辑，不能验证哪些硬件行为？
3. Cheat 入口怎样在 Shipping 从编译和运行两层隔离？

