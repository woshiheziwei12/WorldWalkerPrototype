# 从本机原神资源导出荧模型与动画并接入 Unreal Engine 5.8

记录日期：2026-08-23  
适用项目：`WorldWalkerPrototype`  
提取工作区：`E:\workspace\gensehnimport`

本文记录本项目已经实际走通的完整链路：从本机游戏 `AssetBundles/blocks` 只读定位并导出荧的模型、贴图和 Unity 动画，修复 AnimeStudio 的 GI ACL database bulk-data 解压问题，在 Unity 中将动作烘焙为带完整骨架的 FBX，再导入 Unreal，并通过隐藏动作源驱动可见荧。

这不是通用的一键拆包教程。游戏版本、Bundle 布局、加密方式和动作组织方式变化后，资源 PathID、所在 `.blk` 和需要的修复都可能变化。文中的路径、数量、日志标记和结果对应 2026-08-23 本机实际处理的版本。

## 1. 最终结果与来源边界

当前运行时并不是“把任意动画 FBX直接套到荧模型”，而是两套 SkeletalMesh 协作：

```text
本机原神 AssetBundles/blocks
├── 荧 HeroEntity 模型 + 贴图
│   └── Unreal 可见模型 SK_WW_Lumine_CurrentGame（201 骨/挂点）
└── AnimationClip
    ├── AnimeStudio ACL 修复后导出 .anim
    ├── Unity Humanoid 重定向 + 60 FPS 全骨架烘焙
    └── Unreal 隐藏动作源 SK_WW_Lumine_OriginalSource
        └── 94 个同名骨的旋转增量 → UPoseableMeshComponent 可见荧
```

资源来源必须分开理解：

| 内容 | 实际来源 | 当前用途 |
|---|---|---|
| 荧模型和角色贴图 | 本机当前游戏 `HeroEntity` | W00/W02 可见角色 |
| 待机、走、跑、冲刺 | 本机游戏的通用 `Avatar_Girl` 少女剑士片段，经 ACL 修复 | 默认移动状态 |
| 434 个 `PlayerGirl` 原始 `.anim` | 本机游戏 | 完整保留，供研究和后续筛选 |
| 角色专属 PlayerGirl 层 | 本机游戏 | 已保留；六段攻击把各自脊柱、手臂、手和 `WeaponR` 稀疏层叠加到对应完整 Humanoid 基础 |
| 六段地面剑击 | 本机游戏的 Ayaka `Attack_01～05/ExtraAttack` 完整基础 + 对应 PlayerGirl 攻击层 | `T` 菜单前六项和鼠标左键连续攻击 |
| 上升、下降、落地缓冲 | `Dive` 的 `Air Up`、`Air Down`、`Crouch` | 替代会让身体翻转的提取片段 |
| Wafflus/Mixamo 动作 | 历史试验资产 | 当前运行时完全不加载 |

因此，“模型来自原神”是准确的；“当前每一个动作都是荧在原游戏里的完整原版动作”并不准确。原始动作全部保留，但当前可玩集合优先选择不会倒置、横躺或只动根骨的完整身体动作。

## 2. 权利与仓库边界

- 原神模型、贴图和动作是权利状态未核验的专有游戏资产，只限本地原型。
- `Dive` 仓库内这些动画文件的独立授权也未核验，只限本地原型。
- 不把提取源、导出的 FBX/PNG/`.anim` 或生成的 Unreal `.uasset` 提交到 Git、公开资源包或发布构建。
- 项目授权记录以 `Docs/W00第三方资源授权记录.md` 和 `ThirdPartyAssets.csv` 为准。
- 游戏安装目录在整个流程中只读，所有中间产物写到 `E:\workspace\gensehnimport`。

## 3. 本机工具与目录

本次实际使用：

| 工具 | 版本/位置 |
|---|---|
| AnimeStudio | `Escartem/AnimeStudio`，提交 `1ccfbc16bf7fe625e8295bac8074ac3b1b9a065b` |
| AnimeStudio 运行时 | .NET 9 CI 构建，另有本地 ACL 修复构建 |
| Unity | `E:\app\Unity\6000.5.9f1\Editor\Unity.exe` |
| Unity FBX Exporter | `com.unity.formats.fbx` 5.1.6 |
| Unreal Engine | `E:\app\ue\UE_5.8`，项目记录版本 5.8.1 |
| Blender | `E:\app\Blender 5.2\blender.exe`，仅用于离线检查，不是正式转换必需项 |

关键路径：

```powershell
$GameRoot   = 'D:\app\yuanshen\HoYoPlay\games\Genshin Impact game'
$BlocksRoot = "$GameRoot\GenshinImpact_Data\StreamingAssets\AssetBundles\blocks"
$Extract    = 'E:\workspace\gensehnimport'
$Project    = 'E:\workspace\WorldWalkerPrototype\WorldWalkerPrototype'
$UnityExe   = 'E:\app\Unity\6000.5.9f1\Editor\Unity.exe'
$UECmd      = 'E:\app\ue\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$UProject   = "$Project\WorldWalkerPrototype.uproject"
```

提取工作区当前结构：

```text
E:\workspace\gensehnimport\
├── AnimeStudio/                         上游源码浅克隆
├── tool-net9/                           官方 .NET 9 CI 构建
├── tool-patched/                        本地 ACL/CAB 处理修复构建
├── maps/
│   └── GI_live_character_animation.json 约 1.61 GiB 的资产索引
├── exports/
│   ├── PlayerGirl_UnityAnim/            434 个原始 PlayerGirl .anim
│   ├── PlayerGirl_Model_CurrentGame/    当前游戏荧模型、贴图和结构报告
│   ├── AvatarGirl_GenericSword_UnityAnim_ACLFixed/
│   ├── AvatarGirl_SwordActionSet_UnityAnim_ACLFixed/
│   └── PlayerGirl_FBX_PlayerGirlSwordComboFixed/ 最终 28 个 FBX
├── github/Dive/                         稀疏检出的通用剑士动作
├── UnityProject/                        Unity 转换工程
├── logs/                                每阶段日志
├── work/                                E 盘缓存和临时文件
└── ExtractionReport.md                  首轮提取统计
```

空间估算：游戏 `blocks` 本身约 49.9 GiB，只读取不复制；434 个原始 `.anim` 约 2.59 GB；若把 434 个动作全部导出为带模型 FBX，还需约 6–7 GiB。应至少给 E 盘预留 15 GiB 额外空间。C 盘紧张时，Unity 工程、Library、日志和所有导出目录都必须放在 E 盘；UE 命令行增加 `-DDC-ForceMemoryCache`。

## 4. 建立 AnimeStudio 资产索引

游戏的角色网格、贴图、Animator、AnimationClip 和外部 `.resS` 往往分散在不同 Bundle。直接打开一个猜测的 `.blk` 常见结果是“找到同名根 GameObject，但没有 Mesh”。因此先为整个 `blocks` 建立 AssetMap 和 CABMap。

官方 .NET 9 构建的 CLI 在本机以 DLL 方式运行：

```powershell
$AnimeBin = 'E:\workspace\gensehnimport\tool-net9\AnimeStudio-net9-1ccfbc16bf7fe625e8295bac8074ac3b1b9a065b\bin'
$AnimeCli = "$AnimeBin\AnimeStudio.CLI.dll"

dotnet $AnimeCli `
  $BlocksRoot `
  "$Extract\maps" `
  --game GI `
  --map_op Both `
  --map_type JSON `
  --map_name GI_live_character_animation
```

本次索引结果：

- 扫描 1,840 个 `.blk`。
- AssetMap 含 4,698,763 个资产条目。
- CABMap 构建时记录 195 个碰撞。
- JSON 位于 `maps/GI_live_character_animation.json`。
- CAB 依赖映射位于工具的 `Maps/GI_live_character_animation.bin`，约 169 MiB。
- 日志为 `logs/build_character_animation_map.log`。

索引很大，不要用普通文本编辑器整体打开。用 `rg` 定位：

```powershell
rg -n -m 30 'Avatar_Girl_Sword_PlayerGirl_HeroEntity' `
  "$Extract\maps\GI_live_character_animation.json"

rg -n -m 100 'Ani_Avatar_Girl_Sword_PlayerGirl_' `
  "$Extract\maps\GI_live_character_animation.json"
```

索引中的 `Source` 字段给出实际 `.blk` 路径，PathID/容器信息用于 AnimeStudio Asset Browser 精确选择资产。

## 5. 导出荧模型和贴图

目标不是名字相似的空根对象，而是依赖解析后的：

```text
Avatar_Girl_Sword_PlayerGirl_HeroEntity
```

已验证流程：

1. 在 AnimeStudio 选择 `GI` 游戏类型。
2. 加载 `GI_live_character_animation` CABMap/AssetMap。
3. 在 Asset Browser 搜索 `Avatar_Girl_Sword_PlayerGirl_HeroEntity`。
4. 选择实际带 `SkinnedMeshRenderer` 的 HeroEntity，而不是只有 Transform 的同名父节点。
5. 连同依赖导出 GameObject、Mesh、Material 引用和 Texture2D。
6. 导出目标固定为：

```text
E:\workspace\gensehnimport\exports\PlayerGirl_Model_CurrentGame\HeroEntity\
└── Avatar_Girl_Sword_PlayerGirl_HeroEntity\
    ├── Avatar_Girl_Sword_PlayerGirl_HeroEntity.fbx
    ├── Avatar_Girl_Sword_PlayerGirl_Tex_Body_Diffuse.png
    ├── Avatar_Girl_Sword_PlayerGirl_Tex_Body_Lightmap.png
    ├── Avatar_Girl_Sword_PlayerGirl_Tex_Body_Shadow_Ramp.png
    ├── Avatar_Girl_Sword_PlayerGirl_Tex_Face_Diffuse.png
    ├── Avatar_Girl_Sword_PlayerGirl_Tex_Hair_Diffuse.png
    ├── Avatar_Girl_Sword_PlayerGirl_Tex_Hair_Lightmap.png
    ├── Avatar_Girl_Sword_PlayerGirl_Tex_Hair_Shadow_Ramp.png
    ├── Avatar_Girl_Tex_FaceLightmap.png
    ├── Avatar_Tex_Face_Shadow.png
    ├── Avatar_Tex_MetalMap.png
    └── 其他保留贴图
```

不要只凭“导出命令无异常”判断成功。早期直接导出 `Avatar_Girl_Sword_PlayerGirl` 的日志明确显示 `has no mesh, skipping`。最终模型必须通过结构检查：

| 指标 | 当前结果 |
|---|---:|
| 层级节点 | 201 |
| SkinnedMeshRenderer | 6 |
| 加权骨骼 | 116 |
| 顶点 | 13,739 |
| 子网格/原始材质槽 | 8 |
| 角色贴图 | 13 张保留，UE 导入脚本固定使用其中 10 张 |

结构报告位于 `exports/PlayerGirl_Model_CurrentGame/ModelValidation.txt`。

## 6. 导出原始 PlayerGirl 动画

通过 AssetMap 搜索名称前缀：

```text
Ani_Avatar_Girl_Sword_PlayerGirl_
```

对索引命中的各个源 `.blk`，加载 CABMap 后按 `AnimationClip` 类型和名称前缀导出。单个 Bundle 的等价 CLI 形式如下：

```powershell
$Block = "$BlocksRoot\00\04161624.blk"

dotnet $AnimeCli `
  $Block `
  "$Extract\exports\PlayerGirl_UnityAnim" `
  --game GI `
  --types AnimationClip `
  --names '^Ani_Avatar_Girl_Sword_PlayerGirl_' `
  --group_assets None `
  --export_type Convert `
  --map_op Load `
  --map_name GI_live_character_animation
```

实际结果：

- 识别并导出 434 个 `PlayerGirl` AnimationClip。
- 全部为非空 Unity YAML `.anim`。
- 总大小 2,588,792,282 字节。
- 输出目录：`exports/PlayerGirl_UnityAnim`。
- 主要日志：`export_PlayerGirl_04161624.log`、`export_PlayerGirl_remaining.log`。

这一阶段只证明“资源被读出”，不证明曲线数值正确，也不证明每个片段是完整身体动作。

## 7. 修复 GI ACL database bulk-data 解压

### 7.1 发现的问题

原始 AnimeStudio 的 `GIACLClip` 会把外部 `.resS` 中的 ACL database bulk data 拼到数据库缓冲区，但调用 native DBACL 时始终把第三个 streamer/database bulk 指针传成空指针。结果是某些 Humanoid muscle 曲线被解成约 `10^20` 到 `10^38` 的异常值，表现为：

- 腿或手臂在一帧内翻转；
- 人物横躺、倒立；
- 只有根骨朝向变化；
- 同一动作左右肢体极端不对称。

### 7.2 项目保留的修复文件

修复副本保存在：

```text
Scripts/AnimeStudio/AnimationClip.cs
Scripts/AnimeStudio/ACL.cs
Scripts/AnimeStudio/ACLExtensions.cs
Scripts/AnimeStudio/AnimationClipConverter.cs
```

关键修改：

1. `GIACLClip` 新增 `m_DatabaseBulkDataOffset` 与 `m_DatabaseBulkDataSize`。
2. 读取外部 stream data 时，先记录 bulk 在拼接后数据库缓冲区中的偏移和长度。
3. `ACLExtensions` 把这两个值传给 `DBACL.DecompressTracks`。
4. `DBACL` 对数据库缓冲区做 16 字节对齐，并把 `dbAligned + bulkDataOffset` 作为 native streamer 指针。
5. 增加越界验证和诊断日志，防止错误偏移静默生成坏曲线。

修复后的核心逻辑等价于：

```text
database buffer = inline ACL database + aligned external .resS bulk
streamer pointer = aligned database base + recorded bulk offset
native DBACL(data, database base, streamer pointer)
```

### 7.3 构建修复版

把上述四个文件的修改同步到 `E:\workspace\gensehnimport\AnimeStudio` 后，使用 .NET 9 构建 CLI：

```powershell
Set-Location 'E:\workspace\gensehnimport\AnimeStudio'
dotnet build AnimeStudio.CLI -c Release -f net9.0-windows
```

原生 `AnimeStudio.ACL.DB.dll`、FBX wrapper 等预编译库仍来自仓库/CI 包，不需要因为这次 C# 指针传递修复而重编。

修复版重新导出的结果单独保存，绝不覆盖 434 个原始 `.anim`：

```text
exports/AvatarGirl_GenericSword_UnityAnim_ACLFixed
exports/AvatarGirl_SwordActionSet_UnityAnim_ACLFixed
```

验证重点不是文件数量，而是大腿、膝、小腿、上臂的曲线范围、左右对称性和连续采样是否恢复合理。项目提供：

```text
Scripts/Unity/AvatarGirlClipDiagnostics.cs
Scripts/inspect_playergirl_animation_samples.py
```

## 8. 为什么还要经过 Unity

AnimeStudio 导出的 `.anim` 是 Unity AnimationClip，不是 Unreal 可以直接使用的格式。并且原神的动作组织中同时存在：

- Humanoid muscle 曲线；
- Generic Transform 曲线；
- 角色专属附件/裙摆/武器辅助骨层；
- 只能叠加在基础姿势上的稀疏动作层。

Unity 在这里承担三件事：

1. 为当前游戏导出的荧模型创建有效 Humanoid Avatar。
2. 把 Humanoid 动作重定向到 PlayerGirl 骨架。
3. 以 60 FPS 采样，烘焙为普通骨骼 Transform 曲线，再用 FBX Exporter 导出。

Unreal 最终读取的是烘焙后的骨骼动画，不需要理解 Unity 的 muscle 系统。

## 9. 准备 Unity 转换工程

工程位置：

```text
E:\workspace\gensehnimport\UnityProject
```

`Packages/manifest.json` 固定：

```json
"com.unity.formats.fbx": "5.1.6"
```

关键资产布局：

```text
UnityProject/Assets/
├── DirectPlayerGirl/
│   └── Avatar_Girl_Sword_PlayerGirl_HeroEntity.fbx
├── AvatarGirlGenericFixedSource/        ACL 修复后的通用少女动作
├── PlayerGirlSource/Animations/         原始 PlayerGirl 动作，供可选附件层
├── GenericSwordGithubSource/            Dive 的 6 个 FBX
├── Editor/
│   ├── AvatarGirlOriginalLocomotionExporter.cs
│   ├── AvatarGirlClipDiagnostics.cs
│   └── PlayerGirlAclFixedExportBridge.cs
└── AvatarGirlOriginalLocomotionTemp/     导出时临时创建，结束后删除
```

项目内保存的 Editor 脚本源位于：

```text
Scripts/Unity/AvatarGirlOriginalLocomotionExporter.cs
Scripts/Unity/AvatarGirlClipDiagnostics.cs
Scripts/Unity/PlayerGirlAclFixedExportBridge.cs
```

`AvatarGirlOriginalLocomotionExporter` 的当前行为：

- 模型路径：`Assets/DirectPlayerGirl/Avatar_Girl_Sword_PlayerGirl_HeroEntity.fbx`。
- 角色根：`Avatar_Girl_Sword_PlayerGirl`。
- 主动作目录：`Assets/AvatarGirlGenericFixedSource`。
- 次级 PlayerGirl 层目录：`Assets/PlayerGirlSource/Animations`。
- 烘焙帧率：60 FPS。
- 普通动作默认不合入次级层；只有设置 `WW_AVATARGIRL_INCLUDE_SECONDARY=1` 才把非 Humanoid 的头发、裙摆、饰品、武器辅助骨层叠加进来。六段攻击由 `ClipSpec.UseSecondaryUpperBody` 强制启用各自 PlayerGirl 层，并只保护根、髋和腿，使脊柱、双臂、双手与 `WeaponR` 能按攻击曲线覆盖完整基础。
- 导出后用 FBX SDK 把核心骨骼改成 UE 运行时代码期望的稳定名称。

Unity Personal 不收费。本机使用 Unity Hub 在线激活 Personal；旧的网页手动 `.alf/.ulf` 激活流程已经不支持 Personal，不要在“输入 Plus/Pro 序列号”页面继续操作。

## 10. 生成最终 28 个 FBX

最终映射由 `AvatarGirlOriginalLocomotionExporter.cs` 的 `ClipSpecs` 定义。当前主要策略：

- `Standby`、`WalkCycle`、`RunCycle`、`SprintCycle`：ACL 修复后的原游戏通用少女完整身体动作。
- `RunBS`、`SprintBS`、游泳、攀爬、受击、死亡等：保留当前筛选结果。
- `Attack_01`～`Attack_05`、`ExtraAttack`：六个不同的 Ayaka 完整 Humanoid 基础，逐一叠加同编号 PlayerGirl 上身/手/武器层，不再复用三段源动作。
- `Jump`：`Air Up.FBX`。
- `FlyNormal`：`Air Down.FBX`。
- `FallToGroundL/H`：`Crouch.FBX`。
- 坏的 `WalkStopL/RunStopL/SprintStopL` FBX仍保留，但运行时停步不再调用；`T` 菜单相应槽使用安全 `Standby` 预览。

完整批处理：

```powershell
$env:WW_AVATARGIRL_ORIGINAL_FBX_OUTPUT = `
  'E:\workspace\gensehnimport\exports\PlayerGirl_FBX_PlayerGirlSwordComboFixed'
Remove-Item Env:WW_AVATARGIRL_ORIGINAL_EXPORT_FILTER -ErrorAction SilentlyContinue
Remove-Item Env:WW_AVATARGIRL_INCLUDE_SECONDARY -ErrorAction SilentlyContinue

& 'E:\app\Unity\6000.5.9f1\Editor\Unity.exe' `
  -batchmode -nographics -quit `
  -projectPath 'E:\workspace\gensehnimport\UnityProject' `
  -executeMethod AvatarGirlOriginalExport.AvatarGirlOriginalLocomotionExporter.Export `
  -logFile 'E:\workspace\gensehnimport\logs\AvatarGirlPlayerGirlSwordComboFixedExport.log'
```

只重做少量动作时使用过滤器，例如：

```powershell
$env:WW_AVATARGIRL_ORIGINAL_EXPORT_FILTER = 'Jump,FlyNormal'
```

成功标记：

```text
AVATARGIRL_ORIGINAL_MERGE_COMPLETE count=28 output=...
PLAYERGIRL_FBX_CORE_BONES_RENAMED count=81 file=...
```

每个输出 FBX 都必须大于 1 KiB，并能被 FBX SDK 重新打开。最终目录为：

```text
E:\workspace\gensehnimport\exports\PlayerGirl_FBX_PlayerGirlSwordComboFixed
```

## 11. 导入 Unreal：可见荧模型

必须先关闭 Unreal Editor，再使用命令行 Python 导入。模型脚本：

```text
Scripts/import_lumine_character.py
```

它会：

1. 核验 1 个 FBX 和 10 张实际使用贴图的固定 SHA-256，防止误导入旧 GI-Assets 或另一个游戏版本。
2. 导入 SkeletalMesh，不在这一步导入动画。
3. 生成稳定资产 `SK_WW_Lumine_CurrentGame`。
4. 创建 Body、Dress、Face、Hair、Hidden 五个材质。
5. 将 Body/Dress/Face/Hair 强制为 Opaque；Dress 双面；只有辅助效果槽用 Masked 隐藏。
6. 保存到 `/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine`。

运行命令：

```powershell
& $UECmd $UProject `
  -run=pythonscript `
  -script="$Project\Scripts\import_lumine_character.py" `
  -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache `
  -abslog="$Project\Saved\Logs\ImportLumineCurrentGame.log"
```

成功标记：

```text
WW_LUMINE_IMPORT_COMPLETE mesh=1 textures=10/10 materials=5/5 ...
```

材质注意事项：Body 和 Face Diffuse 的 Alpha 是原 Shader 功能遮罩，不是透明度。把它接到 Opacity Mask 会裁掉胸腹、骨盆、鼻口和下半张脸，形成“只有一层壳”或“里面空的”。可见四槽必须保持 Opaque。

当前 Unlit Toon 亮度：Body `0.08`、Dress `0.09`、Face `0.09`、Hair `0.10`。

UE 可能报告“无效 bind pose，使用零时刻重绑”和六个子网格缺少 smoothing group；在当前模型上是已知非阻断警告。仍要检查蒙皮、材质槽和角色轮廓，不能笼统忽略其他导入错误。

## 12. 导入 Unreal：隐藏骨架载体和动画

### 12.1 导入隐藏动作源网格

脚本：

```text
Scripts/import_playergirl_animated_source_mesh.py
```

它默认用最终导出目录中的 `Standby` FBX 建立独立隐藏载体：

```text
/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/OriginalSource/
└── SK_WW_Lumine_OriginalSource
```

先让源根指向最终目录：

```powershell
$env:WORLDWALKER_PLAYERGIRL_FBX_ROOT = `
  'E:\workspace\gensehnimport\exports\PlayerGirl_FBX_PlayerGirlSwordComboFixed'

& $UECmd $UProject `
  -run=pythonscript `
  -script="$Project\Scripts\import_playergirl_animated_source_mesh.py" `
  -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache `
  -abslog="$Project\Saved\Logs\ImportPlayerGirlSourceMesh.log"
```

成功标记：

```text
WW_PLAYERGIRL_ORIGINAL_SOURCE_IMPORTED mesh=... skeleton=... source=...
```

注意：该脚本会重建 `OriginalSource` 目录。需要保留该目录中手工资产时，不要直接运行，应先修改脚本或备份；当前目录被定义为生成资产区，可以幂等重建。

### 12.2 先导入四个默认移动循环

移动状态从 `OriginalSource/Animations` 单独加载四个循环。隐藏载体脚本刚刚重建过整个 `OriginalSource`，所以这一步不能省略：

```powershell
$env:WORLDWALKER_PLAYERGIRL_ANIMATION_DESTINATION = `
  '/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/OriginalSource/Animations'
$env:WORLDWALKER_PLAYERGIRL_CLIP_FILTER = `
  'Standby,WalkCycle,RunCycle,SprintCycle'

& $UECmd $UProject `
  -run=pythonscript `
  -script="$Project\Scripts\import_playergirl_original_animations.py" `
  -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache `
  -abslog="$Project\Saved\Logs\ImportPlayerGirlLocomotion.log"
```

成功标记：

```text
WW_PLAYERGIRL_ORIGINAL_ANIMATION_IMPORT_COMPLETE animations=4/4 failures=none
```

### 12.3 再导入 28 个动作集合

脚本仍然是：

```text
Scripts/import_playergirl_original_animations.py
```

默认输入：

```text
E:\workspace\gensehnimport\exports\PlayerGirl_FBX_PlayerGirlSwordComboFixed
```

默认目标：

```text
/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/OriginalSource/Actions
```

把目标切回 `Actions` 并清除过滤器后运行：

```powershell
$env:WORLDWALKER_PLAYERGIRL_ANIMATION_DESTINATION = `
  '/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/OriginalSource/Actions'
Remove-Item Env:WORLDWALKER_PLAYERGIRL_CLIP_FILTER -ErrorAction SilentlyContinue

& $UECmd $UProject `
  -run=pythonscript `
  -script="$Project\Scripts\import_playergirl_original_animations.py" `
  -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache `
  -abslog="$Project\Saved\Logs\ImportPlayerGirlFinalActions.log"
```

只补部分动作时再设置过滤器：

```powershell
$env:WORLDWALKER_PLAYERGIRL_CLIP_FILTER = 'Jump,FlyNormal'
```

可覆盖的环境变量：

```text
WORLDWALKER_PLAYERGIRL_FBX_ROOT
WORLDWALKER_PLAYERGIRL_MESH_PATH
WORLDWALKER_PLAYERGIRL_ANIMATION_DESTINATION
WORLDWALKER_PLAYERGIRL_CLIP_FILTER
```

成功标记：

```text
WW_PLAYERGIRL_ORIGINAL_ANIMATION_IMPORT_COMPLETE animations=28/28 failures=none
```

UE 命令行在本机可能因 DDC fallback/内存缓存噪声返回非零码；必须同时检查完成标记、`failures=none` 和目标 `.uasset` 是否存在。不要只看进程退出码，也不要只看日志中无关的 UnifiedErrorTest/SDK 噪声。

### 12.4 旅行者剑

剑的导入脚本为：

```text
Scripts/import_lumine_traveler_sword.py
```

当前脚本读取已经整理好的本地源：

```text
E:\workspace\content\Models\Weapons\Sword\Traveler
```

生成 `SM_WW_Lumine_TravelerSword`。运行时把源动作的 `WeaponR` 同名映射到可见荧并将剑绑定该动画骨；若动作源缺少 `WeaponR`，才回退为 `Bip001-R-Hand` 加模型参考握持偏移。该步骤与角色动作 FBX 转换相互独立。

## 13. Unity 到 Unreal 后骨骼发生了什么

从 Unity 转到 Unreal，骨骼层级并不会自动变成 UE Mannequin。实际变化如下：

1. Unity Humanoid Avatar 只在烘焙阶段充当重定向抽象；导出的 FBX 仍是 PlayerGirl 自己的骨骼层级。
2. Unity 以 60 FPS 把 muscle 结果采样成每根骨的 Transform 曲线。
3. FBX SDK 把 81 个核心节点名改成稳定名称，避免命名空间和空格差异。
4. UE Interchange 可能把 FBX 名称中的 `:` 规范化为 `_`；运行时代码同时接受原名、下划线名和去命名空间名。
5. 可见当前游戏模型含 201 个骨/挂点；隐藏动作载体与其存在 94 个同名骨，运行时采用 `CompatiblePlayerGirl` 映射。
6. 未被动作源覆盖的额外头发、裙摆和附件骨保持目标模型参考局部姿势，并继承父骨运动。
7. 角色世界位移由 Character Capsule 和 `CharacterMovement` 控制，不使用动画 Root Motion。

运行时不能直接复制烘焙动画的平移和缩放。经过 Unity/FBX/Blender 的骨架载体可能带有由重建 rest pose 产生的局部平移/缩放；直接复制会把蒙皮拉成长尖刺。当前 `WorldWalkerCharacter.cpp` 的做法是：

```text
源组件空间旋转增量
= 源动画组件空间旋转 × 源参考组件空间旋转的逆

目标动画旋转
= 源旋转增量 × 目标参考组件空间旋转
```

只传旋转增量，目标骨长、平移和缩放保持荧自己的参考姿势。所有目标变换按骨架父子顺序先完整计算，再按顺序提交给 `UPoseableMeshComponent`，避免手臂子骨读取上一帧的肩/锁骨变换。

脚底高度不从动画根平移取得，而是每帧读取左右脚最低 Z，调整可见模型的局部 Base Z，使鞋底稳定落在胶囊地面附近。

## 14. 运行时资产关系

关键资产：

```text
/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/
├── SK_WW_Lumine_CurrentGame                  可见模型
├── OriginalSource/
│   ├── SK_WW_Lumine_OriginalSource           隐藏动作载体
│   ├── Animations/                            四个默认移动循环
│   └── Actions/                               28 项 T 菜单/状态动作
└── Weapon/
    └── SM_WW_Lumine_TravelerSword
```

关键 C++：

```text
Source/WorldWalkerPrototype/Characters/WorldWalkerCharacter.cpp
├── ConfigureGenshinMotionSource()   加载隐藏模型与动作
├── BuildLumineBoneMap()             建立 94 骨同名映射（包含可用的 WeaponR）
└── UpdateMainWorldLuminePose()       逐帧传递组件空间旋转增量
```

正确运行日志：

```text
WW_PLAYERGIRL_MOTION_SOURCE_READY locomotion=OriginalGenericSword4 picker=OriginalSwordActionSet28 combo=6 skeleton=SK_WW_Lumine_OriginalSource
WW_LUMINE_SWORD_READY bone=WeaponR authoredWeaponPose=true ...
Lumine bone map built. Source=SK_WW_Lumine_OriginalSource Profile=CompatiblePlayerGirl Links=94
Main world avatar configured. Model=Lumine BoneLinks=94 Locomotion=OriginalPlayerGirl28
```

鼠标左键通过 `MainWorldAttack` 进入六段连击，不走 `T` 菜单预览状态。第一次按下从 `Attack_01` 起手；后续按下进入输入缓冲，在当前动作有效时长约 55% 的衔接点消费并依次推进，六段后仍有输入则回到第一段。当前动作到 92% 仍无缓存时恢复移动状态。Jump、Dash、Roll、离地、切换表现或战斗锁定会清空队列。攻击播放期间使用完整手臂/手指姿态权重，剑由同一动作的 `WeaponR` 曲线驱动。

## 15. 常见失败、原因和修复

| 现象 | 原因 | 当前处理 |
|---|---|---|
| 模型没有颜色 | 只导入 FBX，没有显式导入/绑定贴图材质 | `import_lumine_character.py` 固定哈希导入 10 张贴图并创建 5 个材质 |
| 身体或脸像空壳、缺一大片 | 把 Diffuse Alpha 当透明度 | Body/Dress/Face/Hair 全部 Opaque，只有辅助效果槽隐藏 |
| 脸比身体暗 | Unlit Toon 亮度不同 | Face 提升到 `0.09` |
| 手臂摆动错位 | 异骨架肩轴/骨长不同，或子骨读取旧父变换 | 同源 PlayerGirl 94 骨映射；按父子顺序计算完整组件姿势 |
| 模型出现长尖刺 | 复制了重建 rest pose 的骨平移/缩放 | 只传组件空间旋转增量 |
| 选动作后只转 90° | 动画和目标不在同一个 `USkeleton`，或片段只有根/稀疏层 | 所有运行时动作统一导入 `SK_WW_Lumine_OriginalSource_Skeleton` |
| 跳跃后人物横躺/倒立 | ACL 曲线异常或片段包含错误躯干翻转 | ACL streamer 指针修复；上升/下降改用 `Air Up/Down` |
| 松开 W/A/S/D 后短暂躺地 | `WalkStopL/RunStopL/SprintStopL` 含翻转 | 运行时取消 Stop 播放，速度归零后直接回待机 |
| 六段攻击姿势相同或手臂不跟剑 | 只用了三段复用基础，或没有合入 PlayerGirl 稀疏上身层 | 每段使用独立 Ayaka 完整基础，并强制合入同编号 PlayerGirl 脊柱/手臂/手/`WeaponR` 层 |
| 剑停在原地或离开手 | `WeaponR` 未加入运行时姿态映射，只把 StaticMesh 挂到保持参考组件姿势的辅助骨 | 同名传递 `WeaponR`；缺骨时回退到右手加参考握持偏移 |
| 脚埋进地面 | 动画整体组件姿势下移，根平移又不能安全复制 | 读取最低脚骨，逐帧调整可见模型 Base Z |
| C 盘满/UE 导入很慢 | DDC、Unity Library、临时缓存落在系统盘 | 工作区全部放 E 盘，UE 使用 `-DDC-ForceMemoryCache` |
| UE 命令导入失败或资产锁定 | 编辑器仍开着并持有包/模块 | 关闭 Unreal Editor 后再执行 Commandlet |

## 16. 验证与截图

离线检查脚本：

```text
Scripts/inspect_playergirl_ue_animation.py
Scripts/inspect_playergirl_animation_samples.py
Scripts/render_playergirl_action_blender.py
Scripts/Unity/AvatarGirlClipDiagnostics.cs
```

运行时 QA 参数：

```text
-WWMotionPickerActionTest=<零基动作索引>
-WWMotionDiagnostics
-WWAvatarCapture
```

`-WWMotionPickerActionTest` 会在世界时间约 3 秒走与 `T` 菜单相同的播放路径；`-WWMotionDiagnostics` 每 0.25 秒记录根、大腿、小腿和脚的采样；`-WWAvatarCapture` 在约 4 秒保存截图。

已保留的最终视觉证据：

```text
Saved/Screenshots/QA_Lumine_FinalGenericSwordAttack01.png
Saved/Screenshots/QA_Lumine_AirUpFix.png
Saved/Screenshots/QA_Lumine_AirDownFix.png
Saved/Screenshots/QA_Lumine_LandingFixFinal.png
Saved/Screenshots/QA_Lumine_MovementReleaseFinal.png
```

对应日志：

```text
Saved/Logs/FinalGenericSwordActionVisual.log
Saved/Logs/AirborneFixVisual.log
Saved/Logs/AirborneDownFixVisual.log
Saved/Logs/LandingFixFinalVisual.log
Saved/Logs/MovementReleaseFinal.log
Saved/Logs/AttackComboWeaponR.log
```

连击无人值守验证可传 `-WWAttackComboTest -WWAvatarCapture -WWAvatarCaptureMotion=AttackComboWeaponR`。该入口连续调用真实的 `HandleMainWorldAttackPressed()`，成功日志必须至少包含 `Step=1/6` 到 `Step=6/6`，并再次出现 `Step=1/6`；同时应有 `bone=WeaponR authoredWeaponPose=true`。当前基线截图为 `Saved/Screenshots/QA_Lumine_AttackComboWeaponR.png`。

人工验收至少覆盖：

1. 正面、背面和侧面确认身体、脸、裙装没有透明裁切。
2. 待机、走、跑、冲刺时手脚连续，没有背手锁死。
3. 松开四个移动方向后直接恢复站立，不短暂横躺。
4. 上升、下降和轻/重落地保持竖直。
5. `T` 菜单 28 项都能选择和播放，不只改变朝向。
6. 攻击时剑保持绑定右手 `WeaponR`，不脱离角色。
7. 连续点击鼠标左键时日志和画面按第 1～6 段推进；第 6 段后继续点击回到第 1 段。
8. W00 与 W02 都输出 94 骨映射日志。

## 17. 最短重建顺序

已有 `E:\workspace\gensehnimport` 中间产物时，不必重新扫描 49.9 GiB 游戏资源。最短重建顺序是：

1. 确认 `PlayerGirl_Model_CurrentGame` 的模型和贴图存在。
2. 确认 `PlayerGirl_FBX_PlayerGirlSwordComboFixed` 有最终 28 个 FBX。
3. 关闭 Unreal Editor。
4. 运行 `Scripts/import_lumine_character.py`。
5. 运行 `Scripts/import_playergirl_animated_source_mesh.py`。
6. 把目标设为 `OriginalSource/Animations`，过滤 `Standby,WalkCycle,RunCycle,SprintCycle`，运行一次 `Scripts/import_playergirl_original_animations.py`。
7. 把目标设为 `OriginalSource/Actions`，清除过滤器，再运行一次 `Scripts/import_playergirl_original_animations.py` 导入 28 项。
8. 运行 `Scripts/import_lumine_traveler_sword.py`。
9. 编译 `WorldWalkerPrototypeEditor Win64 Development`。
10. 启动 W00，检查三条运行时成功日志和最终动作截图。

当前本机构建命令：

```powershell
& 'E:\app\ue\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  WorldWalkerPrototypeEditor Win64 Development `
  -Project='E:\workspace\WorldWalkerPrototype\WorldWalkerPrototype\WorldWalkerPrototype.uproject' `
  -WaitMutex -FromMsBuild
```

只有在游戏版本变化、源哈希变化、需要新增动作或中间产物损坏时，才从 AssetMap/AnimeStudio 阶段重新开始。

## 18. 不应删除的保留物

- `exports/PlayerGirl_UnityAnim`：434 个原始 PlayerGirl `.anim`。
- `exports/AvatarGirl_GenericSword_UnityAnim_ACLFixed`：已修复通用少女移动源。
- `exports/AvatarGirl_SwordActionSet_UnityAnim_ACLFixed`：已修复动作集合。
- `exports/PlayerGirl_FBX_PlayerGirlSwordComboFixed`：UE 可重复导入的最终 FBX。
- `Scripts/AnimeStudio`：ACL 修复的代码记录。
- `Scripts/Unity`：Humanoid 重定向和 FBX 烘焙逻辑。
- `logs` 与 `ExtractionReport.md`：数量、版本和故障诊断证据。

早期 `PlayerGirl_FBX_Core`、GI-Assets 对照模型、Wafflus/Mixamo 资源和各种测试输出可以视为历史产物，但删除前应先确认当前脚本没有引用；当前运行时代码只加载 Unreal 中 `GenshinExtract/Lumine/OriginalSource/{Animations,Actions}` 下的生成资产，不直接读取 E 盘外部 FBX。
