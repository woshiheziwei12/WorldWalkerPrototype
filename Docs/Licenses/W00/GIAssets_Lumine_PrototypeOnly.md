# GI-Assets / Lumine 本地原型记录

- 资源：Lumine（荧）角色 FBX 与配套纹理
- 本机来源：`E:/workspace/content/Models/Characters/Lumine`
- 上游索引：https://github.com/zeroruka/GI-Assets
- FBX：`Cs_Avatar_Girl_Sword_PlayerGirl #1.fbx`
- FBX SHA-256：`423A6846891074A4D7FC8E911CF187430D5A5A0B9A553BCE91AE06C5CA0E9381`
- 登记日期：2026-08-22
- 当前用途：W00/W02 本地换模原型

## 权利与分发边界

上游仓库没有在本项目中被认定为该角色资产的再分发授权方；本记录不是许可证，也不声称获得原游戏发行、改编或再分发授权。当前权利状态标记为未核验。

因此，源 FBX、纹理以及导入到 `/Game/WorldWalker/Shared/ThirdParty/GIAssets/Lumine` 的 `.uasset` 只用于本机原型验证，由 `.gitignore` 排除，不进入 Git、团队资源包或对外发布构建。若要分发或发布，必须先替换为权利清晰的原创/授权资产，或补齐适用授权并重新审查。

## 可复现导入

`Scripts/import_lumine_character.py` 默认读取 `E:/workspace/content`，也接受环境变量 `WORLDWALKER_GI_ASSETS_ROOT`。脚本会先核验 FBX 与 10 张纹理的固定 SHA-256，再导入网格、骨架、10 张纹理和五个基础材质；`Avatar_Default_Mat` 对应的特效辅助几何使用全透明 Masked 材质隐藏。Body/Face Diffuse 的 Alpha 是原 Shader 的功能遮罩而非表面透明度，因此 Body、Dress、Face、Hair 四个可见槽全部使用 Opaque，Dress 另为双面；不能把这些 Alpha 接到 Opacity Mask，否则会错误裁掉胸腹、骨盆或下半张脸。

当前 FBX 会由 UE 5.8 报告无效 bind pose 并使用零时刻重绑，同时 Face_Eye、Face、Brow、Body、EffectMesh、EyeStar 缺少 smoothing group。导入成功标记为 `WW_LUMINE_IMPORT_COMPLETE mesh=1 textures=10/10 materials=5/5`。
