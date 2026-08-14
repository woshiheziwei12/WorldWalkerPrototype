# Quaternius RPG Character Pack 授权证明

## 资源信息

- 资源名：RPG Character Pack
- 作者：Quaternius
- 官方页面：https://quaternius.com/packs/rpgcharacters.html
- 获取日期：2026-08-13
- 许可证：CC0 1.0 Universal / Public Domain Dedication
- CC0 正文：https://creativecommons.org/publicdomain/zero/1.0/
- 下载归档：`rpg_characters.zip`
- 下载归档 SHA-256：`5399E0CFAF313FF362455DE4086488D093465434B1B93B0CED649F3773A15FD7`
- 包内证明文件：`RPG Characters - Nov 2020/License.txt`
- 包内证明文件 SHA-256：`83D8959F9FC56353ED571FBE2DC52E4BCD64508E2399501CD45AC2CE3DF0BF8C`

官方页面将资源标为 CC0，并说明其中包含 6 个已绑定、带动画和纹理的西幻角色，可用于个人和商业项目。

## 包内许可证原文

```text
LowPoly Models by @Quaternius

License:
CC0 1.0 Universal (CC0 1.0)
Public Domain Dedication
https://creativecommons.org/publicdomain/zero/1.0/
```

## 本项目使用的文件

| 原始文件 | SHA-256 | 用途或 UE 目标资产 |
| --- | --- | --- |
| `FBX/Warrior.fbx` | `5840E821ABA0F4CFBC7316DB456DD28042D35FCD1173F8809E22D25D040B265F` | `/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_Warrior.SK_W01_Warrior` |
| `Textures/Warrior_Texture.png` | `5D016C19DB78E8B6077EEAD3E1086A3BB3453CFAA8ADB5E583B08DC39869FF17` | Warrior 主体纹理 |
| `Textures/Warrior_Sword_Texture.png` | `AE902ECABA0EB6D15D47EF428E81D63F9CA2D8A266AAD7DAE393A8363EA9344A` | Warrior 武器纹理 |
| `FBX/Cleric.fbx` | `DDBD70823DA811625A512EF4CEACCE2FAA9DE8C71AA2C6FEE7E23E0FB52B6DAA` | `/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Cleric/SK_W01_Cleric.SK_W01_Cleric`；同时导入 Idle、Walk、Run、Idle_Weapon、Staff_Attack 等内嵌动画 |
| `Textures/Cleric_Texture.png` | `F17F9208CB86FD98DB429BF8A1A0EDC97C713A8357A44A392E562047BC0FE913` | Cleric 主体纹理 |
| `Textures/Cleric_Staff_Texture.png` | `37FD6917E617738676D5904D15816C11A3A7E8C0CBEF4B3B8F1B95C968D6D785` | Cleric 法杖纹理 |
| `FBX/Wizard.fbx` | `DDD72512FF771380DF9223CEF02731EBF68E31309BE759BF79D7063936F4636D` | `/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_Wizard.SK_W01_Wizard`；同时导入 Idle、Walk、Run、Idle_Weapon、Staff_Attack 等内嵌动画 |
| `Textures/Wizard_Texture.png` | `1D4B9FCF14BE09CA3CE5983950AC0C9F4C8704E8349C3B8D49F63DD7C5EEA88E` | Wizard 主体纹理 |
| `Textures/Wizard_Staff_Texture.png` | `D56876367B9921139BF96A337C9D3433056044B42DF55912272CB0B433D1AE28` | Wizard 法杖纹理 |
| `FBX/Ranger.fbx` | `F1547EBF853BD2F7849642D7A5D98B7DFA0619BBA7A2AA37D43DCAEDC49EB6D2` | `/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Ranger/SK_W01_Ranger.SK_W01_Ranger`；同时导入 Idle、Walk、Run、Idle_Weapon、Bow_Attack_Draw/Shoot 等内嵌动画 |
| `Textures/Ranger_Texture.png` | `21679F0FAF9C5243A58FDC952A5427E2BBB0EA29A00B117308E6FB5FD6D94588` | Ranger 主体纹理 |
| `Textures/Ranger_Bow_Texture.png` | `36F9FF48610B97C0F599C133D95DE62C6678B29C4EC1A54E03C1AEE838B9BA3D` | Ranger 弓纹理 |

`Scripts/import_w01_fantasy_assets.py` 导入 Warrior，`Scripts/import_w01_world_assets.py` 导入 Cleric、Wizard 和 Ranger。脚本保留原始纹理内容，以固定名称创建或替换 Unreal 资产；不重新发布原始下载归档。

## UE 5.8 材质绑定说明

此资源包的 FBX 只使用 `Warrior_Texture`、`Cleric_Texture` 等材质槽名称，没有在 FBX 内嵌或引用同包单独提供的 PNG。UE 5.8 Interchange 会据此创建以 `FBXLegacyLambertSurfaceMaterial` 为父级的 `MaterialInstanceConstant`，但只写入白色 `DiffuseColor`，不会自动引用已经导入到 `Textures/` 的真实 `Texture2D`。这会让网格、骨骼和动画都正常，但角色在场景中呈现近乎纯白。

`Scripts/import_w01_world_assets.py` 在完成 FBX 导入后执行确定性的材质修复：

1. 再次核验并覆盖式导入 8 张角色/武器 PNG。
2. 在每个角色自己的 `Materials/` 目录创建或重置两个原生 `Material`，用 `MaterialExpressionTextureSample` 把对应 PNG 的 RGB 明确连接到 Base Color。
3. 按 FBX 原始材质槽名绑定；仅在槽名不可用时使用已核实的 0/1 槽位回退。
4. 重新编译材质、保存 SkeletalMesh，并回读验证 8 个材质槽均指向预期材质。

稳定材质路径为：

- `.../Warrior/Materials/M_W01_Warrior_Body` 与 `M_W01_Warrior_Weapon`
- `.../Cleric/Materials/M_W01_Cleric_Body` 与 `M_W01_Cleric_Weapon`
- `.../Wizard/Materials/M_W01_Wizard_Body` 与 `M_W01_Wizard_Weapon`
- `.../Ranger/Materials/M_W01_Ranger_Body` 与 `M_W01_Ranger_Weapon`

修复后的成功标记必须包含 `materials=8/8`。旧的 FBX 自动生成白色材质实例可以保留作为导入元数据，但不再绑定到角色网格。
