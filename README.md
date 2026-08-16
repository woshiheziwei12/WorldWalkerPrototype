# World Walker Prototype

使用 Unreal Engine 5.8 和 C++ 开发的单机第三人称 3D 卡牌 RPG/Roguelite 原型。玩家从中国仙侠风格的 W00 主世界，经自动旋涡传送门进入画风和规则独立的异世界；当前 W01 是“灰烬王国”西幻卡牌世界。

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
- W01：西幻营地与村落、四名中文对话 NPC、月星/灵火/空间音景与探索/战斗音乐、红兜帽形态、带实体平台/边界墙/掉落复位的 3D 接触式四层章节路线与事件选择、经典行动力/法力/装备/反制牌区、七套敌方牌组、带对手图案的战斗 HUD、分类出牌动作/音效、敌方逐牌节奏、逐战三选一卡牌成长和无头骑士守关战。公开资料未给出的 copies、星级与 AI 明确作为原型调参。
- 世界旅行：W00 自动进入 W01，W01 可通过返回门回到 W00。

## 内容生成顺序

关闭 Unreal Editor 并完整编译 Editor 后，依次用 UnrealEditor-Cmd 执行：

1. 首次缺基础资源时：`Scripts/import_w01_fantasy_assets.py`、`Scripts/import_w01_world_assets.py`
2. `Scripts/setup_world_content.py`
3. `Scripts/import_w01_audio_assets.py`
4. `Scripts/import_w01_environment_assets.py`
5. `Scripts/import_w00_environment_assets.py`
6. `Scripts/setup_w00_main_world.py`

成功标记、测试步骤和当前架构详见 [`Docs/项目活文档.md`](Docs/项目活文档.md)。

## 协作约定

- 修改项目前先阅读 `AGENTS.md` 和 `Docs/项目活文档.md`。
- 结构、职责、流程、依赖或构建方式变化时，同一次提交必须更新活文档。
- 项目原创 `.uasset` 和 `.umap` 由 Git LFS 管理；提交前用 `git lfs status` 检查，其他成员同步后运行 `git lfs pull`。
- 第三方资源顶层目录和所有 `ThirdParty/` 目录由 `.gitignore` 排除，只通过团队资源包分发；不要使用 `git add -f` 强制提交。
- 第三方资源清单和授权记录位于 `ThirdPartyAssets.csv` 与 `Docs/`。
