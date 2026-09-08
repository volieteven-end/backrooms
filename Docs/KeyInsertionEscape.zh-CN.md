# 中央房间钥匙插入与撤离

停车场开局的四个插槽只有底板和空钥匙孔。队伍先收集散布在柜子中的四把钥匙，HUD 分别显示已收集和已插入数量。集齐后，对每个插槽按 E 插入一把；该插槽显示钥匙，并播放一次插入声。四个位置都插入后，中央门自动打开。玩家走入门后区域才触发全队「逃生成功」页面。

单机结算页可返回主菜单。专服联机保留结算展示五秒后全队回大厅的流程，下一轮重新生成钥匙、清空四个插槽并关闭中央门。

## 场景与实现

- 关卡：`/Game/ReverseAsset/ParkingGarage/LightingStudy/L_MiddleFloor_Dark`。
- 四个 `BR_KeySocket_1` 至 `BR_KeySocket_4` 保留原导入模型的世界位置、缩放和材质，初始隐藏插入钥匙组件。`ABRGarageKeySocket` 负责提示、交互目标、显示和复制状态。
- `ABRGarageKeyManager` 分开记录共享收集数量和插入数量；四把集齐前拒绝插入，同一插槽重复或并发操作只成功一次。目标数仍为四，完成目标发生在插入时。
- `BR_CentralExitDoor` 只控制中央重门门扇，不参与普通房间门的随机解锁。四个插槽完成后才允许自动开启。
- `ABRExtractionZone` 的停车场实例缩到门后，要求中央门完成开启且四个目标完成。`bRequireAllPlayers=false` 使任一未倒地玩家进入即可触发全队结算；其他关卡保留默认全员进入规则。
- `UBRRoundResultWidget` 显示结果。结算时关闭背包、隐藏 HUD、停止移动并锁定移动和视角输入；切图后释放输入锁。

## 修改与验证

`Tools/World/Setup-KeyInsertion.py` 可在编译 Editor 后通过 UE Python 命令行原地配置上述场景；重复执行不创建额外插槽。

`Tools/Testing/Run-KeyInsertionTests.py views --output <目录>` 使用真实 E 输入走完单机流程，并截图验证空插槽、插入提示、已插入模型、中央门和结算页，最后点击返回菜单。

`Tools/Testing/Run-KeyInsertionTests.py network --output <目录>` 启动临时本地专服与两个渲染客户端，通过真实房间协议运行两轮，检查并发 E、重复插入、复制状态、实际穿门、双方结算和下一轮重置。它不代替公网连接验证。

2026-09-08 已通过 Editor/Game Development 编译、13 项原有自动化测试、单机完整流程、两个渲染客户端连续两轮联机流程。重新加载关卡的 44 项检查确认四个插槽在编辑器中也为空，原模型位置和门框均保留；运行截图已核对空插槽、插入状态、开启的门与实际绘制的结算页。
