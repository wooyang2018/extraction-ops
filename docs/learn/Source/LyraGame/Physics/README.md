# Source/LyraGame/Physics

`LyraCollisionChannels.h` 集中声明项目约定的 Trace/Object Channel，避免散落魔法数字；实际配置仍需与项目 Collision Profile 保持一致。`UPhysicalMaterialWithTags` 为物理材质增加 Gameplay Tags，同时被 ContextEffects 和 GameplayEffect/伤害衰减读取。

物理材质 Tag 是命中表面的上下文，不应直接信任客户端上报用于权威伤害；服务端必须从可信 Hit/Trace 结果获取。

## 面试追问

1. Collision Channel C++ 常量与项目配置不一致会怎样？
2. PhysicalMaterial 的 SurfaceType 与 GameplayTags 各有什么扩展性？
3. 为什么同一材质信息可同时服务声音与伤害，但两者信任边界不同？

