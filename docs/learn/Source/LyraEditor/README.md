# Source/LyraEditor

LyraEditor 是仅 Editor Target 装载的开发模块，依赖 LyraGame 运行时类型，但运行时代码不反向依赖它。

## 目录分组

- `Commandlets/`：无交互批处理、内容/流程命令入口。
- `Validation/`：EditorValidator、SourceControl/资产规则与项目验收。
- `Utilities/`：编辑器操作辅助。
- `Private/` 与模块文件：菜单、消息日志、启动注册和关闭清理。

Editor 模块可引用 UnrealEd、DataValidation、GameplayAbilitiesEditor、ToolMenus 等；这些依赖绝不能泄漏到 LyraGame Shipping 模块。Validator 应报告可操作的资产/配置问题，并区分 Error、Warning 与依赖资产尚未加载。

## 面试追问

1. 为什么 Editor 工具应单独模块，而不只用 `#if WITH_EDITOR` 包住运行时代码？
2. Commandlet 与 Editor Utility 的运行环境/交互假设有何不同？
3. DataValidation 如何与 CI/提交前检查结合？

## 练习

沿 `LyraEditor.Build.cs` 检查所有 Editor-only 依赖，确认 LyraGame.Build.cs 没有反向依赖 LyraEditor；选择一个 Validator 追踪注册和执行入口。

