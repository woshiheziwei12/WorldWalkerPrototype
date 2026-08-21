# W02 Kenney Castle Kit 授权证明

## 资源信息

- 资源名：Castle Kit 2.0
- 作者与发行方：Kenney
- 作者官网：https://www.kenney.nl/assets/castle-kit
- 官网固定下载地址：https://www.kenney.nl/media/pages/assets/castle-kit/a395102d20-1711543616/kenney_castle-kit.zip
- 获取日期：2026-08-15
- 许可证：Creative Commons Zero 1.0 Universal（CC0 1.0）
- CC0 正文：https://creativecommons.org/publicdomain/zero/1.0/
- 下载归档：`kenney_castle-kit.zip`
- 下载归档大小：`2232589` bytes
- 下载归档 SHA-256：`921F3F73927BB23106CAE34BC21D5AB4B033A9FC120475E96F714A406E3169DF`
- 包内证明文件：`License.txt`
- 包内证明文件 SHA-256：`AAC944F18106B3A3E29C6FDEEC02523D4CAB4C735ABC01F5A8FA88A79AE173EF`

作者官网将该资源标为 3D 中世纪城堡资源，共 75 个文件，许可证明确标记为 Creative Commons CC0。固定下载归档内的 `License.txt` 进一步确认资源由 Kenney 创建和分发，可用于个人、教育和商业项目，署名 Kenney 仅为自愿支持而非许可证要求。

## 本项目筛选并导入的文件

项目不重新发布原始 ZIP。`Scripts/import_w02_kenney_castle_assets.py` 下载并核验作者官网归档，只把螺旋高塔、平台路线和城堡入口所需的 26 个 FBX 导入到：

`/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/Kenney/CastleKit/Environment/`

每个 StaticMesh 使用独立的稳定同名目录，连同 FBX 引用的 `colormap.png` 所生成的材质与纹理一起保存，避免通用材质名在不同模块间互相覆盖。导入脚本启用场景单位转换、Lightmap UV 生成和自动碰撞；重复执行时使用固定目标名覆盖更新。

| 原始 FBX | SHA-256 | 稳定目标名 |
| --- | --- | --- |
| `tower-base.fbx` | `4C50E4AF142966D2B81597B6A65AFBC0EF7B02ED05721CE95F7983501DBE7CF6` | `SM_W02_CastleRoundTowerBase` |
| `tower-top.fbx` | `DCAE2473F7ED0741FC27D0BF5F25308AF92BA0269BB6ACCDC5569839C7363C70` | `SM_W02_CastleRoundTowerTop` |
| `tower-hexagon-base.fbx` | `ECA5CD4895390C68BE6C4BCF428234E760EBA527BAB030F88B442E328B721475` | `SM_W02_CastleHexTowerBase` |
| `tower-hexagon-mid.fbx` | `CB76D2720B8E4B49E14EAE0D2E8AB3BB36E60DA053EEF1B6EEAD56A3858A162E` | `SM_W02_CastleHexTowerMid` |
| `tower-hexagon-top.fbx` | `F68EE7720A7786CA754F622FA452BAAE51CD561ACA6093B59C637090B8C267B5` | `SM_W02_CastleHexTowerTop` |
| `tower-square-base.fbx` | `032EED379D7399D66527A7B00B27DDAC5420FBF9239871B0AE96C064637203AC` | `SM_W02_CastleSquareTowerBase` |
| `tower-square-mid.fbx` | `2E05E5AC1A041897B1F23E94634E7E66EC98895FA830A8FBC6D7E0ED23349C03` | `SM_W02_CastleSquareTowerMid` |
| `tower-square-mid-door.fbx` | `B07E21C6C08A2D2BB0BD82EF354A6092BCA3307353B2AEF83ED82686215850AB` | `SM_W02_CastleSquareTowerMidDoor` |
| `tower-square-mid-open.fbx` | `97FCAD51A3E37D35E6F9A6AE06C74EA7D0678DE9EDEEB616EC7386EFCD3D6824` | `SM_W02_CastleSquareTowerMidOpen` |
| `tower-square-mid-windows.fbx` | `EEEA9D3280F4C0B0AEFABD88B1C674AC3F7260C52B4A69EB9267A69545E2FFCA` | `SM_W02_CastleSquareTowerMidWindows` |
| `tower-square-top.fbx` | `1386EC47ADF887D6B258A1EF348920B020484AEA767FF8F84AC434685A330951` | `SM_W02_CastleSquareTowerTop` |
| `tower-square-top-roof-high.fbx` | `60FE3458E9606B6ACAF5321A75E35CCC08A10A019203C9D0E3F37A792B57B0A7` | `SM_W02_CastleSquareTowerHighRoof` |
| `tower-square-arch.fbx` | `EEA3751274647B4E478C61ADB0B68A22B2ABDDEBE6B6A42C4743A4498B2DD286` | `SM_W02_CastleSquareTowerArch` |
| `stairs-stone.fbx` | `9BF3222FD4AF6D8D157A75978F7DD5360007AF0038AE5DE3DCE1628150463D13` | `SM_W02_CastleStoneStairs` |
| `stairs-stone-square.fbx` | `065F0C2995D5165DD6ED554AE97F92C0D86D5E68D0A7192F24B75985216AEA50` | `SM_W02_CastleSquareStoneStairs` |
| `bridge-straight.fbx` | `76178A96497B0AF963EBE2DF49467CAE10E79658A85886284BDB1B74821D4FE8` | `SM_W02_CastleStraightBridge` |
| `bridge-straight-pillar.fbx` | `2E229402614EC308BD9D00322B23704935CC4B625F633D294B6FDB32D6D6092A` | `SM_W02_CastleStraightBridgePillar` |
| `wall.fbx` | `69CD3C951EBB02249FA1A4A6FB022BF271DC6B0BE36EAAEF1E90B0A4D35CC0C5` | `SM_W02_CastleWall` |
| `wall-corner.fbx` | `AC689BF5538268FBB83D22FE6F7C2DD94831F1B2466A716FCE4BA5F42071B842` | `SM_W02_CastleWallCorner` |
| `wall-doorway.fbx` | `F9AE53F61A992AA8AC7F761FEE85C28FCF78D67D602409DF0331A358DF92A33B` | `SM_W02_CastleWallDoorway` |
| `wall-narrow.fbx` | `1659641BFD2A8E1D054F7552598B8C1B5E35DD9E8A6A5D2E9F3388EFA6F9DCF2` | `SM_W02_CastleWallNarrow` |
| `wall-narrow-stairs.fbx` | `8B39BAD7DF3746408EC6799D410FBC1DC01DFEE0D94207C8224013957053DE2C` | `SM_W02_CastleWallStairs` |
| `wall-narrow-stairs-rail.fbx` | `6049488128B483B0ED14799F10BB8F8F1BB719059582A5C9EACC60C9F7B85F83` | `SM_W02_CastleWallStairRail` |
| `wall-pillar.fbx` | `306FEF3A327296097CA7B84DC4445746F586B87AC186BD1876FF0566F129DC24` | `SM_W02_CastleWallPillar` |
| `gate.fbx` | `2902B66F40775E9AC2FB272D22B418307CEC3ADBF17C41AA49073958E6B217A6` | `SM_W02_CastleGate` |
| `metal-gate.fbx` | `3233E7EDB7A1B1E4C10B14048BF28879A8963AABB1935AA9B6B17C04FB9811E9` | `SM_W02_CastleMetalGate` |

共享源纹理 `Models/FBX format/Textures/colormap.png` 的 SHA-256 为 `66FD49BE148F32E88F6C8CACE67120250D1943A7856601158AD0FF24651DB0B0`。导入成功标记为 `W02_KENNEY_CASTLE_IMPORT_COMPLETE`，并必须报告 `meshes=26/26`。
