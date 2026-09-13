# Backrooms · 合作逃生

基于 **Unreal Engine 5.8** 的 **1–4 人第一人称合作恐怖游戏原型**。当前可玩内容围绕暗光停车场展开：搜寻物资、管理耐力与 SAN、躲避窃皮者，收集四把钥匙并开启中央出口。

项目使用 C++ 实现玩法与服务端逻辑，配合可编辑的蓝图、UMG 界面和关卡资产。目前处于开发阶段，主要面向 Windows x64。

## 玩法

1. 创建或加入房间，队员准备后由房主开始；也支持房主单人开局。
2. 探索停车场，打开柜子与抽屉，寻找手电、电池、杏仁水和任务钥匙。
3. 避开窃皮者的巡逻与追逐，利用房门阻断视线；队友倒地后可以救援。
4. 全队集齐四把共享钥匙后，在中央房间的四个插槽分别插入钥匙。
5. 中央门开启后，任一未倒地玩家进入门后的撤离区，即可触发全队逃生成功。联机结算后返回大厅，可以继续下一轮。

## 已实现内容

| 系统 | 当前功能 |
| --- | --- |
| 多人房间 | 创建、查询、加入、准备、房主开始、房主转移、跨关卡同步与回合重置 |
| 停车场任务 | 随机物资、四把必需钥匙保底、共享收集进度、独立插槽与中央门撤离 |
| 交互 | 门扇开关、柜子和抽屉搜寻、物品拾取与丢弃、服务端距离及遮挡校验 |
| 背包与生存 | 9 格背包、1 个主手位、2 个口袋；手电电量、备用电池、杏仁水与 SAN |
| 玩家表现 | 第一人称双臂、装备与饮用动作、第三人称装备同步、冲刺耐力与倒地救援 |
| 窃皮者 | 大范围巡逻、近期位置避让、视觉追逐、失去视线后的记忆与退离、动态门导航 |
| 音效 | 环境、交互、脚步、追逐和攻击音效；受击者近身反馈与队友空间音效 |

普通冲刺会消耗耐力；被实体追击的玩家可以无限冲刺，脱离追击后恢复普通规则。任务钥匙由全队共享，不占背包格子。当前停车场通过进入房间、关门和遮挡来躲避追逐，柜子用于搜寻物资。

## 快速开始

### 1. 准备环境并获取工程

- Windows x64。
- Unreal Engine **5.8**。
- 支持 UE 5.8 的 Visual Studio C++ 工具链与 Windows SDK；仓库中的 [.vsconfig](.vsconfig) 可作为组件配置参考。
- Git 与 Git LFS。运行 Python 测试脚本时还需要 Python 3。

```powershell
git lfs install
git clone https://github.com/volieteven-end/backrooms.git
cd backrooms
git lfs pull
```

地图、模型、贴图和音频等二进制资源通过 Git LFS 管理。请获取完整 LFS 资源后再打开工程。

### 2. 编译并打开编辑器

以下 PowerShell 命令均从仓库根目录执行。将 `$ueRoot` 改为本机 UE 5.8 安装目录，后续命令沿用此变量。

```powershell
$ueRoot = 'D:\Unreal5.8\UE_5.8'
$projectPath = (Resolve-Path '.\backrooms.uproject').Path

& "$ueRoot\Engine\Build\BatchFiles\Build.bat" backroomsEditor Win64 Development "-Project=$projectPath" -WaitMutex -NoHotReloadFromIDE
& "$ueRoot\Engine\Binaries\Win64\UnrealEditor.exe" $projectPath
```

当前编辑器启动地图和游戏默认地图均为 `L_MiddleFloor_Dark`，可直接在该地图中运行单机预览。完整的房间流程使用下面的独立进程启动方式。

### 3. 本机多人联调

先启动开发服务器，再启动一个或多个客户端：

```powershell
.\Tools\Multiplayer\Start-Server.ps1 -EditorRuntime -EngineRoot $ueRoot
.\Tools\Multiplayer\Start-Client.ps1 -EditorRuntime -EngineRoot $ueRoot -ServerHost 127.0.0.1

# 每执行一次，增加一个客户端窗口
.\Tools\Multiplayer\Start-Client.ps1 -EditorRuntime -EngineRoot $ueRoot -ServerHost 127.0.0.1
```

客户端会打开主菜单。房主选择「玩游戏」创建房间，其他玩家通过「加入游戏」进入；队员准备后由房主开始。结束联调时停止脚本启动的服务器：

```powershell
.\Tools\Multiplayer\Stop-Server.ps1
```

`-EditorRuntime` 使用 UnrealEditor 的独立 server/game 进程。日志位于 `Saved/Multiplayer/`。

## 操作

| 输入 | 操作 |
| --- | --- |
| WASD / 鼠标 | 移动 / 观察 |
| Shift / Ctrl | 冲刺 / 蹲下 |
| E | 拾取、门柜交互、插入钥匙等 |
| 鼠标左键 | 优先与瞄准的世界目标交互；无交互目标时使用主手物品 |
| Tab | 打开或关闭背包 |
| 1 / 2 | 将对应口袋物品与主手交换 |
| F / R | 手电开关 / 更换电池 |
| G | 丢弃主手物品；背包中丢弃选中物品 |
| Esc | 优先关闭背包；背包关闭时离开房间 |

背包中可单击装备、拖动搬运或交换、右键使用物品。打开背包不会暂停服务器世界。

## 联机配置与打包

房间配置位于 [Config/DefaultGame.ini](Config/DefaultGame.ini) 的 `[/Script/backrooms.BRRoomSettings]`：

| 配置 | 默认值 | 用途 |
| --- | --- | --- |
| `ServerHost` | `127.0.0.1` | 服务器地址 |
| `GamePort` | `7777` | 游戏连接，UDP |
| `BeaconPort` | `15000` | 房间查询与加入预留，UDP |
| `BuildId` | `backrooms-room-v8-skinstealer-ai` | 客户端与服务器的版本匹配 |

启动脚本支持 `-ServerHost`（客户端）、`-GamePort` 和 `-BeaconPort` 覆盖默认值。局域网联机时，将客户端地址改为服务器的内网地址；双方需要相同版本及匹配端口。

使用安装版引擎打包 Game Target 开发客户端：

```powershell
.\Tools\Multiplayer\Build-Packages.ps1 -EngineRoot $ueRoot -InstalledClientOnly
```

默认输出到 `Builds/Multiplayer/Client/`。分发时需要完整打包目录；使用 `Start-Client.ps1 -ClientExe <实际生成的EXE路径>` 可启动主菜单并指定服务器。正式 Client / Server Target 的打包脚本要求已编译的 UE 5.8 源码引擎，具体步骤见 [多人运行与部署](Docs/Multiplayer/README.zh-CN.md)。

源码修改不会自动更新已有的打包程序，联机双方应重新构建匹配版本。

## 工程结构

```text
backrooms.uproject          UE 工程入口
Config/                    地图入口、输入、房间与打包配置
Content/                   关卡、模型、材质、动画、音频和蓝图
Source/backrooms/
  AI/                      窃皮者感知、移动与状态
  Audio/                   游戏音效配置与播放
  Core/                    游戏模式、任务与回合状态
  Interaction/             交互检测与接口
  Online/                  房间协议、Beacon 与网络状态
  Player/                  玩家、背包、耐力与倒地系统
  UI/                      菜单、HUD、背包与结算界面
  World/                   门柜、物资、钥匙插槽与撤离区
  Tests/                   自动化与显式启用的运行探针
Tools/                     启动、打包、场景配置和测试脚本
Docs/                      玩法说明、实现细节与历史验证记录
```

常用编辑入口：

| 内容 | UE 资源路径 |
| --- | --- |
| 暗光停车场 | `/Game/ReverseAsset/ParkingGarage/LightingStudy/L_MiddleFloor_Dark` |
| 主菜单 / 大厅 | `/Game/UI/Menu/Maps/L_MainMenu` / `/Game/UI/Menu/Maps/L_Lobby` |
| 玩家蓝图 | `/Game/Gameplay/Player/BP_BRPlayerCharacter` |
| 背包界面 | `/Game/UI/Inventory/Widgets/WBP_Inventory` |
| 游戏 HUD | `/Game/UI/Menu/Widgets/WBP_GameHUD` |

`Binaries/`、`Intermediate/`、`DerivedDataCache/`、`Saved/`、`Builds/` 和本机测试目录 `work/` 不纳入版本控制。

## 测试与当前状态

编译 Editor 后，可在 UE 控制台执行 `Automation RunTests Backrooms`。场景与联机测试入口如下：

```powershell
# 窃皮者巡逻、关门退离、重新穿门攻击，以及画面和混音录制
python .\Tools\Testing\Run-EntityAITests.py standalone --engine $ueRoot --render --output work/ai-standalone
python .\Tools\Testing\Run-EntityAITests.py network --engine $ueRoot --render --rounds 2 --output work/ai-network

# 钥匙插入、中央门开启、撤离结算与下一轮重置
python .\Tools\Testing\Run-KeyInsertionTests.py network --engine $ueRoot --output work/key-network
```

测试脚本启动独立本地进程，结果、日志和截图保存在指定输出目录。场景测试使用临时运行状态，不保存地图。

2026-09-11 的功能验证记录包括 Editor / Game Win64 Development 构建、13 项自动化、16 项朝向与嘶吼检查，以及单机和双客户端连续两轮各 35 项 AI 场景检查。详情见 [窃皮者行为与验证](Docs/SkinStealerBehavior.md)。

当前每个服务器进程维护一个房间，尚未接入 Steam、账号系统、跨局存档和断线重连恢复。现有本机联调记录不代表公网部署或正式源码专服产品已经验收；这些仍需单独验证。SAN 归零目前只显示警示，未接入额外死亡或幻觉机制。

## 文档导航

- [多人运行与部署](Docs/Multiplayer/README.zh-CN.md)
- [中央房间钥匙插入与撤离](Docs/KeyInsertionEscape.zh-CN.md)
- [背包、物品与交互](Docs/InventoryReferenceRevision.md)
- [玩家耐力与第一人称双臂](Docs/PlayerStaminaAndHands.zh-CN.md)
- [窃皮者 AI 与声音](Docs/SkinStealerBehavior.md)
- [游戏音效与调参](Docs/Audio/README.zh-CN.md)

各功能文档包含带日期的历史记录；运行参数以当前源码和配置为准。部分模型、动画、界面和音频使用既有素材，来源记录可在 [多人素材清单](Docs/Multiplayer/ASSET_MANIFEST.json)、[背包素材清单](Docs/InventoryReferenceAssetManifest.json) 和音效文档中查看。
