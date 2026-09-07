# 背包、第一人称与门柜交互（2026-09-06 修订）

## 本次生效要求
- 背包 3×3（9 格），左侧主手 1 格、口袋 2 格：合计 12 个真实位置，不堆叠。
- 空手在静止、行走、奔跑、蹲下时完全隐藏；手电只显示右臂。杏仁水仍用原来的罐装模型和饮用动画，未替换为参考视频中的瓶子。
- 柜子/抽屉只开关、取物资，不提供柜内躲藏。20 个旧柜内躲藏触发器在客户端和服务器 BeginPlay 都关闭碰撞；32 个柜子的概率掉落和四把必需钥匙保留。
- 平时屏幕中心小点；瞄准满足交互条件的目标显示空心小圆圈。取消世界交互的“按 E”文字。
- 左键优先尝试当前世界目标；没有交互目标才使用主手物品。因此拿杏仁水点门不会误饮。门的平滑开关、第二次点击关闭、服务器距离/遮挡/状态复核保持有效。E 仍作为兼容交互键，但不显示提示。

## 操作
| 操作 | 输入 |
|---|---|
| 背包打开/收起 | Tab；Esc 优先收起背包 |
| 装备 | 背包内单击物品，实际与主手交换 |
| 搬运/交换 | 拖动到背包、主手、口袋任意格，拖到格外取消 |
| 空手 | 把主手拖到空格，或单击空的背包格 |
| 两个口袋快捷切换 | 游戏中 1 / 2，与主手交换 |
| 使用物品 | 背包内右键；游戏中未瞄准交互物时左键 |
| 丢弃 | 背包内选择后 G；游戏中 G 丢弃主手 |
| 手电开关 / 换电池 | F / R |

容量由服务器维护，物品带实例 GUID。拖动同时校验源格和目标格的 GUID，拒绝过期拖动、非法索引、倒地操作和饮用冷却内切换。所有物品与 SAN 只复制给本人；其他玩家只获得装备类型。物资满包时留在世界中。任务钥匙仍全队共享、不占格子。

## UE 编辑位置
- `/Game/UI/Inventory/Widgets/WBP_Inventory`：可编辑 UMG 树。Slot0–Slot8 是背包；Slot9 是主手；Slot10–Slot11 是口袋。保持这些控件名，事件由 BRInventoryWidget 接入。
- `/Game/Gameplay/Player/BP_BRPlayerCharacter`：ArmsAnimation 可调整资源及偏移；ItemGrip 为原包 ItemSocket 参数；FirstPersonFlashlight 为原包 BaseMesh 参数。
- `/Game/Gameplay/Items/AlmondWater/SM_AlmondWater`：现有杏仁水模型未改动。
- `/Game/Gameplay/Items/Reference/Materials/M_Torch`：原手电贴图重建材质。
- `/Game/UI/Inventory/Art`：原菜单/背包图片；电池沿用项目图标，不伪装成原包资源。

## 原包核对证据
本次以 CUE4Parse 读取本机游戏 pak 的实际对象/属性，而非只凭资源名推测。
| 对象 | 已读取的字段 |
|---|---|
| BPCharacter_Demo.Arms | 相对位置 (3,0,-67.5)，Yaw -90，Scale .9；父级位于 FirstPersonCamera 的 SpringArm |
| ArmsMesh_Skeleton.ItemSocket | RightHand；位置 (2.256446,-11.683668,3.849611)，Pitch -90 |
| BP_Item_Flashlight.BaseMesh | 位置 (1.1792569,.9982159,4.339308)，旋转 (12.395878,-166.56046,-30.501467)，Scale (1,.875,1) |
| DT_Items.Flashlight | 原 Flashlight Hold/Equip，ArmVisibility=Right |
| DT_Items.AlmondWater | 罐装模型、Can Hold/Equip，ArmVisibility=Both |
| UI_Inventory / UI_Inventory_SlotPanel | 主手、两个口袋、背包区域；原槽底图 DeactiveIconBG |
| UI_Sanity | cropassets__1_ 边框、Option-1 底图、Level-1/2/3 脑部图片 |

用户后来明确保留杏仁水模型，因此先前核对过的 AlmondConcentrate 瓶装资源没有进入主工程。UGameInventory 等原已烘焙蓝图没有直接导入执行；这里仍为本项目 C++ + 可编辑 UMG。拖放使用 UMG NativeOnDragDetected / NativeOnDrop，参考 Epic 文档：https://dev.epicgames.com/documentation/unreal-engine/creating-drag-and-drop-ui-in-unreal-engine 。

## 房间躲避与地图
当前暗光地图的 20 个 HideSpot 都关联柜子，没有独立房间传送式 HideSpot。停用它们不删除房间、门或遮挡几何；躲进房间关门依靠现有碰撞与 AI 视线遮挡，不再变成“进入柜子”隐身。通用非柜子 HideSpot 组件仍保留。网络回归用运行时非柜子测试点验证隐藏状态复制，该测试点只在测试命令参数下创建，不写入地图。
所有 .umap 及现有杏仁水模型/材质/动画均以 SHA256 检查未改动；低台阶设置维持上一版。

## 回归和恢复
详细实际命令、结果与未通过迭代记录见本次 work/inventory_visual_match_20260906/VERIFICATION.txt。
独立服务器+两客户端为本机分进程开发环境验证，非公网或打包发布结论。
恢复前保存关闭 backrooms 编辑器并停服；执行同目录 ROLLBACK.sh（Git Bash），传入主工程绝对路径，再用匹配 UE5.8 Build.bat 重编译 backroomsEditor。回滚器会检查当前文件与备份的哈希，对后续用户改动中止而不覆盖；测试回滚只作用于独立 rollback_sandbox，主工程保留新版。

联机 BuildId 已更新为 `backrooms-room-v4-inventory12`；客户端和服务器均需本次构建，避免 8 格旧客户端加入 12 格服务器。
