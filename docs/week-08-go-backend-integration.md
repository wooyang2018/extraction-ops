# 第 8 周：Go 后台接入与对局控制面

## 本周目标

新增本地 Go Backend，用 HTTP + JSON + SQLite 完成开发态登录、玩家资料、房间、Match/Run、Join Ticket、Server Registry，并在 UE Client 和 Dedicated Server 中建立统一访问层。Backend 负责控制面和持久化，不处理移动、射击或实时 Tick。

## 与总蓝图同步：Go 后台实施边界

只有第 7 周五分钟玩法闭环通过后才开始后台。第一版是一个 Go 进程和一个 SQLite 数据库，不拆微服务，不引入 Redis、消息队列、ORM、服务发现或容器编排。

先固定三条所有权：

```text
UE Client：登录、房间意图、请求 Join Ticket；不能提交权威奖励
Dedicated Server：实时战局事实、Run 终态与 SettlementEvent
Go Backend：身份、双人房间、Ticket、永久仓库与幂等结算事实
```

Gameplay DTO 与 HTTP DTO 必须分离。UE 内部的 `FExtractionRunSnapshot` 不直接序列化为 HTTP；定义明确的 Request/Response 类型，在 Backend Client 边界做转换，避免网络协议随 UPROPERTY 改名而变化。

推荐最小实施顺序：

1. `/healthz`、配置加载、结构化日志和 SQLite migration；
2. `POST /v1/session/login` 与 `GET /v1/players/{player_id}`；
3. 双人房间 create/join/ready，数据库约束容量为 2；
4. Match/Run 创建和一次性、短过期 Join Ticket；
5. Server register/heartbeat；
6. UE Client/Server 各自的薄 HTTP 访问层；
7. 最后才加入 settlement，事务细节在第 10 周完成。

每个 I/O 方法第一个参数为 `context.Context`，Handler 设置请求超时，Service 返回可分类的领域错误，HTTP 层统一映射状态码。不要为了“以后可能替换 SQLite”提前为每个结构体创建大接口；只在消费者确实需要替身测试时定义 1–3 个方法的小接口。

每完成一个端点都写三类测试：正常路径、领域拒绝、存储/超时失败。完成门槛不是 Postman 返回 200，而是重启 Backend 后状态仍正确、重复创建类请求返回同一结果、错误响应包含稳定 `request_id` 和机器可读 code。

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
POST /v1/matches/{match_id}/settlements
GET  /v1/runs/{run_id}
GET  /healthz
```

所有响应包含 `request_id`；错误统一为 `{request_id, error:{code,message,retryable}}`。客户端请求默认 5 秒，Server 心跳请求 2 秒。创建类请求使用 `Idempotency-Key`。

端点权限固定为：登录无需 Session；玩家/房间/Match/Ticket 使用 Client Bearer Session；register 使用仅部署环境持有的 Server Bootstrap Credential；register 成功后返回绑定本次 `server_instance_id` 的 Server Session/Fencing Token，heartbeat 和普通 Server 上报必须携带它；settlement 还需第 10 周定义的 Match Replay Token。Client Token 与 Server Token 不得互用。

### 1.3 建表与连接设置

创建 `players`、`sessions`、`rooms`、`room_members`、`matches`、`runs`、`server_instances`、`join_tickets`、`idempotency_records`。主键使用字符串 UUID，外键显式声明，状态使用 CHECK 约束或集中常量。`matches` 必须有“同一 room 同时至多一个非终态 match”的唯一约束；不能只靠 Service 先查再插入。

SQLite 驱动固定为纯 Go 的 `modernc.org/sqlite`。通过该驱动支持的 DSN `_pragma` 参数或逐连接 hook，保证池中每一条新连接都启用 `foreign_keys=ON` 和 `busy_timeout=5000`；WAL 在数据库初始化时设置并验证。不要只对 `*sql.DB` 执行一次 PRAGMA 后假设整个连接池生效。写操作使用短事务，不共享一个永不关闭的 transaction。

验证：空数据库启动自动/显式迁移成功；重复运行迁移无副作用；从连接池取得多条实际连接逐一检查 `foreign_keys=1` 和 busy timeout；`PRAGMA foreign_key_check` 无结果；`/healthz` 返回 200。

## 工作单元 2：Session、Player 与 Room

### 2.1 开发态登录

`POST /v1/session/login` 接收稳定且唯一的 `dev_account_id`、可变的 `display_name` 和固定开发凭据，创建/返回稳定 `player_id`、短期 session token 和过期时间。Token 至少签名并绑定 player_id/expiry；密钥来自环境变量，不提交仓库。`display_name` 只用于展示，不能决定玩家身份。

重复登录同一开发身份返回同一 player_id，但生成新 session。`GET /players/{id}` 只允许本人读取当前 MVP 资料。

### 2.2 房间流程

实现 Create、Join、Ready：房间容量固定 2，创建者为 leader，成员唯一约束 `(room_id, player_id)`。所有写入在 transaction 中，状态固定为 `Open -> Ready -> Matched/Closed`。Ready 只原子标记成员状态，不隐式创建 Match。

测试：重复 Idempotency-Key + 相同 body 返回原 room；同 key + 不同 body 返回 `IdempotencyMismatch`；第三人加入返回 `RoomFull`；重复加入返回现有成员关系；重复 Ready 不改变代次；过期/伪造 token 返回 401；非成员 Ready 返回 403。

## 工作单元 3：Match、Run、Ticket 和 Server Registry

### 3.1 Match/Run

`POST /v1/matches` 是唯一 Match 创建命令：只允许 Room leader 在两名成员均 Ready 时调用，请求体携带 `room_id`。同一事务中创建一个 match、为两个成员各创建唯一 run，并把 Room 关联到该 active match。相同 Idempotency-Key 返回原响应；不同 key 再次请求同一非终态 Room 返回已存在的同一 match，不创建第二局。状态使用 `Pending/Allocated/InGame/Completed/Failed` 与 `InProgress/Extracted/Dead/Abandoned`，禁止散落字符串。

并发测试让两个请求同时为同一 room 创建 Match，数据库中仍只能有一个非终态 match 和每位成员一个 run；失败请求读取并返回赢家结果，不留下孤立 run。

### 3.2 Server Registry

Server 每次进程启动生成新的 `server_instance_id`，注册字段包括 build、map、mode、host、port、capacity、status 和启动 nonce。register 用 Bootstrap Credential 认证并返回 Server Session/Fencing Token；heartbeat 只能更新这个实例的健康信息，不能通过客户端提交的 status 把 Backend 已分配或判定 Unhealthy 的实例改回 Available。

幂等记录键使用 `(actor_scope, route, idempotency_key)`，同时保存规范化请求的 `request_hash`、HTTP 状态码和响应体。同 scope/key 且 hash 相同才重放原响应；hash 不同返回 409 `IdempotencyMismatch`，不能误当成功重试。

### 3.3 Join Ticket

Ticket 包含唯一 `ticket_id`，并绑定 `player_id`、`room_id`、`match_id`、`run_id`、`server_instance_id`、fencing generation、`purpose=connect|reconnect`、签发时间和过期时间。第一版返回签名 token；Dedicated Server 本地验证签名和所有绑定字段，绝不相信客户端 payload 中单独的 player_id，也不在 PreLogin 中同步等待 Backend HTTP。

一次性语义由目标 Dedicated Server、仅服务器存在且不复制的 `TicketAdmissionRegistry` 权威执行；建议由本局 GameMode/专用服务对象持有，其生命周期覆盖 PreLogin 到对局结束。Client 每次 Travel 生成随机 `connection_attempt_id`，在同一次自动重试中复用并与 Ticket 一起发送；它只用于预留幂等关联，不能替代 Ticket 身份校验。

```text
Unseen -> Reserved(connection_attempt_id, reserved_until)
Reserved -> Consumed
Reserved -> Unseen（明确握手失败或 15 秒预留超时）
Consumed 为终态
```

`TryReserve` 在 Server Game Thread 或单个锁保护的临界区内原子执行。同一 `ticket_id + connection_attempt_id` 重试返回已有 Reservation；不同连接并发争抢同一 Ticket 时只允许一个进入 Reserved，其余返回 `TicketInUse`；Consumed 返回 `TicketConsumed`。只有 PostLogin 完成 PlayerState/Run 绑定后才转为 Consumed。PreLogin 后网络握手失败可以在 Reservation 释放/超时后使用同一 Ticket 重试，但不得超过 Ticket expiry。

Registry 还维护 `run_id -> active/reserved connection`，避免同一玩家拿两个不同但有效的 Ticket 建立两个活跃连接。Server 重启会生成新的 `server_instance_id`/generation，因此旧 Ticket 即使 Registry 丢失也无法在新进程使用。Reconnect 必须由 Backend 签发新的 `purpose=reconnect` Ticket，不复用首次 connect Ticket。

测试过期、错误 Server、错误 Match、非房间成员、篡改签名、同 Ticket 并发双开、两个不同 Ticket 竞争同一 run、握手失败后窗口内重试、预留超时后重试、Consumed 后再次使用。

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
