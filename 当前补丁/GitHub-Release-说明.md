# 世界地图探索器 1.23.0

世界地图探索器是一款 Windows 10/11 64 位离线地图工具，可浏览世界底图、使用多级经纬网格定位，并查看历史疆域、历史事件和多类地理专题资料。

## 下载与启动

下载附件 `WorldMapExplorer-1.23.0.zip`，完整解压后启动 `WorldMapExplorer-1.23.0/WorldMapExplorer.exe`。请保留压缩包内的 `assets`、`data`、`licenses` 和数据来源文件。地图与日常运行所需数据均随包提供，无需联网。

发行包 SHA-256：`476C02639BEADC6D1F3A42915A8894BE36CCAE478D16283BB48808BE712B0979`

## 功能概览

- Natural Earth 标准及高清底图、NASA Blue Marble 底图。
- 9,800、39,200、156,800、627,200 格四档经纬网格；可按编号、坐标、地点或土地覆盖类别搜索。
- 601 个历史时间点，覆盖公元前 3400 年至公元 2024 年；事件面板收录 164 条事件、45 个类别。
- 历史政权、2026 年国家／地区分类、2023 年人口估算和 2020 年土地覆盖图层。
- 地图缩放、平移、全屏、网格显示、图层透明度调节和历史时间播放。

## 数据使用说明

网格按经纬度划分，不是等面积单元；实际面积随纬度变化。更细网格提升定位精度，但区域信息中的地形、地点和人口等摘要主要以 9,800 格基础网格统计。人口图层使用 WorldPop 模型估算，不是人口普查。历史疆域资料覆盖不均；空白表示来源中没有匹配记录，不代表无人居住或主权／法律边界结论。各图层的来源、统计口径和许可见包内 `DATA_SOURCES.md` 与 `licenses/`。

## 附带素材

Release 同时提供 Natural Earth 高清底图原始包 `NE1_HR_LC_SR_W_DR.zip` 和 WorldPop 2023 原始栅格 `global_pop_2023_CN_1km_R2025A_UA_v1.tif`，用于重建输入或核对数据。它们不是日常运行的必需文件。恢复路径、许可和 SHA-256 见仓库根目录 [`SUPPLEMENTAL-DATA.md`](https://github.com/zaohong/world-map-explorer/blob/main/SUPPLEMENTAL-DATA.md)。

## 检查记录

1.23.0 归档的真实窗口交互检查 794 项通过，历史疆域抽样 4,250 项通过，网格与坐标逻辑检查 270 项通过；完整记录和界面截图见仓库 [`1.23.0/qa`](https://github.com/zaohong/world-map-explorer/tree/main/%E5%BD%93%E5%89%8D%E8%A1%A5%E4%B8%81/%E8%BF%AD%E4%BB%A3%E5%B7%A5%E4%BD%9C/1.23.0/qa)。
