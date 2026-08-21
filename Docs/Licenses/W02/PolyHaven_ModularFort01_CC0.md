# Poly Haven Modular Fort 01 / Wood Planks — CC0 授权记录

## 资源身份

- 资源名称：Modular Fort 01
- 作者：Rico Cilliers
- 发布方：Poly Haven
- 官方资源页：https://polyhaven.com/a/modular_fort_01
- 官方文件 API：https://api.polyhaven.com/files/modular_fort_01
- 官方许可页：https://polyhaven.com/license
- 许可证：Creative Commons CC0 1.0 Universal（公共领域贡献）
- 获取日期：2026-08-15

官方资源页把该资源标记为 CC0，并将其描述为包含风化石墙、城垛、塔楼、步道和拱门的模块化堡垒模型；官方许可页说明 Poly Haven 的 HDRI、纹理和 3D 模型均采用 CC0，可用于商业用途、修改和再分发且无需署名。

同时采用的木材资源：

- 资源名称：Wood Planks
- 作者：Amal Kumar
- 官方资源页：https://polyhaven.com/a/wood_planks
- 官方文件 API：https://api.polyhaven.com/files/wood_planks
- 许可证：Creative Commons CC0 1.0 Universal（公共领域贡献）
- 获取日期：2026-08-15

## 本项目采用范围

项目只下载并导入 Modular Fort 01 官方 1K FBX、Wall/Trim/Plaster 三组 1K JPG，以及 Wood Planks 的 1K JPG；每组只采用 Diffuse、DirectX Normal 和 Roughness 三图。没有采集预览图、网页文字、AO、位移、ARM、OpenGL Normal 或其他分辨率文件。

Unreal 私有目标目录：

`/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/PolyHaven/ModularFort01`

`Scripts/import_w02_polyhaven_fort_assets.py` 会：

1. 从 Poly Haven 官方下载域名获取固定文件；
2. 在导入前校验下列 SHA-256；
3. 以 `combine_meshes=false` 导入独立墙段、拐角、门、塔、楼梯和步道网格；
4. 创建 Wall、Trim、Plaster 三个堡垒 Unreal PBR 材质并自动替换 FBX 材质槽；
5. 创建独立 `M_W02_WoodPlanks` PBR 材质，供 W02 木板跳台与房梁使用；
6. 请求 FBX 导入器生成简单碰撞。W02 的实际通关碰撞仍以 C++ 基础碰撞体为准，第三方网格碰撞只用于可见模块的附加阻挡。

## 固定文件与 SHA-256

| 官方文件 | SHA-256 |
| --- | --- |
| `modular_fort_01_1k.fbx` | `267369DC754AFD545A91F302EF1819893BF8A907100010C097B58A2C576EB281` |
| `modular_fort_01_plaster_diff_1k.jpg` | `D453B45CEC06DA3DA9E67B38D016F6358C7C310A99DB9B066E07E3A494731F4D` |
| `modular_fort_01_plaster_nor_dx_1k.jpg` | `7A04D3035CC35244B8A9E2D357C8AA2EC3E82FB45644C6D9AE6B0A70D5D916B3` |
| `modular_fort_01_plaster_rough_1k.jpg` | `815A90BD3084A1360721219A4DD4A838DB3C88388E399BD111F5D269DAD97601` |
| `modular_fort_01_trim_diff_1k.jpg` | `FE5811560CF0471321FE6AF08114F279A7F957412B8AA6E2C224112990760556` |
| `modular_fort_01_trim_nor_dx_1k.jpg` | `831D0FCFAA8B92280A1BC61818EB979FC1EF70A34BC50D7863D038396DF0DBAF` |
| `modular_fort_01_trim_rough_1k.jpg` | `493BAE77B6125278A9560B9DCB9E0E73C50B77AC8AA37ABFBBF8E399EF59FC97` |
| `modular_fort_01_wall_diff_1k.jpg` | `8BDEFADCA0688E9B3FD91B4D12F341918B0A3670C25537C18A777FA51BB90BEA` |
| `modular_fort_01_wall_nor_dx_1k.jpg` | `B282548A4ECD3265BFE770CC7A9209D1D6F02CED55AFD21B0A44F4E0037844FC` |
| `modular_fort_01_wall_rough_1k.jpg` | `68D3291814689B2AC1D104E4998693FB3B433E04687CB2FE6D31D3F850B97255` |
| `wood_planks_diff_1k.jpg` | `3B0669F683E4BF10F5A55A381CFA9669A7B8DFD921901829DAA3B35ACC2BBDEC` |
| `wood_planks_nor_dx_1k.jpg` | `D54A8AD99A94B9A7A7185CEBCE2411EB74AE9031D34C5D208DD53DC42CD53917` |
| `wood_planks_rough_1k.jpg` | `1B7F115BFA25619B0A2DB554EB1AB88A6FC5EF0B74BA4890611EAE04D00B9829` |

下载 URL 由脚本固定为：

- FBX：`https://dl.polyhaven.org/file/ph-assets/Models/fbx/1k/modular_fort_01/<filename>`
- 堡垒 JPG：`https://dl.polyhaven.org/file/ph-assets/Models/jpg/1k/modular_fort_01/<filename>`
- 木板 JPG：`https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/wood_planks/<filename>`

## 修改说明

- 选择 1K 版本以控制原型工程的本地 Content 体积和显存占用。
- 使用 Unreal 重新构建三组材质，DirectX Normal 贴图不翻转绿色通道。
- 使用相同规则单独构建写实木板材质，供程序化木梁和窄木板平台复用。
- FBX 保持模块分离，材质槽被替换为 W02 私有材质；导入器可生成简单碰撞。
- 资源可按场景需要缩放、旋转、重复实例化和与程序化基础碰撞组合。
- UE 5.8 对 22 个 FBX 模块分别报告源文件未导出 smoothing group 的上游警告；幂等实导仍以 0 个脚本错误完成，22 个网格、12 张纹理和 4 个材质均成功保存。该警告不由导入脚本静默修改，便于后续资产审计。

本记录用于资源来源、许可和文件完整性审计，不替代 CC0 法律文本。
