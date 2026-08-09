# 第 11 周：测试、性能与可观测性

## 本周目标

把“能演示”提升为可重复、可测量、可定位：建立 Go 单元/集成测试、UE 功能测试、多人冒烟与故障矩阵；统一跨 Client/Server/Backend 日志；用 Unreal Insights 和网络分析完成一次有证据的优化。

## 前置条件与周门槛

- 第 10 周不存在重复奖励、重复 Pawn 或可覆盖终态的问题。
- 本周先固定环境和基线，再优化；禁止凭感觉改复制频率或 Tick。
- 性能报告必须注明硬件、构建、地图、玩家/AI 数、网络条件和测试时长。

## 本周时间预算（15–20 小时）

| 工作单元 | 内容 | 时间 |
| --- | --- | ---: |
| 1 | 测试矩阵与 Go 自动化 | 3–4 小时 |
| 2 | UE 功能测试与多人冒烟 | 4–5 小时 |
| 3 | 结构化日志与端到端排障 | 3 小时 |
| 4 | Insights/网络性能基线 | 4 小时 |
| 5 | 单项优化与回归报告 | 2–4 小时 |

## 先读什么

- `Source/LyraGame/Tests`、`Build/LyraTests.xml` 和 `Build/Scripts/Automation/LyraTest.*`；
- UE Automation/Functional Test/Gauntlet；
- Unreal Insights、Networking Insights、Network Profiler；
- Go `testing`、`httptest`、race detector 和 SQLite 测试隔离方式。

## 工作单元 1：测试分层和 Go 自动化

先创建测试矩阵，列出用例 ID、层级、前置、步骤、预期、日志 ID、是否自动化。

Go 单元测试至少覆盖：Token 过期/签名、状态转换、幂等键、错误映射。Store/Service 集成测试使用每个测试独立临时 SQLite DB，运行真实 migration，覆盖：房间容量、外键、原子 Server 分配、顺序/并发 Settlement、Commit 后重复请求。

HTTP 使用 `httptest` 覆盖认证、JSON 错误、超时、Idempotency-Key 和状态码。运行：

```powershell
Set-Location '<RepoRoot>\Backend'
go test ./...
go test -race ./...
```

成功标准：重复运行三次通过；测试不依赖执行顺序和已有 `data` 数据库；race detector 无 Go 内存竞态。

## 工作单元 2：UE 功能与多人冒烟

在 ExtractionOps 测试模块或现有 ShooterTests 模式中增加最小自动化，优先测试纯规则和可控组件：

- Armor/Health 伤害和 Dead 终态；
- Inventory 堆叠、拆分、容量、version；
- Pickup 距离与重复请求；
- Extraction 开始/取消/成功；
- 恢复快照版本。

多人冒烟脚本启动 Backend、Server、两个 Client，验证 Login → Room → Match → Join → Loot → A Dead → B Extracted → Settlement → Return/Results。无法自动操作的 UI 步骤允许半自动，但脚本必须自动判断进程、端口、关键日志和退出码。

故障冒烟至少包含 100 ms 延迟/5% 丢包、Backend 20 秒不可用、Server 被杀、重复 Settlement 和重连。

## 工作单元 3：结构化日志和排障

Client、Server、Backend 统一字段：

```text
timestamp service level event request_id player_id room_id
match_id run_id server_instance_id error_code duration_ms
```

每个业务事件有稳定 event 名，不用自然语言代替字段。缺失 ID 用省略/明确 null，不生成无关联的新 ID。HTTP 入口创建/透传 request_id；Server 战局日志始终带 match/run；敏感 Token 只记录摘要或完全不记录。

注入一次“重连 Ticket 指向旧 server_instance_id”和一次重复 Settlement。只凭日志，从 Client 错误找到 Server 拒绝，再找到 Backend/Ticket 或 already_processed 记录，写出时间线、根因和修复/预期行为。

## 工作单元 4：性能与网络基线

固定测试环境：Development/Shipping 构建类型、硬件、1280×720 固定画质、`L_ExtractionTest`、60 秒、2 人/4 人/4 人+20 AI 或 100 个可复制 Pickup、RTT/丢包值。

每组记录：Client FPS/Game/Render/GPU、Server frame/tick、内存、发送/接收带宽、复制 Actor/RPC、Backend API p50/p95/p99、Settlement 成功/重试。

抓两份 Unreal Insights：正常 2 人 60 秒、压力场景 60 秒；再抓 Networking Insights/Network Profiler。按 Game Thread → Server Tick → Tick/Timer → Replication/RPC → 加载/GC/UI 顺序分析。为前三个热点写“证据、假设、是否行动”。

## 工作单元 5：一次可证明的优化

从测量结果选择一个最大且低风险问题，只改一类行为。优先候选：Inventory owner-only 字段、远处 Pickup relevancy/频率、无用 Actor Tick、UI 全量刷新、Backend 超时堆积。

使用相同构建、地图、人数、网络和 60 秒窗口重测；报告绝对值和百分比，同时检查体验/正确性未回归。若指标没有改善，诚实记录假设失败并回滚该优化，不挑选不同测试条件制造提升。

最后运行 Go tests、UE 规则测试、多人冒烟、故障冒烟和完整对局一次。

## 验收目标

- [ ] Go unit/integration/race 测试稳定通过；
- [ ] UE 规则测试覆盖 GAS、Inventory、Pickup、Extraction、Snapshot；
- [ ] 两客户端端到端冒烟可重复；
- [ ] 故障矩阵覆盖网络、Backend、Server、重复请求和重连；
- [ ] 日志可用稳定 ID 跨三个进程关联；
- [ ] 2 人、4 人和压力场景有固定基线；
- [ ] 有 Insights 与网络分析报告；
- [ ] 一次优化使用相同条件给出前后数据；
- [ ] 优化后正确性测试全部通过。

## 实现原理

测试证明行为，日志关联一次业务事件，Profiler 证明成本。没有固定条件的数字不可比较，没有 request/match/run ID 的日志无法跨进程定位，没有回归测试的优化可能只是在转移问题。

## 常见问题与停止条件

- 测试偶尔失败：固定时间、随机种子、端口和数据隔离，不用盲目重试隐藏 flaky。
- 日志多但不可查：增加稳定 event/ID，不增加更多自然语言。
- FPS 低：区分 Game/Render/GPU/网络，不先入为主。
- 优化数据变差：保留报告并回滚，不伪造成功结论。

端到端冒烟或关键一致性回归失败时，不开始最终录制。

## 本周作品集产出

- 测试矩阵与自动化命令；
- 结构化日志排障记录；
- Unreal/Networking Insights 报告；
- 性能基线表；
- 优化前后对比图。

## 参考资料

- [Unreal Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-in-unreal-engine)
- [Testing Networked Games](https://dev.epicgames.com/documentation/en-us/unreal-engine/testing-and-debugging-networked-games-in-unreal-engine)
- [项目测试源码](../Source/LyraGame/Tests)
