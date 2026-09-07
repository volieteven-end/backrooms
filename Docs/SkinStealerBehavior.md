# 窃皮者：移动朝向与追逐嘶吼

更新日期：2026-09-07。

## 当前行为

- 模型正面与角色前进方向对齐，巡逻和追击按移动方向平滑转身。
- 首次发现有效玩家并进入追逐时发出一次嘶吼。
- 持续追逐时默认每 **4 秒**再次嘶吼；反复感知通知共用冷却，不会每帧叠播。
- 离开追逐、目标倒地/隐藏或回合结算后停止继续触发；攻击叫声与发现叫声避免重叠。
- 服务器决定触发时机，实时 multicast 通知客户端播放；声音从怪物位置发出，随怪物移动，支持距离衰减和遮挡。纯服务器不播放声音。

## 本次修正

本机实际读取 Idle/Walk/Run 的骨骼姿态，确认导入模型的前向为局部 **+Y**。地图中原 Mesh Yaw 为 0，而 UE 角色移动前向为 **+X**，形成约 90° 偏差。

代码现在将 Mesh 相对 Yaw 设为 **-90°**，关闭 Controller Yaw/DesiredRotation，启用 OrientRotationToMovement，Yaw 转速为 **540°/秒**。所有追击及巡逻路径请求均关闭侧移。BeginPlay 对已放置的 Actor 应用相同设置，并更新网络平滑使用的 Mesh 初始偏移，防止客户端恢复旧的 0° 偏移。

地图几何、角色位置/缩放、骨骼和动作资源没有重导入。

## 声音与可调参数

UE：**Project Settings → Game → Parking Garage Audio**。

| 设置 | 默认值 |
|---|---|
| Roar Interval | 4 秒；最小 2 秒 |
| Sounds → SkinStealerRoar → Sound | 复用 `skinstealer_gotcha1__1_`，约 1.73 秒 |
| Volume | 0.85，再乘游戏音效 Master Volume |
| Spatial / Occlusion | 开启 |
| Falloff Distance | 2200 cm |

现有追逐底音保留；新增的是独立的一次性嘶吼事件。13 个游戏声音事件复用 12 个 SoundWave 文件，可在同一设置页替换独立嘶吼素材。

## 工程位置

- [角色朝向与服务器嘶吼触发](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/AI/BREntityCharacter.cpp)
- [AI 路径请求](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/AI/BREntityAIController.cpp)
- [声音默认设置](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/Audio/BRGameplayAudioSettings.cpp)
- [回归探针](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/Tests/BREntitySmokeProbe.cpp)

联机 BuildId 更新为 `backrooms-room-v5-skinstealer`，双方使用本次同版本源码构建。开发服务器重启后生效；旧的打包客户端没有自动更新。

## 验证说明

`Backrooms.Entity.FacingAndRoarContracts` 检查转向配置、模型前向和 multicast 属性。

显式开发参数 `-BREntitySmoke` 在独立运行的测试进程中建立临时地面，检查四个移动方向、真实视觉感知发现玩家、首次/重复嘶吼、感知重复通知、失去目标、倒地和结算停止条件。多人测试同时核对两客户端朝向和嘶吼事件接收；测试对象不保存到地图，普通启动不带此参数。

本次具体命令、运行结果和恢复验证见 [VERIFICATION.txt](E:/UNREAL/ue%20projects/backrooms/work/skinstealer_facing_roar_20260907/VERIFICATION.txt)。
