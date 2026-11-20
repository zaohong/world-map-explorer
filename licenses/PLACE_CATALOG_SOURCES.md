# 离线地点目录来源

本地快照日期：2026-11-20。运行文件为 data/place-search.tsv，来源、计数及校验值见 data/place-search-source.json。

34248 个城市来自 [GeoNames cities15000](https://download.geonames.org/export/dump/)，使用 [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)；归属 GeoNames 及其贡献者。字段、坐标与署名依据 [GeoNames 数据说明](https://download.geonames.org/export/dump/readme.txt)。同时使用 countryInfo.txt 和 admin1CodesASCII.txt 提供国家与一级行政区上下文。

258 个国家/地区参考点来自 [Natural Earth Admin 0 5.1.1](https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-admin-0-countries/)，公有领域。它们是地图标注点，不是首都或全国范围的中心测量。

本项目把来源字段整理为离线搜索表，复用既有 GeoNames 双语名称和来源别名，补充少量常用国家中文简称，并在匹配时规范化大小写、繁简体及重音。城市源坐标保留；人口字段只用于同名结果排序，不作为当前城市人口显示。来源未提供中文名称的条目显示英文。

检索得到一个来源参考点，不会把周边基础分区统计变成城市统计。旧分区资料中的河流、地貌和遗产名称没有新增未经核实的精确坐标。
