# WorldPop 2023 原始数据与汇总源码

此处保留重建人口分区摘要所需的官方原始 GeoTIFF 和汇总程序源码。项目运行时直接读取当前运行版 data/population-2023.json，不依赖此原始栅格。

来源、许可、分辨率、汇总口径及限制：

## WorldPop 全球人口分布（2023）

- 官方数据页：[WorldPop 2023 全球人口分布](https://hub.worldpop.org/geodata/summary?id=80029)；官方全球 1 km 文件：[GeoTIFF](https://data.worldpop.org/GIS/Population/Global_2015_2030/R2025A/2023/0_Mosaicked/v1/1km_ua/constrained/global_pop_2023_CN_1km_R2025A_UA_v1.tif)；数据集 DOI：10.5258/SOTON/WP00845。
- 数据表示 2023 年每个约 30″（赤道附近约 1 km）像元的人口模型估值，WGS 84。Global2 R2025A v1 是 alpha 版本，各国估算精度不同；不是人口普查，也不是精确到个人或建筑物的记录。WorldPop 页面注明 CC BY 4.0，随包保留来源署名。
- `data/population-2023.json` 由 1 km 源栅格按像元中心汇总到 9,800 个 Z9800 格网；人口密度以对应格的 NOAA ETOPO 2022 陆地面积为分母。跨格像元不做面积拆分。源范围约为 60°S 至 84°N；超出范围的分区不提供人口值，边缘格记录覆盖比例。
- 汇总总量约 8,056,334,738 人。作为口径合理性检查，联合国《World Population Prospects 2024》给出的 2023 年 7 月世界人口估计为 8,091.7 百万；两者估算口径不同，本应用未把栅格重新校准到联合国总量。
- 原始 275.86 MiB GeoTIFF 通过仓库 v1.23.0 Release 的独立附件分发；恢复路径为 `当前补丁/迭代工作/必要数据源/WorldPop-2023/`。运行程序使用包内已生成的分区摘要，不依赖源栅格或联网。`population-aggregation.cpp` 可读取 BigTIFF/LZW 并生成摘要。
