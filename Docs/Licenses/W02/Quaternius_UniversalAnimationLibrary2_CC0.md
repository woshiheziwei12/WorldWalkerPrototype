# W02 Quaternius Universal Animation Library 2 授权证明

## 资源信息

- 资源名：Universal Animation Library 2 — Standard
- 作者与发行方：Quaternius
- 作者官网：https://quaternius.com/packs/universalanimationlibrary2.html
- 官方 itch.io 页面：https://quaternius.itch.io/universal-animation-library-2
- 获取日期：2026-08-19
- 许可证：Creative Commons Zero 1.0 Universal（CC0 1.0）
- CC0 正文：https://creativecommons.org/publicdomain/zero/1.0/
- 官方 Standard ZIP 大小：`18735003` bytes
- 官方 Standard ZIP SHA-256：`4008EA208A604773A2B2177D965F0F5D3195498B5BF838C3F5785D68E95F2A68`
- 包内 FBX：`Universal Animation Library 2[Standard]/Unity/UAL2_Standard.fbx`
- 包内 FBX SHA-256：`D26D0E9F4A202D473194C056045143095A605A53BA1D823EF24055BE4B86851D`

作者官网与官方 itch.io 页面均把该动作库标记为 CC0。官方 itch.io 页面说明完整库包含 130 余个动作、提供原地与 Root Motion 版本并测试支持 Unreal Engine 5；Standard 免费包提供 43 个动作。获取时官方页面显示用户评分为 5.0/5（36 个评分）。

## 本项目使用方式

项目不重新发布原始 ZIP。`Scripts/import_w02_quaternius_animation_library.py` 通过 itch.io 官方零价下载流程取得 Standard ZIP，逐字核对归档与 FBX 的 SHA-256，只解出固定 FBX 和许可证文件，并把 43 个动作导入到 W02 私有目录：

`/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/Quaternius/UniversalAnimationLibrary2/Animations/`

导入目标使用项目现有 AnimeCharacters 女性角色骨架。W02 玩法固定引用并重命名以下四个动作：

| 原始动作 | 稳定目标名 | 用途 |
| --- | --- | --- |
| `NinjaJump_Start` | `A_W02_Jump_Start` | 大跳、小跳与冲刺跳起跳 |
| `NinjaJump_Idle_Loop` | `A_W02_Jump_Loop` | 空中循环 |
| `NinjaJump_Land` | `A_W02_Jump_Land` | 按冲击强度变化时长的落地硬直 |
| `ClimbUp_1m` | `A_W02_Climb_1m` | 抓边、爬梯与翻越 |

脚本支持 `WORLDWALKER_W02_UAL2_ROOT` 指向离线缓存，并在每次执行时重新验证固定哈希、动作数量、目标骨架与四个必需动作。成功标记必须为 `W02_UAL2_ANIMATION_IMPORT_COMPLETE animations=43 required=4/4`。
