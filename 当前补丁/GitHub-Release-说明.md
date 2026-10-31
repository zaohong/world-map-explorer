# 世界地图分区档案 1.23.0

完整 Windows 10/11 64 位离线运行包。下载附件 `WorldMapExplorer-1.23.0.zip`，解压后启动 `WorldMapExplorer-1.23.0/WorldMapExplorer.exe`。请保留包内全部 assets、data、licenses 与数据来源文件。

## 更新内容

- 地图底图、专题图层、网格线和透明度使用分层缓存；拖动与滚轮缩放先给出缓存预览，再根据原始高清素材重绘。
- 历史事件列表缓存年份筛选与文字布局，只绘制当前视窗中的卡片。
- 保留四档格网、离线历史疆域与事件、世界2026、人口、土地覆盖、高清底图、全屏和查询功能。

## 检查

真实窗口交互 794 项、历史模块 4,250 项、格网逻辑 270 项检查通过；当前包含164条历史事件、45个类别。Natural Earth 与 NASA 全球底图及瓦片原始素材均保留。

发行包 SHA-256：`476C02639BEADC6D1F3A42915A8894BE36CCAE478D16283BB48808BE712B0979`

## 补充原始数据

Release 同时附带 Natural Earth 高清底图原始包 `NE1_HR_LC_SR_W_DR.zip` 和 WorldPop 2023 原始栅格 `global_pop_2023_CN_1km_R2025A_UA_v1.tif`。程序日常运行不依赖它们；若要复现数据处理，请按仓库根目录 `SUPPLEMENTAL-DATA.md` 中的路径、许可和 SHA-256 说明恢复。
