# 第一人称、门和柜内物资

## 操作

- 对准门扇按 **E** 打开，重新对准已打开的门扇按 **E** 关闭；门按铰链平滑转动，不再隐藏门板。站在关门位置时会阻止关闭。
- 对准柜门 / 抽屉按 **E** 搜寻。柜门绕铰链打开，抽屉向外滑动。打开后对准物品按 **E** 拾取。
- 打开高柜后，对准柜内空处按 **E** 躲藏，再按 **E** 出来。瞄准柜内物品时优先拾取；进入后柜门关闭。
- 出生电梯外地面放置一个 `BR_EntryFlashlight`。按 **E** 拾取、**F** 开关、**R** 消耗一组备用电池充满电。HUD 显示电量与备用电池数。初始角色不自带手电。
- 手电耗电默认每秒 0.25%，满电约 400 秒；关闭、躲藏或倒地时不耗电。备用电池最多 5 组。电量低于 99% 后可以更换。

## UE 中的入口

实际游戏地图：`/Game/ReverseAsset/ParkingGarage/LightingStudy/L_MiddleFloor_Dark`。
明亮工作地图 `L_MiddleFloor` 也同步了这次交互配置；房间子关卡 `L_PuzzleRoom` 与两张地图共用。
保持现有停车场几何、灯光、导航、随机房门和电梯通关逻辑，不重新导入地图。

Outliner 中展开 **Gameplay / InteractiveItems**：

| 对象 | Details 中可调项目 |
|---|---|
| `BR_LootCabinet_*` | `Backrooms > Loot`：电池 / 手电概率、物品偏移、是否允许任务钥匙、房间入口门 Tag；`Backrooms > Door Motion`：开启角度、开启时间 |
| `BR_LootDrawer_*` | 同上；`Sliding`、`Slide Offset` 控制滑动而非旋转 |
| `BR_EntryFlashlight` | 位置、旋转和 `Supply Type` |
| 角色蓝图（父类 `BRPlayerCharacter`） | 组件 `FirstPersonArms`、`FirstPersonFlashlight`；`Backrooms > Equipment` 中的 `Flashlight Lumens`、`Battery Drain Per Second` |

原生角色被 GameMode 直接使用。若要永久调整角色默认数值，可以新建其子蓝图、调整 Defaults，再将停车场 GameMode 的 Default Pawn Class 指向该蓝图；在场景中临时放置一个角色并不会替换 GameMode 出生角色。

## 物资规则

- 一局的 32 个可搜寻单元 = 主地图 15 个高柜 + 9 个抽屉 + 子关卡 5 个高柜 + 3 个抽屉。
- **服务器开局一次性分配内容，开柜才显示；关开不会重新抽奖。**
- 随机选择 4 个允许任务钥匙的单元，各放一把钥匙。任务需要的 4 把钥匙有保底，不能单纯用概率导致通关缺钥匙。
- 钥匙所在房间的入口门会被解锁，避免把必要钥匙分配到随机锁住的房间。
- 其余柜子默认：35% 电池组、10% 手电筒、55% 空。每个单元可以单独调整概率。
- 同一物品仅由一名玩家成功拾取一次。任务钥匙增加全队进度；手电和电池属于拾取者。
- 旧地图的地面随机钥匙候选在柜内分配生效时停用；没有足够配置柜子的其他地图继续使用原来的地面钥匙规则。

## 实现变化

1. 原来完整人物 Mesh 对本人设置了 `Owner No See`，却没有独立第一人称手臂组件；现增加专用黄袖手臂和匹配的 Arms Idle 动画。第三人称身体继续对本人隐藏，避免头部遮挡镜头。
2. 第一人称手臂和手电仅本人可见，使用 UE 第一人称渲染参数；其他玩家看到原来的完整身体和右手装备。
3. 门的 `bOpen`、`OpenDirection` 由服务器决定并复制，客户端插值到目标角度；E 检测既能识别关闭位置，也能识别打开后的门扇。
4. 柜体与抽屉外壳保留 Pawn 阻挡，交互查询使用单独的组件响应；关闭状态隐藏物资并禁用其交互，房间墙体仍参与遮挡检查。
5. 新增 `BRLootCabinet`、`BRSupplyPickup`；原钥匙、任务、躲藏及音频模块继续使用。
6. 新增手电外壳 / 镜片材质，修正原导入网格两个材质槽都引用灯具发光材质的问题。

## 联机和启动

编辑器开发服务器启动命令：

```powershell
& 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Start-Server.ps1' -EditorRuntime
```

房间协议版本改为 `backrooms-room-v2-items`。联机双方使用本次相同的代码和资源；旧打包客户端仍是旧功能，需要重新打包后才包含这些修改。
原服务器若仍在运行，先通过 `Stop-Server.ps1` 停止，再用上面的命令启动。

## 验证与恢复

本次独立验证记录、截图、原始备份、补丁与回滚入口位于：
`E:\UNREAL\ue projects\backrooms\work\interaction_items_20260906`。

回滚前先保存并关闭 backrooms 编辑器及该项目服务器。脚本逐文件检查 SHA-256，发现后续修改时停止，避免覆盖新工作。

```powershell
& 'E:\UNREAL\ue projects\backrooms\work\interaction_items_20260906\ROLLBACK.ps1' -TargetRoot 'E:\UNREAL\ue projects\backrooms'
& 'D:\Unreal5.8\UE_5.8\Engine\Build\BatchFiles\Build.bat' backroomsEditor Win64 Development '-Project=E:\UNREAL\ue projects\backrooms\backrooms.uproject' -WaitMutex -NoHotReloadFromIDE
```

回滚恢复本次之前的代码和地图；已有多人大厅、暗光停车场与上一次接入的声音系统保留。
