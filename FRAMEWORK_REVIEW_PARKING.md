# 停车场单关卡：现有代码框架复用评审

检查日期：2026-08-31  
工程：E:\UNREAL\ue projects\backrooms  
本机验证引擎：UE 5.8.1；工程 EngineAssociation：5.8  
目标流程：找钥匙 → 进屋关门躲避窃皮者 → 全队集合 → 按钮启动电梯下降 → 本关结算。

## 一、结论

**原框架可以继续用，适合“保留底层、修正规则、补齐本关玩法”，不需要推倒重写，也没有引入 GAS 的必要。**

现有代码主要是通用的角色、交互、复制状态、目标计数、撤离门禁和 LAN 会话基础，并未绑定“两张地图”的玩法。尤其是 [BRGameMode.h:21](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/Core/BRGameMode.h#L21) 的 bAutoTravelOnExtraction 已经默认为 false；改为电梯下降后本关结算，不要求重建整个 GameMode。

但“框架可复用”与“停车场已经能完整游玩”是两回事。目前默认地图仍使用模板类，钥匙、房门、安全房间、追杀、电梯和结算尚未串成完整流程。

本次只检查并生成评审材料，没有修改玩法源码、配置或关卡资产。核对的 280 个文件 SHA-256 全部保持一致。

## 二、已有模块如何处理

| 模块 | 结论 | 面向本关的处理 |
|---|---|---|
| BRGameMode / BRGameState / BRPlayerState | 保留 | 继续管理服务器规则和复制状态；补 Completed、Failed、重开和团队钥匙数据 |
| BRPlayerCharacter | 保留 | 复用移动、冲刺、蹲下、相机和输入接口；创建并接入本关角色蓝图 |
| BRInteractionComponent / IBRInteractable | 保留并加固 | 共用钥匙、门、按钮、救援入口；补服务器视线、倒地和阶段校验 |
| BRObjectiveActor | 扩展 | 保留服务器完成与 RepNotify；将“任意目标计数”明确为配置驱动的钥匙目标 |
| BRExtractionZone | 复用检测思路，改造出口实现 | 当前是自动触发区域；新增电梯按钮、关门取消、下降同步和成功结算状态机 |
| BRDownedComponent | 保留并修正 | 禁止倒地施救；从瞬间救起改为计划中的持续救援；补团灭与重开 |
| BREntityCharacter / BREntityAIController | 保留基础，补行为 | 绑定专用控制器；补巡逻、调查、追击、搜索、攻击和安全房间规则 |
| BRSessionSubsystem | 保留 | LAN 开房/查找/加入/退出 API 可继续用；补局前加入、UI 和多人流程验收 |

## 三、优先修正的已确认问题

### P1：默认地图没有接入 BR 框架

**证据类型：实际加载地图与读取蓝图类默认值。**

- [DefaultEngine.ini:7–16](E:/UNREAL/ue%20projects/backrooms/Config/DefaultEngine.ini#L7) 指向模板地图，并设置了全局 BRGameMode。
- 实际加载 [Lvl_FirstPerson.umap](E:/UNREAL/ue%20projects/backrooms/Content/FirstPerson/Lvl_FirstPerson.umap) 后，World Settings 的 GameMode Override 仍为 BP_FirstPersonGameMode。
- 该蓝图继承 Engine.GameModeBase，默认 Pawn 为模板 BP_FirstPersonCharacter，GameState 为 Engine.GameStateBase，PlayerState 为 Engine.PlayerState，均非 BR 类型。
- 地图读取到的 63 个已放置 Actor 中没有 BR 游戏 Actor；工程资产登记中也未发现 BR 派生的角色/敌人/目标蓝图。

**影响：** 按当前默认配置进入地图，会走模板角色与规则，而不是新增 C++ 框架。只改全局默认 GameMode 并不足以完成接线。

**最小改法：** 创建 BRGameMode、BRPlayerCharacter 的蓝图子类，给本关地图显式指定 BR GameMode，并检查其 Pawn、GameState、PlayerState 三项。保留模板输入和美术资源，但不把模板 GameMode 当作已接好的 BR GameMode。

### P1：倒地玩家仍可救起另一名倒地玩家

**证据类型：源码检查 + 原生方法实际复现。**

- [BRPlayerCharacter.cpp:169–188](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/Player/BRPlayerCharacter.cpp#L169) 没有禁止倒地玩家发起交互，救援资格只检查了被救者倒地。
- [BRDownedComponent.cpp:23–33](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/Player/BRDownedComponent.cpp#L23) 只校验权限、对象、非自身和 300 cm 距离，没有校验施救者是否站立。
- 在未保存的编辑器世界创建两个临时原生角色，间距 150 cm；让两者均倒地，再调用 target.interact(reviver)。
- 实际结果：can_interact_while_reviver_downed=true，施救者仍倒地，但目标的 target_downed_after=false。

**影响：** 两名倒地队员之间的救援入口仍被接受，与“全员倒地失败”规则冲突。当前救援还是即时完成，不是计划中的持续 3 秒。

**最小改法：** 服务器交互入口与 Revive 末端都校验施救者状态；持续救援期间重复检查距离、视线、施救者状态及中断条件；全队倒地由 GameMode 统一进入失败阶段。

探针调用了真实原生方法，但不是联网 PIE 测试。两个临时 Actor 均已销毁，地图没有保存。

### P1：旧撤离门禁会跳过倒地队员

**证据类型：源码确定的规则冲突。**

[BRExtractionZone.cpp:67–81](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/World/BRExtractionZone.cpp#L67) 仅统计 PlayerState、Pawn 有效且非倒地的队员。

例：钥匙条件已满足，A 站在电梯里，B 倒地且在外面。现有计数得到 ActivePlayers=1、PlayersInZone=1，撤离条件成立。这符合旧框架“所有非倒地玩家集合”的规则，却不符合新计划“先救起、全部仍连接队员站立并到齐”的规则。

此外，当前区域进入满足条件后会自动触发一次；它没有启动按钮、关门过程取消、下降表现或 Completed 转换。

**最小改法：** 让新电梯门禁显式检查本局仍连接的参与者及其 Pawn/倒地/在舱状态。钥匙齐全只解锁按钮；按按钮后才进入关门；关门期间有人离开或门受阻则回到 Ready。

### P2：房门玩法需要补服务器交互视线校验

**证据类型：源码检查。**

[BRInteractionComponent.cpp:56–71](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/Interaction/BRInteractionComponent.cpp#L56) 的服务器 RPC 校验接口和距离，但没有重新检查遮挡、玩家倒地和当前关卡阶段。射线遮挡检查只发生在本地选中目标时。

**影响：** 客户端选中目标后房门关闭、再由服务器处理请求时，服务器仍可能接受已被遮挡的目标。这里不是指普通本地射线天然能够穿过静止墙体。

**最小改法：** 保留交互组件，在服务器端统一补可达性/视线与状态校验；交互提示委托也需接上更新逻辑。目前声明了 OnFocusedInteractableChanged，但 C++ 没有广播它。

## 四、尚需实现的本关玩法

这些属于待开发功能，不是“编译失败”。

### 1. 窃皮者完整行为

实际读取 BREntityCharacter 默认值：AIControllerClass 是通用 AIModule.AIController，不是 BREntityAIController；AutoPossessAI 为仅关卡放置时接管。

[BREntityAIController.cpp:37–63](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/AI/BREntityAIController.cpp#L37) 仅设置巡逻/追逐等状态。当前项目未登记 BehaviorTree、BlackboardData 或 StateTree 资产，也没有对应 MoveTo、攻击判定、最后目击点搜索和噪声上报接线。

建议保留控制器、感知组件、复制状态，补一套 Behavior Tree + Blackboard + NavMesh 即可。先不同时维护第二套 StateTree。整合视觉/听觉结果，避免听觉事件把仍在视觉中的追击目标降为调查状态。

### 2. 钥匙目标

[BRObjectiveActor.cpp:42–56](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/World/BRObjectiveActor.cpp#L42) 已经通过 bCompleted 防止同一 Actor 重复完成，这部分保留。

需要把 [BRGameState.cpp:11–28](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/Core/BRGameState.cpp#L11) 的“所有 ObjectiveActor 自动累加总数”改为明确的钥匙配置与 KeyId 登记。默认数量按计划设为 3、保持可配置；门和电梯按钮不参与钥匙总数。多人争抢同一钥匙只记一次；掉线/倒地保留团队进度，整局重开才清空。

### 3. 房门与安全房间

建议新增 BRDoorActor 和 BRSafeRoomVolume，统一处理服务器门状态、碰撞/导航阻挡、房间范围、AI 视线与目标失效规则。现有模板 BP_DoorFrame 不等于已完成这套本关逻辑。

房间是否安全应与门关闭、玩家位置和暴露情况关联；开门后可重新被发现。AI 攻击也应检查距离和遮挡。

### 4. 电梯与结算

建议新增 BRElevatorExit，复用旧区域的集合检测思路，状态采用：

**Locked → Ready → Closing → Departing → Escaped**

服务器同步电梯状态和开始时间；下降结束后把 GameState 设为 Completed。单人被抓/全队倒地进入 Failed，重开时清理钥匙、门、AI、电梯状态与计时器。现有枚举包含 Completed/Failed，但当前代码没有完整驱动这些转换。

### 5. 输入、HUD 与 LAN 流程接线

- 原生角色的 Enhanced Input 资产引用目前均为空，代码另有传统按键路径；创建角色蓝图时明确绑定移动/视角/跳跃/冲刺/蹲下/交互，并验证客户端接管及重生后的输入初始化。
- 工程现有 Widget 是模板触屏控件，未发现本关钥匙进度、交互提示、救援进度或结算 HUD。
- [BRSessionSubsystem.cpp:61](E:/UNREAL/ue%20projects/backrooms/Source/backrooms/Online/BRSessionSubsystem.cpp#L61) 允许中途加入，而新计划采用局前加入；应统一服务器入局规则，不只修改菜单显示。

## 五、最小改造顺序

1. **先接线：** 本关灰盒地图 → BRGameMode → BRPlayerCharacter → 正确的 BRGameState/BRPlayerState。
2. **修公共规则：** 倒地交互、施救资格、服务器遮挡校验；补对应回归测试。
3. **先跑无敌人的通关闭环：** 配置驱动钥匙 → 按钮门禁 → 电梯关门/取消/下降 → 成功；同步补失败与整局重开。
4. **再加恐怖玩法：** 房门与安全房间 → NavMesh → 窃皮者巡逻、发现、追击、搜索、攻击。
5. **最后补表现和联机验收：** HUD、动画、声音、素材替换；先双人完整跑通，再验收四人、掉线、倒地救援、多人连点和重复开局。

这一顺序保留原有模块职责，不把逻辑堆进 Level Blueprint，也不因缩小关卡范围而删除可用的复制和会话基础。

## 六、本次验证结果

| 验证项 | 实际结果 | 覆盖范围 |
|---|---|---|
| backroomsEditor / Win64 / Development | Result: Succeeded；exit 0；目标为最新 | Editor 增量构建检查 |
| backrooms / Win64 / Development | Result: Succeeded；exit 0；执行 6 个构建动作 | Game 目标编译与链接，不是打包验收 |
| Backrooms.Framework.GameplayRules | 1 项测试成功、0 失败；exit 0 | 7 条目标计数/撤离纯规则断言 |
| 地图与类默认值读取 | 载入成功；exit 0 | 确认模板 GameMode 覆写、AI 控制器与输入默认值 |
| 倒地施救探针 | 成功复现缺陷；exit 0 | 临时原生 Actor 方法测试；非联网 PIE |
| 源码/配置/资产/工程/原框架说明 SHA-256 | 280/280 一致 | 无新增、删除、内容修改 |

**构建和现有单测通过，不代表新关卡已经完成。** 门、AI、多人复制、电梯与重开的完整流程仍需要新增集成测试。

完整命令、输出与证据见 [本次验证记录](E:/UNREAL/ue%20projects/backrooms/work/framework_review_parking/VERIFICATION.txt)；地图与原生探针结果见 [inspection.json](E:/UNREAL/ue%20projects/backrooms/work/framework_review_parking/inspection.json)。
