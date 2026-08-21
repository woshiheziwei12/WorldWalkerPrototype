# W01 Qwantani Moon Noon HDRI 授权记录

记录日期：2026-08-15

- 名称：Qwantani Moon Noon (Pure Sky)
- 作者：Greg Zaal（摄影）、Jarod Guest（处理）
- 来源页：https://polyhaven.com/a/qwantani_moon_noon_puresky
- 2K HDR：https://dl.polyhaven.org/file/ph-assets/HDRIs/hdr/2k/qwantani_moon_noon_puresky_2k.hdr
- 许可证：CC0 1.0 Universal
- 许可证正文：https://polyhaven.com/license
- SHA-256：`05E53B6A04836127BD7A91A69F09E66D0185F24D1FCBB59D8BB5AC5F7BE6E683`
- 项目内目录：`Content/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/PolyHaven/Sky/`
- 用途：W01“灰烬王国”的可见月夜天空。W01 使用独立的 TextureCube 与天空材质，不引用 W00 内容资产。

`Scripts/import_w01_environment_assets.py` 会先校验源文件哈希，再生成稳定的 `T_W01_QwantaniMoonNoon` 和 `M_W01_QwantaniMoonNoonSky`。源 HDR 只保存在被 Git 忽略的 `Saved/` 缓存中；当前可复用已经校验的 W00 源缓存，但两个世界的 Unreal 资产保持独立。
