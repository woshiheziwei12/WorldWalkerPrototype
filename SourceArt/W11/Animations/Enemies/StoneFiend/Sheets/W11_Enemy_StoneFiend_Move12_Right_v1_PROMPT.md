# 石甲妖 · 12 帧右向移动样板

生成方式：Codex 内置 `imagegen`，参考图为 `SourceArt/W11/Generated/W11_StoneFiend_v1.png`。

最终提示词：

```text
Using the referenced Stone Fiend as the strict creature-design and painterly rendering reference, create one production-ready 2D top-down action-roguelite movement sprite sheet. Exactly 4 columns × 3 rows, twelve sequential full-body heavy running frames read left-to-right then top-to-bottom. Fixed high three-quarter top-down camera, the Stone Fiend travels and faces screen-right in every frame, identical scale, cell size and ground contact. Make a seamless weighty locomotion loop with clear contact, compression, passing, push-off, airborne and landing phases: huge stone arms counter-swing, torso and shoulder plates rotate subtly, knees compress under mass, orange magma cracks pulse consistently, and a few small orbiting rocks lag and settle continuously. Preserve the exact dark basalt body, bead-like stone collar, angular head, orange fissures and brown waist cloth from the reference. Premium original Chinese xianxia game enemy sprite, painterly cel-shaded, crisp rock facets and clean readable silhouette suitable for a 4K game. Transparent RGBA background with generous padding. No scenery, floor, shadow, text, labels, grid lines, weapons, magic attack effects, duplicate poses, cropped body, merged frames, extra limbs, design drift, blur or watermark.
```

生成源图：`W11_Enemy_StoneFiend_Move12_Right_v1_source.png`。后处理脚本按 512×512 单帧、底部中心 Y=484 统一锚点，生成左右镜像帧、图集与 12 FPS GIF。
