# 重建输入 / Reconstruction inputs

正常使用只需下载 v1.28.0 的完整运行包。以下输入用于核对或重建资料，**不需要额外安装到程序中**。

## 当前来源快照

[v1.28.0 Release](https://github.com/zaohong/world-map-explorer/releases/tag/v1.28.0) 的 `WorldMapExplorer-1.28.0-source-data.zip` 保存与本版匹配的来源快照，包括 Cliopatria、GeoNames、Natural Earth 国界、事件与“小猎犬号”资料。解压后，将其中的 `source-data` 文件夹放到仓库的 `当前补丁/迭代工作/1.28.0/` 下。

快照的来源、修订、署名和许可随文件保留。下载校验值见同一 Release 的 `SHA256SUMS.txt`。

## 可复用的大型原始输入

下列文件与此前公开版本相同，继续使用原 Release 的附件，避免重复上传。

| 输入 | 下载 | 字节数 | SHA-256 |
| --- | --- | ---: | --- |
| Natural Earth 高清地形原始包 | [NE1_HR_LC_SR_W_DR.zip](https://github.com/zaohong/world-map-explorer/releases/download/v1.23.0/NE1_HR_LC_SR_W_DR.zip) | 324,320,812 | `8841D08155415F0556462846DCB845A4CCB8F12390B0E0BA36FFDE55EFADEB56` |
| WorldPop 2023 原始栅格 | [global_pop_2023_CN_1km_R2025A_UA_v1.tif](https://github.com/zaohong/world-map-explorer/releases/download/v1.23.0/global_pop_2023_CN_1km_R2025A_UA_v1.tif) | 289,259,387 | `6F3D1B80B7D289C2C5E9EC8A72F1A8ED34FF7463B9CFEAD8CBA6E92E72B529E8` |

Natural Earth 为公有领域；WorldPop 使用 CC BY 4.0，保留来源署名。WorldPop 分区聚合参考程序保存在 `tools/population-aggregation.cpp`。原始栅格不会提高既有区域摘要的统计分辨率；方法及年份口径见 [DATA_SOURCES.md](docs/DATA_SOURCES.md)。

The complete runtime already contains all data needed for everyday use. The current source-data snapshot is optional and extracts into the versioned Windows source directory. Large unchanged Natural Earth and WorldPop inputs remain available from the earlier release; their hashes above identify the exact files.
