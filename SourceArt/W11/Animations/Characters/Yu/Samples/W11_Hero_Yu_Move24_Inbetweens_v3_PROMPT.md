# 玉 24 帧移动样板：12 张中间帧生成记录

生成模式：内置 `imagegen`，以 `W11_Hero_Yu_Move12_Right_v2_source.png` 为本地参考图进行编辑。

最终提示词：

> Edit the provided 4 columns by 3 rows, 12-cell sprite-sheet reference into a new 4x3 sprite sheet containing ONLY the twelve in-between poses for the same heroine Yu. Read the input cells in row-major order 1 through 12. Output cell 1 must be the exact halfway motion pose between input cells 1 and 2; output cell 2 halfway between input 2 and 3; continue this rule; output cell 12 halfway between input cell 12 and input cell 1 for a seamless loop. Preserve the exact same Chinese xianxia heroine identity, face, white-and-pale-jade layered hanfu, lavender cords, black hair, ornament, painterly high-detail game sprite style, rightward running direction, character scale, camera, and consistent bottom-foot anchor. Every new pose must be genuinely intermediate in body lean, arm swing, leg stride, sleeves, ribbons, and hair flow—not a duplicate. Use an exact clean 4x3 layout with one centered full-body figure per equal cell, generous transparent padding, no part crossing cell boundaries. Transparent RGBA background. No black backing, no checkerboard, no grid lines, no borders, no labels, no text, no UI, no shadows, no cropped body parts, no extra limbs, no weapons, no style change. Production sprite-sheet asset, 2048x1536 if supported.

产物 `W11_Hero_Yu_Move24_Inbetweens_v3_source.png` 只包含 12 张中间帧。处理脚本按 `原关键帧 n → 中间帧 n` 交错为 24 帧，保持 1 秒循环时长。
