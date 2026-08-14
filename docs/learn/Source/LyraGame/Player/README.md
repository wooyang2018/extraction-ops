# Source/LyraGame/Player

## 目录职责

Player 目录承载“连接/用户身份对应的运行时 Actor”与出生点策略。理解时必须区分 LocalPlayer（进程本地用户对象）、PlayerController（控制与拥有连接）、PlayerState（可复制玩家状态）和 Pawn（可替换身体）。

## 文件分组

| 文件组 | 作用 |
|---|---|
| `LyraLocalPlayer.*` | 本地用户设置、SharedSettings、平台/输入上下文 |
| `LyraPlayerController.*` | 输入 Tick、ASC 输入处理、相机、服务端重启、消息入口 |
| `LyraPlayerState.*` | ASC/属性集、PawnData、Team、AbilitySet 授予 |
| `LyraPlayerBotController.*` | Bot 控制与服务端重启 |
| `LyraPlayerStart.*` | 出生点占用状态与 claim |
| `LyraPlayerSpawningManagerComponent.*` | GameState 侧收集出生点、选择和重启策略扩展点 |
| `LyraCheatManager.*`、`LyraDebugCameraController.*` | 非 Shipping 调试控制 |

## PlayerState 是玩家 Gameplay 状态锚点

`ALyraPlayerState` 创建 Lyra ASC、HealthSet、CombatSet，并在 Authority 设置 PawnData 时授予其中 AbilitySets。它实现 TeamAgent 接口，使 Team 信息可以在 Pawn 重生之外保持。

ASC 的复制模式与 NetUpdateFrequency 会直接影响 GameplayEffect/Tag 的远端时效。把 ASC 放在 PlayerState 后，应特别关注 PlayerState 默认更新频率和 relevancy。

## PlayerController 的输入职责

HeroComponent 负责把输入动作绑定为 Tag；PlayerController 每帧在合适时机调用 ASC `ProcessAbilityInput`。这种拆分让绑定随 Pawn/Feature 变化，而输入缓冲与 AbilitySpec 处理集中在 ASC。

Controller 同时连接 CommonUser/Session、LoadingScreen、Cheat/DebugCamera 与 PlayerState/Pawn，但不应存放需要其他客户端观察的玩家状态。

## 出生点选择

`ULyraPlayerSpawningManagerComponent` 挂在 GameState，使玩法插件可替换 `OnChoosePlayerStart` 等策略。`ALyraPlayerStart` 维护占用/Claim 语义，避免多个并发重启只做一次无锁的距离查询。

GameMode 仍是调用 `ChoosePlayerStart`/`RestartPlayer` 的 Authority 入口；ManagerComponent 是策略提供者，不改变最终生成权限。

## 面试追问

1. 哪些玩家数据应该放 PlayerController，哪些必须放 PlayerState？
2. 为什么 LocalPlayer 不能在 Dedicated Server 上作为逻辑依赖？
3. ASC 放 PlayerState 后，Pawn 重生时哪些东西保留、哪些必须重建？
4. Spawn point claim 为什么比“选择最近空点”更可靠？

## 练习

画出服务端、拥有客户端、另一个客户端分别拥有哪些 PlayerController/PlayerState/Pawn 对象，并标出 Team、ASC、输入和相机读取路径。

