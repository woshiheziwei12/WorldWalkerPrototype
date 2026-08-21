# W02 巴比伦阶梯神塔设计依据

## 采用的建筑事实

- UNESCO 的 Babylon 世界遗产申报材料记载 Etemenanki（“天地的基台”）约为 `91 m × 91 m`，理想化高度约 `91 m`，顶层神庙很可能使用与伊什塔尔门相近的蓝色釉砖：<https://whc.unesco.org/document/166322>
- 法国文化部东方考古专题将其数学模型概括为七层（顶庙作为第七层），泥砖核心外覆烧制砖，并说明顶庙围绕内院组织多个礼拜空间：<https://archeologie.culture.gouv.fr/orient-cuneiforme/fr/larchitecture-de-la-ziggurat>
- Cambridge 公开论文对“通天塔石碑”的摄影测量重建再次给出约 `91.5 m` 的下层尺度、泥砖核心/烧制砖外皮和蓝釉顶层建筑等证据：<https://www.cambridge.org/core/services/aop-cambridge-core/content/view/A712F186B2FA1965854911081620467B/S0021088924000044a.pdf/the-tower-of-babylon-stele-found-in-babylon.pdf>

W02 不是考古复原。为了第三人称相机、既有跳跃弹道和约 10–15 分钟的原型流程，运行时塔高保持 `52.141 m`、基台为 `52 m × 52 m`。建筑语汇采用六层泥砖塔体加第七层蓝釉圣所，六座大型露台对应现有六个存档点。

## 艺术构图参考

勃鲁盖尔《通天塔》用于参考层叠轮廓、重复拱廊、外露施工木架和环绕式施工路线；该作品为公版艺术品：<https://commons.wikimedia.org/wiki/File:The_Tower_of_Babel_(Bruegel).jpg>

项目没有复制画作纹理或下载来源不明的整塔模型。塔体与碰撞由 C++ 运行时基础几何生成，外观只复用项目已登记的 CC0 Poly Haven Modular Fort 01、Wood Planks 与 Kenney Castle Kit 模块。因此关卡在第三方网格缺失时仍可完整游玩，且没有新增授权不清晰的资产依赖。

## 运行时对应关系

- 六段、每段 16 个节点的既有弹道路线改为神塔外墙施工路线；96 个落点与攀爬、梯子、体力系统保持不变。
- 第 16/32/48/64/80/96 节落点扩展为大型方形露台，作为修正节奏、叙事泥版和存档点。
- 四面使用重复砖墙、三联拱廊、角部扶壁与实体木脚手架；东侧设置不承担通关捷径的仪式长阶。
- 第七层蓝釉圣所连接日出观景台，玩家仍可自行选择停留或按 `E` 返回 W00。
