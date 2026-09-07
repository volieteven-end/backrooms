# 停车场素材 UE5.8 导入说明

目标虚拟目录：/Game/ReverseAsset/ParkingGarage  
目标磁盘目录：E:\UNREAL\ue projects\backrooms\Content\ReverseAsset\ParkingGarage

## 已导入

- 27 个 StaticMesh：停车场结构、汽车、路锥、桌柜、门、钥匙组件和一套电梯。
- 1 个 SkinStealer SkeletalMesh、1 个 Skeleton、1 个 PhysicsAsset。
- 4 个 SkinStealer AnimSequence：Attack、Idle、Run、Walk；统一重采样为 30 FPS，时长分别保持为 3.6、4.266667、0.8、1.4 秒。
- 74 个 Texture2D：法线和数据贴图已设置为非 sRGB，并使用 Normalmap / Masks 压缩。
- 12 个 SoundWave：环境、交互、房门、电梯及 SkinStealer 声音。
- 26 个基础 Material；28 个网格的 40 个材质槽均已改为引用这些共享材质。

资产登记总数：146。glTF 自动生成的 40 个重复占位材质已在确认无网格引用后清理。

## 常用目录

- /Game/ReverseAsset/ParkingGarage/Meshes/Environment
- /Game/ReverseAsset/ParkingGarage/Meshes/Elevator
- /Game/ReverseAsset/ParkingGarage/Meshes/Keys
- /Game/ReverseAsset/ParkingGarage/Meshes/SafeRoom
- /Game/ReverseAsset/ParkingGarage/Characters/SkinStealer
- /Game/ReverseAsset/ParkingGarage/Materials
- /Game/ReverseAsset/ParkingGarage/Textures
- /Game/ReverseAsset/ParkingGarage/Audio

## 整合提示

- 材质是根据 PNG 与 MAT/props 对应关系重建的基础 PBR 图，不包含原工程的复杂 Shader Graph；放入关卡前按实际灯光校正透明、发光、粗糙度和金属度。
- 模型保留 glTF 导入后的原始尺寸、轴向和枢轴；正式布置时校对 SkinStealer 的角色缩放、碰撞体和根运动。
- 4 段动画已经具有 68 条骨骼轨道并绑定同一 Skeleton，结构和时长已验证；正式接 AnimBP 前检查动作朝向、脚底接触和循环衔接。
- 未复制 Cooked UE4 包。BIN 已由 glTF 导入过程消费；OGG 有对应 WAV；PSA 已转换为 AnimSequence；PSK 的骨架信息由 glTF 骨骼网格覆盖。

详细源文件哈希、生成资产路径和验证结果见同目录 IMPORT_MANIFEST.json。
