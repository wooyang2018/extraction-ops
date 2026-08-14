# 仅插件描述或内容模块

以下范围内插件没有可讲解的 C++ 源码模块，本次只读取 `.uplugin` 以理解类型、加载阶段和依赖，不分析 Content：

- `Plugins/GreenRoom/GreenRoom.uplugin`
- `Plugins/RedRoom/RedRoom.uplugin`
- `Plugins/LyraExampleContent/LyraExampleContent.uplugin`
- `Plugins/GameFeatures/ShooterExplorer/ShooterExplorer.uplugin`
- `Plugins/GameFeatures/ShooterMaps/ShooterMaps.uplugin`

它们仍能体现 Lyra 的内容插件/GameFeature 边界：玩法或地图内容可作为插件交付，不要求每个插件都有 C++ 模块。是否被某 Experience 实际激活取决于 Content 配置，超出本次范围。

