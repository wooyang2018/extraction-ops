# Week 01–03 验收记录

日期：2026-08-11

## 环境边界

- 工程：`D:\Document\AI\Codex\extraction-ops\LyraStarterGame.uproject`
- Engine：`D:\Software\UE_5.8`
- 构建：`LyraEditor Win64 Development`
- Server：`UnrealEditor.exe -server -NullRHI`
- Client：两个 `UnrealEditor.exe -game -NullRHI`
- 明确未使用：`D:\Software\UE_5.8.1_Source`

## 构建结果

最终命令：

```powershell
& 'D:\Software\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  LyraEditor Win64 Development `
  '-Project=D:\Document\AI\Codex\extraction-ops\LyraStarterGame.uproject' `
  -WaitMutex -NoHotReloadFromIDE
```

最终结果：`Result: Succeeded`，Target up to date。

新增 C++ 编译通过：

- `ExtractionDebugDataLibrary.*`
- `ExtractionDebugHUDWidget.*`
- `ExtractionDefaultLoadoutComponent.*`
- DebugSnapshot 自动化测试

## Unreal MCP 资产流程

1. 标准 JSON-RPC `initialize` 成功，协议版本 `2025-06-18`。
2. 发现 `list_toolsets`、`describe_toolset`、`call_tool`。
3. 读取 Blueprint Basics Agent Skill。
4. 使用 AssetTools、ObjectTools、BlueprintTools、UMGToolSet 和 ProgrammaticToolset。
5. 资产修改期间串行调用；修改后编译、保存，再检查 Dirty 状态。
6. 没有通过文件脚本直接制造或修改二进制 `.uasset`。

可重复入口：

```powershell
Scripts/Invoke-UnrealMcp.ps1
Scripts/Mcp/Create-Week02CoreAssets.json
Scripts/Mcp/Configure-Week02CoreAssets.json
Scripts/Mcp/Create-Week03WeaponAssets.json
Scripts/Mcp/Configure-Week03WeaponAssets.json
Scripts/Mcp/Create-Week03InputAssets.json
Scripts/Mcp/Configure-Week03InputAssets.json
Scripts/Mcp/Verify-Week02-03Assets.json
Scripts/Mcp/Verify-Week03InputAssets.json
```

## 资产契约结果

- 核心资产检查：23/23 存在，Missing 0，Dirty 0。
- 输入专项检查：Interact 映射 1 个，InputConfig 绑定 1 个，键位 `F`，Tag 为 `InputTag.Ability.Interact`。
- Experience ActionSets：项目 InputActionSet、Lyra StandardComponents、项目 HUD ActionSet。
- Rifle/Shotgun Item 的直接依赖均指向自己的 `WID_Extraction*`。
- 两个 WID 均指向自己的 WeaponInstance、AbilitySet 和装备 Actor。
- 两个 Fire Ability 均指向自己的 `GE_Damage_Extraction*`。

## Automation 结果

```text
ExtractionOps.DebugSnapshot.InvalidContext  Success
ExtractionOps.StateRules.Match              Success
ExtractionOps.StateRules.Run                Success
ExtractionOps.StateRules.Threat             Success
ExtractionOps.StateRules.Zone               Success
passed=5 failed=0 skipped=0 warnings=0
```

最终冷态命令行复验日志：`Saved/Logs/Week03-FinalAutomation.log`，包含 5 条 `Test Completed. Result={Success}` 与 `TEST COMPLETE. EXIT CODE: 0`。

## Extraction 双客户端 E2E

最终运行：

```powershell
Scripts/Start-Week01-Multiplayer.ps1 `
  -EngineRoot 'D:\Software\UE_5.8' `
  -Experience 'B_ExtractionExperience' `
  -AutomatedSmokeSeconds 5 `
  -ReadyTimeoutSeconds 150 `
  -NumBots 2
```

最终日志：

- `Saved/Logs/Week01/Multiplayer-Server-20260811-233349.log`
- `Saved/Logs/Week01/Multiplayer-Client1-20260811-233349.log`
- `Saved/Logs/Week01/Multiplayer-Client2-20260811-233349.log`

关键 Server 事实：

```text
Identified experience LyraExperienceDefinition:B_ExtractionExperience (Source: OptionsString)
EXPERIENCE: OnExperienceLoadComplete(... B_ExtractionExperience, Server)
Granted Extraction rifle and shotgun to LyraPlayerController_0
Join succeeded: <client 1>
Granted Extraction rifle and shotgun to LyraPlayerController_1
Join succeeded: <client 2>
```

脚本断言通过：

- 7777 监听；
- 两个客户端完成独立 Join；
- Extraction Experience 自动激活；
- 两名玩家获得默认 Rifle/Shotgun；
- Client 1 退出后 Server 与 Client 2 继续；
- Server 退出后 Client 2 记录明确断线；
- 脚本只清理自己创建的进程。

最终三份日志未匹配：`Fatal error`、`LogBlueprint: Error`、`LogWindows: Error`、`ensure condition failed`、`LogExtractionLoadout: Error`。

## ShooterCore 回归

日志：`Saved/Logs/Week01/Multiplayer-*-20260811-232538.log`。

相同双客户端脚本在不指定 Extraction Experience 时通过两次 Join、单客户端退出和 Server 退出断线断言，证明本轮变更没有破坏默认 ShooterCore 多人启动链。

## 解释边界

以上是源码、资产、自动化与网络生命周期验收。由于本轮明确不使用 Windows 界面控制，没有声称完成真人 10/25/50 米主观枪感采样、五分钟人工连续战斗或视频录制。
