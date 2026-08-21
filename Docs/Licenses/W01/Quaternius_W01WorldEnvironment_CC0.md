# W01 / W02 Quaternius 西幻环境资源授权证明

## 采用的资源包

以下三套资源均由 Quaternius 上传至 OpenGameArt，页面明确标记为 CC0。下载归档仅保存在本机资源缓存中，项目只导入经过筛选的 FBX。

| 资源包 | 来源与许可证明 | 下载归档 | 归档 SHA-256 |
| --- | --- | --- | --- |
| LowPoly Medieval Village Pack | https://opengameart.org/content/lowpoly-medieval-village-pack | `medieval_village_pack_-_dec_2020.zip` | `C38D8632C3C883043809A5DB9B2F47B7033E298AB4BD073BB5C29AD323DF60FC` |
| LowPoly Medieval Buildings | https://opengameart.org/content/lowpoly-medieval-buildings | `Modular Medieval Pack by @Quaternius_0.zip` | `A54AB75EBA5EB7FF4A95736BF6D16016684370EEBD986E3EF64F281D66290891` |
| LowPoly Nature Pack | https://opengameart.org/content/lowpoly-nature-pack | `Nature pack vol.3.zip` | `7BF32888A521E93C94D82FC2CEBDA482DE52C014CDE9D406DC62DA1575F9070D` |

- 作者：Quaternius
- 获取日期：2026-08-14
- 许可证：CC0 1.0 Universal / Public Domain Dedication
- CC0 正文：https://creativecommons.org/publicdomain/zero/1.0/
- 村庄包内证明文件：`Medieval Village Pack - Dec 2020/License.txt`

村庄包内的许可证原文为：

```text
LowPoly Models by @Quaternius

License:
CC0 1.0 Universal (CC0 1.0)
Public Domain Dedication
https://creativecommons.org/publicdomain/zero/1.0/
```

## 本项目筛选并导入的文件

所有 StaticMesh 首先导入到 `/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Environment/`。每个 FBX 使用独立的同名子目录，避免不同资源包中的通用材质或纹理名互相覆盖；表中的稳定目标名同时是子目录、package 名和 object 名，例如 `SM_W01_HouseA/SM_W01_HouseA.SM_W01_HouseA`。

W02 对本页三套 Quaternius 资源不新增下载或重新导入。`Scripts/setup_w02_spiral_tower.py` 通过 Unreal 的资产复制接口，把 `SM_W01_Tower`、`SM_W01_Wall`、`SM_W01_Arch`、`SM_W01_Campfire` 和 `SM_W01_Path` 连同各自私有材质目录复制到 `/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/Quaternius/Environment/`。运行时只引用 W02 副本；副本缺失时使用 Engine 基础几何体，不跨世界读取 W01 package。W02 新增的 Kenney Castle Kit 来源和校验记录另见 `Docs/Licenses/W02/Kenney_CastleKit_CC0.md`。

| 原始 FBX | SHA-256 | 稳定目标名 |
| --- | --- | --- |
| `Buildings/FBX/House_1.fbx` | `437024ABC6B756F859B69844B644C4BDBC3178C759AF900F40D913F34FFCE536` | `SM_W01_HouseA` |
| `Buildings/FBX/House_3.fbx` | `42A853231116C948F42ED0106B6599F9BA212E23BA6BD5A989E1A7161DCEFAA1` | `SM_W01_HouseB` |
| `FBX/LargeSquareTowerBricks.fbx` | `43229A1553F5954376AD03297B78A01A5765FBFD1A4935D138D06CD111289E0F` | `SM_W01_Tower` |
| `FBX/TallWallBricks.fbx` | `F8D42EDB44BF9E38260F02608F0B3896F93B0F7285051BC72B76B4AD5E341CFA` | `SM_W01_Wall` |
| `FBX/WallEntranceBricks.fbx` | `5870A562CF2F988CCBEA1496540C9B4B1E6743D06E21F26B94C03CAC8367FFEB` | `SM_W01_Arch` |
| `FBX/Tree2.fbx` | `5EA88C268C80B9F2AFFFECB84CC08CA836BA5C110A7EF0A217A21598C7739C94` | `SM_W01_Tree` |
| `Props/FBX/Barrel.fbx` | `4D7E1C91BA831F0E8E47E8805CEF951B1D3620A1D32E80BD8B200098941F77B2` | `SM_W01_Barrel` |
| `Props/FBX/Crate.fbx` | `1AD18751ED1898CB1FE668F5DFAAAFC100FB2E55DD85D14480E213A262B1FEA3` | `SM_W01_Crate` |
| `Props/FBX/Bonfire_Lit.fbx` | `581AAF1BA3EB7CF647FA9C2745BE990F73FCD87399194AFDD64F15DB64EFCEBE` | `SM_W01_Campfire` |
| `Props/FBX/Cart.fbx` | `4486C2C2EBB6C4A7EFC4566FF3A87CFF2749848C4C7633CB0CFAD1B52B5208D8` | `SM_W01_Cart` |
| `Props/FBX/Fence.fbx` | `693D8E47DF04A81BF3688ADEA9572884AAB01126B7024CDF48251E85C7CB39B3` | `SM_W01_Fence` |
| `Props/FBX/Gazebo.fbx` | `A459DFAC73559B26BDE0F4A35ACCB7932ECCF512EA197D17F838EDB7E1FC6111` | `SM_W01_Gazebo` |
| `Props/FBX/MarketStand_1.fbx` | `BB8BF43800EA3F26188DD1B47FD04B2BE3411019D035855D9906E645D4B73B7A` | `SM_W01_MarketStand` |
| `Props/FBX/Path_Straight.fbx` | `72378811BC35900C0F5B8ADB108E180B7E8F19257084FF105D31358D67173A72` | `SM_W01_Path` |
| `Props/FBX/Well.fbx` | `4A634278D8DB32DC46C180DE54B29132C0D9AC17D6AB12BF55CCF56120966595` | `SM_W01_Well` |
| `FBX/Bush2.fbx` | `072181D5B16468A00827C5F4D9EFB74197E1C58EF822FCC61E9F3EF025F7773D` | `SM_W01_Bush` |
| `FBX/Grass2.fbx` | `E9B0D9D58276F8D0FF64ED54F2370055DFFF699FBA404FE77F75BD0537C672CE` | `SM_W01_Grass` |
| `FBX/Rock2.fbx` | `8A4C889BB1043078B39C93175A20CBE15AC3A4CEFD1FD088FA73204C0533DE8F` | `SM_W01_Rock` |

`Scripts/import_w01_world_assets.py` 在下载归档、解压和导入前均执行 SHA-256 检查，并以固定名称覆盖式导入。所选包没有独立路灯模型；W01 的路灯应继续使用项目生成的几何体与 PointLight，或使用 `SM_W01_Campfire` 作为火光资源，不将其他物件误标为路灯。
