# Source/LyraGame/Tests

BootTest Controller 等待项目启动条件；StartElimination 测试驱动特定体验；GameplayRpcRegistrationComponent 在非 Shipping 通过 External RPC/HTTP 暴露测试动作与状态读取；Spec 文件组织自动化断言。

这些测试证明的是对应环境中的启动/PIE/功能路径，不自动等价于 Packaged Dedicated Server、Shipping 或真实网络条件。Build.cs 在 Shipping 移除 RPC/HTTP listener，是测试攻击面隔离的重要证据。

## 面试追问

1. Until 型等待为什么比固定 Delay 更可靠，仍可能怎样永久等待？
2. 测试 HTTP 接口如何确保不进入 Shipping？
3. PIE 多人通过后还需要哪些 packaged/network emulation 证据？

