# 第 9 周：Dedicated Server、房间与对局会话

## 本周目标

把第 8 周控制面接到真实专服生命周期：Backend 发现健康 Server、原子分配 Match、签发 Join Ticket、玩家连接、Server 上报 InGame/Completed，并在异常退出后由心跳过期识别。第一版只管理本机已有进程，不做自动扩缩容。

## 前置条件与周门槛

- 第 8 周登录到 Ticket 的身份链和 SQLite 重启持久化已通过。
- 能构建/运行无 Editor 依赖的 Development Server；若 Cook 尚未完成，本周先完成 Cook/package。
- Backend、Server、Client 的 `build_version` 必须一致。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 打包 Server/Client 与启动参数 | 4 小时 |
| 2 | Server 状态机、注册和心跳 | 3–4 小时 |
| 3 | SQLite 原子分配器 | 4 小时 |
| 4 | Ticket 校验与进局 | 3–4 小时 |
| 5 | 生命周期故障和一键启动 | 2–4 小时 |

## 先读什么

- `Source/LyraServer.Target.cs`、BuildGraph/打包脚本和目标地图 Cook 配置；
- `GameModes/LyraGameMode.*` 的登录入口（PreLogin/Login/PostLogin）；
- Command-Line Arguments 与 Dedicated Server packaging；
- 第 8 周 `server_instances`、`matches`、`join_tickets` schema。

## 工作单元 1：独立进程与参数契约

Cook/package `L_ExtractionTest`、Extraction Experience 与全部引用资产，分别生成 Development Client/Server。先从命令行直接启动，不依赖 Editor。

固定 Server 参数：

```text
map、experience、port、server_instance_id、build_version、region=local、
backend_url、log_dir、network_debug
```

启动日志逐项回显非敏感参数；完整 Token/密钥禁止打印。缺少 instance_id、backend_url 或 build_version 时启动失败并给明确错误，不用默认空字符串注册。

成功信号：Server 独立进程加载地图并监听指定端口，Client 打包进程可直连；关闭 Editor 不影响运行。

## 工作单元 2：Server 生命周期、注册和心跳

Backend 与 Server 共用状态：

```text
Starting -> Registering -> Available -> Allocated -> InGame
                                      -> Draining -> Stopped
任意非终态 -> Unhealthy
```

Server 启动完成地图/Experience 后才注册 Available；每 5 秒心跳一次，携带状态、当前玩家数和 match_id。连续失败记录 Degraded，但短暂 Backend 故障不立刻杀进程。Backend 以 15 秒无心跳标记 Unhealthy，不能仅凭进程创建请求判定 Available。

状态转换由 Service 层校验；不允许 `InGame -> Available` 跳过 Completed/Draining 清理。注册使用 instance_id 幂等 upsert，但 build_version/端口冲突必须拒绝。

## 工作单元 3：最小原子分配器

定义 `ServerAllocator` 接口，首个实现为 SQLite allocator：

1. 开启短 transaction；
2. 查询 `Available`、版本/地图/模式匹配且容量足够的 Server；
3. 使用带旧状态条件的 UPDATE 将其改为 Allocated 并绑定 match_id；
4. 检查 rows affected=1；否则回滚并重试另一实例，最多 3 次；
5. 更新 Match 为 Allocated；
6. 为成员创建短期 Join Ticket；
7. 提交后返回地址、端口、match_id/run_id、expiry。

没有可用 Server 返回可重试 `NoCapacity`，不创建虚假的地址。SQLite busy 超过 5 秒返回 `AllocationBusy`，不无限阻塞。

并发测试：对同一个 Available Server 同时发 10 个分配请求，只允许一个 Match 获得它；其余返回已有幂等结果或 NoCapacity。

## 工作单元 4：Join Ticket 和 Server 连接

Ticket 绑定 player_id、room_id、match_id、run_id、server_instance_id、build_version、用途和 expiry。Client 将 Ticket 放入受控连接参数；Server 在 PreLogin/Login 阶段验证签名/Backend 关系，然后把稳定 ID 写入 PlayerState/连接上下文。

校验顺序：解析 → 签名 → 过期 → Server ID/版本 → Match 状态 → 玩家成员关系 → Ticket 使用策略 → 容量。失败返回安全的公开错误码，详细原因只记 Server 日志。

四名本地开发玩家进入同一 Match；Server 由 `Allocated -> InGame`，Match/Run 更新；玩家离开与对局结束上报 Backend。完成后 Server 进入 Draining，清理地图状态，再按本地单局进程策略选择 Stopped；本计划默认一局一进程，结束后关闭，避免残留状态复用。

## 工作单元 5：故障与启动脚本

规划并实现 `Scripts/launch-backend.ps1`、`launch-server.ps1`、`launch-client.ps1` 和总控脚本。脚本使用参数传路径和端口，不写死个人绝对路径；分别保存 PID 和日志，关闭时只终止自己启动的进程。

故障矩阵：注册失败、心跳停止、Allocated 后无人连接、部分玩家退出、Server 被杀、版本不匹配、满员后新 Join、Ticket 用于错误实例、全部玩家完成。

对每项检查 Backend 状态、Server 日志、Client 提示和可恢复动作。Server 被杀后 15 秒左右标记 Unhealthy，Match 标记 Failed/NeedsRecovery，而不是继续返回旧地址。

## 验收目标

- [ ] 独立打包 Server/Client 不依赖 Editor；
- [ ] 参数契约完整且日志不泄密；
- [ ] 注册、5 秒心跳和 15 秒过期生效；
- [ ] Server 状态转换受控；
- [ ] SQLite 并发分配不会把同一实例分给两个 Match；
- [ ] 四名玩家使用有效 Ticket 进入同一局；
- [ ] 过期、篡改、错误实例/版本和满员 Ticket 被拒绝；
- [ ] 一局结束后进程 Draining 并关闭；
- [ ] 一键脚本可启动整套本地环境并安全清理自身进程。

## 实现原理

Dedicated Server 是数据面权威，Backend 是控制面。Available 表示 Server 已加载且能接人，Allocated 表示已绑定 Match，InGame 表示对局实际开始；混淆这些状态会产生空占、重复分配和无效地址。条件更新与 transaction 让 SQLite 中的分配具有单一赢家。

## 常见问题与停止条件

- 返回地址但连不上：检查 Available 的就绪条件、Cook 地图、端口和版本。
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
