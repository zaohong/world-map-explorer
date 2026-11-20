# 离线数据来源、版本与使用条件

## 世界 2026 国家／地区填色（1.15.0，可单独开关的细格图层）

- 离线模块：`data/world-2026.json`，与原 `zones.json`、土地覆盖、人口和遗产数据分开保存。它只给 627,200 个细格附加国家／地区颜色，不改动 9,800 个父格档案，也不改动任何底图图片。格网菜单选粗层级时，仍显示同一份 627,200 格填色，菜单格线独立显示。地区记录同时保留 Natural Earth 英文名、来源所带中文名和地图代码；点击后右侧优先显示中文名。
- 来源：[Natural Earth Admin 0 – Countries 1:10m](https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-admin-0-countries/)，本地采用的来源快照版本为 5.1.1，共 258 个国家／地区地图单元。Natural Earth 说明默认边界按实际控制状态绘制，并提供不同观点的可选版本；本模块采用默认版本，不额外裁决争议。[Natural Earth 使用条款](https://www.naturalearthdata.com/about/terms-of-use/) 将其数据列为公有领域。
- 本项目快照日期为 2026-10-30，来源压缩包 SHA-256 为 `CE1AC7036499A0EDD641FBC093CD209A98F96A49D2ECA8480AAACAD35138A7F6`。这是在快照日采用的 Natural Earth 5.1.1 边界数据，不表示 Natural Earth 在该日重新发布了边界。
- 全局网格是 WGS 84 经纬度矩形，560 行 × 1,120 列，共 627,200 格；每格宽、高约 0.3214286°。通常按细格中心落入哪个国家／地区多边形来归属，格心不在陆地国界面中的格子透明。填色以最近邻方式显示，不插值，格网切换、缩放和平移都复用同一离线数据。
- Natural Earth 的 `MAPCOLOR7` 为相邻区域设计 7 种制图颜色。颜色仅用于区分地图区域，无数值、人口或重要性含义；非相邻地区可能重复使用同色。
- 小于细格的地区无法按真实边界完整表达。对 57 个未能包含任何细格中心的小区域，使用 Natural Earth 标签点指定一个代表细格；若两个标签点落入同一细格，第二个区域分配到最近的空闲细格。此规则让 258 个地图单元都能在图层中出现，但这些代表格不表示实际面积或精确国界，微小岛屿及海岸／国界格仍是约 35 km 级的格心近似。
- 图层可独立开关，也可叠加在其他专题层上。打开后点击地图，右侧显示该位置所属的细格国别；关闭图层会隐藏此项而保留其他图层状态。历史层使用下节所述独立矢量档案，不写入父格档案。
- 本图用于全球浏览和专题表达，不是法律边界、航海或工程测绘资料。`迭代工作/1.16.0/native/build_world_2026.cpp` 与归档中的源数据可重建该模块；地图程序离线运行。

## 历史疆域矢量（1.27.0）

- 运行文件为 `data/historical-boundaries.wmh2`，共 57,470,845 字节；取代逐年保存的格心归属 JSON。文件通过只读映射访问，重复几何仅保存一次。显示和点击查询使用多边形，网格仅作为定位工具。
- 来源为 [Cliopatria / Seshat Global History Databank](https://github.com/Seshat-Global-History-Databank/cliopatria)，固定文件提交 `5f433377139a6eeaeacbdc894b70f8805b72c8b5`、Git blob `a1e7093b64990bf97cd3c31bda96de1cafed822c`。归档名为 `cliopatria_polities_only_v021.geojson`；ZIP 和 GeoJSON 的 SHA-256 见 `data/historical-boundaries-source.json`。论文：[Bennett et al. (2025), Cliopatria, Scientific Data 12:247](https://doi.org/10.1038/s41597-025-04516-9)。
- 原始资料13797条；本地派生记录13800条，含13415条POLITY及385条RELATION。关系记录供资料页查阅，不作为主权领土填色。原始及补充数据共11317份不同几何、3422262个坐标点，保留多部件、内洞和日期变更线；坐标按float64存储，没有用格网中心代替边界。
- 可选择前3400年至2024年的5424个非零年份。每个年份依据来源的包含式FromYear / ToYear筛选；每年可选不表示每年都有边界观测，不在年份之间推演疆域。播放跳到来源边界变更、事件和辅助纪年节点，跨过的事件不会丢失。
- 来源存在概化、空白、重叠和命名错误。资料有效年段不等于王朝建立、灭亡日期。古代“Later Zhou”误名仅校订显示为东周，原几何及年段保留。中国秦汉等节点应结合独立事件资料理解，不把有效年段冒充历史定论。
- 六个2024年小国参考补充使用Natural Earth 5.1.1：摩纳哥、列支敦士登、圣马力诺、梵蒂冈、安道尔、卢森堡。它们替换该节点缺漏或粗糙的对应记录，标注“现代参考补充”；不回推早期年份，也不声称是2024年实测。Natural Earth为公有领域，1:10m指一千万分之一制图比例尺，不是10米精度。
- 历史边界按[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)保留Seshat / Cliopatria署名。许可见 `licenses/CLIOPATRIA_RAW_LICENSE.md`。1420个Wikidata标识的中英文标签快照用于名称补充，Wikidata结构化数据为CC0；无中文名时保留英文原名。
- 空白表示缺少对应来源记录；重叠记录按面积较小者优先列出。颜色只用于区分区域。图层不用于法律边界、导航或工程测绘。更细的绘图不消除史料本身的不确定性。

## UNESCO 世界遗产名录（1.9.0，可选离线专题层）

- 官方数据集：[UNESCO DataHub — World Heritage List (`whc001`)](https://data.unesco.org/explore/dataset/whc001/)；官方名录：[UNESCO World Heritage List](https://whc.unesco.org/en/list/)。本版保存 2026-10-15 数据快照，并记录数据集修改日期、快照日期、来源链接和署名。
- 官方导出共 1,273 项；1,271 项具有组成部分坐标，合计 6,397 个点。系列/多组成部分遗产按官方 GeoJSON `MultiPoint` 的每一点分别归入 Z9800 网格；测试确认坐标与官方组成部分列表逐项一致。2 项没有官方组成点坐标，不会被伪造位置。
- 离线文件 `data/world-heritage-2026.json` 是独立派生汇总，包含每格对应遗产的稳定编号、中英文名称、类别、列入年份、组成部分名称和官方短说明；可按中英文名称搜索。只有点落入某格才会出现在该格，非遗产地点不会从附近范围推断关联。
- 该独立派生模块按官方数据集注明的 [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) 发布，署名 UNESCO；若再分发或改编此模块，应保留来源署名并按相同许可共享改编内容。此许可说明仅对应 UNESCO 模块，不自动适用于程序、其他专题数据或底图。
- UNESCO 的名录描述和坐标用于离线地理参考，不等同现场勘查或完整历史研究。跨国/系列遗产可能在多个格重复出现，这是组成部分点的真实归属，不是重复记录错误。

## NASA Blue Marble Next Generation（可选高清底图，1.2.0）

- 官方资料页：[Blue Marble: Next Generation with Topography and Bathymetry](https://science.nasa.gov/earth/earth-observatory/blue-marble-next-generation/base-topography-bathymetry/) 提供按月份的全球合成图，并列出 21,600 × 10,800（2 km/像元）全球 JPEG 与更高分辨率分块版本。当前随包采用 January 图层的全球 JPEG：`assets/world-map-nasa.jpg`，源文件 21,600 × 10,800、29,698,041 字节；配套 `.jgw` 和 `.prj` 以 WGS 84 经纬度记录完整全球覆盖范围。
- 这是一张包含地形与海底阴影的月度合成底图，不是当前实时卫星影像；细节、季节雪/植被外观和海洋纹理与 Natural Earth 默认制图底图不同。海洋颜色/纹理不能当作逐点水深观测，地形明暗也不是逐像元高程值；数值档案仍以独立 NOAA ETOPO 汇总为准。底图仅作浏览参考，不用于导航或工程测量。
- NASA 大图作为独立可选资源随“NASA 高清地图层补丁”分发，不打进主 EXE，也不替换默认地图/分区数据；启用时程序核验像素尺寸、全幅经纬边界和 WGS 84 标识。原图数据 SHA-256：`5dbb5f101c3336f260506dcd574aaba4d5e9af2ff8533161367098affea86408`。

## NASA Blue Marble Next Generation 500 m 来源的 1 km 细节瓦片（1.3.0）

- NASA 技术资料说明 500 m 全球月度合成图的 8 个 21,600 × 21,600 分块均为 240 像素/度，并逐一列出 A1–D2 的经纬边界；例如 D1 覆盖 90°E–180°E、0°–90°N。来源：[BMNG 技术说明 PDF](https://earthobservatory.nasa.gov/features/BlueMarble/bmng.pdf)。NASA 说明这些月图基于 2004 年 MODIS 资料并表现季节变化：[Blue Marble Next Generation 总览](https://earthobservatory.nasa.gov/features/BlueMarble/BlueMarble.php)。
- 原始 January 2004 八张 JPG 共 432,492,862 字节（约 412.46 MiB），仅留在开发源数据目录，不塞进主 EXE 或桌面包。`tools/build-nasa-1km-tiles.ps1` 对每个源象限按高质量双三次方法从 15″像素降采样到 30″（赤道附近名义约 1 km），并分割为 10° × 10°、1,200 × 1,200 的 648 张 JPEG；每张都带独立 `.jgw`、`.prj`，模块内含完整 SHA-256 清单。输出模块为 108,015,979 字节（约 103.01 MiB）。
- 程序仅在启用 NASA 底图且缩放达到 800% 后加载视口内瓦片；最多缓存 24 张，瓦片尺寸、世界文件边界和 WGS 84 标识逐张校验。缺少细节模块时仍能使用 2 km 全球 NASA 底图；该模块可独立安装/更新，不改动主档案或分区编号。
- 这仍是 2004 年月度合成影像，不是当前实时卫星图。NASA 提醒开阔海域存在纹理噪声、深海使用统一蓝色、部分云/短期雪难以完全区分；使用或再发布应署名 NASA Earth Observatory。详见上述官方资料页。

## NASA Blue Marble Next Generation 原始分辨率瓦片（1.4.0，可选）

- 延续 1.3.0 的 1 km 视口瓦片，新增 `assets/nasa-500m`：直接按整数像素裁切同一组 January 2004 官方 500 m 分幅，不做空间降采样。18 × 36 全球 10°网格共 648 张 2,400 × 2,400 JPEG；目标像素间距 `1/240° = 15″`，对应 NASA 标称 500 m/像元（赤道附近名义间距）。每张附 `.jgw`、WGS 84 `.prj`，模块带逐文件 SHA-256 清单。
- 该模块比 1 km 层大，实际约 445.9 MiB（数据字节数除以 1,048,576），独立分发，不进入主 EXE。NASA 底图缩放 800%–1199% 时使用 1 km；达到 1200% 且安装此模块后，自动优先载入 500 m 瓦片；最多缓存 12 张 2,400 × 2,400 图片以控制内存。未装 500 m 时回退到 1 km；两层都无时使用 2 km 全球图。
- 配准直接由官方 240 像素/度 Plate Carrée 分幅边界推导，而不是从图上人工找控制点；边界定义及限制与前一节相同。此版本提升的是视觉纹理采样，不会使 2004 年合成影像变成实时观测，也不提高 ETOPO 数值高程的精度。

## Natural Earth 10m 彩色地形底图（0.9.0）

- 官方规格：[1:10m Raster Data](https://www.naturalearthdata.com/downloads/10m-raster-data/) 说明栅格与 Natural Earth 10m 矢量配准，提供 `.tfw` 世界文件，覆盖 −180°–+180° 与 −90°–+90°；系列包含 16,200 × 8,100 中分辨率与 21,600 × 10,800 高分辨率。当前产品页：[Natural Earth I](https://www.naturalearthdata.com/downloads/10m-raster-data/10m-natural-earth-1/)。
- 输入包：`NE1_LR_LC_SR_W_DR.zip`，Natural Earth 版本 2.0.0，包含 Natural Earth I 彩色陆表、阴影、水体与排水系 TIFF，16,200 × 8,100；随包的 `.prj` 标识 WGS 84，`.tfw` 像元大小为 0.0222222222°，左上像元中心约 −179.9888889°、89.9888889°。
- 应用底图由该原始栅格生成：`world-map.jpg` 4,050 × 2,025（像元 0.0888888889°），`world-map-hd.jpg` 8,100 × 4,050（像元 0.0444444444°）。两套 `.jgw` 重新计算像元中心坐标，边缘精确覆盖完整全球经纬范围；`.prj` 随图分发。构建脚本为 `tools/build-map-assets.ps1`。
- Natural Earth 数据为 public domain，详见[使用条款](https://www.naturalearthdata.com/about/terms-of-use/)。它是全球制图比例尺的概化地形/地表栅格，不是实时卫星影像或数字高程模型；程序坐标注册精确不代表每条地物线都具有测绘精度。不要用于导航或工程测量。

## Natural Earth 10m Shaded Relief Basic（旧版 0.7/0.8 资源）

- 旧版保留 `SR_LR.zip`（版本 2.0.0）来源：灰度 16,200 × 8,100 TIFF 带 `.tfw`。在 0.7/0.8 中曾缩至 8,100 × 4,050 并融合到无坐标元数据的原始彩色地图，融合区局部配准不可靠；该资产不再用作 0.9 默认底图。历史应用包仍可回退。

## NOAA NCEI ETOPO 2022

- 官方产品页：[ETOPO Global Relief Model](https://www.ncei.noaa.gov/products/etopo-global-relief-model)；引用：NOAA NCEI, ETOPO 2022, DOI [10.25921/fd45-gt74](https://doi.org/10.25921/fd45-gt74)。
- 本档案内容版本 0.9.0 使用 NOAA ETOPO 2022 15 arc-second（15″）全球冰面版本，共 288 个 15°×15° NetCDF 瓦片、3,732,480,000 个有效源像元；赤道处名义像元尺度约 463 m。产品和瓦片规格详见 [NOAA ETOPO 产品页](https://www.ncei.noaa.gov/products/etopo-global-relief-model) 与 [官方用户指南](https://www.ngdc.noaa.gov/mgg/global/relief/ETOPO2022/docs/1.2%20ETOPO%202022%20User%20Guide.pdf)。
- 每个源像元按其经纬边界与 2.57143°目标格边界的实际相交面积归属，并以 WGS 84 椭球面积元加权；分别汇总陆地面积加权均高、最小/最大值、标准差，以及海洋平均/最大水深、陆地面积、海洋面积（km²）和各自占比。面积统计独立于 2020 年土地覆盖分类。极地采用冰面版本，冰盖上表示冰面高程，不应解读为冰下基岩高程。
- 源栅格高程单位为米，垂直参考为 EGM2008；输出闭合全球椭球面积为约 510,065,621.724 km²，完整覆盖校验没有缺测像元。像元计数指与分区有面积相交的源像元数，边界像元可能同时出现在相邻格的计数中；面积和高程汇总按交叠面积分配。
- 结果是全球 relief 模型的分区尺度概况，不是实地测量、工程或导航数据；狭窄地形仍受源格分辨率与 ETOPO 产品本身精度限制。应保留 NOAA/NCEI 与 DOI 引用。NOAA 公开数据按 CC0 使用。

## GeoNames

- 官方下载与数据条款：[GeoNames Export](https://www.geonames.org/export/)；全球转储：[GeoNames dump directory](https://download.geonames.org/export/dump/)；字段说明在转储目录中的 `readme.txt`。
- 本版快照：2026-10-12 的 `allCountries` 与 `alternateNamesV2`。提供主要聚落、河流、水域、地形点、历史地点，以及有来源记录的多语言名称和带年代别名。
- GeoNames 条款为 CC BY（需署名 GeoNames）；其数据按原样提供，准确性、时效性和完整性不作保证。应用会在各格档案显示来源类别；本文件保留完整署名信息。

## Natural Earth 10m

- [Physical Vectors](https://www.naturalearthdata.com/downloads/10m-physical-vectors/)：河流／湖泊中心线、湖泊、水文与地理区域、山峰/高程点等主题。
- [Cultural Vectors](https://www.naturalearthdata.com/downloads/10m-cultural-vectors/)：国家边界和聚居地点。
- [使用条款](https://www.naturalearthdata.com/about/terms-of-use/)：Natural Earth 数据为 public domain，可自由使用。各图层更新周期不同，本包涉及的 10m 图层为 5.0.x–5.1.x。
- Natural Earth 是概化制图数据，适合全球概览，不等同精细水系或测绘边界。格网通过点样本/线段抽样/面内点测试匹配，细窄地物可能漏记。

## RESOLVE Ecoregions 2017

- 官方项目页与下载：[Ecoregions 2017](https://ecoregions.world/)；全球 Shapefile 压缩包约 150 MB，许可为 [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)。
- 学术来源：Dinerstein et al. (2017), “An Ecoregion-Based Approach to Protecting Half the Terrestrial Realm,” *BioScience* 67(6), 534–545, [DOI 10.1093/biosci/bix014](https://doi.org/10.1093/biosci/bix014)。
- 图层包含 846 个陆地生态区及其生物群系分类。每个 2.57143°格用内部 3×3 样点归属，列出命中最多的类别并显示样点数；样点覆盖比例不是精确陆地面积占比。
- 这是 2017 年生态地理分类，不是当前/历史土地覆盖栅格，也不等同当地实测植被、土壤或保护现状。小岛和狭窄生态区可能没有被 9 个样点命中。

## 柯本—盖格气候分类（1991–2020）

- 数据文件：[Beck et al. Köppen–Geiger map series on Figshare](https://figshare.com/articles/dataset/koppen_geiger_tif_zip/21937571)，其中使用历史期 `1991_2020/koppen_geiger_0p1.tif`（0.1°，约 10 km）；该数据文件许可为 [CC0](https://creativecommons.org/publicdomain/zero/1.0/)。
- 学术来源：Beck et al. (2023), “High-resolution (1 km) Köppen-Geiger maps for 1901–2099 based on constrained CMIP6 projections,” *Scientific Data* 10, 724, [DOI 10.1038/s41597-023-02549-6](https://doi.org/10.1038/s41597-023-02549-6)。此应用使用其历史 1991–2020 分类图层，不使用未来情景。
- 每个地图格按内部 3×3 样点统计气候类型，显示命中最多的类别和样点数。样点数不是面积百分比，边界格可能列出多个类别；抽样以全球格网中心为口径，不等同全分辨率逐格统计。
- 柯本—盖格分类是长期气候分区，不是实时天气、逐月预报或土壤/植被观测。海洋无陆地分类像元时不表示海洋没有气候。

## ESA CCI / Copernicus C3S 全球土地覆盖（2020）

- 官方产品：[Copernicus Climate Data Store 土地覆盖分类图](https://cds.climate.copernicus.eu/datasets/satellite-land-cover?tab=overview)；产品指南：[C3S v2.1.1 Product User Guide](https://dast.copernicus-climate.eu/documents/satellite-land-cover/WP2-FDDP-LC-2021-2022-SENTINEL3-300m-v2.1.1_PUGS_v1.1_final.pdf)；DOI：[10.24381/cds.006f2c9a](https://doi.org/10.24381/cds.006f2c9a)。
- 源数据为全球年度分类图；本包使用 2020 年 v2.1.1、WGS 84 / Plate Carrée、300 m 产品，含 22 个 FAO LCCS 类别。该分类是遥感产品，不是现场逐地调查；源产品还提供分类质量标志，但当前轻量摘要没有把质量标志映射到分区统计。
- 可重复构建脚本从 [Microsoft Planetary Computer 的 esa-cci-lc STAC 集合](https://planetarycomputer.microsoft.com/api/stac/v1/collections/esa-cci-lc)读取瓦片与官方类别色表，并请求每个 45°瓦片的 512 × 512 最近邻分类预览。预览在赤道处约 9.8 km/像元。通过像元中心坐标归入当前 140 × 70 网格，用 cos(纬度) 加权汇总；每格只显示占比最高的五个类别，比例分母按该格全部有效分类统计，因此前五项之和可小于 100%。海洋水体类作为有效类别保留。
- `data/land-cover-2020.json` 约 5.6 MB，是上述预览的分区摘要而非原始 300 m 栅格；平均每格约 856 个预览像元。schema v2 附带 22 类官方色表，并为每格记录加权占比最高的主类别码与颜色，供半透明分区概览和完整图例使用。该主类仍只是约 2.57°格内的概览，不是逐像元类别边界。显示口径明确标注源分类分辨率、实际预览采样尺度和近似面积权重，不得将结果描述成逐像元 300 m 分区统计。地图程序运行时完全离线；构建数据需要联网。
- 分发和再利用应遵循产品页列出的 ESA CCI、CC-BY 与 VITO 许可条件，并保留 ESA CCI / Copernicus C3S 数据来源署名。此文件只含类别汇总、没有原始栅格数据。

## 内容解释

“历史”字段表示来源数据中定位到的历史聚落、历史政区／地点，以及 GeoNames 中标为 historic 或带 from/to 年代的备用名称；它不是百科式历史叙述。曾用名保留语言和起讫年代（若源数据有记录）。每个分区显示的是受条目上限筛选后的名称，不表示该分区所有地名的穷尽列表。

## WorldPop 全球人口分布（2023）

- 官方数据页：[WorldPop 2023 全球人口分布](https://hub.worldpop.org/geodata/summary?id=80029)；官方全球 1 km 文件：[GeoTIFF](https://data.worldpop.org/GIS/Population/Global_2015_2030/R2025A/2023/0_Mosaicked/v1/1km_ua/constrained/global_pop_2023_CN_1km_R2025A_UA_v1.tif)；数据集 DOI：10.5258/SOTON/WP00845。
- 数据表示 2023 年每个约 30″（赤道附近约 1 km）像元的人口模型估值，WGS 84。Global2 R2025A v1 是 alpha 版本，各国估算精度不同；不是人口普查，也不是精确到个人或建筑物的记录。WorldPop 页面注明 CC BY 4.0，随包保留来源署名。
- `data/population-2023.json` 由 1 km 源栅格按像元中心汇总到 9,800 个 Z9800 格网；人口密度以对应格的 NOAA ETOPO 2022 陆地面积为分母。跨格像元不做面积拆分。源范围约为 60°S 至 84°N；超出范围的分区不提供人口值，边缘格记录覆盖比例。
- 汇总总量约 8,056,334,738 人。作为口径合理性检查，联合国《World Population Prospects 2024》给出的 2023 年 7 月世界人口估计为 8,091.7 百万；两者估算口径不同，本应用未把栅格重新校准到联合国总量。
- 原始 275.86 MiB GeoTIFF 作为可选重建输入单独提供；运行程序只需约 1.4 MiB 的分区摘要，不依赖源栅格或联网。输入和校验值见 SUPPLEMENTAL-DATA.md，聚合参考代码见仓库 tools/population-aggregation.cpp。

## 历史事件库（1.27.0）

- 主文件 `data/world-events-v2.tsv` 含346条事件、19个整理后的类别，覆盖前2560年至2024年；182条为去重后新增条目。旧六列 `world-events.tsv` 仅作兼容备份。摘要用于快速了解背景，不是完整史学论证。
- 新表保留中文标题、地点、项目编写的中文摘要、英文阅读字段、年份/年段、来源链接及年代说明。120条具有Wikidata提供的地球参考坐标。人物传记坐标、月球或火星表面坐标不会被当作地球事件位置。参考点不表示精确事件范围。
- 公开Wikidata快照提供CC0标签、说明、时间与坐标，并保留逐条QID链接。个人即位、著作出版、任务着陆等里程碑不强行套用实体的创建或出生年份；源年与里程碑年不一致的条目记录于源码 `qa/development/event-source-audit.json`。该核对不是对所有史学争论的穷尽验证。
- 校正旧表东汉建立的公元1年为公元25年，参考[哥伦比亚大学中国历史年表](https://afe.easia.columbia.edu/timelines/china_timeline.htm)；爱奥尼亚起义调整到通行的前499年。撒马利亚征服采用约年并说明史料分歧，参考[宾夕法尼亚大学ORACC材料](https://oracc2.museum.upenn.edu/saao/aebp/essentials/countries/israel/index.html)。天工开物、加纳独立及巴拿马运河交接等条目提供图书馆、档案或管理机构资料。
- 独立检索可搜索中英文标题、地点与摘要，按类别及包含式年份区间筛选。负数表示公元前，无公元0年。可查看事件年份、定位有坐标的条目，或由用户点击打开来源。所有筛选和地图查询均可离线使用。

## 历史路线（1.27.0）

- 42条路线保留原来的选择、逐步网格着色、播放、暂停、隐藏与定位功能。12条新增长途行程包含达尔文、库克、德雷克、塔斯曼、伊本·白图泰、玄奘、马可·波罗、阿蒙森、沙克尔顿、哥伦布、迪亚士与白令。
- 贝格尔号相关行程依据[Darwin Online整理的1740天行程](https://darwin-online.org.uk/converted/Ancillary/Rookmaaker_Beagle_Itinerary_A575.html)，提取596个不同位置，其中407点带FitzRoy海上日志标记，其他点为整理者定位的陆上/停泊参考点。含达尔文陆上活动，不是全程同船的船迹。334个日期缺少完整坐标或被排除，其中4条因明显经度异常排除；13处较长资料间隔断开。日期变更线导致的日历差异保留来源日期，不推算缺失位置。
- 其余路线以记载地点和明示的路径辅助点连接。修正旧航线在巴西、非洲南端、婆罗洲等处的明显大陆捷径；未复原的巨港河道断开。现代陆地掩膜检查是发现粗大穿陆连接的诊断，不能证明精确历史航迹。古道路、港口、水道与海岸可能已改变。
- 播放按参考点之间的距离推进，避免密集点与稀疏点出现不同的视觉速度；不模拟真实行进时长。说明中的连线长度不是实际走行或航行里程。辅助点不列作历史停靠地，争议点与资料间断单独说明。
- 逐条地点及来源见 `路线地点与资料清单.md`；编译目录的完整坐标参照导出为 `data/route-catalog-reference.json`，供审阅追溯。运行中的路线来自程序内置目录，直接编辑参照JSON不会改变程序。

## 离线地点搜索（1.27.0）

34248个城市采用GeoNames来源参考坐标，258个国家/地区采用Natural Earth地图标注点。快照日期2026-11-20。运行表为data/place-search.tsv，SHA-256为`e86596396509312c760ca0c13e8d845da155023cb427f7fe40eed7b3677ea61e`。城市与国家参考点不改变周边基础分区统计的含义。


本地快照日期：2026-11-20。运行文件为 data/place-search.tsv，来源、计数及校验值见 data/place-search-source.json。

34248 个城市来自 [GeoNames cities15000](https://download.geonames.org/export/dump/)，使用 [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)；归属 GeoNames 及其贡献者。字段、坐标与署名依据 [GeoNames 数据说明](https://download.geonames.org/export/dump/readme.txt)。同时使用 countryInfo.txt 和 admin1CodesASCII.txt 提供国家与一级行政区上下文。

258 个国家/地区参考点来自 [Natural Earth Admin 0 5.1.1](https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-admin-0-countries/)，公有领域。它们是地图标注点，不是首都或全国范围的中心测量。

本项目把来源字段整理为离线搜索表，复用既有 GeoNames 双语名称和来源别名，补充少量常用国家中文简称，并在匹配时规范化大小写、繁简体及重音。城市源坐标保留；人口字段只用于同名结果排序，不作为当前城市人口显示。来源未提供中文名称的条目显示英文。

检索得到一个来源参考点，不会把周边基础分区统计变成城市统计。旧分区资料中的河流、地貌和遗产名称没有新增未经核实的精确坐标。
