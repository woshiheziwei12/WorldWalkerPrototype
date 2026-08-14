# 现成游戏资源方案

本项目不要求自行制作美术资源。第一原则是先选定一种统一风格，再从少数稳定来源补齐角色、环境、动画、UI 和音频。

## 推荐来源

| 来源 | 适合内容 | 授权注意事项 |
| --- | --- | --- |
| [Fab](https://www.fab.com/) | UE 环境、角色、动画、特效、UI、音频 | 每个条目单独确认许可证和 UE 版本。Fab Standard License 通常允许私用、商用和修改，但禁止把资产原样单独转售或免费再分发。标记为 UE-Only 的内容只能用于 Unreal Engine 产品。 |
| [Epic 免费 UE 内容](https://dev.epicgames.com/documentation/en-us/unreal-engine/free-epic-games-content-for-unreal-engine) | Paragon、Infinity Blade、示例项目、角色和环境 | 适合本项目，但部分内容是 UE-Only。不要用原游戏商标给自己的游戏命名或宣传。 |
| [Mixamo](https://www.mixamo.com/) | 人形角色、自动绑定、行走/攻击/受击动画 | Adobe 允许角色和动画用于个人、商业和非营利游戏。只适合双足人形；中国区 Adobe ID 可能无法使用该服务。 |
| [Poly Haven](https://polyhaven.com/) | HDRI、PBR 材质、环境道具 | 全站资产 CC0，可修改、可商用、无需署名。 |
| [Kenney](https://kenney.nl/assets) | 低多边形模型、UI、图标、音效 | 游戏资产页内容为 CC0，可修改、可商用、无需署名。 |
| [Game-icons.net](https://game-icons.net/) | 卡牌图标、状态图标、技能图标 | CC BY 3.0，必须在游戏 Credits 中标注对应作者和网站。 |
| [Sonniss GameAudioGDC](https://sonniss.com/gameaudiogdc/) | 大量专业环境、打击、UI 和魔法音效 | 面向游戏等媒体制作的永久免版税授权；不要重新分发原始音频文件，也不要用于 AI/ML 训练。 |
| [Freesound](https://freesound.org/) | 缺少的单个音效和环境声 | 每个文件许可证不同。优先 CC0，其次 CC BY 并记录署名；谨慎使用 BY-NC。 |
| [Pixabay](https://pixabay.com/music/) | 临时背景音乐、环境音和音效 | 可免费使用和修改，通常无需署名；不可把内容以原始独立形式重新分发，仍需检查商标和第三方权利。 |
| [Incompetech](https://incompetech.com/music/) | 可循环背景音乐和气氛音乐 | 免费 Creative Commons 方式要求署名；不想署名时需购买标准许可证。 |
| [OpenGameArt](https://opengameart.org/) | 2D UI、图标、音乐和少量 3D 内容 | 条目许可证差异很大。优先 CC0 或清楚的署名许可，避免不了解的 GPL/ShareAlike 组合。 |

## 对 World Walker 的实际组合

第一世界建议选择一种完整的低多边形或风格化 Fab 环境包作为视觉基底，然后这样补齐：

1. 角色：Fab 免费角色、Epic Paragon 角色，或一个统一风格的付费角色包。
2. 动画：Epic Animation Starter Pack；能使用 Mixamo 时，再补交互、受击和攻击动画。
3. 环境：主风格包负责建筑和植被，Poly Haven 只补材质、HDRI 和不显眼的通用道具。
4. 卡牌/UI：自制简单边框布局，图标来自 Game-icons.net 或 Kenney。
5. 特效：优先 Fab 的 Niagara 免费包，避免在核心战斗确认前购买大量特效。
6. 音效：Sonniss 作为基础库，Freesound 只补少数缺口。
7. 音乐：原型阶段使用 Pixabay 或 Incompetech，确定整体气质后再选择最终曲目。

## 使用规则

- 即便项目暂时非商业，也按“未来可能公开发布”的标准保存授权记录。
- 不使用从其他商业游戏中提取的模型、动画、贴图、音乐或音效。
- 不把“免费下载”误认为“无版权限制”；必须看具体许可证。
- 不把购买或下载的原始资源文件放到公共 Git 仓库。
- 每引入一个资产，在 `ThirdPartyAssets.csv` 增加一行，并保存商品页或许可证截图。
- 尽量让一个世界由一个主资产包定义风格，避免产生明显的“素材商店拼接感”。
