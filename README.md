# World Walker Prototype

使用 Unreal Engine 5.8 和 C++ 开发的单机第三人称 3D 卡牌 RPG/Roguelite 原型。玩家从中国仙侠风格的 W00 主世界，经自动旋涡传送门进入画风和规则独立的异世界；当前 W01 是“灰烬王国”西幻卡牌世界。

## 开发环境

- Unreal Engine 5.8.1
- Visual Studio 2022，安装“使用 C++ 的游戏开发”和项目根目录 `.vsconfig` 中列出的组件
- Git

## 首次获取项目

```powershell
git clone https://github.com/woshiheziwei12/WorldWalkerPrototype.git
cd WorldWalkerPrototype
```

`Content/` 不上传 Git，必须从项目成员处取得独立资源包 `WorldWalkerPrototype_Resources_20260814.zip`。其 SHA-256 是 `C469B0C3E3AC62967780856E404290D16D540B97A974FB81979DD001BF2B8F25`。把压缩包直接解压到仓库根目录，确认最终路径为 `WorldWalkerPrototype/Content/...`，并保持 `AnimeCharacters`、`Asian_Village`、`Portals` 和 `Realistic_Starter_VFX_Pack_Vol2` 顶层目录名不变。

然后双击 `WorldWalkerPrototype.uproject`。首次打开可能需要重新生成 Visual Studio 工程、编译 C++ 模块和 Shader；`.sln`、`Binaries/`、`Intermediate/`、`Saved/` 等均为本机生成物，不进入版本控制。

## 当前可玩内容

- W00：大型东方村落夜景、动漫女性角色、待机/行走/奔跑/跳跃、`Shift` 冲刺，以及无需按 `E` 的自动旋涡门。
- W01：西幻营地与村落、四名中文对话 NPC、角色形态切换、十张起始牌、三印共鸣/英勇/格挡/状态系统，以及黑棘誓约骑士 Boss 战。
- 世界旅行：W00 自动进入 W01，W01 可通过返回门回到 W00。

## 内容生成顺序

关闭 Unreal Editor 并完整编译 Editor 后，依次用 UnrealEditor-Cmd 执行：

1. `Scripts/setup_w00_main_world.py`
2. `Scripts/import_w01_fantasy_assets.py`
3. `Scripts/import_w01_world_assets.py`
4. `Scripts/setup_world_content.py`

成功标记、测试步骤和当前架构详见 [`Docs/项目活文档.md`](Docs/项目活文档.md)。

## 协作约定

- 修改项目前先阅读 `AGENTS.md` 和 `Docs/项目活文档.md`。
- 结构、职责、流程、依赖或构建方式变化时，同一次提交必须更新活文档。
- `Content/` 由 `.gitignore` 排除，只通过团队资源包分发；不要使用 `git add -f` 强制提交资源。
- `.gitattributes` 保留 Unreal 二进制的 Git LFS 规则，仅作为未来显式纳管单个资产时的保护措施。
- 第三方资源清单和授权记录位于 `ThirdPartyAssets.csv` 与 `Docs/`。
