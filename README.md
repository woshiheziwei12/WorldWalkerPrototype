# World Walker Prototype

使用 Unreal Engine 5.8 和 C++ 开发的多世界 RPG/Roguelite 原型。玩家从中国仙侠风格的 W00 主世界，经不同传送门进入画风和规则独立的异世界；W01 是“灰烬王国”西幻卡牌世界，W02 是沿巴比伦式七级阶梯神塔外墙攀升的 3D 平台跳跃世界，W11“太虚问道”正在搭建纯 2D 仙侠动作肉鸽 PvE 与 1～4 人合作架构。

## 开发环境

- Unreal Engine 5.8.1
- Visual Studio 2022，安装“使用 C++ 的游戏开发”和项目根目录 `.vsconfig` 中列出的组件
- Git 与 Git LFS 3.x

## 首次获取项目

```powershell
git lfs install
git clone https://github.com/woshiheziwei12/WorldWalkerPrototype.git
cd WorldWalkerPrototype
git lfs pull
```

`Content/WorldWalker` 中由项目维护的地图、数据资产和共享内容通过 Git LFS 自动取得。第三方资源分两个包，必须按顺序解压到仓库根目录：

1. `WorldWalkerPrototype_Resources_20260814.zip`，SHA-256 `C469B0C3E3AC62967780856E404290D16D540B97A974FB81979DD001BF2B8F25`。
2. `WorldWalkerPrototype_Resources_Incremental_20260815.zip`，SHA-256 `20623A38D9BF3D921616D1BC5E0E24170EF6AA1E5B846880297F8C0F263A8C0C`；出现同名文件时允许覆盖四个 W01 角色网格。

确认最终路径为 `WorldWalkerPrototype/Content/...`，并保持 `AnimeCharacters`、`Asian_Village`、`Portals`、`Realistic_Starter_VFX_Pack_Vol2` 以及各世界 `ThirdParty` 目录不变。增量包同时包含 W00/W01 月夜 HDRI、W01 环境音和离线重建缓存。

然后双击 `WorldWalkerPrototype.uproject`。首次打开可能需要重新生成 Visual Studio 工程、编译 C++ 模块和 Shader；`.sln`、`Binaries/`、`Intermediate/`、`Saved/` 等均为本机生成物，不进入版本控制。

## 当前可玩内容

- W00：大型东方村落 HDRI 夜景、动漫女性角色、待机/行走/奔跑/跳跃、`Shift` 冲刺，以及在 12–18 米安全位置生成、无需按 `E` 的双层水纹自动旋涡门。
- W01：西幻营地与村落、四名中文对话 NPC、月星/灵火/空间音景与探索/战斗音乐、女骑士/法师/游侠/修女四职业、可随时查看的旅途牌组、三章十八层完整旅途、经典行动力/法力/装备/反制牌区、50 套敌方牌组（34 普通、10 精英、6 Boss）、211 张卡牌定义与 80 张启用奖励、金币、商店、删牌、宝箱、12 个祝福、休整点和战后三选一或跳过。法师、游侠、修女各有三条构筑与单级升级。公开资料未给出的 copies、星级与 AI 明确作为原型调参。
- W02：复用 W00 主世界的模块化动漫女性、长发和 `abp_Human` 动作，不再加载低模刚性人物或 Warrior 外观。W02 为 14 个角色材质槽创建独立动态实例，把 Toon 边缘光降为 0，并以 `0.035` 中性暗色压低近似 Unlit 的表面输出；W00 原材质不受影响。W02 中禁用 `Q` 外观切换且不加载 W01 战斗。轻点 `Space` 是小跳，按住为大跳，`Shift + Space` 是冲刺跳；三种落地分别有 `0.16/0.27/0.34` 秒硬直。出生基台可拾取“攀岩手甲”，解锁抓边、爬梯、悬挂和翻越；冲刺与攀爬共用 100 点体力。固定 Seed 的 96 段外墙路线每 16 段组成一级神塔，混合窄石台、木梁/破板、连续旋转石阶和实体爬梯，并在第 16/32/48/64/80/96 段生成大型方形存档露台。六座露台依次展示“筑塔者泥版”，天色随玩家到达过的最高高度从深夜单向推进到蓝调、破晓和日出。六层泥砖塔体、第七层蓝釉圣所、Poly Haven 写实堡垒模块与 Wood Planks PBR 共同构成巴比伦式阶梯神塔。
- W11 架构纵切：独立 `WorldWalkerW11` 模块、正交俯视相机、平面移动、方向攻击、全角色服务器权威闪避、敌人/命中/属性/奖励/修炼三选一/仙坊购买流程，以及可切换 Online Subsystem 的 Host/Find/Join/Leave 会话边界。生成内容含 10 名可选主角、6 个门派与初始术法、12 个悟道属性、9 类秘籍、6 件宝物、2 项仙坊服务、3 类敌人与 3 段试炼。十名角色均已有待机/移动/通用施法/受击 Flipbook，闪避专属帧制作前复用移动动作；六个门派术法和通用命中火花作为角色无关的共享技能资源独立加载。首批角色仅有左右专属帧，上下移动保持最后水平朝向。真正四方向美术、闪避专属帧、正式中文战斗 UMG、战斗声音、Steam 邀请与双机联调尚未完成。
- W11 入口流程已完成：2560×1440 原创仙侠标题背景经缓慢推镜、双层流雾与灵尘组成动态页，配有 64 秒原创循环音乐和完整菜单。“独行启程”会先进入十主角选择，再选择六个门派之一；角色属性与门派初始术法由服务端重新校验并写入本局状态。默认进入后停留在服务器 Lobby，确认道途或使用 `-W11AutoStart` 后才刷出第 1 重敌人。
- W11 主线采用“屠龙者终成恶龙”：玩家与最终 Boss 顾长渊都是界外穿越者。顾长渊曾带领太虚界击败无相魔庭，却在 1372 次世界重启中把“避免悲剧”变成了替所有人决定人生的永久天律。剧情最终考验的不是能否杀死独裁者，而是玩家胜利后是否愿意拆掉王座，而不是成为下一条恶龙。
- W11 开场动画已有 96 秒 AI 制作包，包含完整分镜、逐镜头提示词、顾长渊/魔庭视觉锚点、中文旁白、SRT、镜头 CSV 与音乐音效方向；另有面向 Wan3 的轻量目录，每镜只使用一份完整提示词及零到数张编号图片。当前属于制作材料，尚未作为最终视频接入游戏。
- 世界旅行：W00 青蓝自动旋涡进入 W01；侧方使用同款牌楼和圆形特效、但改为紫金风系配色的交互门进入 W02；W00 中通往 W01/W02 的两座可见牌楼各有 9 个只响应 Pawn/Camera 的 `QueryOnly` 结构碰撞代理，门心保持 330 uu 净空。W01 从出生点附近返回；W02 登顶后穿过无隐形阻挡的塔顶门洞到达露天日出台，玩家可留在塔外看日出，只有主动按 `E` 才返回 W00。

## 主世界动作操作

- `WASD`：移动；按实际速度自动切换 Idle、Walk、Run、Sprint。
- `Shift + WASD`：主世界冲刺；`Shift + Space` 在 W02 触发冲刺跳。
- `Space`：Jump；到达顶点后自动切 Fall，落地按冲击速度自动切 LightLanding、HardLanding 或 Roll。
- `F`：立即触发 Dash，并沿当前输入、当前速度或角色朝向短距离前冲。
- `R`：立即触发 Roll，并沿当前移动方向翻滚；静止时向角色正前方翻滚。
- `鼠标左键`：触发六段剑击。连续点击会缓存输入，在当前动作的衔接窗口依次播放第 1～6 段；第 6 段后仍持续点击会回到第 1 段。跳跃、Dash、Roll 或战斗锁定会结束当前连击。
- 松开移动键：按松开时速度自动选择 LightStop、MediumStop 或 HardStop。
- `T`：站立不动时打开 28 项动作菜单；`W/S` 或方向键选择，`Enter` 播放，`Esc/T` 关闭。移动或跳跃会立即退出动作预览。

## W11 原型操作

- `方向键`：十名可选角色统一支持上、下、左、右平面移动；按键立即改变对应朝向，松开后立即停止，不保留滑行惯性。战斗中保留可见鼠标光标。
- `I / J / K / L`：分别向上、左、下、右发动基础攻击。
- `Space`：沿当前移动方向闪避；静止时沿最后朝向闪避。手柄使用底部面键。
- `鼠标右键`：施展所选门派的初始术法；手柄使用左侧面键。
- `1`～`3`：修为突破时选择一项属性；仙坊阶段 `1`～`6` 购买对应秘籍、宝物或服务。
- `Enter`：仙坊整备完成；联机时等待所有玩家确认后开始下一重试炼。

## 内容生成顺序

关闭 Unreal Editor 并完整编译 Editor。普通 `python` 后处理脚本直接在仓库根运行，其余导入/生成脚本按项目既有方式用 UnrealEditor-Cmd 执行：

1. 首次缺基础资源时：`Scripts/import_w01_fantasy_assets.py`、`Scripts/import_w01_world_assets.py`
2. `Scripts/setup_world_content.py`
3. `Scripts/import_w01_audio_assets.py`
4. `Scripts/import_w01_environment_assets.py`
5. `Scripts/import_w00_environment_assets.py`
6. `Scripts/setup_w00_main_world.py`
7. `Scripts/setup_w02_spiral_tower.py`
8. `Scripts/import_w02_kenney_castle_assets.py`
9. `Scripts/import_w02_polyhaven_fort_assets.py`
10. `Scripts/import_w02_quaternius_animation_library.py`
11. `Scripts/import_w11_hero_portraits.py`，幂等导入十张 1024×1536 透明全身立绘
12. `Scripts/setup_w11_rogue_survival.py`，幂等创建 W11 地图、世界注册、十主角、六门派/术法、属性/秘籍/宝物/服务/敌人/遭遇和标准规则资产
13. `Scripts/import_w11_sect_selection_art.py`，幂等导入六张 7680×4320 AI 超分宗门画卷，并绑定六个 Sect 的风格短句与选择图
14. `Scripts/import_w11_generated_art.py`，幂等导入 W11 原创原型背景与静态角色 Sprite，并绑定三种敌人定义
15. `Scripts/import_w11_entry_assets.py`，幂等导入 W11 2K 标题背景、透明雾层和原创入口音乐
16. `Scripts/process_w11_combat_animation_art.py`，把十角色与七套共享技能源图处理为透明分帧、图集、GIF 预览和机器可读清单
17. `Scripts/import_w11_combat_animations.py`，当前幂等导入 472 帧、89 个 PaperFlipbook、10 个角色动画集、1 组敌人移动样板和 7 个技能表现资产，并分别绑定 Hero、Enemy 与 Ability

`import_w02_kenney_castle_assets.py` 首次执行需要联网访问 Kenney 作者官网；脚本会校验固定 SHA-256 和 ZIP 成员路径，只把安全成员解到系统临时缓存，导入 26 个已登记的 CC0 城堡模型并支持幂等重跑。

`import_w02_polyhaven_fort_assets.py` 首次执行需要联网访问 Poly Haven 官方下载域名；所有 FBX/JPG 都有固定 URL 和 SHA-256，幂等导入 22 个 StaticMesh、12 张 Texture2D 和 4 个 PBR Material。Modular Fort 01 与 Wood Planks 均为 CC0，重复执行会验证并复用相同稳定资产，因此可从源码和[许可登记](Docs/Licenses/W02/PolyHaven_ModularFort01_CC0.md)复现。

`import_w02_quaternius_animation_library.py` 从 Quaternius 官方 itch.io 零价流程取得 CC0 Standard 动作库，核验固定 ZIP/FBX SHA-256，并导入 43 个动作作为 W02 动作参考库。该外部动作源不会以 Single Node 直接套到运行时角色；当前 W02 与 W00 共用 `abp_Human`，攀爬在兼容姿势上叠加程序化挂边和抬升。详见[授权与哈希登记](Docs/Licenses/W02/Quaternius_UniversalAnimationLibrary2_CC0.md)。

## 最近验证

- `WorldWalkerPrototypeEditor Win64 Development` 联合编译结果为 `Succeeded`。
- W11 默认运行入口已改为 DPI 感知的主显示器物理原生分辨率、全屏和 `r.ScreenPercentage=100`；活动首战底图使用 6×6 镜像无缝高清玉石地块与 8192×8192 圆阵叠层，并关闭 Paper2D Motion Blur。玉与石甲妖已接入左右向 12 帧/12 FPS/512×512 移动样板，定向自动化 2/2 通过；风格和主观流畅度通过前不批量扩展到其他角色。既有规则与运行时冒烟仍确认十角色/六门派资源解耦、真实扣除真元并播放共享特效；Steam 双客户端和实际窗口观感仍需人工联调。
- W01 M3 的 `WorldWalker.W01` 13/13 自动化测试通过；20 个固定 Seed 的十八层完整旅途得到 14 次完成、6 次正常战败，无 Fatal、Assert、死锁或流程卡死。女骑士自动策略 10/10，法师 4/10，后者仍需平衡与人工体验验证。
- W01 M5 最终内容生成结果为 211 张卡、80 张启用奖励和 50 名敌人；`WorldWalker.W01` 16/16 通过。游侠 Seed 31001、修女 Seed 31004 均在最终 `W01-M5-v1` 的 Normal 完成十八层并返回 W00。人工试玩与三构筑平衡仍单独保留为 QA，不由自动结果替代。
- W02 实测为 `Run=460`、`Sprint=650`、`AirControl=0.28`、`Gravity=980`。小跳/大跳/冲刺跳理论顶点分别为 `41.4/132.7/157.2 cm`；大跳支持 0.19 秒可变高度、0.12 秒土狼时间与 0.14 秒输入缓冲。W02 使用 W00 同一模块化人物和 `abp_Human`，不切换到不兼容的外部骨架动作。
- 路线实测 3.07 圈、高 5214.1 cm；96/96 个落点、95/95 个相邻段、6/6 座存档露台、6 组实体爬梯和 120 根碰撞脚手架构件均已生成并通过自动闭环。第 32/64 段坠落复位、四段天色推进、顶层选择呈现，以及自动测试代选 `E` 返回 W00 的闭环通过；正常游戏不会替玩家做出返回选择。
- 攀岩手甲专项自动测试通过：拾取、真实跳台边缘检测、0.78 秒动作、落点与胶囊碰撞恢复均为 `Passed`，终点误差 1.85 cm。体力耗尽会中断攀爬并恢复坠落；落地延迟后以每秒 34 点恢复，存档露台立即补满。
- W02 人物运行日志确认 `Model=W00Shared`、14/14 材质槽完成独立暗化，`RimLightIntensity=0.00 ToonTint=0.035`；跳跃离屏图 `HighresScreenshot00037.png` 已确认不再出现整个人物过曝。主世界 W00 的人物材质没有被修改。

成功标记、测试步骤和当前架构详见 [`Docs/项目活文档.md`](Docs/项目活文档.md)。

## 协作约定

- 修改项目前先阅读 `AGENTS.md` 和 `Docs/项目活文档.md`。
- 结构、职责、流程、依赖或构建方式变化时，同一次提交必须更新活文档。
- 项目原创 `.uasset` 和 `.umap` 由 Git LFS 管理；提交前用 `git lfs status` 检查，其他成员同步后运行 `git lfs pull`。
- 第三方资源顶层目录和所有 `ThirdParty/` 目录由 `.gitignore` 排除，只通过团队资源包分发；不要使用 `git add -f` 强制提交。
- 第三方资源清单和授权记录位于 `ThirdPartyAssets.csv` 与 `Docs/`。
