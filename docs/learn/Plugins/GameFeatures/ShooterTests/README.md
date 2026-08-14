# Plugins/GameFeatures/ShooterTests

ShooterTests 使用 CQTest 构建地图、输入、动画和多人 PIE 测试。Helper 封装 Actor 查找、输入注入与动画检查；NetworkComponent 将步骤明确分到 Server/Client；latent `StartWhen/Until` 等待跨 Tick 条件。

测试源码存在不等于当前 Build 一定执行：还受 `WITH_AUTOMATION_TESTS`、ENABLE 宏、资产路径和运行拓扑影响。PIE 网络测试不能替代 Packaged Dedicated Server/Shipping 证据。

## 面试追问

1. Do/Then 与 StartWhen/Until 的执行模型有何不同？
2. 固定 Delay 为什么容易产生假阴性？
3. 多人动画测试如何保证命令在正确网络端执行？
4. 哪些结果仍需 packaged server 复验？

