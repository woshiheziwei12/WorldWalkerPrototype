# World Walker Prototype

使用 Unreal Engine 5.8 和 C++ 开发的单机第三人称多世界 RPG/Roguelite 原型。玩家从中国仙侠风格的 W00 主世界，经不同传送门进入画风和规则独立的异世界；W01 是“灰烬王国”西幻卡牌世界，W02 是沿巴比伦式七级阶梯神塔外墙攀升的 3D 平台跳跃世界。

## 开发环境

- Unreal Engine 5.8.1
- Visual Studio 2022，安装“使用 C++ 的游戏开发”和项目根目录 `.vsconfig` 中列出的组件
- Git

## 首次获取项目

```powershell
git clone https://github.com/woshiheziwei12/WorldWalkerPrototype.git
cd WorldWalkerPrototype
```

`Content/` 不上传 Git，必须从项目成员处取得独立资源包 `WorldWalkerPrototype_Resources_20260814.zip`。先用 `Get-FileHash .\WorldWalkerPrototype_Resources_20260814.zip -Algorithm SHA256` 确认 SHA-256 为 `C469B0C3E3AC62967780856E404290D16D540B97A974FB81979DD001BF2B8F25`。只把压缩包中的 `Content/` 目录解压到仓库根；若解压工具不能筛选目录，先解到仓库外暂存位置，再只复制 `Content/`。必须跳过压缩包内附带的旧版 `ThirdPartyAssets.csv` 和 `Docs/`，不要覆盖当前源码仓库的资源登记与授权记录。确认最终路径为 `WorldWalkerPrototype/Content/...`，并保持 `AnimeCharacters`、`Asian_Village`、`Portals` 和 `Realistic_Starter_VFX_Pack_Vol2` 顶层目录名不变。

然后双击 `WorldWalkerPrototype.uproject`。首次打开可能需要重新生成 Visual Studio 工程、编译 C++ 模块和 Shader；`.sln`、`Binaries/`、`Intermediate/`、`Saved/` 等均为本机生成物，不进入版本控制。

## 当前可玩内容

- W00：大型东方村落夜景、动漫女性角色、待机/行走/奔跑/跳跃、`Shift` 冲刺，以及无需按 `E` 的自动旋涡门。
- W01：西幻营地与村落、四名中文对话 NPC、角色形态切换、十张起始牌、三印共鸣/英勇/格挡/状态系统，以及黑棘誓约骑士 Boss 战。
- W02：复用 W00 主世界的模块化动漫女性、长发和 `abp_Human` 动作，不再加载低模刚性人物或 Warrior 外观。W02 为 14 个角色材质槽创建独立动态实例，把 Toon 边缘光降为 0，并以 `0.035` 中性暗色压低近似 Unlit 的表面输出；W00 原材质不受影响。W02 中禁用 `Q` 外观切换且不加载 W01 战斗。轻点 `Space` 是小跳，按住为大跳，`Shift + Space` 是冲刺跳；三种落地分别有 `0.16/0.27/0.34` 秒硬直。出生基台可拾取“攀岩手甲”，解锁抓边、爬梯、悬挂和翻越；冲刺与攀爬共用 100 点体力。固定 Seed 的 96 段外墙路线每 16 段组成一级神塔，混合窄石台、木梁/破板、连续旋转石阶和实体爬梯，并在第 16/32/48/64/80/96 段生成大型方形存档露台。六座露台依次展示“筑塔者泥版”，天色随玩家到达过的最高高度从深夜单向推进到蓝调、破晓和日出。六层泥砖塔体、第七层蓝釉圣所、Poly Haven 写实堡垒模块与 Wood Planks PBR 共同构成巴比伦式阶梯神塔。
- 世界旅行：W00 青蓝自动旋涡进入 W01；侧方使用同款牌楼和圆形特效、但改为紫金风系配色的交互门进入 W02；W00 中通往 W01/W02 的两座可见牌楼各有 9 个只响应 Pawn/Camera 的 `QueryOnly` 结构碰撞代理，门心保持 330 uu 净空。W01 从出生点附近返回；W02 登顶后穿过无隐形阻挡的塔顶门洞到达露天日出台，玩家可留在塔外看日出，只有主动按 `E` 才返回 W00。

## 内容生成顺序

关闭 Unreal Editor 并完整编译 Editor 后，依次用 UnrealEditor-Cmd 执行：

1. `Scripts/setup_w00_main_world.py`
2. `Scripts/import_w01_fantasy_assets.py`
3. `Scripts/import_w01_world_assets.py`
4. `Scripts/setup_w02_spiral_tower.py`
5. `Scripts/import_w02_kenney_castle_assets.py`
6. `Scripts/import_w02_polyhaven_fort_assets.py`
7. `Scripts/import_w02_quaternius_animation_library.py`
8. `Scripts/setup_world_content.py`

`import_w02_kenney_castle_assets.py` 首次执行需要联网访问 Kenney 作者官网；脚本会校验固定 SHA-256 和 ZIP 成员路径，只把安全成员解到系统临时缓存，导入 26 个已登记的 CC0 城堡模型并支持幂等重跑。

`import_w02_polyhaven_fort_assets.py` 首次执行需要联网访问 Poly Haven 官方下载域名；所有 FBX/JPG 都有固定 URL 和 SHA-256，幂等导入 22 个 StaticMesh、12 张 Texture2D 和 4 个 PBR Material。Modular Fort 01 与 Wood Planks 均为 CC0，重复执行会验证并复用相同稳定资产，因此可从源码和[许可登记](Docs/Licenses/W02/PolyHaven_ModularFort01_CC0.md)复现。

`import_w02_quaternius_animation_library.py` 从 Quaternius 官方 itch.io 零价流程取得 CC0 Standard 动作库，核验固定 ZIP/FBX SHA-256，并导入 43 个动作作为 W02 动作参考库。该外部动作源不会以 Single Node 直接套到运行时角色；当前 W02 与 W00 共用 `abp_Human`，攀爬在兼容姿势上叠加程序化挂边和抬升。详见[授权与哈希登记](Docs/Licenses/W02/Quaternius_UniversalAnimationLibrary2_CC0.md)。

## 最近验证

- `WorldWalkerPrototypeEditor Win64 Development` 联合编译结果为 `Succeeded`。
- W02 实测为 `Run=460`、`Sprint=650`、`AirControl=0.28`、`Gravity=980`。小跳/大跳/冲刺跳理论顶点分别为 `41.4/132.7/157.2 cm`；大跳支持 0.19 秒可变高度、0.12 秒土狼时间与 0.14 秒输入缓冲。W02 使用 W00 同一模块化人物和 `abp_Human`，不切换到不兼容的外部骨架动作。
- 路线实测 3.07 圈、高 5214.1 cm；96/96 个落点、95/95 个相邻段、6/6 座存档露台、6 组实体爬梯和 120 根碰撞脚手架构件均已生成并通过自动闭环。第 32/64 段坠落复位、四段天色推进、顶层选择呈现，以及自动测试代选 `E` 返回 W00 的闭环通过；正常游戏不会替玩家做出返回选择。
- 攀岩手甲专项自动测试通过：拾取、真实跳台边缘检测、0.78 秒动作、落点与胶囊碰撞恢复均为 `Passed`，终点误差 1.85 cm。体力耗尽会中断攀爬并恢复坠落；落地延迟后以每秒 34 点恢复，存档露台立即补满。
- W02 人物运行日志确认 `Model=W00Shared`、14/14 材质槽完成独立暗化，`RimLightIntensity=0.00 ToonTint=0.035`；跳跃离屏图 `HighresScreenshot00037.png` 已确认不再出现整个人物过曝。主世界 W00 的人物材质没有被修改。

成功标记、测试步骤和当前架构详见 [`Docs/项目活文档.md`](Docs/项目活文档.md)。

## 协作约定

- 修改项目前先阅读 `AGENTS.md` 和 `Docs/项目活文档.md`。
- 结构、职责、流程、依赖或构建方式变化时，同一次提交必须更新活文档。
- `Content/` 由 `.gitignore` 排除，只通过团队资源包分发；不要使用 `git add -f` 强制提交资源。
- `.gitattributes` 保留 Unreal 二进制的 Git LFS 规则，仅作为未来显式纳管单个资产时的保护措施。
- 第三方资源清单和授权记录位于 `ThirdPartyAssets.csv` 与 `Docs/`。
