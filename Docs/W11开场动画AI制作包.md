# W11《太虚问道》开场动画 AI 制作包

版本：W11-Cinematic-v1  
日期：2026-08-27  
定位：可直接交给文生视频、图生视频、配音、音乐与剪辑工具的生产资料

> 生产状态（2026-08-28）：本文件继续保留 96 秒完整导演稿。此前 Wan3 的 70 秒成本优先版已转为历史方案；当前实际生产平台为即梦，14 个叙事片段按信息量分配为 5、6 或 7 秒，总长 88 秒，内部拆为 38 个真实摄影镜头。现有 JPG 用作人物、场景和画风参考，不再作为固定首帧；详见 `Docs/W11开场动画即梦输入方案.md`。

## 1. 成片目标

- 建议片长：96 秒。
- 画幅：16:9，建议母版 3840×2160，最低 1920×1080。
- 帧率：24 fps；每个 AI 生成片段控制在 4～8 秒，再由剪辑衔接。
- 播放位置：点击“独自启程”后、选择角色前。
- 主角策略：不展示任何固定玩家正脸，十名可选主角均可共用。
- 首次播放：允许长按跳过；看完后在“万法图鉴 → 太虚旧史”中重播。
- 叙事目的：提出“拥有前世知识是否足以过好一生”的问题，讲清魔族旧难、顾长渊救世与太平异化，但不提前泄露 1372 次重启、天魔心全部规则和最终结局。

## 2. 创意总纲

开场不是“反派履历介绍”，而是一则逐渐变冷的英雄史诗：

1. 穿越者相信自己带着答案而来。
2. 太虚界曾被无相魔庭奴役。
3. 同为穿越者的顾长渊靠知识、组织与信任拯救世界。
4. 人们为了避免灾难重演，把越来越多的选择交给英雄。
5. 英雄没有放下权力，救人的制度变成规定所有人一生的天律。
6. 新的界外之人醒来，故事把问题交给玩家。

情绪曲线：`迷惘 → 恐惧 → 希望 → 壮烈 → 温暖 → 不安 → 寒冷 → 苏醒`。

## 3. 视觉一致性圣经

### 3.1 总体画风

原创高品质 2D 仙侠电影概念动画；中国水墨空间、手绘赛璐璐人物、克制的半写实材质。远景允许墨色晕染，人物脸部必须清晰稳定。不要照片感、塑料 3D 感、日漫校园感或现有仙侠 IP 既视感。

主色演进：

- 穿越：墨黑、冷白、星蓝。
- 魔庭：焦黑、病态暗红、脏铜。
- 起义：灰蓝、布衣褐、微弱暖火。
- 胜利：天青、日金、朱红。
- 镇世：玉白、冷金、规则化青绿。
- 苏醒：雨灰、碑石黑、远灯琥珀。

### 3.2 顾长渊统一造型

年轻时期：约 27 岁，东方男性，身形高而精瘦，眉骨清晰、眼神坚定，黑色长发高束，面部无胡须；穿旧白灰短袍、深色护腕和磨损布靴，唯一高饱和物是暗红围巾。不是天生帝王，首先像会和普通人一起搬粮、包扎伤口的行动者。

镇世时期：约 45 岁外观，必须保留相同脸型、眉眼、鼻梁和暗红围巾；鬓边出现一缕银发，服装变为黑玉与冷金层叠长袍，肩部不过度宽大，背后出现由六段玉简组成的淡金法环。神情疲惫、克制、绝对确信，不做癫狂邪笑。

一致性锚点：`相同眉眼 + 左眉尾细小旧伤 + 暗红围巾 + 修长手指 + 无胡须`。

### 3.3 无相魔庭统一造型

- 魔族不是传统西方恶魔，不使用蝙蝠翼、羊角和地狱火套装。
- 核心视觉是“欲望被掏空”：黑烟般的人形躯体、破损白瓷面具、面具内没有五官，胸腔悬着暗红愿火。
- 高阶魔将穿脏铜与黑铁拼接甲，动作像被无形丝线牵引。
- 愿火以暗红记忆碎片出现，偶尔闪过普通人的手、眼睛、旧家门和摇篮，不出现可读文字。

### 3.4 太虚界统一元素

- 建筑：山地木石城、悬崖栈道、青瓦、灵田与六州长桥；不堆砌黄金宫殿。
- 起义标记：普通人把一小段暗红布系在手腕或兵器上，来自顾长渊的围巾颜色。
- 镇世标记：六段玉简组成的圆环，象征六州被同一秩序闭合。
- 英雄碑林：数百块没有姓名的深灰石碑，只有远处一块完整巨碑；雨、低雾、风吹荒草。

## 4. 全局生成提示词

每个镜头都在自己的场景描述后附加这一段：

```text
Use case: stylized-concept
Asset type: cinematic shot for an original Chinese xianxia 2D roguelite opening film
Style/medium: premium hand-painted 2D cinematic animation, Chinese ink-wash atmosphere blended with refined painterly cel shading, restrained semi-realistic anatomy, layered parallax depth, subtle film grain
Composition/framing: strict 16:9 cinematic composition, clear focal hierarchy, safe crop for 2.39:1 letterbox, physically plausible camera movement
Lighting/mood: dramatic but restrained, volumetric mist, readable silhouettes, natural contrast
Continuity: preserve all established character facial features, costume colors, props and world motifs across shots
Constraints: original design only; no readable text inside generated footage; no subtitles; no UI; no logo; no watermark; no existing franchise resemblance; no photorealism; no plastic 3D render; no extra limbs or fingers; no rapidly changing faces; no costume morphing; no random weapons; no modern objects
```

通用负面提示词：

```text
watermark, logo, subtitles, readable letters, modern city, modern firearm, western demon horns, bat wings, superhero costume, sci-fi armor, photorealistic live action, glossy 3D, plastic skin, chibi, comedy, oversaturated neon, excessive particles, flickering face, identity drift, costume change, duplicated person, extra limbs, malformed hands, broken anatomy, unstable architecture, camera teleport, fast chaotic motion
```

## 5. 96 秒逐镜头分镜与提示词

### 镜头 01｜0:00–0:06｜重来之声

画面：纯黑中出现极短的记忆闪片——雨夜车窗、病房垂落的手、烧毁的旧信、空荡餐桌。每个画面都无法确认具体时代和人物，最后被墨水吞没。

旁白：

> 如果能重来……我一定不会再输。

关键帧提示词：

```text
Black void where four fragmented human memories briefly surface like torn reflections in black water: a rain-covered window, a hand slipping beside a hospital bed, the edge of a burned letter, an empty dinner table. No identifiable face, no readable writing. The memories dissolve into spreading Chinese ink.
```

运动提示：极慢推近；记忆以 8～12 帧闪现；墨迹从画面边缘向中心吞没。不要快速炫技转场。

声音：远处心跳、潮水倒流、三次极轻的玻璃共振。

### 镜头 02｜0:06–0:13｜界外坠落

画面：一个没有明确性别的人形剪影从无数破碎世界倒坠。碎片中有不同文明，但全部抽象化。剪影穿过月形裂隙，化为一粒白光。

旁白：

> 如果去到另一个世界，带着曾经的一切……总能活得更好吧。

关键帧提示词：

```text
An anonymous gender-neutral human silhouette falling slowly through a vertical ocean of shattered worlds, fragments suggesting many civilizations only through abstract architecture and light, never specific brands or eras. Ahead is a pale moon-shaped dimensional crack above an immense ink-black xianxia realm. The silhouette becomes one tiny white soul-light.
```

运动提示：镜头与人物一同倒坠，碎片缓慢反向上升；最后穿裂隙时白场两帧。

声音：风声由倒放转为正常，加入低沉编钟第一音。

### 镜头 03｜0:13–0:20｜答案并不属于这里

画面：白光落入古老城镇。一只手试图在尘土上画出复杂图纸，线条却被陌生灵气吹散；远处百姓关闭门窗。

旁白：

> 可记得答案，不等于看懂问题。

关键帧提示词：

```text
Ground-level close shot in an ancient mountain town at dusk. An anonymous traveler's hand draws an ingenious mechanical plan in dust, but unfamiliar jade-colored spiritual wind lifts and scatters every line. Local townspeople quietly close wooden doors in the soft-focus background, wary rather than hostile. No readable symbols.
```

运动提示：先锁定手和图，灵气掠过后轻微拉焦到关闭的门；不出现主角正脸。

声音：砂砾摩擦、木门合拢、音乐短暂停顿。

### 镜头 04｜0:20–0:28｜魔庭百年

画面：无相魔庭占领的山城。黑烟魔兵从人群胸口抽出愿火，愿火汇向天空中的巨大暗红漩涡，灵田枯败。

旁白：

> 太虚曾被无相魔庭统治。它们不只夺走性命，也吞食人的愿望。

关键帧提示词：

```text
Wide establishing shot of a conquered xianxia mountain city beneath ash-filled clouds. Faceless black-smoke demon soldiers wearing cracked white porcelain masks draw dim crimson wish-flames from kneeling civilians. Thousands of memory-like red embers flow upward into a vast restrained vortex. Terraced spirit fields wither along the valley. Tragic, dignified, no gore.
```

运动提示：高位缓慢横移，先见城市规模，再下降到一盏被抽走的愿火；避免大量高速群演。

声音：低频人声合唱、木鱼般的规律敲击、愿火被抽离的吸气声。

### 镜头 05｜0:28–0:36｜另一个界外之人

画面：年轻顾长渊在人群中跪着替伤者止血，而不是高高站立。随后他抬头观察魔族运输队和粮道。

旁白：

> 直到另一个界外之人来到这里。他叫顾长渊。

关键帧提示词：

```text
Young Gu Changyuan, 27-year-old lean East Asian man with tied long black hair, same angular determined eyes, a tiny scar at the tail of his left eyebrow, worn white-gray short robes, dark bracers and a muted crimson scarf, kneeling among ordinary refugees while tightly bandaging a wounded farmer's arm. His expression is focused and compassionate. In the far background a demon supply convoy crosses a mountain bridge.
```

运动提示：从染血绷带上移到顾长渊的眼睛，再沿他的视线切到远处粮道。

声音：音乐第一次出现温暖古琴动机；伤者呼吸、布带收紧。

### 镜头 06｜0:36–0:44｜知识成为方法

画面：连续蒙太奇——顾长渊教人净水、统一粮袋重量、绘制无文字地图符号、组织伤员转运；百姓主动加入。

旁白：

> 他带来的不是神谕，而是方法。净水、医伤、记账、协作……还有相信彼此的勇气。

关键帧提示词：

```text
Hopeful resistance camp montage centered on the same young Gu Changyuan. Ordinary villagers boil and filter water, weigh grain with simple balanced scales, organize medical stretchers, and arrange colored wooden tokens on a map without readable writing. Gu works at the same level as everyone else. Small muted crimson cloth strips are tied around wrists as a shared resistance sign.
```

运动提示：四个动作以匹配剪辑衔接；所有镜头保持中近景、手部动作明确；最后由一条红布带甩动转场。

声音：木器、沸水、脚步逐层组成节奏；加入箫和鼓的上行音型。

### 镜头 07｜0:44–0:53｜众人之战

画面：六派与平民军在山谷共同作战。顾长渊只是阵线中的一个人，信号旗、救护队、剑阵与雷法协同击破魔军。

旁白：

> 后来，人们把彼此交给彼此。六州同盟，凡人与修士并肩，向魔庭举起了剑。

关键帧提示词：

```text
Epic wide battlefield in a misty mountain valley. Six distinct xianxia schools and organized civilian resistance fighters act in coordinated layers: shield line, stretcher teams, sword formation, restrained thunder arts, signal banners and supply runners. Young Gu Changyuan with crimson scarf is visible within the formation, not larger than everyone else. Faceless porcelain-mask demons break under coordinated pressure. Heroic, readable, no gore.
```

运动提示：侧向长镜头穿过后勤、伤员、平民军、六派阵线，最后抵达顾长渊；不要只拍个人无双。

声音：战鼓达到第一峰值，剑鸣、雷声、呼喊均压在旁白下方。

### 镜头 08｜0:53–1:01｜无明渊

画面：顾长渊在深渊边抓住一颗巨大暗红心核，愿火沿手臂进入胸口。他痛苦但清醒，随后以一剑斩开魔庭王座。

旁白：

> 在无明渊，他把最后的魔心封进自己体内，换来一场所有人都以为不会结束的胜利。

关键帧提示词：

```text
At the edge of the colossal Wuming Abyss, the same young Gu Changyuan grips a floating dark-crimson crystalline heart made from trapped wish-flames. Controlled red memory-light travels through his fingers into his chest while his crimson scarf whips in the storm. He remains conscious and resolute. Behind him, the black-metal demon throne splits under a clean white sword arc. No gore, no monstrous transformation yet.
```

运动提示：环绕半圈，心核脉动与鼓点同步；白色剑光擦满画面转入日出。

声音：战鼓骤停，只留心跳三次；斩击后进入真空，再出现晨鸟。

### 镜头 09｜1:01–1:09｜太平初日

画面：同一山城重建，灵田恢复，孩子从挂着红布的门下跑过。顾长渊站在人群中，被人们拉到临时高台上。

旁白：

> 魔庭覆灭。人们请英雄再守十年，等伤口愈合，等天下学会自己站立。

关键帧提示词：

```text
Warm sunrise over the rebuilt version of the same mountain city. Restored terraces glow green, carpenters repair roofs, healers teach apprentices, children run beneath a faded crimson cloth ribbon. Young Gu Changyuan is gently pulled by grateful citizens onto a modest wooden platform; he looks reluctant and exhausted, not triumphant.
```

运动提示：由灵田水面倒影抬升至全城；人群动作自然克制；暖色达到全片最高。

声音：笛、古琴、轻弦乐；远处孩童笑声一次，不渲染盛典。

### 镜头 10｜1:09–1:17｜十年又十年

画面：固定机位时间流逝。木台变石台，石台变冷玉阶；顾长渊从青年变为中年，围巾仍在，身后逐渐形成六段玉简法环。

旁白：

> 可十年之后，还有叛乱。又二十年，仍有人想让灾难重来。

关键帧提示词：

```text
Locked symmetrical time-lapse composition in the city square. A modest wooden platform gradually becomes stone steps and finally austere pale-jade architecture. Gu Changyuan ages from 27 to an outward 45 while preserving the same face, left eyebrow scar and crimson scarf; one silver streak appears at his temple, his clothing evolves into restrained black-jade and cold-gold layered robes, and six jade slips slowly form a dim circular halo behind him. Seasons and crowds pass while he remains.
```

运动提示：相机完全锁定；只让季节、建筑、服装和人群变化。年龄变化分段完成，禁止脸部抖动。

声音：每次年代跨越响一次编钟；温暖配器逐渐抽走，仅余规则化节拍。

### 镜头 11｜1:17–1:25｜太平天律

画面：天律带来真实秩序，也夺走选择。粮仓开启、药品发放，与此同时迁徙者被拦、孩子被分配职业、史册页面被涂白。

旁白：

> 于是他替人们决定何处安居、修何种道、记住什么，又忘记什么。

关键帧提示词：

```text
Cold, precisely composed three-part visual montage under the mature Gu Changyuan's jade-order motif: disciplined granaries distribute food fairly; an efficient public clinic gives medicine to a child; at the same time a family carrying travel bundles is calmly stopped at a checkpoint, young apprentices are separated into predetermined profession lines by colored tokens, and a historian watches blank white paint cover old illustrated records. No readable text, no overt torture.
```

运动提示：使用相同的横向构图把“公共利益”和“失去选择”并置；守卫表情平静，不做夸张暴徒。

声音：规整木梆节拍；药碗、门闩、刷子涂纸声形成不舒服的节奏。

### 镜头 12｜1:25–1:32｜英雄与恶龙

画面：中年顾长渊独坐巨大空殿，不享乐也无群臣。他身后的法环投影在墙上，恰似盘踞的龙；胸口深处有一次暗红脉动。

旁白：

> 人们曾把选择交给英雄。后来，再没有人知道……英雄与恶龙究竟在何处交换了名字。

关键帧提示词：

```text
Mature Gu Changyuan sits alone in an immense austere dark-jade hall with no luxury, no feast and no courtiers. Same angular face, left eyebrow scar, crimson scarf beneath black-jade robes, one silver temple streak, expression exhausted and absolutely convinced. The six-segment jade halo behind him casts a huge dragon-like shadow across the wall without becoming a literal dragon. One restrained dark-red pulse glows beneath his chest.
```

运动提示：极慢后拉，让人物越来越小、龙影越来越完整；胸口只脉动一次；禁止邪笑。

声音：低音弦与心跳合并；旁白结束后留 0.8 秒寂静。

### 镜头 13｜1:32–1:36｜碑林苏醒

画面：第一人称模糊视角在暴雨中睁开眼。无名碑林倒映冷月，远处唯一完整巨碑和一盏移动的追兵灯火。手掌撑进泥水，但看不到角色身份。

旁白：

> 而现在，你醒来了。

关键帧提示词：

```text
First-person awakening in a rain-soaked field of hundreds of dark gray nameless hero steles at night. The viewer's anonymous hand presses into mud, with no sleeve color or gender clue. Vision sharpens from heavy blur to reveal one immense intact stele in the distance, low fog, windblown dead grass, cold moonlight, and one tiny amber patrol lantern moving between stones. No readable inscription.
```

运动提示：从黑场睁眼，两次眨眼式曝光变化；镜头轻微失衡后稳定；雨滴在“镜头”表面滑落。

声音：暴雨、急促呼吸、泥水；音乐只留一个持续低音。

### 镜头 14｜1:33–1:36（片尾叠化长度 3 秒，剪辑时总长仍为 96 秒）｜题名

说明：该镜头与镜头 13 尾部叠化，不额外增加总时长。

画面：一道闪电照亮碑林，剪辑软件叠加游戏题名“太虚问道”，副标题“屠龙者，后来如何”。生成视频本身不要生成文字。

屏幕文字由剪辑软件添加：

> 太虚问道  
> 屠龙者，后来如何

运动提示：题名淡入 12 帧，停留约 2 秒，随后直接切入实时场景；闻砚台词“别信碑上写的。也别急着信我。”应由游戏内演出接续，不烘焙进视频。

声音：闪电后出现一记远钟；标题落下时播放两音主题动机。

## 6. 完整旁白台本

旁白建议使用同一名中性、略有疲惫感的成年声音；语速 190～215 汉字/分钟，避免预告片式大喊。前三句可用三名不同声线轻声叠入，随后统一为主旁白。

```text
如果能重来……我一定不会再输。

如果去到另一个世界，带着曾经的一切……总能活得更好吧。

可记得答案，不等于看懂问题。

太虚曾被无相魔庭统治。它们不只夺走性命，也吞食人的愿望。

直到另一个界外之人来到这里。他叫顾长渊。

他带来的不是神谕，而是方法。净水、医伤、记账、协作……还有相信彼此的勇气。

后来，人们把彼此交给彼此。六州同盟，凡人与修士并肩，向魔庭举起了剑。

在无明渊，他把最后的魔心封进自己体内，换来一场所有人都以为不会结束的胜利。

魔庭覆灭。人们请英雄再守十年，等伤口愈合，等天下学会自己站立。

可十年之后，还有叛乱。又二十年，仍有人想让灾难重来。

于是他替人们决定何处安居、修何种道、记住什么，又忘记什么。

人们曾把选择交给英雄。后来，再没有人知道……英雄与恶龙究竟在何处交换了名字。

而现在，你醒来了。
```

## 7. 音乐生成材料

当前 88 秒即梦剪辑采用“前世耳语 + 主旁白 + 无歌词女声吟唱 + 低声混合合唱”的三层人声方案，完整时间、台词、配器和混音规则见 `Docs/W11开场动画88秒人声配乐与旁白设计.md`。

音乐长度建议生成 92 秒，前后各留约 2 秒余量。可将以下内容交给 AI 音乐工具：

```text
Create an original 92-second cinematic score for a dark hand-painted Chinese xianxia game opening. Include restrained human voices but no intelligible sung lyrics: intimate adult whispers at the beginning, a distant solo female alto vocalise, and a low mixed choir humming beneath the occupied-world and authoritarian sections. Begin with reversed breath, sparse low guqin harmonics and a distant temple bell; introduce a five-note xiao and guqin motif as people organize resistance; build to controlled war drums and bowed strings; briefly open into warm morning strings and female vocalise after victory; then make the same theme colder and mechanically quantized as peace becomes authoritarian order. End with heartbeat, rain, the final two notes sung wordlessly by the solo alto, and one distant bell. Leave clear space for Mandarin narration. No lyrical words, no recognizable melody, no pop vocal, no opera singing, no modern drum kit, no EDM, no triumphant brass fanfare, no excessive trailer booms.
```

分段音乐提示：

- 0:00–0:17：前世耳语、倒放呼吸、古琴泛音与远处单人女声。
- 0:17–0:30：低男声合唱和木鱼节拍，顾长渊出现时奏出五音主题。
- 0:30–0:44：无歌词混合合唱逐渐温暖，木器形成协作节奏，战鼓最后进入。
- 0:44–0:56：魔心处抽空音乐；日出后短暂出现暖弦和女声吟唱。
- 0:56–1:10：主题机械化、降调，合唱呼吸被规整节拍切断。
- 1:10–1:23：只留低女声、冷弦、心跳与雨。
- 1:23–1:28：女声无歌词唱出最后两音，一记远钟收尾。

## 8. 音效素材清单

| 编号 | 音效 | 用途 | 备注 |
|---|---|---|---|
| SFX-01 | 倒放潮水与呼吸 | 穿越记忆 | 必须轻，不能盖旁白 |
| SFX-02 | 低沉编钟 | 世界切换、年代跨越 | 同一口钟保持听觉标识 |
| SFX-03 | 愿火抽离 | 魔庭统治 | 人的吸气与风声混合，不做怪兽吼 |
| SFX-04 | 布带收紧 | 顾长渊初登场 | 近距离干声 |
| SFX-05 | 木器、沸水、秤砣 | 建立抵抗 | 组成有节奏的生活声 |
| SFX-06 | 战鼓、剑阵、远雷 | 六州之战 | 战鼓不能全程轰鸣 |
| SFX-07 | 魔心三次脉动 | 封印与独裁暗示 | 三处复用同一采样 |
| SFX-08 | 木梆与门闩 | 太平天律 | 规律得近乎不自然 |
| SFX-09 | 暴雨、泥水、喘息 | 玩家苏醒 | 与实时碑林环境声无缝衔接 |
| SFX-10 | 两音题名动机 | 游戏标题 | 后续主菜单音乐可引用 |

## 9. AI 视频生产建议

1. 先生成顾长渊年轻版、镇世版、魔庭山城、碑林四张视觉锚点。
2. 对每个镜头先做静态首帧和尾帧，确认人物身份、服装与光线，再做图生视频。
3. 单段不要超过 8 秒；长镜头拆成两个 4 秒段，使用相同末帧/首帧续接。
4. 群战镜头优先做横移和视差，不要求 AI 同时生成数十个复杂独立动作。
5. 角色一致性工具中锁定顾长渊脸部与暗红围巾；镇世版只允许年龄、发丝和外袍变化。
6. AI 不生成中文标题和碑文。所有文字、字幕和 UI 在剪辑软件中叠加。
7. 输出时保留无字幕母版、中文字幕版、单独旁白、单独音乐、单独音效五组文件。
8. 镜头 13 尾部应与游戏内实时碑林使用相同雨声和色温，降低视频切回引擎时的割裂感。

## 10. 推荐交付目录

```text
SourceArt/W11/Cinematics/Opening/
├── W11_Opening_GlobalPrompt.txt
├── W11_Opening_VO_ZH.txt
├── W11_Opening_Subtitles_ZH.srt
├── W11_Opening_ShotList.csv
├── References/
│   ├── W11_Opening_Styleframe_DemonRule.png
│   ├── W11_Opening_Styleframe_YoungGu.png
│   └── W11_Opening_Styleframe_SteleAwakening.png
├── GeneratedClips/
├── Audio/
└── Masters/
    ├── W11_Opening_4K_Clean.mp4
    ├── W11_Opening_4K_ZH.mp4
    └── W11_Opening_1080p_ZH.mp4
```

当前已生成三张 1672×941 的 16:9 视觉锚点并保存到 `References/`。生成方式为 OpenAI ImageGen 内置模式；三张图的无删减最终提示词见 `W11_Opening_ReferencePrompts.txt`。它们只用于统一视频模型的画风、顾长渊身份与碑林转场，不直接视为最终视频帧。

逐镜头通用设计源保留在 `SourceArt/W11/Cinematics/Opening/Shots/`，旧 `ShotPackages_Zip/` 只作为可追溯的通用交付版本。当前即梦投喂材料继续复用 `SourceArt/W11/Cinematics/Opening/Wan3_Packages/` 这一既有目录，避免无意义移动大批图片；14 个片段各自保留 JPG 参考图、无损 PNG 与详细提示词。提示词已改为多镜头参考生视频，在 88 秒内明确安排 38 个真实机位和硬切；若即梦界面只支持基础首帧 I2V，则按子镜头分别生成后在剪映硬切。

## 11. 验收清单

- 不固定玩家性别、服装或脸部。
- 顾长渊年轻与中年版本能被一眼认成同一个人。
- 顾长渊前半段确实像英雄，不能从第一次出现就阴沉可疑。
- 魔族压迫清晰但不靠血腥和猎奇。
- 同时表现镇世秩序的真实公共利益与自由代价。
- 不提 1372 次重启，不解释真结局。
- 全片没有 AI 生成的乱码中文。
- 字幕、旁白与关键动作时间对齐。
- 视频结尾能自然接入英雄碑林的实时游戏画面。
- 使用的 AI 工具、模型、音乐和音效许可可用于 Steam 商业发行，并保留生成记录。
