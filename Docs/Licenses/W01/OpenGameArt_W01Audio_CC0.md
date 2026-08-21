# W01 OpenGameArt 环境音授权记录

记录日期：2026-08-15

## Fireplace Sound loop

- 作者：PagDev
- 来源：https://opengameart.org/content/fireplace-sound-loop
- 原始文件：https://opengameart.org/sites/default/files/fire.wav
- 页面标注许可证：CC0 1.0 Universal
- 原始文件 SHA-256：`85CA0CC60D0C037FFF8B185E31AD1FCDBDA6CE45EEE17C3EE1318D1B8F59E330`
- 用途：W01 营火空间循环声。

## Dark Ambiences

- 作者：Ogrebane
- 来源：https://opengameart.org/content/dark-ambiences
- 原始归档：https://opengameart.org/sites/default/files/dark_ambiences.zip
- 页面标注许可证：CC0 1.0 Universal
- 归档 SHA-256：`4E95C28A468CBDE21D6300AA0BB9A5AEDBEE01B393E712418C49D6B4B8A758BB`
- 用途：W01 路线外围随机播放的五种暗黑环境声。

归档内采用文件 SHA-256：

- `ambience-1.wav`：`23C9438B65835F8E493A00DE244387BE33E0303E774E15022692CABB1B707DF0`
- `ambience-2.wav`：`B03D50AA11DF1FD7D859A5776299D73F9D9444565AC6501720D0035DF0AB3424`
- `ambience-3.wav`：`CEEFB1041A70357F8D5A9D4A22C200992C38E34468D5717663402435290D8AA6`
- `ambience-4.wav`：`C5AA2E32B681654DBF510EE4A5E85C5A7968A9A8D3C329CF78108542381D2D2D`
- `ambience-5.wav`：`2A07090EB244E42445F7286DC68811BCB79D2F236825F770058B02669A11828F`

`Scripts/import_w01_audio_assets.py` 会在导入前校验归档与逐文件哈希，并把 24/32 位 PCM 确定性转换为 UE 可移植的 16 位 PCM。源文件仅缓存在临时目录，最终 SoundWave 只写入 W01 自己的 `ThirdParty/OpenGameArt/Audio`。

## Dark Shrine Loop

- 作者：qubodup
- 来源：https://opengameart.org/content/dark-shrine-loop
- 原始文件：https://opengameart.org/sites/default/files/qubodup-yd-DarkShrineLoop-OpenGameArt.ogg
- 页面标注许可证：CC0 1.0 Universal
- 原始文件 SHA-256：`9580618DC851F70C3A11B5FF87672867DE44CD38B35DABF926D6BEFDE20E78D0`
- 用途：W01 探索阶段低音量循环音乐。

## Epic March Loop

- 作者：Eldritch Grim
- 来源：https://opengameart.org/content/epic-march-loop
- 原始文件：https://opengameart.org/sites/default/files/the_march_of_devils_dome_loop.wav
- 页面标注许可证：CC0 1.0 Universal
- 原始文件 SHA-256：`7DFFCB82140F3ACED075B2766F135C17757C60BD8F7A2717B11AB49318C824D6`
- 用途：W01 卡牌战斗阶段循环音乐。

## Card Game sounds

- 作者：HaelDB
- 来源：https://opengameart.org/content/card-game-sounds
- 原始归档：https://opengameart.org/sites/default/files/Cardsounds.zip
- 页面标注许可证：CC0 1.0 Universal
- 归档 SHA-256：`0C01B7807909119A4D93364FA3661F7C4BE1421721E65DA8A816EDD71CF3F899`
- 采用：出牌、抽牌、洗牌、结束回合、路线显现五种提示音。

## 80 CC0 RPG SFX

- 作者：rubberduck
- 来源：https://opengameart.org/content/80-cc0-rpg-sfx
- 原始归档：https://opengameart.org/sites/default/files/80-CC0-RPG-SFX_0.zip
- 页面标注许可证：CC0 1.0 Universal
- 归档 SHA-256：`1C2F06FF4E8563B5B8B745B23CF213C1474142A69BB82BD8F5E10D9B3F7A7BBD`
- 采用：`blade_01.ogg`、`spell_01.ogg`、`metal_01.ogg`，分别用于攻击、法术、格挡/装备反馈。

扩充后的导入脚本仍只把派生的 `SoundWave` 写入 W01 专属目录；下载归档保留在被 Git 忽略的 `Saved/W01AudioSources` 缓存，便于离线重建。
