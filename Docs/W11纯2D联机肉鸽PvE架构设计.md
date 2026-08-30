# W11 纯 2D 联机肉鸽 PvE 架构设计

> 文档状态：架构基线 v0.4  
> 日期：2026-08-29  
> 实现状态：首轮架构纵切已实现。独立模块、服务器权威状态、Listen Session 边界、地图与数据资产已落地；正式 Sprite/UMG、Steam 邀请与双机验证仍未完成。
> 机制设计：`Docs/W11纯2D肉鸽PvE游戏机制设计.md`
> 仙侠经济、秘籍与宝物：`Docs/W11仙侠经济秘籍与宝物机制设计.md`
> 核心体验冻结：`Docs/W11核心体验冻结表.md`。其中未落地部分是已确认计划，不得误写成当前实现。

## 1. 已确认决策

| 项目 | 决策 |
|---|---|
| 世界编号 | `W11` |
| 暂定技术名 | `W11_RogueSurvival`；玩家可见名称后续确定 |
| 游戏类型 | 纯 2D 俯视角动作肉鸽 PvE |
| 联机范围 | 单人可玩，并保留 1～4 人 Steam 好友合作目标 |
| PvP | 不需要，不设计玩家互相伤害、竞技排名或对战匹配 |
| 动画方案 | Sprite Sheet + Paper2D Flipbook；不使用骨骼、蒙皮、动作重定向 |
| 联机模型 | 房主权威的客户端—服务器模型 |
| 首选服务器 | Steam Lobby + Listen Server；单人使用 Standalone |
| Dedicated Server | 暂不作为首发前置，只保留兼容边界 |
| 关卡与局长 | 约 50 分钟章节路线，五场章节小 Boss 战和一场最终 Boss 战；仙坊由地图节点决定 |
| 战斗能力 | 普通攻击由 `BasicAttackAbility` 决定；四个主动技能逻辑槽；所有角色共有独立基础闪避并共用同一技能效果池 |
| 单人死亡 | 气血归零结束当前构筑并开始轮回；退出/崩溃与死亡分开处理 |
| 联机共同决策 | 章节路线、共享事件、剧情和结局由房主决定；个人构筑与个人仙坊由玩家本人决定 |
| 局外保留 | 保留界痕认知、关系、结局、图鉴与横向内容解锁，不保留永久战力 |

本文把用户最初的“支持联机”理解为合作 PvE。如果以后决定只做纯单机，可以删除 Session/Steam 联机层，其他玩法和数据设计仍可保留。

## 2. 目标与非目标

### 2.1 当前目标

- 在现有 World Walker 工程中增加相互隔离的 W11 世界。
- 玩家看到和制作的内容均采用纯 2D Sprite 表现。
- 单人能够完成完整 PvE 肉鸽流程。
- 允许 1～4 名玩家通过 Steam 房间共同进行同一局游戏。
- 联机时由房主服务器决定怪物、命中、伤害、掉落、随机数和结算。
- 玩法规则和内容数据可扩展，但不提前锁死具体模式。
- 约 50 分钟的单局可以在已完成地图安全节点保存和恢复，死亡轮回不能恢复活动局快照。

### 2.2 当前非目标

- 不设计 PvP、排位、赛季、排行榜或反竞技作弊体系。
- 不确定世界观、美术风格、职业、武器数量和完整数值。
- 不承诺第一版支持房主迁移、断线重连、跨平台或社区专服。
- 不为 W11 接入 W01 的卡牌战斗、职业成长或敌人数据。
- 不把 W00/W01/W02 改造成联机世界。
- 不在原型阶段建设微服务、数据库、Kubernetes 或服务器集群。

## 3. Unreal 与纯 2D 方案

### 3.1 继续使用 Unreal 的理由

纯 2D 本身不是 Unreal 最轻量的使用场景，但 W11 属于现有 Unreal 多世界项目，并且仍需要 Steam 合作联机。继续使用 Unreal 可以复用：

- C++ 工程、打包链、输入和 UMG；
- Actor、Component、GameMode、GameState、PlayerState 等联机框架；
- 属性复制、RPC、角色移动、AI 和导航；
- Steam Online Subsystem 与 Steam Sockets；
- 当前 `WorldDefinition` 自动发现机制。

因此本项目选择 Unreal 的核心原因是“现有工程集成 + 联机能力”，不是因为 Unreal 的 2D 编辑效率高于所有其他引擎。

### 3.2 “纯 2D”在 Unreal 中的含义

W11 的玩家画面和美术资产是纯 2D，但 Unreal 内部 Actor 仍存在于三维坐标系。约定如下：

- 玩法只在一个平面移动，第一版不做跳跃和高度玩法；
- 使用固定俯视摄像机，优先正交投影；
- 角色、怪物、武器、场景和特效使用 Sprite 或 Flipbook；
- 碰撞使用简单 Capsule、Circle、Box 或服务器侧形状检测；
- 不从 Sprite 的不规则透明轮廓生成权威碰撞；
- 深度只用于遮挡排序和调试，不作为可操作维度。

这种实现对玩家和美术是纯 2D，对程序则保留 Unreal 成熟的碰撞、AI 和网络系统。

### 3.3 不需要骨骼动画

W11 不使用：

- Skeletal Mesh；
- Skeleton/骨架；
- 蒙皮权重；
- Animation Blueprint；
- 动作重定向；
- W00/W02 的荧模型和 94 骨映射。

角色和怪物动画使用图片帧顺序播放。基础状态暂定为：

```text
Idle_Up / Idle_Down / Idle_Left / Idle_Right
Move_Up / Move_Down / Move_Left / Move_Right
Attack_Up / Attack_Down / Attack_Left / Attack_Right
Hit
Defeat
```

第一版可以镜像左右方向减少绘制量。武器可使用独立 `PaperSpriteComponent`，按瞄准方向旋转或切换四方向图片，不需要绑在手部骨骼上。

### 3.4 场景制作建议

第一版优先采用小型竞技场式 PvE 地图，而不是先建立复杂 TileMap 管线：

1. 一张或数张大背景图负责地面美术；
2. 独立场景 Sprite 负责墙、掩体和装饰；
3. 简单 Box/Circle 组件负责真实碰撞；
4. 服务器与客户端共用同一份碰撞和出生布局；
5. 通过 Y 坐标计算 `TranslucentSortPriority`，处理角色前后遮挡。

以后确认需要程序化地图或大量房间后，再评估 TileMap、关卡块或外部地图编辑器。Paper2D 的 TileMap 目前仍应谨慎用于发布，因此它不是 W11 原型的关键依赖。

## 4. 总体系统架构

```text
单人模式
┌─────────────────────────────────────────────┐
│ Standalone Game                             │
│ 本地同时运行规则、AI、玩家和纯 2D 表现       │
└─────────────────────────────────────────────┘

Steam 合作模式
┌──────────────── Steam ──────────────────────┐
│ 登录 / 好友邀请 / Lobby / 加入房间          │
└───────────────────────┬─────────────────────┘
                        │
               ┌────────▼────────┐
               │ 房主 Listen     │
               │ Server + Client │
               ├─────────────────┤
               │ W11GameMode     │
               │ RunDirector     │
               │ EnemyDirector   │
               │ Loot/Upgrade    │
               └───────┬─────────┘
                       │ UE Replication
            ┌──────────┼──────────┐
            ▼          ▼          ▼
         Client 2   Client 3   Client 4
```

W11 首发不需要独立后端：

- Steam 负责登录、好友邀请和房间发现；
- 房主进程同时运行服务器规则和自己的客户端；
- 其他玩家作为客户端加入；
- 一局状态只存在于房主内存；
- 本局结束后返回本地 W00 或 W11 大厅。

## 5. 服务器架构选择

### 5.1 当前推荐：Listen Server

合作 PvE 第一阶段使用 **Steam Lobby + Listen Server** 最合适：

- 不需要持续租用服务器；
- 不需要匹配服务、服务器分配器或运维后台；
- 房主可直接邀请 Steam 好友；
- 单机逻辑和联机逻辑可以共用服务器权威实现；
- 对小团队和原型阶段更容易验证完整闭环。

Listen Server 的缺点是房主延迟最低、房主退出会终止本局、房主机器承担 AI 和网络负载。对无排位的好友合作 PvE，这些问题通常可以接受。第一版明确采用“房主退出，本局结束”，不实现主机迁移。

### 5.2 单人仍按服务器规则编写

单人模式使用 Standalone，但战斗代码仍遵循服务器权威入口：

- 不为单机写一套直接改血的捷径；
- 同一个 GameMode/Combat/Run 逻辑在 Standalone 和 Listen Server 运行；
- 客户端 UI 只读取状态并提交意图；
- 这样从单人升级到联机时不需要重写核心玩法。

### 5.3 什么时候才需要 Dedicated Server

出现以下情况之一，再评估无渲染 Dedicated Server：

- 需要公开匹配而不是好友房间；
- 需要陌生人合作且不能接受房主作弊；
- 需要断线后继续存在的长局；
- 需要排行榜、赛季或可信的跨局奖励；
- 房主机器无法承受目标怪物数量；
- 需要社区服务器或官方常驻房间。

如果以后启用 Dedicated Server，Gameplay Framework 和服务器权威边界可以复用，只需替换会话创建、服务器分配和返回流程。UE 官方 Dedicated Server 标准流程以源码版引擎为前提，但这不再是 W11 第一阶段的阻塞项。

## 6. Steam 层职责

建议使用 `OnlineSubsystemSteam`，并在项目中包一层接口，避免玩法代码直接依赖 Steam API：

```text
IW11SessionService
├── FNullSessionService      本地、LAN、自动化测试
└── FSteamSessionService     Steam 登录、邀请、创建/查找/加入 Lobby
```

Steam 层负责：

- 识别 Steam 用户；
- 创建、搜索、加入和离开 Lobby；
- 好友邀请和 Overlay 加入；
- 保存房间元数据：版本、模式、难度、地图、人数、是否进行中；
- 解析并加入房主 Listen Server；
- 可选使用 Steam Sockets/Relay，避免直接暴露房主 IP。

Steam 层不负责：

- 生成怪物；
- 运行 AI；
- 判定攻击命中；
- 计算伤害和掉落；
- 生成升级候选；
- 判定本局结束和奖励。

## 7. Unreal Gameplay Framework 职责

### 7.1 `AW11GameMode`

只在 Standalone 或服务器一侧存在：

- 接受、移除和初始化玩家；
- 校验内容版本、人数和难度；
- 驱动 Lobby、Countdown、Playing、Results 状态；
- 创建权威 `RunSeed`；
- 启动和结束 `RunDirector`；
- 判定全队失败或通关；
- 拒绝非法 RPC 和状态跳转；
- 生成结构化运行日志。

它不创建 UI，不直接播放 Sprite 动画，不调用 Steam Overlay。

### 7.2 `AW11GameState`

服务器写、所有客户端读：

- RunId；
- RunSeed；
- RunPhase；
- 服务器时间和阶段结束时间；
- 难度、地图和规则集 ID；
- 当前阶段、剩余敌人或公共目标；
- 全队共享资源（如果玩法采用）；
- 结束原因。

客户端倒计时基于同步后的服务器时间，不使用各自本地计时器作为权威。

### 7.3 `AW11PlayerState`

保存需要跨角色死亡或重生继续存在的玩家状态：

- 网络玩家标识的安全映射；
- 玩家槽位和准备状态；
- 等级、局内构筑和稳定能力 ID；
- 击杀、伤害、救援等统计；
- 存活、倒地、死亡或掉线状态；
- 最终个人结果。

未确定的玩法字段不提前加入。

### 7.4 `AW11PlayerController`

- 采集本地移动、瞄准和选择输入；
- 把玩家意图发送给服务器；
- 持有仅本地存在的 HUD、菜单和输入模式；
- 接收连接错误、运行结果和返回 W00 指令；
- 不直接修改怪物生命、经验或关卡进度。

### 7.5 `AW11Character`

- 表示局内可控制角色；
- 使用简单碰撞体和 2D 表现组件；
- 通过 Movement、Attributes、Combat、Ability 等组件组合能力；
- 服务器保存真实位置、生命、状态和冷却；
- 客户端显示插值、预测动画和特效。

### 7.6 `AW11Enemy` 与 `AW11EnemyController`

- 只由服务器生成和驱动；
- 客户端不运行决定性 AI；
- 服务器选择目标、寻路、攻击、受击和死亡；
- 客户端只表现复制的位置、状态和动画；
- 敌人表现资产缺失时允许显示占位 Sprite，但不能影响规则。

### 7.7 `AW11RunDirector`

- 持有当前局的阶段状态机；
- 使用独立随机流生成遭遇、升级和掉落；
- 根据实际玩家人数调整强度；
- 请求 EnemyDirector 生成或回收敌人；
- 驱动升级选择、阶段完成和最终结算；
- 不直接操作 HUD 或 Flipbook。

### 7.8 数据驱动规则集

具体玩法尚未确定，因此使用数据资产隔离规则：

```text
UW11RunRuleSet
├── RuleSetId
├── MinPlayers / MaxPlayers
├── DifficultyScaling
├── RunDurationPolicy
├── RevivePolicy
├── ArenaId
├── EncounterTableId
└── BuildPoolId
```

后续改变单局时长、复活方式、升级节奏或人数时，不应要求重写 Steam Session 和底层网络。

## 8. 网络权威边界

| 行为 | 客户端职责 | 房主服务器职责 |
|---|---|---|
| 移动 | 提交方向并本地预测 | 校验、模拟并复制真实位置 |
| 瞄准 | 提交方向或目标点 | 校验频率和合法范围 |
| 攻击 | 请求使用能力 | 校验状态、冷却、资源和施放频率 |
| 命中 | 可播放预测特效 | 执行射线、形状或投射物判定 |
| 伤害 | 显示服务器结果 | 唯一计算者 |
| 怪物 AI | 不做决定 | 选目标、移动、攻击和死亡 |
| 出生/掉落 | 显示 | 唯一生成者 |
| 随机数 | 只消费公开结果 | 创建并推进权威随机流 |
| 升级选择 | 提交候选索引 | 生成候选并校验选择 |
| 普通攻击/主动技能 | 提交能力槽、方向或目标 | 解析共享能力定义并校验状态、冷却、资源与频率 |
| 闪避 | 提交方向 | 校验冷却、位移合法性和无敌窗口 |
| 章节路线/共享剧情 | 展示并接收房主结果；非房主不可提交权威选择 | 只接受房主选择，推进共享路线、事件和结局状态 |
| 个人商店/构筑 | 提交自己的候选或 Offer | 校验所有权并只修改该玩家状态 |
| 阶段/结算 | 展示 | 唯一判定者 |

RPC 必须进行调用者所有权、当前阶段、频率、冷却、资源、数值边界和重复请求校验。不能相信客户端上传的命中对象、伤害数值、瞬移结果、经验或掉落。

## 9. 肉鸽系统的数据边界

建议的数据资产：

```text
UW11CharacterDefinition
UW11EnemyDefinition
UW11AbilityDefinition
UW11ModifierDefinition
UW11EncounterDefinition
UW11LootTableDefinition
UW11BuildPoolDefinition
UW11RunRuleSet
```

约束：

- 定义使用稳定 ID 和 `UPrimaryDataAsset`；
- 运行时复制稳定 ID 和动态数值，不复制整个资产；
- 每个内容资产带 `ContentVersion` 或进入统一内容版本；
- 服务器生成升级候选并保存真实构筑；
- 地图、遭遇、掉落、升级分别使用独立随机流；
- 相同版本、Seed 和决策输入能够复现关键局面；
- 不按怪物名字在 GameMode 中堆积特殊分支。

是否采用 Gameplay Ability System 在玩法原型后决定。技能、Buff、叠层、触发器很多时可只在 W11 使用 GAS；第一版只有少量攻击时，轻量组件更容易维护。

## 10. W11 与现有多世界的集成

### 10.1 当前目录

```text
Source/
└── WorldWalkerW11/                     已实现 Runtime Module
    ├── WorldWalkerW11.Build.cs
    ├── Core/
    ├── Components/
    ├── Game/
    ├── Data/
    ├── Online/
    ├── UI/
    └── Tests/

Content/WorldWalker/Worlds/
└── W11_RogueSurvival/
    ├── Maps/
    │   └── L_W11_RogueSurvival
    ├── Data/
    │   ├── DA_W11_RogueSurvival
    │   ├── DA_W11_RunRules
    │   └── StatChoices/Manuals/Treasures/Services/Enemies/Encounters/
    ├── Art/
    ├── UI/
    └── Audio/
```

`.uproject`、Target.cs、Build.cs、Asset Manager、输入和项目活文档已随 M1 基线更新。后续目录只按实际内容需要扩展。

### 10.2 模块依赖方向

```text
WorldWalkerW11 ──────► WorldWalkerPrototype 当前共享世界接口

WorldWalkerPrototype 不直接 include W11 类型
W00 通过 WorldDefinition 资产发现 W11
```

W11 使用独立 GameMode，禁止继续往当前 `AWorldWalkerGameModeBase` 增加 PvE 肉鸽分支。当前由 `L_W11_` 地图前缀选择 `AW11GameMode`。

### 10.3 进入流程

当前 `UWorldTravelSubsystem::TravelToWorld()` 使用本地 `OpenLevel`，适合 W00/W01/W02 单机切换，但不能完成 Steam 客户端加入房主。W11 使用两阶段流程：

```text
W00 进入 W11 门
    │
    ▼
本地 L_W11_Lobby
    ├── 单人开始 → Standalone 打开 W11 地图
    ├── 创建房间 → 创建 Steam Lobby → 房主 OpenLevel ?listen
    └── 加入房间 → 查找/接受邀请 → JoinSession 连接房主
```

运行结束后：

- 单人直接返回 W11 Lobby 或 W00；
- 房主先向所有客户端公布结算，再销毁 Session；
- 客户端断开后在本地打开 W11 Lobby 或 W00；
- 不让每个客户端在比赛中随意调用本地 `OpenLevel`。

## 11. 2D 表现与规则分离

```text
AW11Character / AW11Enemy
├── CollisionRoot                真实位置与碰撞
├── MovementComponent            移动或服务器 AI 移动
├── AttributeComponent           生命和属性
├── Combat/AbilityComponent      服务器战斗状态
├── UPaperFlipbookComponent      身体表现
├── UPaperSpriteComponent        可选独立武器
└── StatusWidgetComponent        可选血条/名字
```

规则：

- Flipbook 当前帧不能决定命中；
- 攻击时间窗由服务器能力定义决定；
- Sprite 缺失时可以显示占位图，但本局仍能结算；
- 客户端根据复制的状态枚举选择 Flipbook，不复制逐帧序号；
- 一次性 VFX 和飘字由客户端根据规则事件本地生成；
- 如果以后制作 Dedicated Server，服务端可完全跳过表现组件。

## 12. 怪物数量与网络性能

纯 PvE 肉鸽的主要技术风险不是骨骼，而是大量怪物、投射物和特效的联机同步。第一版采用以下策略：

- 服务器运行全部 AI 和伤害逻辑；
- 客户端不为每个怪物运行完整决策树；
- 怪物只复制必要位置、生命、状态和少量动作事件；
- 远处或不活跃怪物降低更新频率；
- 死亡怪物及时销毁或进入对象池，不继续复制；
- 不把每个飘字、火花和音效做成 Replicated Actor；
- 高频子弹优先使用“服务器逻辑弹道 + 客户端表现弹道”，不默认让每一颗视觉子弹成为网络 Actor；
- 第一版先用普通 Actor 和简单 AI，实测证明瓶颈后再考虑 Mass Entity、Replication Graph 或专用批量复制。

初始性能假设，不是最终玩法承诺：

- 1～4 名玩家；
- 同屏几十名活动怪物起步；
- 服务器 Tick Rate 可配置，先以 30 Hz 验证；
- 客户端渲染帧率与服务器 Tick Rate 解耦；
- 目标怪物上限必须通过房主低配机器和真实 Steam 网络测试后确定。

## 13. 存档与跨局成长

### 13.1 第一阶段

- 当前实现的局内状态只存在于 Standalone 或房主服务器内存；目标存档分为“活动局快照”和“界痕档案”两层；
- 房主保存权威 `RunId`、内容版本、RunSeed、章节安全节点、玩家构筑、遭遇和结算；
- 单人和房主只在已完成的地图安全节点写入一个活动局快照。主动退出、崩溃或断电可以恢复该节点；战斗中不做逐帧存档；
- 气血归零时删除活动局快照，清空角色/门派选择、普通攻击、四个主动技能、修为、秘籍、宝物、灵石、路线和临时状态，然后开始新轮回；
- 界痕档案保留史料、众生证词、人物关系阶段、已见结局、最高章节、Boss 图鉴、横向内容解锁、外观、回忆、成就和设置；
- 界痕档案不保存永久气血、攻伐、护体、会心、灵石收入或其他数值战力；
- 史料在抵达下一个安全节点时提交；关键人物关系在对应章节小 Boss 完成后提交；结局在最终 Boss 结算后提交；
- 联机共同路线使用房主档案和房主决定。实际参与到提交点的客户端可以记录已见史料和结局，但未完成相应章节时不能获得人物关系完成标记；
- 暂不制作账号后端和云端角色数值；
- 可使用 Steam Cloud 保存本地档，但不能把客户端档案当作防作弊权威数据。

### 13.2 如果以后增加公共匹配或长期成长

- 再决定是否需要官方后端和 Dedicated Server；
- 奖励结算需要幂等 RunId，避免重复领取；
- 跨局档案应与单局进程分离；
- 存档版本必须兼容内容版本升级；
- 房主作弊风险需要在决定是否做公共经济系统时重新评估。

## 14. 故障处理

第一版至少明确处理：

- Steam 未登录；
- 创建或加入 Lobby 失败；
- 版本不一致；
- 房间已满或本局已开始；
- 连接超时；
- 客户端主动退出或掉线；
- 房主退出或崩溃；
- 结算后无法返回本地世界。

第一版策略：

- 普通客户端退出不终止其他人的本局；
- 房主退出则整局结束并提示原因；
- 不实现主机迁移；
- 是否允许掉线重连在具体局长确定后决定；
- 所有失败都返回明确 UI 状态，不能停在无限加载界面。

## 15. 测试策略

### 15.1 无网络规则测试

- 相同 Seed 和决策输入的遭遇与升级候选可复现；
- 怪物出生表、掉落表和能力引用完整；
- 非法状态不能攻击或重复结算；
- 冷却、伤害、死亡、复活和全队失败边界；
- 玩家人数变化时难度缩放合法；
- DataAsset 稳定 ID 和内容版本校验。

### 15.2 多实例测试

- Listen Server + 一个或多个客户端；
- 加入、准备、开始、战斗、升级、结算、退出完整闭环；
- 客户端伪造伤害、经验、掉落和过快 RPC 被拒绝；
- 50/100/200 ms 延迟下移动和攻击仍可接受；
- 丢包、短暂断网和客户端退出；
- 房主退出时其余客户端得到明确失败原因；
- 单人 Standalone 与多人使用相同规则测试集。

### 15.3 Steam 实机测试

- 两台电脑、两个 Steam 账号；
- 创建、邀请、加入、离开和 Overlay；
- 不同 NAT/网络环境；
- Shipping 构建身份校验；
- 房间和客户端版本不一致时安全拒绝；
- 房主低配机器在目标怪物数量下的 CPU、带宽和帧时间。

## 16. 建议实施里程碑

### M0：冻结边界

状态：已完成。

- 确认 W11、纯 2D、PvE、无骨骼和合作联机方向；
- 确认第一版人数上限；
- 讨论最小玩法循环；
- 不修改现有运行时代码。

### M1：纯 2D 单人切片

状态：架构与规则纵切已完成；正式 Sprite 和场景美术未完成。

- W11 独立模块和测试地图；
- 一个占位玩家、一个占位怪物；
- 四方向移动、一个攻击、生命、死亡；
- 最小遭遇和结束闭环；
- 不接 Steam。

### M2：本地合作闭环

状态：服务器权威类与复制/RPC 边界已实现；双客户端人工闭环和非法 RPC 专项测试未完成。

- Listen Server + 一个客户端；
- GameMode/GameState/PlayerState 权威边界；
- 怪物 AI 只在服务器运行；
- 两名玩家完成同一局；
- 延迟和非法 RPC 测试。

### M3：最小局内构筑

状态：属性、秘籍、宝物、敌人、遭遇和规则数据资产，以及服务器候选/兑现已实现。

- Ability、Modifier、Encounter、BuildPool 数据资产；
- 服务器生成候选、客户端选择、服务器兑现；
- 触发节奏在玩法讨论后确定。

### M4：Steam 好友联机

状态：Online Subsystem 会话边界与 Steam 插件已接入；好友邀请和两账号跨设备验证未完成。

- OnlineSubsystemSteam；
- 创建、发现、加入和好友邀请；
- Steam Lobby + Listen Server；
- 两账号跨设备验证。

### M5：玩法与上线收口

状态：核心体验已部分冻结，尚未实现和验证。

- 已确定约 50 分钟章节路线、五场小 Boss、最终 Boss、单人死亡轮回、房主共同决策和无永久战力的界痕档案；
- 仍需确定目标怪物密度、合作救援、掉线重连、房主退出恢复和横向解锁池管理；
- 根据公开匹配、作弊风险和房主负载决定是否需要 Dedicated Server。

## 17. 后续玩法专题需要回答的问题

已经冻结的局长、章节路线、Boss 结构、单人死亡、房主共同决策、共享技能池和界痕保留规则见 `Docs/W11核心体验冻结表.md`。仍需回答：

1. 普通攻击使用方向键、鼠标瞄准、自动索敌还是可切换；
2. 当前 `F + Q/W/E/R + Space` 试玩布局的发布默认值、完整重绑定和手柄映射；
3. 五章各自的普通战、事件、休整和仙坊节点数量；
4. 合作玩家倒地后是队友救援、章节复归还是淘汰；
5. 难度按玩家人数如何缩放：怪物数量、生命、伤害还是组合；
6. 掉落和修为是个人独立、全队共享还是实例化；
7. 是否需要公共匹配，还是只支持 Steam 好友邀请；
8. 50 分钟长局是否允许掉线重连，房主退出时如何处理；
9. 目标同屏怪物和投射物数量；
10. 横向内容解锁池如何启用、禁用和防止候选稀释。

在这些问题确定前，不提前把未确认的联机救援、发布键位或房主恢复规则固化进内容；当前试玩映射可继续用于验证。

## 18. 架构验收标准

W11 第一阶段架构达到以下条件才算成立：

- W11 技术 ID、目录和类名不误用 W03；
- W11 使用独立 GameMode，不向现有大 GameMode 添加肉鸽分支；
- 角色和怪物动画完全不依赖骨骼资产；
- 单人 Standalone 可以完成一局；
- 两个客户端可以通过 Listen Server 完成同一局；
- 客户端无法自行修改怪物伤害、经验、掉落、升级候选或结算；
- 怪物 AI 只在服务器运行；
- 房主退出会使客户端明确结束，而不是卡死；
- W11 退出后可安全回到本地 W00/W11 Lobby；
- Null 与 Steam 会话实现通过同一个项目接口调用；
- W00/W01/W02 原有单机闭环和自动测试保持通过。

## 19. 参考资料

- Unreal Engine 5.8：[Paper 2D Overview](https://dev.epicgames.com/documentation/unreal-engine/paper-2d-overview-in-unreal-engine)
- Unreal Engine 5.8：[Networking Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-overview-for-unreal-engine)
- Unreal Engine 5.8：[Online Subsystem Steam](https://dev.epicgames.com/documentation/en-us/unreal-engine/online-subsystem-steam-interface-in-unreal-engine)
- Unreal Engine 5.8：[Using Steam Sockets](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-steam-sockets-in-unreal-engine)
- Unreal Engine 5.8：[Setting Up Dedicated Servers（后续可选）](https://dev.epicgames.com/documentation/unreal-engine/setting-up-dedicated-servers-in-unreal-engine)
- Steamworks：[Steam Matchmaking & Lobbies](https://partner.steamgames.com/doc/features/multiplayer/matchmaking)
