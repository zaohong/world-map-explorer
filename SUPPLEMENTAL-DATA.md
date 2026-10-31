# 1.23.0 补充原始数据

以下原始数据文件作为 [v1.23.0 Release 附件](https://github.com/zaohong/world-map-explorer/releases/tag/v1.23.0) 提供，未放入普通 Git 历史。它们便于重建源数据或核对输入；**日常运行程序不需要单独下载**，完整运行所需文件已包含在 `WorldMapExplorer-1.23.0.zip` 内。

下载附件后，如需恢复本地完整项目目录，请按下表放回对应相对路径：

| Release 附件名 | 放回路径 | 大小 | SHA-256 | 来源/许可 |
| --- | --- | ---: | --- | --- |
| `NE1_HR_LC_SR_W_DR.zip` | `当前补丁/迭代工作/必要数据源/Natural-Earth-High-Resolution/NE1_HR_LC_SR_W_DR.zip` | 324,320,812 字节 | `8841D08155415F0556462846DCB845A4CCB8F12390B0E0BA36FFDE55EFADEB56` | Natural Earth，公有领域 |
| `global_pop_2023_CN_1km_R2025A_UA_v1.tif` | `当前补丁/迭代工作/必要数据源/WorldPop-2023/global_pop_2023_CN_1km_R2025A_UA_v1.tif` | 289,259,387 字节 | `6F3D1B80B7D289C2C5E9EC8A72F1A8ED34FF7463B9CFEAD8CBA6E92E72B529E8` | WorldPop，CC BY 4.0，需保留署名 |

WorldPop 来源、估算方法、时间范围及数据限制见 [`WorldPop-2023-来源说明.md`](./当前补丁/迭代工作/必要数据源/WorldPop-2023/WorldPop-2023-来源说明.md)。下载后可使用 PowerShell 的 `Get-FileHash -Algorithm SHA256 <文件路径>` 核对文件完整性。
