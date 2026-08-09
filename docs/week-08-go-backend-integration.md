# 第 8 周：Go 后台接入与对局控制面

## 本周目标

新增本地 Go Backend，用 HTTP + JSON + SQLite 完成开发态登录、玩家资料、房间、Match/Run、Join Ticket、Server Registry，并在 UE Client 和 Dedicated Server 中建立统一访问层。Backend 负责控制面和持久化，不处理移动、射击或实时 Tick。

## 前置条件与周门槛

- 第 7 周完整对局连续通过三次。
- 新增后端路径统一为 `<RepoRoot>\Backend`；UE 访问代码进入 ExtractionOps 插件。
- 使用 Go 标准 `net/http`、`database/sql` 和单个 SQLite 驱动；不加入 Web 框架、Redis、消息队列或云服务。
- 本地开发 Token 仍需不可伪造和有过期时间，但不实现第三方 OAuth。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 契约、目录和 SQLite 迁移 | 4 小时 |
| 2 | Session/Player/Room API | 4 小时 |
| 3 | Match/Run/Ticket/Server API | 4–5 小时 |
| 4 | UE Client 与 Server 访问层 | 3–4 小时 |
| 5 | 端到端与不可用实验 | 2–3 小时 |

## 先读什么

- UE `HTTP`、`Json`/`JsonUtilities` 模块和 `UGameInstanceSubsystem` 生命周期；
- Go `net/http`、`context`、`database/sql`、`log/slog`；
- SQLite foreign keys、transaction、unique index、WAL 和 busy timeout；
- 第 7 周的 match_id/run_id 来源和结果快照。

## 推荐架构

```text
UE Client --HTTP--> Go Backend --SQLite
UE Client --game connection--> Dedicated Server
Dedicated Server --HTTP--> Go Backend
```

Backend 是身份、房间、票据和持久化事实来源；Dedicated Server 是实时战局事实来源。

## 工作单元 1：接口契约和 SQLite

### 1.1 创建后端结构

计划新增：

```text
Backend/
  go.mod
  cmd/server/main.go
  internal/httpapi/
  internal/service/
  internal/store/sqlite/
  internal/domain/
  migrations/
  data/                 # Git 忽略
```

Handler 只解析/输出 HTTP；Service 执行业务规则；Store 使用参数化 SQL；domain 保存状态和错误码。请求使用 `context` 超时，禁止在 Handler 直接拼 SQL。

### 1.2 固定 API 外形

```text
POST /v1/session/login
GET  /v1/players/{player_id}
POST /v1/rooms
POST /v1/rooms/{room_id}/join
POST /v1/rooms/{room_id}/ready
POST /v1/matches
POST /v1/matches/{match_id}/join-ticket
POST /v1/servers/register
POST /v1/servers/{server_instance_id}/heartbeat
GET  /v1/runs/{run_id}
GET  /healthz
```

所有响应包含 `request_id`；错误统一为 `{request_id, error:{code,message,retryable}}`。客户端请求默认 5 秒，Server 心跳请求 2 秒。创建类请求使用 `Idempotency-Key`。

### 1.3 建表与连接设置

创建 `players`、`sessions`、`rooms`、`room_members`、`matches`、`runs`、`server_instances`、`join_tickets`、`idempotency_records`。主键使用字符串 UUID，外键显式声明，状态使用 CHECK 约束或集中常量。

每次连接初始化：`foreign_keys=ON`、WAL、`busy_timeout=5000`。设置合理连接池，写操作使用短事务；不要共享一个永不关闭的 transaction。

验证：空数据库启动自动/显式迁移成功；重复运行迁移无副作用；`PRAGMA foreign_key_check` 无结果；`/healthz` 返回 200。

## 工作单元 2：Session、Player 与 Room

### 2.1 开发态登录

`POST /v1/session/login` 接收本地 `display_name` 和固定开发凭据，创建/返回稳定 `player_id`、短期 session token 和过期时间。Token 至少签名并绑定 player_id/expiry；密钥来自环境变量，不提交仓库。

重复登录同一开发身份返回同一 player_id，但生成新 session。`GET /players/{id}` 只允许本人读取当前 MVP 资料。

### 2.2 房间流程

实现 Create、Join、Ready：房间容量固定 4，创建者为 leader，成员唯一约束 `(room_id, player_id)`。所有写入在 transaction 中，状态固定为 `Open -> Ready -> Matched/Closed`。

测试：重复 Idempotency-Key 返回原 room；第五人加入返回 `RoomFull`；重复加入返回现有成员关系；过期/伪造 token 返回 401；非成员 Ready 返回 403。

## 工作单元 3：Match、Run、Ticket 和 Server Registry

### 3.1 Match/Run

Room 全员 Ready 后创建一个 match，并为每个成员创建唯一 run。状态先使用 `Pending/Allocated/InGame/Completed/Failed` 与 `InProgress/Extracted/Dead/Abandoned`，禁止散落字符串。

### 3.2 Server Registry

Server 注册字段：`server_instance_id`、`build_version`、`map`、`mode`、`host`、`listen_port`、`capacity`、`status`、`last_heartbeat_at`。注册和心跳按 instance_id upsert/更新，不创建重复行。

### 3.3 Join Ticket

Ticket 绑定 `player_id`、`room_id`、`match_id`、`run_id`、`server_instance_id`、过期时间和一次/重连用途。第一版返回签名 token；Dedicated Server 调 Backend 或本地验证签名后建立连接身份，绝不相信客户端 payload 中单独的 player_id。

测试过期、错误 Server、错误 Match、非房间成员和篡改签名。

## 工作单元 4：UE 统一 Backend 访问层

在 ExtractionOps 中建立：

- `UExtractionBackendSubsystem`：生命周期、Session、业务调用入口；
- HTTP Client/Request 封装：Base URL、Headers、request_id、超时和取消；
- DTO：只负责 JSON，不直接作为 Gameplay Actor 状态；
- Error 映射：HTTP/网络错误转换为固定项目错误；
- Server 专用 Client：注册、心跳、Ticket 校验，不依赖 UI。

UI 只调用 `Login/CreateRoom/JoinRoom/Ready/RequestJoinTicket`，显示 Loading/Success/Error；离开页面取消请求，超时后不无限转圈。Token 仅保存在开发态内存或受控配置，不写日志。

Build.cs 只在本周加入 `HTTP`、`Json`、`JsonUtilities` 等真实需要的模块。

## 工作单元 5：端到端和后台不可用

固定流程：启动 Backend → 注册 Dedicated Server → A/B 登录 → 创建/加入 Room → Ready → 创建 Match/Run → 获取 Ticket → 两人连接 Server → 查询 Run。

故障矩阵：

- Backend 未启动/请求超时：UI 5 秒内结束 Loading，显示可重试错误；
- Login token 过期：401 并回到登录状态；
- 创建 Room 响应丢失：相同 Idempotency-Key 返回原结果；
- Ticket 过期/篡改：Server 拒绝连接并记录原因；
- Server 注册失败/心跳超时：Server 进入明确 NotRegistered/Degraded 日志状态；
- Backend 重启：SQLite 数据仍在，客户端可重新登录，已有 ID 不变。

## 验收目标

- [ ] Go Backend 和 SQLite 可从空目录启动；
- [ ] 外键、WAL、busy timeout、transaction 和唯一约束生效；
- [ ] 登录、玩家、房间、Match/Run、Ticket、注册/心跳 API 可用；
- [ ] 创建接口支持 Idempotency-Key；
- [ ] UE Client 通过 BackendSubsystem 调用，UI 不直接发 HTTP；
- [ ] Dedicated Server 不信任客户端声明身份；
- [ ] 每条链路有 request_id/player_id/match_id/run_id/server_instance_id；
- [ ] 超时、Token 错误和 Backend 重启都有确定行为；
- [ ] 日志不包含密钥和完整 Token。

## 实现原理

实时战局与控制面有不同时间尺度。Server 高频处理战斗，Backend 处理身份、房间、票据和持久化。稳定 ID 和短期 Ticket 把两个世界连接起来；request_id、错误码、超时和幂等让失败可定位、可重试。

## 常见问题与停止条件

- UI 直接拼 HTTP：立即收回 BackendSubsystem，否则后续无法统一超时和 Token。
- SQLite 出现 locked：缩短 transaction、设置 busy timeout/WAL，不用无限重试。
- 重试创建重复房间：Idempotency-Key 必须在同一事务保存请求与响应。
- Server 信任 player_id：必须验证 Ticket 绑定关系。

端到端身份链或重启持久化未通过时，不进入服务器分配。

## 本周作品集产出

- 架构图与 OpenAPI/接口契约；
- SQLite schema/迁移说明；
- 登录到进局时序图；
- Backend 不可用视频；
- Go 启动、迁移和本地复现 README。

## 参考资料

- [Unreal HTTP API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/HTTP)
- [Go net/http](https://pkg.go.dev/net/http)
- [Go database/sql](https://pkg.go.dev/database/sql)
