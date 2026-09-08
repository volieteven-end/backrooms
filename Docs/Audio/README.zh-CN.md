# 停车场音效 · C++ 接入与 UE 调整

## 声音在哪

- 游戏音效：`E:\UNREAL\ue projects\backrooms\Content\ReverseAsset\ParkingGarage\Audio`（12 个 SoundWave）。
- 菜单音效：`E:\UNREAL\ue projects\backrooms\Content\UI\Menu\Audio`（已有点击、悬停）。
- 原始 WAV：`E:\ReverseAsset\ParkingGarage\Audio_WAV`。
- 在 UE 内容浏览器中选择 SoundWave，点击播放图标试听。

## 在 UE 中调音

打开 **Edit → Project Settings → Game → Parking Garage Audio**。

- `Enabled`：停车场音效总开关；不影响原有菜单按钮声音。
- `Master Volume`：停车场音效总音量，默认 0.8。
- `Sounds`：展开对应事件，更换 `Sound`，调整 `Volume`。
- `Spatial`：开启后使用场景位置；环境底噪默认关闭，为全局声床。
- `Inner Radius`：此距离内保持最大音量，单位 cm。
- `Falloff Distance`：从内半径向外衰减的距离，单位 cm。
- `Occlusion`：使用真实几何表面的遮挡检测；隔墙后降低音量和高频。
- `Walk Step Distance / Run Step Distance`：窃皮者每次脚步的移动距离。
- `Roar Interval`：持续追逐时重复嘶吼的间隔，默认 4 秒，最小 2 秒。首次发现目标立即触发；反复感知通知受同一冷却约束。

这些设置保存到项目 Game 配置；修改后重新开始运行。声音可以留空以禁用该事件。正在播放的循环不热替换，重开当前游戏运行即可应用新资源。

## 已接入的触发点

| 事件 | 触发 | 空间处理 |
|---|---|---|
| Ambience | 停车场开始运行；切图停止 | 2D 循环 |
| DoorOpen | 可交互房门由关闭变为打开 | 门的位置，3D |
| KeyPickup | 钥匙由未收集变为已收集 | 钥匙的位置，3D |
| ElevatorBell / ElevatorDoorOpen | 出生电梯入场提示 | 第一个 PlayerStart 附近 |
| ElevatorBell | 任务进入 ExtractionReady | 撤离区域位置 |
| KeyInsert | 停车场插槽由空变为已插入，每个插槽一次 | 对应插槽位置，3D |
| KeyInsert / ElevatorDoorClose / ElevatorMotor | 未配置插槽的旧关卡保留撤离成功音效序列；停车场使用实际插入事件 | 旧关卡撤离区域位置 |
| SkinStealerChase | 复制状态进入 Chasing；离开追逐或结算时停止 | 跟随窃皮者，3D 循环 |
| SkinStealerAttack | 已有攻击事件复制到客户端 | 窃皮者的位置，3D |
| SkinStealerRoar | 发现站立玩家、进入追逐时一次；持续追逐每 4 秒一次；离开追逐、目标倒地/隐藏或结算后停止继续触发 | 服务器发出实时 multicast；客户端在窃皮者位置播放，3D，跟随移动 |
| SkinStealerStep1 / Step2 | 根据实际移动距离交替；停止、离地、攻击和传送时不触发步行声 | 窃皮者的位置，3D |

停车场的钥匙插入声对应实际插槽状态，中央门沿用门开启声；入场电梯声音仍使用原有入场阶段。

当前导入目录还没有主角脚步和呼吸声音；本轮不使用窃皮者脚步代替主角音效。原游戏的混响分区、精确音量、动画脚步标记及逐帧声画对齐仍需专门调音。

## 多人处理

- 开门与钥匙使用已有服务器复制状态，在每台客户端播放状态变化的声音；同一次 RepNotify 重入不重复播放。
- 声音播放不新增客户端请求服务器的 RPC，也不改变交互权限。
- 独立服务器不加载、播放音频；玩家电脑负责播放。
- 结束或切图清理循环音频与计时器；循环使用临时 SoundCue，不改写导入 SoundWave 的 Looping 属性。

## 调试与验证

普通启动不需要任何音频参数。可添加 `-BRAudioLog` 记录声音触发。

开发测试使用显式 `-BRAudioSmoke`，会在独立测试进程中播放 13 类声音事件、记录主混音并自动退出；该功能不进入 Shipping。新增嘶吼事件复用现有 `skinstealer_gotcha1__1_`，游戏 SoundWave 文件仍为 12 个。

```powershell
python -X utf8 'E:\UNREAL\ue projects\backrooms\work\audio_restore_20260906\run_tests.py' framework --project 'E:\UNREAL\ue projects\backrooms\backrooms.uproject' --name LIVE
```

音效基础接入的历史记录见：
`E:\UNREAL\ue projects\backrooms\work\audio_restore_20260906\VERIFICATION.txt`。

2026-09-07 的窃皮者朝向与嘶吼回归（13 类声音事件、四方向移动及双客户端同步）使用：

```powershell
& 'C:\Python314\python.exe' -X utf8 'E:\UNREAL\ue projects\backrooms\work\skinstealer_facing_roar_20260907\verify.py' MODIFIED
```

当前行为与调参见 [窃皮者行为说明](E:/UNREAL/ue%20projects/backrooms/Docs/SkinStealerBehavior.md)，本次命令和结果见 [VERIFICATION.txt](E:/UNREAL/ue%20projects/backrooms/work/skinstealer_facing_roar_20260907/VERIFICATION.txt)。

编译过的开发工程与现有打包文件夹是两份产品。旧 `Builds\Multiplayer\Client\Windows` 不会自动包含新音效逻辑；重新打包后再分发。

## 回滚

本次嘶吼与朝向修正的源文件恢复脚本为 `E:\UNREAL\ue projects\backrooms\work\skinstealer_facing_roar_20260907\ROLLBACK.sh`，通过 Git Bash 执行并传入目标目录。它逐项核对修改后内容，再恢复本次修改前的源码/配置并移除本次新增文件；测试已在独立临时数据目录完成，主工程保留修正。恢复实际工程前关闭本项目编辑器与服务器，恢复后重新编译对应目标再启动。此脚本不恢复已编译的二进制。

参考：[Epic 音量衰减与遮挡](https://dev.epicgames.com/documentation/en-us/unreal-engine/sound-attenuation-in-unreal-engine)、[Sound Cue](https://dev.epicgames.com/documentation/en-us/unreal-engine/sound-cue-reference-for-unreal-engine)。
