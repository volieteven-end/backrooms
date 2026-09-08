# 玩家耐力与空手视角

2026-09-08。适用于房间版本 `backrooms-room-v6-stamina`。

## 当前行为

- 普通冲刺消耗耐力，满值约能连续跑 5 秒；耗尽后回到行走速度。
- 停止冲刺后等待 1.5 秒开始恢复，每秒恢复 25 点。耗尽后至少恢复到 20 点才能再次冲刺；继续按住 Shift 会自动恢复冲刺。
- 被任意实体追击时可以无限冲刺，包含进入追击前耐力已经为零的情况。追击期间保留原有耐力值；最后一个追击者停止、换目标或被销毁后恢复普通限制。
- 无限耐力仅属于被追击的玩家，不影响其他队友，也不会解除倒地、躲藏或蹲下的移动限制。
- HUD 显示当前耐力；被追击时显示“耐力 ∞ · 被追击中”。
- 空手站立、行走、奔跑和蹲下均启用双臂。正常低头即可看到手；平视时手位于视野下方。极低视角使用 55° 的姿态补偿上限，避免露出肩部模型接缝。手电、持罐、饮用与收起动作仍使用现有资源。

## C++ 分工与调参

| 位置 | 职责 |
|---|---|
| `Source/backrooms/Player/BRStaminaComponent.h/.cpp` | 服务器计算消耗、延迟恢复、耗尽门槛和多个追击来源；耐力及追击标记仅复制给所属客户端 |
| `Source/backrooms/Player/BRPlayerCharacter.h/.cpp` | 持有耐力组件，分别维护冲刺按键意图与实际冲刺状态，根据耐力和移动限制切换速度 |
| `Source/backrooms/AI/BREntityCharacter.h/.cpp` | 通用实体基类在进入/退出追击、换目标及销毁时维护目标玩家的追击来源 |
| `Source/backrooms/Player/BRFirstPersonArmsComponent.cpp` | 空手启用双臂，补偿相机俯仰；持物与一次性动作保留原有相机姿态 |
| `Source/backrooms/UI/BRGameHUDWidget.cpp` | 显示普通、耗尽恢复和无限耐力状态 |

在角色蓝图的 `Stamina` 组件上调整以下参数，也可修改 C++ 默认值：

| 参数 | 默认值 | 含义 |
|---|---:|---|
| `MaxStamina` | 100 | 满耐力 |
| `SprintDrainPerSecond` | 20 | 在地面实际移动并冲刺时，每秒消耗；原地按 Shift 不扣除 |
| `RecoveryPerSecond` | 25 | 每秒恢复 |
| `RecoveryDelay` | 1.5 | 最后一次消耗后开始恢复的等待秒数 |
| `SprintResumeThreshold` | 20 | 耗尽后重新允许冲刺的门槛 |

未来实体继承 `ABREntityCharacter`，并通过 `SetEntityState(EBREntityState::Chasing, Player)` 设置目标，即可自动接入。若使用其他 Actor 基类，在**服务器**的追击开始/结束处调用玩家耐力组件的 `SetChasedBy(Entity, true/false)`；蓝图也可调用。每个实体使用自己的 Actor 作为来源，不能用一个全局布尔值覆盖多个追击者。该接口没有客户端 RPC。

## 验证入口

以下测试使用临时运行场景，不保存或修改关卡资产。

- `Automation RunTests Backrooms`：13 项自动化测试，新增耐力消耗/恢复、耗尽、多实体、换目标、销毁、零耐力被追击及倒地限制检查。
- `-BRInventorySmoke`：原有 77 项背包、装备、饮用、移动与台阶检查；空手预期更新为显示双臂。
- `-BRArmsViews -BRInventoryCapture=<目录>`：在停车场生成 10 张平视、低头、蹲下、持物、饮用和无限耐力 HUD 截图，并检查低头时双手投影处于画面内。
- `-BREntitySmoke`：实际 AI 视觉发现玩家后检查无限耐力，丢失目标后检查恢复普通限制，并保留原有朝向和吼声检查。
- `-BRStaminaSmoke`：独立服务器加两个客户端，通过真实客户端冲刺输入检查耗尽减速、零耐力追击、持续超过满耐力时长、队友隔离、多追击者、换目标与恢复。客户端沿用 `-BRTestRole=host/guest -BRExpectedPlayers=2 -BRTestRounds=1` 房间往返流程。

本次本地构建、测试日志及截图位于 `work/player_stamina_hands_20260908/`；运行证据不纳入 Git。Editor 与 Game 的 Win64 Development 构建、13 项自动化、77 项背包回归、16 项实体运行检查、双客户端耐力测试及 10 张画面检查均已通过。本次未重新 Cook/Package。
