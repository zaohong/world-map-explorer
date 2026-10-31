# 世界地图分区档案｜交接汇总

盘点日期：2026-10-28（Asia/Shanghai）  
用途：后续维护者/自动化任务接手时，快速确认当前可运行版本、源代码、数据、构建产物、历史版本和已知风险。

## 一、当前状态（先读）

- 当前桌面可运行版本：应用 **1.9.0**；主数据档案 **0.9.1**；网格版本 **Z9800**（WGS 84 / EPSG:4326，140 列 × 70 行，共 9,800 格）。
- 桌面安装目录仍叫 `世界地图分区档案-v1.8.0`，这是为了保留已有路径/快捷方式；目录名旧，目录内程序与 `update-manifest.json` 才是当前 1.9.0。
- 当前桌面主 EXE：`WorldMapExplorer.exe`，893,440 字节，SHA-256 `e21b773313376299a57971ef61b7165eb8b8dbf7b4c8a04b2683dc117c75ba9d`。
- 1.9.0 新增 UNESCO 世界遗产离线模块 `data/world-heritage-2026.json`，3,481,912 字节，SHA-256 `93dabe238d2d6e2ff2a66fb18ffa87b37d8f8d2b8beefa6ac66bce362f91e661`；模块可独立更新，不替换地图、主档案或其他专题数据。
- 1.9.0 补丁压缩包：`[本机历史路径已省略]`，854,865 字节，SHA-256 `700fa54cba4aeb8cd7ad49fef6554d11cf9728d900c3dec66594a14eda524e5f`。包内有 EXE、UNESCO JSON、manifest 和安装说明。
- 已验证：官方坐标映射/格网测试、C++档案索引和中文搜索回归、错误格网版本拒收、实际安装文件哈希，以及桌面 EXE 隐藏启动烟测均通过。
- 先前对话提出“每天14:15继续迭代”；本地项目文件无法证明该计划当前仍由自动化任务承载。若交接后需要按时续做，请在 Codex 自动化/任务面板核实计划仍有效，不要只依赖本说明。

## 二、关键路径

| 内容 | 当前路径 | 说明 |
| --- | --- | --- |
| 工作区根目录 | `[本机历史路径已省略]` | 交接文档和 `outputs` 所在处 |
| 应用源项目 | `[本机历史路径已省略]` | C++源码、数据工具、测试和部分候选数据 |
| 当前桌面应用 | `[本机历史路径已省略]` | 目录名虽标 1.8.0，实装程序为 1.9.0；更新时保持原目录，避免破坏快捷方式 |
| 当前桌面主程序 | `[本机历史路径已省略]` | 当前 1.9.0，约 0.89 MB |
| 当前桌面数据 | `[本机历史路径已省略]` | zones、土地覆盖、UNESCO 模块及两个历史档案备份 |
| 当前桌面地图/细节层 | `[本机历史路径已省略]` | Natural Earth底图、NASA全球底图及1 km/500 m瓦片 |
| 当前补丁目录 | `[本机历史路径已省略]` | 可展开检查的补丁文件 |
| 历史发行包 | `[本机历史路径已省略]` | 各版本 ZIP、应用补丁、数据补丁与校验清单 |
| 原始/下载数据 | `[本机历史路径已省略]` | 下载源数据目录；并非所有可重建输入都还在 |

**注意：工作区当前不是 Git 仓库**（项目目录没有 `.git`）。接手者不要假设有可用提交历史；做大改动前先自行建立版本控制或另存快照。

## 三、版本工作概要

详细逐项记录以项目根目录 `CHANGELOG.md` 为准。主程序版本与档案内容版本独立递增。

| 版本 | 已完成内容 |
| --- | --- |
| 0.2.1 | 修正分区 JSON 字段查找边界，防止读到其他对象内容。 |
| 0.3.0 | 增加分区编号、坐标与地名搜索。 |
| 0.4.0 | 增加 RESOLVE 生态区/生物群系离线层和覆盖口径说明。 |
| 0.5.0 | 增加 1991–2020 柯本—盖格气候分类摘要。 |
| 0.6.0 | 增加缩放、拖动平移、双击复位与坐标变换回归。 |
| 0.7.0 | 增加按需加载的高清地形阴影图层；此期旧底图融合仍有配准局限。 |
| 0.8.0 | 网格升级为 140×70、9,800 格；重建新格数据并保留旧编号迁移。 |
| 0.9.0 | 默认/高清底图加入 `.jgw`、`.prj`；校验 WGS 84 全球边界，避免底图与网格静默错位。 |
| 1.0.0 | 增加 2020 土地覆盖离线摘要。 |
| 1.0.1 | 支持搜索土地覆盖类别。 |
| 1.1.0 | 增加 22 类土地覆盖着色、图例和独立色表模块。 |
| 1.1.1 | 增加鼠标悬停实时经纬度/分区号。 |
| 1.2.0 | 增加可选 NASA Blue Marble 全球合成底图（约 2 km/像元）。 |
| 1.3.0 | 增加 NASA 1 km 局部瓦片，按视口懒加载。 |
| 1.4.0 | 增加 NASA 原始 500 m 瓦片，按放大级别切换并限制缓存。 |
| 1.5.0 | 增加 WGS 84 椭球面网格单元面积；验证全球面积总和。 |
| 1.6.0 | 多结果搜索可循环定位，自动放大/居中目标格。 |
| 1.7.0 | 明确区分格中心、地图点击坐标与输入坐标的来源。 |
| 1.8.0 | 点击/坐标搜索的精确位置增加地图靶心；随平移缩放同步。 |
| 内容 0.9.0 | `zones.json` 使用 NOAA ETOPO 2022 15″统计陆地高程、海深及陆海比例。 |
| 内容 0.9.1 | 增加每格陆地和海洋面积（km²）；主程序和网格不变。 |
| 1.9.0 | 增加 UNESCO 世界遗产名称检索和分区档案独立模块；按每个官方组成部分坐标归格。 |

## 四、当前运行程序与数据

### 桌面安装内容

- `WorldMapExplorer.exe`：1.9.0 原生 Windows/GDI+程序，静态链接，893,440 字节。
- `data/zones.json`：9,800格档案，内容版本 0.9.1，36,538,164 字节。
- `data/land-cover-2020.json`：2020土地覆盖模块，约 5.60 MB。
- `data/world-heritage-2026.json`：UNESCO独立模块，约 3.48 MB；数据集 `whc001`，2026-10-15 数据快照，1,273条记录；1,271条有组成点坐标，6,397点，映射到846格，2条无坐标记录明确不推断位置。
- `assets/world-map.jpg`：Natural Earth彩色底图，4,050×2,025；`world-map-hd.jpg`：8,100×4,050；两者有世界文件/投影 sidecar。
- `assets/world-map-nasa.jpg`：NASA January 2004全球合成底图，21,600×10,800。
- `assets/nasa-1km`：648张10°瓦片及 sidecar/清单，约103.01 MiB。
- `assets/nasa-500m`：648张10°瓦片及 sidecar/清单，约445.89 MiB。
- 整个桌面 `assets` 目录盘点为3,903个文件、613,324,582字节；`data`目录含当前文件和旧档案备份，共109,144,778字节。

### 必须区分的“当前版”和“旧副本”

| 文件 | 版本/大小 | 接手时的处理 |
| --- | --- | --- |
| 桌面 `data/zones.json` | 内容0.9.1，36,538,164字节 | 当前用户运行数据；SHA-256 `26121ad065bc38e2fde1a0611ee344f00ad0553b110242a5f8033bd624750846` |
| 项目 `work/zones-etopo-15s.json` | 内容0.9.1，36,538,164字节 | 与桌面 `zones.json` SHA-256相同，可作为0.9.1档案候选/备份 |
| 项目 `data/zones.json` | 内容0.6.0，28,115,426字节 | **旧文件，不是当前桌面档案**；若打完整包或重建主数据，必须先处理这个版本差异 |
| 项目根 `WorldMapExplorer.exe` | 1,647,104字节 | 与当前候选主程序不同，不要用它作为最新发行版 |
| 项目 `work/WorldMapExplorer-v1.9.0.exe` | 893,440字节 | 当前构建产物；与桌面安装 EXE 的 SHA-256相同 |

桌面更新保留了回滚文件：`WorldMapExplorer-v1.8.0.exe.bak`（旧EXE）以及 `data/zones-before-0.9.0.json.bak`、`data/zones-before-0.9.1.json.bak`。桌面文档/manifest在 `.before-v1.9.0.bak` 后缀下也有备份。

## 五、已下载且可供后续使用的数据

本节区分本机仍保留的文件、已生成的派生产品和尚未留存的原始输入。具体引用/署名/精度限制另见 `DATA_SOURCES.md`、`MAP_CALIBRATION.md`。

| 本机路径/文件 | 状态与可能用途 | 重要限制/许可 |
| --- | --- | --- |
| `work/source-data/NaturalEarth_NE1_10m/NE1_LR_LC_SR_W_DR.tif`（393,768,290字节）及 `.prj/.tfw/README/VERSION` | Natural Earth 10m原始全球彩色地形栅格；已用于现有默认/高清底图的构建基础。另保留根目录 `NE1_LR_LC_SR_W_DR.zip`（192,948,748字节）和 `NaturalEarth_10m_SR_LR.zip`（25,179,752字节）。 | Natural Earth数据为 public domain；全球比例尺概化图，不是卫星影像/DEM。 |
| `work/source-data/NASA_Blue_Marble_2004_January/world.topo.bathy.200401.3x21600x10800.jpg`（29,698,041字节） | NASA 2 km/像元全球合成图；已安装为可选NASA底图。 | January 2004合成，不是实时卫星；海洋纹理不是逐点海深。使用时署名 NASA Earth Observatory。 |
| `work/source-data/NASA_Blue_Marble_2004_January/world_500m/`（8个A1–D2分幅，合计约432.5 MB） | NASA原始约500 m全球合成分幅；已派生桌面 `assets/nasa-1km` 与 `assets/nasa-500m`。 | 同为2004年月度合成；保留原分幅，不要当作当前影像。 |
| `work/source-data/ETOPO_2022_v1_15s_N45E075_surface.nc`（32,417,849字节） | 本机留存的ETOPO 15″样例瓦片。 | 不是完整288瓦片原始集。 |
| `work/etopo15s-state.json` 与 `work/etopo15s-state-0288.bin` | 当前checkpoint记录处理288/288个15°瓦片、3,732,480,000个有效像元、0缺测；产物在0.9.1档案中。 | `work/source-data/etopo15s-current` 盘点为空；完整原始15″瓦片当前未在该目录留存，若要从源头重建/复核需重新下载。 |
| `work/source-data/ETOPO_2022_v1_60s_N90W180_surface.nc`（478,290,125字节） | 本机存在的60″ NetCDF文件，可供低成本地形处理/对照研究。 | 当前没有证明它已进入桌面0.9.1档案；使用前先检查文件内部维度、地理覆盖和垂直基准，不要仅凭文件名推断覆盖。 |
| `work/source-data/unesco-world-heritage-whc001.json`（24,695,334字节） | UNESCO DataHub原始导出；可重新生成独立模块。 | 数据集标注CC BY-SA 4.0，归属UNESCO；官网：[whc001](https://data.unesco.org/explore/dataset/whc001/)。派生模块路径 `data/world-heritage-2026.json`。 |

### 目前未随项目保存、重建时可能需要重新取得的输入

- GeoNames `allCountries`、`alternateNamesV2` 原始转储；当前档案中已有其派生城市、水系、地形点和曾用名文本，但原始文件不在现有 `work/source-data` 清单内。
- RESOLVE Ecoregions 2017原始边界、Beck Köppen–Geiger 1991–2020原始栅格、ESA/Copernicus 2020分类预览/源瓦片，以及 Natural Earth河流/湖泊/矢量专题输入；相关来源和构建方法写在 `DATA_SOURCES.md` 与 `build-world-database.js`，但本地原始下载未盘点到。
- ETOPO 15″全套288源瓦片：checkpoint和派生档案在，源瓦片目录当前为空；将来若改聚合方法，应先恢复可验证的原始输入。

## 六、项目源码、构建工具与测试

- 原生程序：`native/world_map_explorer.cpp`；回归测试：`native/test_archive_lookup.cpp`。
- 核心数据重建：`build-world-database.js`；地理栅格/地图和专题数据脚本位于 `tools/`。
- UNESCO构建器/测试：`tools/build-unesco-heritage.mjs`、`tools/test-unesco-heritage.mjs`。
- 默认Windows编译器：`work/toolchains/llvm-mingw-20261020-msvcrt-x86_64-extract/bin/x86_64-w64-mingw32-g++.exe`（clang 23.1.4）；Node当前盘点为 v24.22.1，`h5wasm` 0.10.3在`node_modules`中。
- 主程序构建（在项目根目录 PowerShell执行）：

```powershell
$compiler = '.\work\toolchains\llvm-mingw-20261020-msvcrt-x86_64-extract\bin\x86_64-w64-mingw32-g++.exe'
& $compiler -std=c++11 -O2 -s -mwindows -municode -static-libgcc -static-libstdc++ native\world_map_explorer.cpp -o work\WorldMapExplorer-v1.9.0.exe -lgdiplus -lgdi32 -luser32
```

- UNESCO数据重建/单元校验：

```powershell
node tools\build-unesco-heritage.mjs work\source-data\unesco-world-heritage-whc001.json data\world-heritage-2026.json
node tools\test-unesco-heritage.mjs work\source-data\unesco-world-heritage-whc001.json data\world-heritage-2026.json
```

- Native回归测试EXE已留在项目根：`test_archive_lookup-v1.9.0-unesco.exe`。运行时需要传入档案、默认/高清世界文件、土地覆盖、NASA世界文件和UNESCO模块这6个数据路径；本次实际桌面数据回归已通过。
- 本次验证记录：官方导出中1,273条/6,397个组成点均完成坐标一致性核验；生成模块中9,800个格键唯一，诺曼底登陆海滩样例位于 `Z9800-015-069`；中文名称搜索可定位；错误 `gridVersion` 被拒绝；桌面程序启动烟测通过。

## 七、补丁与回滚方法

- 当前补丁包目录 `[本机历史路径已省略]`；压缩包在同一 `outputs` 目录。
- 1.9.0 manifest要求应用基线1.8.0，更新主EXE并加入 `data/world-heritage-2026.json`。地图、`zones.json`、土地覆盖、NASA数据均不替换。手动更新，没有联网更新器、数字签名或自动回滚。
- 仅刷新UNESCO名录且字段/schema不变时，重建并校验 JSON 即可单独替换数据文件；若改变UI、搜索逻辑、schema读取或校验，则需重编EXE并同步升级manifest。
- 替换任何现存文件前先关闭程序、备份目标、比对SHA-256；桌面当前旧EXE回滚副本为 `WorldMapExplorer-v1.8.0.exe.bak`。
- 早期完整 ZIP、应用补丁和数据补丁均在工作区根目录 `[本机历史路径已省略]`，不要将不同版本的 `WorldMapExplorer.exe`、`zones.json` 或地图sidecar混装。

## 八、后续接手优先事项

1. **先修正源项目与桌面版数据不同步**：项目根 `data/zones.json` 仍是0.6.0，而候选与桌面档案已经是0.9.1且哈希相同。经备份和验证后，应明确哪份成为规范源，再更新清单和可重复构建/打包流程；当前桌面安装是可运行的，但不能直接从项目根 `data/zones.json` 打当前完整包。
2. **恢复数据可重建性**：重新归档GeoNames、RESOLVE、Beck、ESA/Copernicus、Natural Earth矢量及完整ETOPO 15″原始输入，并登记下载日期、版本、许可、SHA-256；大源文件可单独存放，不要塞进主EXE。
3. **继续提高实际地物覆盖**：评估并验证更完整的河流、湖泊/海湾、山系与山峰矢量数据；以几何与分区相交归属，不用附近点/格中心伪造归属，保持来源、许可和误差说明。
4. **维护发行一致性**：继续保留小EXE与独立数据模块；生成补丁前核对应用版本、`gridVersion=Z9800`、manifest哈希、ZIP内目录结构，并对桌面目标作回归烟测。

## 九、主要来源索引

- UNESCO：[DataHub名录数据集](https://data.unesco.org/explore/dataset/whc001/)；派生模块单独按CC BY-SA 4.0署名UNESCO。
- NOAA ETOPO 2022：[官方产品页](https://www.ncei.noaa.gov/products/etopo-global-relief-model)，DOI 10.25921/fd45-gt74。
- NASA Blue Marble：[Blue Marble Next Generation说明](https://science.nasa.gov/earth/earth-observatory/blue-marble-next-generation/base-topography-bathymetry/)；现用影像为2004年1月合成。
- Natural Earth：[10m raster](https://www.naturalearthdata.com/downloads/10m-raster-data/)与[使用条款](https://www.naturalearthdata.com/about/terms-of-use/)。
- GeoNames：[下载与条款](https://www.geonames.org/export/)；RESOLVE、Beck气候与ESA/Copernicus土地覆盖来源详见 `DATA_SOURCES.md`。
