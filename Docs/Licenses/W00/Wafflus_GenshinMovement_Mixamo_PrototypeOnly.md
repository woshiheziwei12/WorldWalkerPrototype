# Wafflus Genshin Movement / Mixamo 动作本地原型记录

- 上游仓库：https://github.com/Wafflus/unity-genshin-impact-movement-system
- 固定提交：`3bb190862b96cee17ec0127b857ce9fa37d6de14`
- 本机仓库：`E:/workspace/unity-genshin-impact-movement-system`
- 上游代码许可证：MIT，Copyright (c) 2022 Gustavo Vieira
- 动作来源声明：上游 README 明确说明随附动作下载自 Adobe Mixamo，且这些动作不包含在上游 MIT 许可证中
- 登记日期：2026-08-23
- 当前用途：W00/W02 荧角色本地移动原型

## 权利与分发边界

本项目只把上游代码的 MIT 许可视为适用于该仓库作者自行编写的代码；不把 MIT 标记扩展解释为随附 Mixamo 动作的再分发许可。当前导出的 FBX 与导入 UE 的动作 `.uasset` 仅用于本机原型，由 `.gitignore` 排除，不进入 Git、团队资源包或对外发布构建。

若要发布，必须根据届时适用的 Adobe Mixamo 条款重新核对可分发范围，并同时解决荧角色模型自身未核验的权利状态；否则应替换为权利清晰的原创或授权模型与动作。

## 可复现转换

1. Unity 6.5 `6000.5.9f1 (b57deb96f08d)` 安装在 `E:/app/Unity/6000.5.9f1`。
2. `Scripts/Unity/GenshinAnimationFbxExporter.cs` 配合官方 `com.unity.formats.fbx@5.1.5`，从上游项目批量导出 `WWMixamo_Skeleton.fbx` 和 13 个 `WWMixamo@*.fbx` 到 `ExportedFBX/`。
3. `Scripts/import_wafflus_genshin_movement.py` 把骨架和 13 个动作幂等导入 `/Game/WorldWalker/Shared/ThirdParty/Wafflus/GenshinMovement`。
4. 运行时使用隐藏 Mixamo 源骨架评估动作，将 52 根核心骨骼的旋转映射到荧；UE Interchange 可能把 `mixamorig:Hips` 规范化为 `mixamorig_Hips` 或 `Hips`，运行时代码兼容三种写法。

导出成功标记为 `WW_UNITY_ANIMATION_EXPORT_COMPLETE count=13`；运行时成功标记为 `WW_WAFFLUS_MOTION_SOURCE_READY clips=13`、`Profile=Mixamo Links=52` 与 `Locomotion=WafflusGenshin13`。
