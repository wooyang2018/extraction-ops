# 第 8 周：Go 后台接入与对局控制面

## 本周目标

把客户端、Dedicated Server 和你熟悉的 Go 后台连起来，完成登录、玩家资料、房间、进入对局和基础心跳。后台负责控制面和持久化，不承担实时战斗 Tick。

## 推荐架构

```text
UE Client
  | HTTP/WebSocket
  v
Go Control Plane
  |-- Account / Profile
  |-- Lobby / Party
  |-- Match Ticket
  |-- Settlement Intake
  |-- Server Registry
  |
  +-- PostgreSQL or SQLite for MVP
  +-- Redis optional, not required for first version

UE Client -- game connection --> Dedicated Server
Dedicated Server -- HTTP/gRPC --> Go Backend
```

## 验收目标

- 客户端可以登录并获得短期会话令牌；
- 可以创建房间、加入房间和查看房间成员；
- Dedicated Server 能向后台注册实例；
- 客户端能获取加入对局所需的短期 Join Token；
- Dedicated Server 能校验玩家和房间关系；
- 后台关闭或不可用时，客户端收到可理解的错误状态；
- 所有请求有 `request_id`、超时、错误码和结构化日志；
- 能从日志还原“玩家如何进入一局游戏”。

## 操作步骤

### 1. 先写接口契约

建议 MVP 接口：

```text
POST /v1/session/login
GET  /v1/players/{player_id}
POST /v1/rooms
POST /v1/rooms/{room_id}/join
POST /v1/rooms/{room_id}/ready
POST /v1/matches/{match_id}/join-ticket
POST /v1/servers/register
POST /v1/servers/{server_id}/heartbeat
GET  /v1/runs/{run_id}
```

每个接口写清：

- 请求字段；
- 响应字段；
- 认证方式；
- 超时；
- 幂等键；
- 可重试错误；
- 不可重试错误；
- 数据由谁最终负责。

### 2. 先用本地数据库完成控制面

不要一开始部署复杂云服务。用 SQLite 或 PostgreSQL 建立最小表：

- `players`；
- `sessions`；
- `rooms`；
- `room_members`；
- `matches`；
- `server_instances`；
- `join_tickets`；
- `runs`。

所有表都带创建时间和更新时间。状态枚举不要用散落的字符串。

### 3. 客户端封装后台访问

建立一个客户端侧 Backend Subsystem 或 Service Layer，负责：

- 请求构造；
- Token 保存和刷新；
- 超时和取消；
- JSON 编解码；
- 错误码转换；
- 断网时的重试策略；
- 请求日志和 `request_id`。

UI 不应直接调用 HTTP。UI 调用业务接口，业务接口再调用 Backend Service。

### 4. 做服务器注册和 Join Token

Dedicated Server 启动后向后台注册：

- `server_instance_id`；
- 版本；
- 地区；
- 端口；
- 当前状态；
- 当前玩家数；
- 最近心跳时间。

后台为玩家生成短期 Join Token，至少绑定：

- `player_id`；
- `room_id`；
- `match_id`；
- `server_instance_id`；
- 过期时间；
- 签名或不可伪造校验信息。

Dedicated Server 通过后台校验 Token，不接受客户端直接声称“我属于这个房间”。

### 5. 做后台不可用实验

依次模拟：

- 登录请求超时；
- 房间创建失败；
- Join Token 过期；
- Dedicated Server 注册失败；
- 心跳连续失败；
- 后台短暂重启。

每个场景都要有用户可理解的 UI 状态，不能停在无限加载。

## 实现原理

游戏实时对局和业务控制面有不同的时间尺度。Dedicated Server 需要高频处理移动、战斗和交互；Go 后台更适合处理账号、房间、票据、持久化和结算。两者通过明确的 ID 和短期凭证连接，而不是共享所有内存状态。

Join Token 的作用是把“玩家可以连接哪个对局”变成服务器可验证的事实。它不是永久账号 Token，也不是客户端自行生成的字符串。

## 常见问题

### 客户端每次启动都重复登录

区分会话 Token、刷新 Token 和 Join Token。Join Token 只服务于一局或一次连接，不要混用。

### 后台接口重试导致创建多个房间

创建类请求必须支持幂等键。客户端超时后重试，后台应返回原来的结果，而不是重复创建资源。

### Dedicated Server 直接相信客户端上报的玩家 ID

服务端应从连接和 Join Token 中建立玩家身份，不应直接相信客户端 payload 中的身份字段。

## 本周作品集产出

- Client/Backend/Dedicated Server 架构图；
- OpenAPI 或等价接口契约；
- 登录到进局的时序图；
- 一段后台不可用时客户端正确失败的视频；
- Go 服务的启动、迁移和本地复现 README。

## 参考资料

- [Unreal Online Services](https://dev.epicgames.com/documentation/en-us/unreal-engine/online-services-in-unreal-engine)
- [Using Lyra with Epic Online Services](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-lyra-with-epic-online-services-in-unreal-engine)
- [HTTP Module](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/HTTP)
- [JSON Serialization](https://dev.epicgames.com/documentation/en-us/unreal-engine/json-utilities-in-unreal-engine)

