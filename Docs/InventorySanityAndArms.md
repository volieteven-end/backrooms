> 本文是上一版（8 格）记录；当前 9 格背包＋主手＋两个口袋、空手隐藏与左键交互以 [InventoryReferenceRevision.md](InventoryReferenceRevision.md) 为准。杏仁水仍保留罐装模型。

# 背包、SAN、第一人称手臂与低台阶

本轮在现有停车场工程中接入原游戏的选定图片、手臂动作与杏仁水模型；背包逻辑在 UE 5.8 中用 C++ + 可编辑 UMG 重建。原游戏已烘焙 Widget 蓝图不是本工程的运行逻辑。停车场地图、暗光灯光、门柜布局未重新导入或重建。

## 运行与操作

重新打开 `E:\UNREAL\ue projects\backrooms\backrooms.uproject`，进入现有主菜单联机流程，或直接在 `L_MiddleFloor_Dark` 中 PIE。

| 操作 | 按键 |
|---|---|
| 打开、关闭背包 | Tab |
| 关闭背包，不离开房间 | 背包打开时 Esc |
| 选择格子 | 1–8；背包内也可单击 |
| 使用选中物品 | 背包内 E / 使用按钮；背包关闭时鼠标左键 |
| 丢弃选中物品 | G / 背包内丢弃按钮 |
| 地面拾取、门柜交互 | E，原有交互检测保留 |
| 手电开关、换电池 | F、R |

背包打开时暂停本人的移动和视角输入，不暂停服务器世界。关闭背包后恢复游戏输入。倒地、躲藏或切图时自动关闭背包。背包之外的 Esc 仍沿用离开房间功能。

## 八格物品与 SAN

- 固定 **8 个格子**，一件物品占一格，不堆叠。
- 手电筒、电池组、杏仁水接入拾取、选取、使用、丢弃与再次拾取。
- 最多一支手电、五组备用电池，延续现有手电限制；这些物品共同占用八格。
- 满包时地面物品保留，不会被误删。丢出的手电保留实际电量。
- **任务钥匙继续由全队共享，不占八格**，避免满包导致四把钥匙任务停滞。
- SAN 初始 100。探索阶段由服务器每秒结算，默认每秒下降 0.1；躲藏时默认暂停下降，倒地及非探索阶段不下降。
- 杏仁水默认恢复 35 点，上限 100；SAN 已满时不消耗。喝水动作约 4.5 秒，期间防止重复使用同一物品。
- HUD 与背包均显示 SAN。背包有三档原游戏脑部图标；低于 25 显示警示。
- 当前 SAN 归零只显示警示，不额外触发死亡、幻觉或刷怪规则。
- 柜子中原有保证四把钥匙的分配保留；非钥匙柜默认电池 35%、手电 10%、杏仁水 25%、空柜 30%。每个柜子的参数仍可调整。

## 联机数据

`BRInventoryComponent` 在服务器维护八格与每件物品的唯一 InstanceId。使用、丢弃和换格请求校验槽位、物品 ID、角色状态和操作时间；旧 ID 及重复请求不消耗后来放入同格的新物品。

槽位、SAN、选中槽位只复制给物品拥有者；手持类型向相关客户端复制。地面掉落物沿用复制 Actor。任务钥匙仍走原有服务器任务状态。每轮新生成角色重置背包和 SAN；本轮没有增加跨局存档。

联网 BuildId 更新为 `backrooms-room-v3-inventory`。客户端和服务器应使用同一轮代码。既有已打包的旧 v2 程序不会自动变成新版；本轮主要交付更新后的 UE 工程与开发运行模块。

## 在 UE 中调整

| 内容 | UE 资产 / 参数 |
|---|---|
| 背包布局、图标、文字、颜色 | `/Game/UI/Inventory/Widgets/WBP_Inventory` |
| 游戏内 SAN 小 HUD | `/Game/UI/Menu/Widgets/WBP_GameHUD` |
| 角色可编辑蓝图 | `/Game/Gameplay/Player/BP_BRPlayerCharacter` |
| SAN 下降、杏仁水恢复、躲藏暂停 | 上述蓝图的 `Inventory` 组件 |
| 手臂动作与位置 | 上述蓝图的 `ArmsAnimation` 组件 |
| 低台阶与地面检查 | 上述蓝图的 `CharacterMovement` 组件 |
| 柜子杏仁水概率 | 地图内 `BRLootCabinet` 的 `AlmondWaterChance` |

`BRGameMode` 的默认原生角色会在 InitGame 时选择这个可编辑角色蓝图；如果用户已经给 GameMode 指定另一种自定义角色，不会强行覆盖。

背包 Widget 的 `Slot0`–`Slot7`、`UseButton`、`DropButton`、`CloseButton` 等名称与 C++ 绑定有关。可以编辑位置、字体与样式；改名时需要同步修改绑定。

## 手臂修正

原先第一人称一直播放单个 idle，并带有把手抬高的固定变换。现在改为：

1. 空手 idle、行走、跑步及蹲走状态切换，实际骨骼轨道随动作播放。
2. 使用原游戏的拿起/手持/开关手电、拾取、收起、持罐、喝水动作。
3. 轻微呼吸、步幅与转头摆动叠加；手持物跟随手指握持位置。
4. 移除旧固定 70 度抬手旋转，分别配置空手、持物、喝水位置。
5. 喝水只在动作中段前移手臂，回收时恢复持物位置，避免肩部开口出现在摄像机前。
6. 人物视角单独使用 1 厘米近裁剪面，解决第一人称 0.25 缩放叠加旧 5 厘米近裁剪面切断袖子的问题；编辑器及菜单摄像机不变。

动作资源位于 `/Game/ReverseAsset/Player/Animations/Arms`。所有导入动作与现有 ArmsMesh 使用同一 49 骨 Skeleton，沿用已验证的 ActorX 坐标转换，避免上次倒置扭曲问题。

`A_BR_Arms_Walk` / `A_BR_Arms_Run` 是本轮基于兼容骨架补做的基础移动动作，并非声称从原游戏导出的原始步行动画。当前是 C++ 单节点动画状态驱动，不是完整 AnimBP 状态机；后续可以在同一骨架上更换更细致的动作。

## 低台阶

角色原本已有 45 厘米 `MaxStepHeight`，因此这轮不是盲目提高跨越高度，而是保持 45，并启用平底地面检查、持续地面检查与 8 厘米 PerchRadiusThreshold。胶囊碰撞保留，墙体仍阻挡人物。

实际电梯出口及独立物理测试中的 5、20、35 厘米台阶已验证无跳跃通过；60 厘米障碍仍挡住角色。测试用台阶只在测试运行期间生成，不保存进地图。若后续另一个网格的简单碰撞把门洞封死，仍应修正那个网格的碰撞，不能用关闭人物碰撞来解决。

## 素材与证据

原始选定导出：`E:\Reverse Assets\EscapeTheBackrooms\Inventory`。

本轮素材只涉及 25 个选定原包：16 张 UI 图片、8 段手臂动作、1 个杏仁水网格及其材质依赖。UI 纹理导入到 `/Game/UI/Inventory/Art`，杏仁水到 `/Game/Gameplay/Items/AlmondWater`。电池图标用 UMG 基础图形绘制，不假称原游戏图片。

来源、导入路径与 SHA-256：`E:\UNREAL\ue projects\backrooms\Docs\InventoryAssetManifest.json`。

运行测试、原始日志、截图、修改清单与原件备份：
`E:\UNREAL\ue projects\backrooms\work\inventory_sanity_arms_step_20260906`。

`VERIFICATION.txt` 记录最终部署版本与确切命令、退出码和结果。测试区分 UE 自动化、独立进程联机与实际渲染截图；独立进程测试是本机开发运行，不作为跨公网或成品打包的验收结论。

## 恢复

停止本工程编辑器、客户端和开发服务器，再在 PowerShell 执行：

```powershell
& 'E:\UNREAL\ue projects\backrooms\work\inventory_sanity_arms_step_20260906\ROLLBACK.ps1' -TargetRoot 'E:\UNREAL\ue projects\backrooms'
& 'D:\Unreal5.8\UE_5.8\Engine\Build\BatchFiles\Build.bat' backroomsEditor Win64 Development '-Project=E:\UNREAL\ue projects\backrooms\backrooms.uproject' -WaitMutex -NoHotReloadFromIDE
```

Git Bash 的同等入口是同目录 `ROLLBACK.sh`。恢复脚本会预检变更文件及备份哈希，保留与本轮无关的文件；发现用户后续改动会停止，不盲目覆盖。恢复后重编译模块，再启动编辑器或服务器。回滚测试在另一份工程副本执行，主工程保留本轮新功能。

## 参考

- [Epic：Character Movement 参数](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent)
- [Epic：UMG UI](https://dev.epicgames.com/documentation/unreal-engine/umg-ui-designer-quick-start-guide-in-unreal-engine)
