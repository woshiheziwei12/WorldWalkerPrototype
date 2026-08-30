# 玉 · 12 帧右向移动样板

生成方式：Codex 内置 `imagegen`，参考图为 `SourceArt/W11/Animations/Characters/Yu/Sheets/W11_Hero_Yu_CombatSheet_v1_source.png`。

最终提示词：

```text
Using the referenced Yu combat sprite sheet as the strict identity, costume and painterly rendering reference, create one production-ready 2D top-down action-roguelite movement sprite sheet for Yu. Exactly 4 columns × 3 rows, twelve sequential full-body running frames read left-to-right then top-to-bottom. Fixed high three-quarter top-down camera, Yu faces screen-right in every frame, identical character scale, cell size and foot contact point. Build one smooth loop with clear contact, recoil, passing, airborne and landing phases; alternate legs and arms naturally, and let the long black hair, translucent white-green sleeves, layered robe and purple ribbons follow through continuously without changing design. Preserve her face, jade hair ornament, pale green-white costume, dark belt and boots from the reference. Premium original Chinese xianxia game sprite, painterly cel-shaded, crisp clean outline and fine fabric detail suitable for a 4K game. Transparent RGBA background, generous padding in every cell. No scenery, floor, shadow, text, labels, grid lines, weapons, magic effects, duplicate poses, cropped body, merged frames, extra limbs, costume drift, blur or watermark.
```

生成源图：`W11_Hero_Yu_Move12_Right_v2_source.png`。后处理脚本移除生成图中的烘焙棋盘格和跨单元格碎片，按 512×512 单帧、底部中心 Y=484 统一锚点，生成左右镜像帧、图集与 12 FPS GIF。
