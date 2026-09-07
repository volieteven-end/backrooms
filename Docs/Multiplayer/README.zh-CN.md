# 主菜单与多人停车场 · 使用与部署

## 本轮工程入口

- 项目：`E:\UNREAL\ue projects\backrooms\backrooms.uproject`。
- 可编辑 UMG：`Content\UI\Menu\Widgets\WBP_MainMenu.uasset`，四页共用一个 WidgetSwitcher；父类 `UBRMenuWidget` 处理按钮与异步回调。
- 游戏 HUD：`Content\UI\Menu\Widgets\WBP_GameHUD.uasset`，显示钥匙、交互提示、倒地与结算。
- 菜单地图：`/Game/UI/Menu/Maps/L_MainMenu`。客户端默认打开此图。编辑器启动地图保持原设置；在内容浏览器打开菜单地图再运行即可。
- 独立服务器默认地图：`/Game/UI/Menu/Maps/L_Lobby`。
- 游戏地图：`/Game/ReverseAsset/ParkingGarage/LightingStudy/L_MiddleFloor_Dark`。
- 原暗光停车场文件保持原始 SHA256：`3f51e6edfd22f33933a14a6df832df4e04e7e33c43bbd81a8390efaed12cf47b`。

> 当前交付状态以 `E:\UNREAL\ue projects\backrooms\work\menu_multiplayer_20260906\VERIFICATION.txt` 为准。编辑器运行时独立进程、打包客户端、源码 Server 产品、公网测试是四种不同验收，不互相替代。

## 运行流程

1. 先启动服务器。服务器没有玩家窗口，等待创建房间。
2. 每名玩家各开一个客户端，在右下方输入昵称。
3. 房主点击 **玩游戏**，填写房间名与 1–4 人上限，再点击 **故事模式 · 创建房间**。
4. 队员点击 **加入游戏**，从真实服务器查询结果中加入该房间。列表每 5 秒刷新，也有手动刷新。
5. 大厅显示昵称、房主、准备状态与暗光停车场预览。队员准备；房主开始。允许房主单人开局。
6. 大家通过无缝切图进入同一停车场。WASD 移动，鼠标观察，Shift 奔跑，Ctrl 蹲下，E 交互，Esc 离开房间。
7. 随机门、钥匙与任务进度由服务器决定。所有存活玩家进入撤离区且任务完成后结算；全员倒地也会结算。5 秒后返回大厅重新准备。
8. 房主退出后，最早加入的剩余玩家接任；专服进程继续运行。全员退出后，房间重置。

首版只有一个同时存在的房间；不使用 Steam，不包含账号、私人房间、存档、邀请码、聊天和中途重连恢复。昵称不是身份凭据或房主权限。

## 本机开发联调

PowerShell：

```powershell
& 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Start-Server.ps1' -EditorRuntime
& 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Start-Client.ps1' -EditorRuntime -ServerHost 127.0.0.1
# 再执行一次 Start-Client 以启动另一名玩家。
& 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Stop-Server.ps1'
```

`EditorRuntime` 明确运行 UnrealEditor 的独立 server/game 进程，不是同进程 PIE，也不是已打包的 Server Target。脚本只停止自己记录且核对过 PID、路径、启动时间和日志标记的服务器，不处理其他 UE 项目。

## 打包与产品启动

当前安装引擎 `D:\Unreal5.8\UE_5.8` 可以构建 Game Target 客户端。命令：

```powershell
& 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Build-Packages.ps1' -InstalledClientOnly
```

对应输出：`E:\UNREAL\ue projects\backrooms\Builds\Multiplayer\Client\Windows`。完整文件夹一起复制到玩家电脑，不只复制根目录 EXE。玩家电脑需要 Windows 64 位与匹配的 Microsoft Visual C++ 运行库；本机已有该运行库，本次开发包未内置安装器。

正式 Client / Server Target 使用匹配的、已经编译好的 UE5.8 **源码引擎**，并先用该引擎编译项目的 Editor Target：

```powershell
# 将 E:\UE5.8-source 换成实际源码引擎目录。
& 'E:\UE5.8-source\Engine\Build\BatchFiles\Build.bat' backroomsEditor Win64 Development '-Project=E:\UNREAL\ue projects\backrooms\backrooms.uproject' -WaitMutex
& 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Build-Packages.ps1' -EngineRoot 'E:\UE5.8-source'
# ServerExe 指向该次打包生成的 backroomsServer.exe；以实际输出清单为准。
& 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Start-Server.ps1' -ServerExe 'E:\UNREAL\ue projects\backrooms\Builds\Multiplayer\Server\WindowsServer\backrooms\Binaries\Win64\backroomsServer.exe'
& 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Start-Client.ps1' -ClientExe 'E:\UNREAL\ue projects\backrooms\Builds\Multiplayer\Client\Windows\backrooms\Binaries\Win64\backrooms.exe' -ServerHost 127.0.0.1
```

源码引擎可能输出 `WindowsClient` 和 `backroomsClient.exe`；使用对应的实际路径。不要混用不同引擎构建、不同 BuildId 的客户端与服务器。不要删除安装版引擎的 InstalledBuild 标记冒充源码构建。

## 公网部署

`Config\DefaultGame.ini` 的 `[/Script/backrooms.BRRoomSettings]` 存放主机地址、GamePort、BeaconPort 与 BuildId；按钮里没有硬编码公网地址。

- 默认 UDP **7777** 是游戏连接，UDP **15000** 是房间发现 Beacon。
- 局域网客户端使用服务器内网地址；异地客户端使用服务器的公网 IPv4 或 DNS 名。可用 `Start-Client.ps1 -ServerHost 实际地址` 覆盖默认地址。
- 将这两个 UDP 端口转发到运行服务器的 Windows 电脑的固定内网地址，并在 Windows 入站规则中允许对应服务器程序/端口。
- 自定义端口时，服务器与所有客户端的两个端口都要匹配；启动脚本支持 `-GamePort` 与 `-BeaconPort`。
- 本机成功不证明路由器入站可达。公网验收使用另一网络上的电脑（例如另一宽带或手机热点），记录房间发现、加入、准备、进入停车场和完成游戏。
- 如果公网地址属于运营商共享 NAT，上游入站转发也需要相应的可达网络配置。脚本不自动更改路由器、防火墙或账号设置。
- 运行 `Stop-Server.ps1` 会结束房间，客户端返回主菜单。日志保存在 `Saved\Multiplayer`。

## 实现边界与扩展点

- `UBRRoomDirectorySubsystem`：客户端异步查询、创建、加入、离开；专服保存跨关卡的唯一房间状态。
- `FBRRoomModel`：容量、准备、房主转移、版本检查、预留到期、单次票据。预留占容量，30 秒到期；两名玩家争抢最后一位时，服务端游戏线程串行确认。
- `ABRRoomBeaconClient / HostObject`：在正式游戏连接前查询与预留。切图销毁旧 World 的监听并在新 World 重建；短时失败保留旧列表快照但禁用加入。
- `ABRNetworkGameMode`：PreLogin / Login 复核并消费票据；PostLogin / Logout / 无缝切图处理身份。
- `ABRLobbyGameMode` 与 `ABRMenuGameMode`：不启动停车场任务。停车场 `ABRGameMode` 使用确定性的不同 PlayerStart。
- GameState / PlayerState 复制房间、身份、准备、任务与倒地状态；客户端通过自己拥有的 PlayerController 提交准备和开始。
- 交互由客户端瞄准，再由服务器重新核对距离、遮挡、目标状态与玩家状态。退出自己的躲藏点单独处理。
- 躲藏同步 Occupant / CurrentHideSpot，四人角色保持相关性以及时同步隐藏与退出；断线释放占位。
- 玩家移动使用 CharacterMovement 网络复制；玩家和窃皮者用现有修复后的动画按速度选片，窃皮者攻击事件也复制。未引入 GAS。
- 窃皮者使用服务端感知、导航巡逻、追击和近距离攻击；导航数据沿用现有地图。
- 保留 `BRSessionSubsystem` 作为既有局域网调试代码；新菜单不调用“加入首个搜索结果”。

UI 蓝图可以直接在 UMG Designer 修改。`BRMenuAssetTools.build_menu_assets()` 是生成器，会重建这两个生成型蓝图的控件树；手工美术编辑后不要随意再运行它。

## 自动化与后续验收

```powershell
python -X utf8 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Run-NetworkTests.py' --suite smoke --outcome extract --output 'E:\UNREAL\ue projects\backrooms\Saved\Multiplayer\Tests\Extract'
python -X utf8 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Run-NetworkTests.py' --suite smoke --outcome down --output 'E:\UNREAL\ue projects\backrooms\Saved\Multiplayer\Tests\Downed'
python -X utf8 'E:\UNREAL\ue projects\backrooms\Tools\Multiplayer\Run-NetworkTests.py' --suite faults --output 'E:\UNREAL\ue projects\backrooms\Saved\Multiplayer\Tests\Faults'
```

每个进程记录命令、PID、退出码和日志。传入 `--client-exe` / `--server-exe` 可测试匹配的打包产品。测试只连接 127.0.0.1。`BRNetworkSmoke`、`BRTestRole` 等是显式开发测试参数，正式启动脚本不携带它们。

Smoke 测试会在服务端控制角色进入躲藏点、拾取钥匙、移动到撤离区或设置倒地，验证真实复制与结算路径；它不代替真人从出生点完整走到钥匙、开门、躲避怪物和到达电梯的可玩性验收。源码专服产品、远端公网及完整人工游玩仍需分别记录结果。

## 素材与恢复

- 选择性导出目录：`E:\Reverse Assets\EscapeTheBackrooms\UI`。
- 来源、包内路径、哈希、用途：本目录 `ASSET_MANIFEST.json`。
- 修改前备份：`work\menu_multiplayer_20260906\original`。
- 回滚入口、精确命令和已验证结果：同工作目录的 `ROLLBACK.sh`、`ROLLBACK.ps1` 与 `VERIFICATION.txt`。回滚前关闭本项目编辑器和测试进程。恢复源文件之后重新编译项目，不把旧资产与本轮新 DLL 混用。

## Epic 文档

- [Online Beacons](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-online-beacons-in-unreal-engine)
- [Dedicated Server 构建](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers-in-unreal-engine)
- [多人关卡切换](https://dev.epicgames.com/documentation/en-us/unreal-engine/travelling-in-multiplayer-in-unreal-engine)

## 本轮测试发现并修复

- UMG 人数下拉框的 AddOption 只改变运行时选项；改为初始化时明确填充 1–4 并默认选中 4，已通过真实 UMG 按钮委托链路验证创建。
- 四人角色隐藏后需要仍保持网络相关性；已确保其他客户端收到隐藏/退出状态，而不只验证角色拥有者。
- 打包版无缝切图中旧 PlayerController 可能暂时保留 Local 标记而失去 LocalPlayer；界面更新现在检查真实 LocalPlayer，并检查 CreateWidget 返回值。
- 自定义端口联调发现客户端查询与正式连接可能使用不同端口配置；现在两个启动脚本分别传入 `-BRGamePort`、`-BRBeaconPort`，7778 / 15001 已通过四人打包客户端联调。
- 界面截图计时改为在新地图界面建立后开始，避免新 PlayerController 的 BeginPlay 尚未调用时提前截取空大厅。最终截图使用 `ui_final_02`，不是先前的截图批次。
- 回滚在独立项目副本执行，原 Source/Config 恢复后重新构建 Editor 与 Game，原 GameplayRules 自动测试通过。

## 2026-09-06 验收记录

所有下列联机测试的服务器均为 **UnrealEditor-Cmd 独立 -server 进程**，网络地址均为 **127.0.0.1**。

| 检查 | 已观察结果 | 证据目录（工作目录下） |
|---|---|---|
| 编译、开发客户端打包 | Editor 编译成功；Game Target 开发客户端打包成功，退出 0 | `MODIFIED_build_12.log`、`CLIENT_PACKAGE_07.log` |
| 框架测试 | 原框架 1 项通过；修改后 4 项通过 | `FRAMEWORK_BASELINE.log.json`、`FRAMEWORK_MODIFIED.log.json` |
| 真实 UMG 控件链路 | 两个带渲染的打包客户端，通过真实 OnClicked 委托执行创建/浏览/加入/准备/开始，进入停车场并返回大厅，客户端正常退出 0 | `ui_integration_packaged_04/RESULT.json` |
| 四人撤离闭环 | 四个打包客户端，两轮进入停车场、四个不同出生点、隐藏/退出/钥匙/门复制、导航存在、AI 移动；触发真实撤离区结算后返回大厅 | `network_extract_packaged/RESULT.json` |
| 四人倒地闭环 | 四个打包客户端，两轮全员倒地，服务端实际判断失败并返回大厅 | `network_down_packaged/RESULT.json` |
| 房间异常 | 空列表、版本不匹配、四人满员、第二次创建、非房主开始 RPC、普通玩家断线、房主转移、全员退出、服务器停止后返回菜单 | `network_faults_packaged_02/RESULT.json` |
| UI 画面 | 打包版离屏渲染；四页各 1080p/1440p，另有空列表，共 9 张。中文、人数选项与真实大厅名单已检查 | `ui_final_02/RESULT.json` |

工作目录：`E:\UNREAL\ue projects\backrooms\work\menu_multiplayer_20260906`。

这些自动测试不是 OS 鼠标点击，也不是真人通关录像。抢最后一个预留位、票据过期/单次消费与重复开始由模型自动测试覆盖；尚未做两个异地客户端同时抢位的网络实测。交互距离/遮挡复核已实现，非法目标 RPC 的专项实测尚待补充。角色与怪物沿用已修复的动画资源，但本轮没有把所有动画逐帧比对原游戏。

**发布前尚待完成：**

1. 提供匹配 UE5.8 源码引擎的实际路径，构建并运行正式 `backroomsServer` 产品；安装版 Server Target 探测返回退出码 6。
2. 另一网络的电脑完成发现、加入、游玩与断线验收，并记录双方日志。当前公网结果为 `NOT_RUN`。
3. 真人完整游玩停车场，确认出生可达性、实际开门/拿钥匙/躲藏、怪物追逐和电梯撤离；补充人为失败切图与非法交互 RPC 测试。

已有自动化中发生过的失败日志保留在工作目录。最终采用上表所列批次；旧批次的编译锁、人数下拉选项、切图空 LocalPlayer、端口覆盖和截图时序问题均有后续修正记录，旧失败不被改写成通过。
