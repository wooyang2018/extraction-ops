# 第 9 周：Dedicated Server、房间与对局会话

## 本周目标

把第 8 周控制面接到 Editor Dedicated Process 生命周期：Backend 发现健康 Server、原子分配 Match、签发 Join Ticket、玩家连接、Server 上报 InGame/Completed，并在异常退出后由心跳过期识别。第一版只管理本机已有进程，不做自动扩缩容。

## 执行基线

开始前完整阅读[12 周执行基线](execution-baseline.md)。本周使用 `D:\Software\UE_5.8` 的 Editor Dedicated Process 和 Editor Client Process；不得访问受保护的 ue5-main 源码目录。本周验证控制面和进程生命周期，不声称完成 Packaged Dedicated Server。

## 与当前工程同步：从直连证据升级到后台会话

第 4 周应已经证明“Editor Dedicated Process + 两个直连 Client”的实时玩法一致性。本周只增加身份、房间、分配和 Ticket，不重新设计射击/撤离复制。不要用 Listen Server 或同进程 PIE 截图代替三个独立进程验收。

实施时按以下纵向切片推进：

1. **静态直连**：一个 Editor Dedicated Process、两个 Editor Client Process、固定地址，三个独立进程能完成一局；
2. **注册心跳**：Server 启动后注册，Backend 只把有新鲜心跳的实例视为可用；
3. **双人房间分配**：两名 Ready 玩家触发 Match，SQLite 事务原子占用一个 Server；
4. **Ticket 进局**：Ticket 绑定七个稳定 ID 和 build version，Server 在 PreLogin 阶段校验；
5. **生命周期收尾**：一局一进程，Completed 后停止，不复用可能残留状态的 World。

必须保存一次完整时序日志：`room_id -> match_id -> server_instance_id -> player_id/run_id -> ticket -> connection -> InGame -> Completed`。任何一步失败都应有稳定错误码和明确的可重试性，不能只返回连接失败。

## 前置条件与周门槛

- 第 8 周登录到 Ticket 的身份链和 SQLite 重启持久化已通过。
- 第 4 周三个独立进程网络证据和第 7 周完整对局已经通过。
- Backend、Server、Client 的 `build_version` 必须一致，格式固定为 `<project_commit>+ue5.8-<installed_build_version>`；不得读取引擎源码 commit。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 三进程启动参数与 Backend 接线 | 4 小时 |
| 2 | Server 状态机、注册和心跳 | 3–4 小时 |
| 3 | SQLite 原子分配器 | 4 小时 |
| 4 | Ticket 校验与进局 | 3–4 小时 |
| 5 | 生命周期故障和一键启动 | 2–4 小时 |

## 先读什么

- [12 周执行基线](execution-baseline.md)和第 1/4 周三进程启动脚本；
- `GameModes/LyraGameMode.*` 的登录入口（PreLogin/Login/PostLogin）；
- Unreal Command-Line Arguments、PreLogin/Login/PostLogin；
- 第 8 周 `server_instances`、`matches`、`join_tickets` schema。

## 工作单元 1：Editor 独立进程与参数契约

使用 `D:\Software\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe` 启动 `L_ExtractionTest` 的 Editor Dedicated Process，并用两个 `-game` Editor Client Process 连接。Server 使用 `-NullRHI -unattended -NoSound`，不得创建本地玩家或客户端 UI。

固定 Server 参数：

```text
map、experience、port、build_version、region=local、startup_nonce、
backend_url、log_dir、network_debug
```

`server_instance_id` 不从命令行传入；Server 每次进程启动自行生成全新 ID。启动日志逐项回显非敏感参数；完整 Token/密钥禁止打印。缺少 backend_url、build_version 或 startup_nonce 时启动失败并给明确错误，不用默认空字符串注册。

成功信号：Server 独立进程加载地图并监听指定端口，两个 `-game` 客户端可直连；Editor UI 不需要保持打开。限制必须记录：进程仍来自 UnrealEditor Installed Build，不是发布级 Server 包。

## 工作单元 2：Server 生命周期、注册和心跳

Server Instance 与 Match 使用两套状态，不共用一个枚举：

```text
Server：Starting -> Registering -> Available -> Allocated -> InGame
                                                  -> Draining -> Stopped
        任意非终态 -> Unhealthy

Match：Pending -> Allocated -> InGame -> Completed
                                  \-> Failed
```

Server 每次进程启动生成全新的 `server_instance_id`；注册成功得到 fencing token。完成地图/Experience 后才申请进入 Available；每 5 秒心跳一次，携带 fencing token、当前玩家数和 match_id。连续失败记录 Degraded，但短暂 Backend 故障不立刻杀进程。Backend 以 15 秒无心跳标记 Unhealthy，不能仅凭进程创建请求判定 Available。

状态转换由 Backend Service 层校验。Heartbeat 只报告观测值，不能自行推进分配状态或把 Unhealthy/Allocated 写回 Available；fencing token 不匹配的旧心跳直接拒绝。对局结束时 Match 进入 Completed/Failed，Server 独立进入 Draining -> Stopped；不允许 Server `InGame -> Available` 复用本局 World。重复 register 仅对同一启动 nonce 幂等，instance/build/端口冲突必须拒绝。

## 工作单元 3：最小原子分配器

定义 `ServerAllocator` 接口，首个实现为 SQLite allocator：

1. 开启短 transaction；
2. 查询 `Available`、版本/地图/模式匹配且容量足够的 Server；
3. 使用带旧状态条件的 UPDATE 将其改为 Allocated 并绑定 match_id；
4. 检查 rows affected=1；否则回滚并重试另一实例，最多 3 次；
5. 使用 `WHERE status='Pending'` 更新 Match 为 Allocated；
6. 为成员创建短期 Join Ticket；
7. 提交后返回地址、端口、match_id/run_id、expiry。

没有可用 Server 返回可重试 `NoCapacity`，不创建虚假的地址。SQLite busy 超过 5 秒返回 `AllocationBusy`，不无限阻塞。

并发测试：对同一个 Available Server 同时发 10 个分配请求，只允许一个 Match 获得它；其余返回已有幂等结果或 NoCapacity。

## 工作单元 4：Join Ticket 和 Server 连接

Ticket 绑定 ticket_id、player_id、room_id、match_id、run_id、server_instance_id、该实例 fencing generation、build_version、purpose 和 expiry。Client 将 Ticket 放入受控连接参数；Server 在 PreLogin 中完成本地签名/绑定校验，并通过 `TicketAdmissionRegistry::TryReserve` 原子预留。只有 PostLogin 成功绑定 PlayerState/Run 后才 `Consume`。进程重启后的新 instance/generation 必须拒绝旧 Ticket。

校验顺序：解析 → 签名 → 过期 → Server ID/generation/版本 → Match 状态 → 玩家成员关系 → purpose → run 是否已有活跃/预留连接 → Ticket 原子预留 → 容量。失败返回安全的公开错误码，详细原因只记 Server 日志。容量或后续握手失败必须释放 Reservation，无法确认的断开由 15 秒预留超时回收；Consumed Ticket 永不回到可用状态。

两名本地开发玩家进入同一 Match；Server 由 `Allocated -> InGame`，Match/Run 更新；玩家离开与对局结束上报 Backend。完成后 Server 进入 Draining，清理地图状态，再按本地单局进程策略选择 Stopped；本计划默认一局一进程，结束后关闭，避免残留状态复用。

## 工作单元 5：故障与启动脚本

规划并实现 `Scripts/launch-backend.ps1`、`launch-server.ps1`、`launch-client.ps1` 和总控脚本。脚本使用参数传路径和端口，不写死个人绝对路径；分别保存 PID 和日志，关闭时只终止自己启动的进程。

故障矩阵：注册失败、心跳停止、旧 fencing token 继续心跳、Allocated 后无人连接、部分玩家退出、Server 被杀、版本不匹配、满员后新 Join、Ticket 用于错误实例、同 Ticket 并发双开、握手失败后重试、两个 Ticket 竞争同一 run、全部玩家完成。

对每项检查 Backend 状态、Server 日志、Client 提示和可恢复动作。Server 被杀后 15 秒左右标记 Unhealthy，Match 标记 Failed/NeedsRecovery，而不是继续返回旧地址。

## 验收目标

- [ ] Editor Dedicated Process 与两个 Editor Client Process 作为三个独立进程运行；
- [ ] 参数契约完整且日志不泄密；
- [ ] 注册、5 秒心跳和 15 秒过期生效；
- [ ] Server 状态转换受控；
- [ ] SQLite 并发分配不会把同一实例分给两个 Match；
- [ ] 两名玩家使用有效 Ticket 进入同一局；
- [ ] 过期、篡改、错误实例/版本和满员 Ticket 被拒绝；
- [ ] Ticket 预留/消费原子化，同 Ticket 或同 run 并发连接只有一个赢家；
- [ ] 握手失败只在预留释放/超时且 Ticket 未过期时允许重试；
- [ ] 一局结束后进程 Draining 并关闭；
- [ ] 一键脚本可启动整套本地环境并安全清理自身进程。
- [ ] 证据明确披露当前没有 `LyraServer.exe`、Server Cook/Stage/Package。

## 实现原理

Dedicated Server 是数据面权威，Backend 是控制面。Available 表示 Server 已加载且能接人，Allocated 表示已绑定 Match，InGame 表示对局实际开始；混淆这些状态会产生空占、重复分配和无效地址。条件更新与 transaction 让 SQLite 中的分配具有单一赢家。

## 常见问题与停止条件

- 返回地址但连不上：检查 Available 的就绪条件、地图、Experience、端口和版本。
- 两 Match 分到同一 Server：UPDATE 必须包含旧状态并检查 rows affected。
- 重启后旧 Ticket 可用：Ticket 必须绑定新的 instance_id 和 expiry。
- 脚本误杀其他进程：只使用自己记录的 PID，不按进程名全杀。

并发分配或 Ticket 身份链未通过时，不做重连。

## 本周作品集产出

- Server 生命周期图；
- 启动参数/部署说明；
- Ticket 校验时序图；
- 异常退出状态截图；
- 一键启动脚本演示。

## 参考资料

- [Setting Up Dedicated Servers](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers-in-unreal-engine)
- [Packaging Projects](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-unreal-engine-projects)
- [Command-Line Arguments](https://dev.epicgames.com/documentation/en-us/unreal-engine/command-line-arguments-in-unreal-engine)
- [12 周执行基线](execution-baseline.md)
