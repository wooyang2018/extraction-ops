1. **宿主层**：Target、Module、Engine、GameInstance、AssetManager 决定进程形态、模块装载与全局服务。
2. **体验编排层**：Experience Definition 选择一组 PawnData、ActionSet 和 GameFeature；ExperienceManagerComponent 负责异步装载、激活和卸载。
3. **模块化 Gameplay 层**：GameFeatureAction 与 `UGameFrameworkComponentManager` 向已存在或未来生成的 Actor 注入组件、能力、输入和 UI。
4. **运行时玩法层**：Pawn/PlayerState/ASC、Character、Equipment、Inventory、Weapon、Team、UI 等通过初始化状态、Gameplay Tag 和消息协作。