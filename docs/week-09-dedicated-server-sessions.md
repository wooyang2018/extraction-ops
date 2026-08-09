# 第 9 周：Dedicated Server、房间和对局会话

## 本周目标

把第 8 周的控制面真正连接到专用服务器，完成“后台选服/分配 -> Server 启动或复用 -> 玩家加入 -> 对局结束 -> 服务器回收”的最小闭环。

## 验收目标

- Dedicated Server 可以作为独立进程启动，不依赖编辑器；
- 后台可以注册和查询服务器实例；
- 4 名玩家可以被分配到同一个对局；
- 服务器启动时能加载指定地图和玩法配置；
- 玩家只能使用有效 Join Token 进入目标服务器；
- 对局结束后服务器向后台上报状态并回到可用或关闭状态；
- 服务器异常退出时，后台能识别心跳过期并标记异常；
- 能清楚解释 Listen Server、Dedicated Server 和后台控制面的职责差异。

## 操作步骤

### 1. 规范服务器生命周期

建议状态：

```text
Starting -> Registering -> Available -> Allocated
                                      -> InGame
                                      -> Draining
                                      -> Stopped
```

后台和服务器对状态的理解必须一致。服务器自身负责报告事实，后台负责控制分配和回收；不要让后台仅凭一个“创建请求成功”就认为服务器已经可用。

### 2. 编写启动参数约定

统一记录：

- 地图名；
- 模式名；
- 端口；
- 服务器实例 ID；
- 版本；
- 区域；
- 日志目录；
- 后台地址；
- 是否启用网络调试。

所有启动参数都能在 Server 日志中回显，但不要打印密钥和完整 Token。

### 3. 实现 Server Registry

服务器注册接口至少包含：

- `server_instance_id`；
- `build_version`；
- `map`；
- `mode`；
- `listen_port`；
- `capacity`；
- `status`；
- `last_heartbeat_at`。

服务器每隔固定时间发送心跳，后台根据心跳判断可用性。心跳请求重复是正常的，所以接口应使用服务器实例 ID 和时间戳更新，而不是每次创建新记录。

### 4. 实现最小分配器

第一版不做自动扩缩容，只做：

1. 查询状态为 `Available` 且容量足够的服务器；
2. 原子地标记为 `Allocated`；
3. 创建 `match_id`；
4. 生成玩家 Join Token；
5. 返回连接地址和过期时间；
6. 客户端连接并由服务器再次校验 Token。

把“寻找服务器”的逻辑抽象成 `ServerAllocator` 接口，为以后启动新进程或接入云服务留出空间。

### 5. 测试异常生命周期

测试：

- Server 启动后注册失败；
- 注册成功但心跳停止；
- 被分配后客户端没有连接；
- 一名玩家提前退出；
- 所有玩家都结束对局；
- Server 进程被杀死；
- 服务器版本与后台要求不一致；
- 满员服务器继续收到新 Join 请求。

## 实现原理

Dedicated Server 是对局权威进程，不等于业务后台。服务器拥有一局中的实时状态；后台保存房间、对局元数据、玩家身份和结算记录。通过 Server Registry 和短期 Join Token 建立控制面与数据面的连接。

“已分配”和“已进入”是两个不同状态。把它们混为一个状态，会产生玩家拿到地址但服务器未准备好、服务器空占、重复分配等问题。

## 常见问题

### 后台返回地址但玩家连不上

增加明确的 `Ready` 或 `Available` 状态，并检查端口、地图加载、版本和服务器健康状态。不要只依赖进程存在。

### 服务器重启后旧 Token 仍可使用

Token 必须绑定服务器实例、对局和过期时间。服务器启动后使用新的实例 ID 或版本号，旧 Token 默认失效。

### 多个请求同时分配同一服务器

后台需要使用数据库事务、乐观锁或原子更新保证容量分配。先查再写但没有并发保护是不够的。

## 本周作品集产出

- Server 生命周期状态图；
- 启动参数和部署说明；
- Join Token 校验时序图；
- 服务器异常退出后的后台状态截图；
- 一键启动 Client、Backend、Dedicated Server 的脚本。

## 参考资料

- [Setting Up Dedicated Servers](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers-in-unreal-engine)
- [Packaging Projects](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-unreal-engine-projects)
- [Command-Line Arguments](https://dev.epicgames.com/documentation/en-us/unreal-engine/command-line-arguments-in-unreal-engine)
- [Lyra with EOS](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-lyra-with-epic-online-services-in-unreal-engine)

