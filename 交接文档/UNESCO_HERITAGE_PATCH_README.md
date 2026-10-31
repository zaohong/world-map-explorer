# UNESCO 世界遗产离线数据补丁 1.9.0

本补丁为已有的“世界地图分区档案”加入可搜索的 UNESCO 世界遗产离线专题。它不会替换地图、9,800 格主档案、土地覆盖、地形或 NASA 资源。

## 安装

1. 完全退出“世界地图分区档案”。
2. 在应用主目录中备份现有 `WorldMapExplorer.exe`，例如命名为 `WorldMapExplorer-v1.8.0.exe.bak`。
3. 用本包中的 `WorldMapExplorer.exe` 替换应用主目录下同名文件。
4. 将本包的 `data/world-heritage-2026.json` 复制到应用主目录的 `data` 子目录；如果目标文件已存在，先另存备份。
5. 重新启动程序。在搜索框输入遗产中英文名称可定位；点击分区后，右侧“世界遗产（UNESCO）”栏显示本格包含的遗产信息。

若程序无法启动，可退出程序后恢复第 2 步备份的 EXE，并移除本次新增的数据文件；原来的档案和地图不会受到影响。

## 包含文件

- `WorldMapExplorer.exe`：893,440 字节；支持 UNESCO 模块的轻量主程序。
- `data/world-heritage-2026.json`：3,481,912 字节；1,273 项官方名录摘要、6,397 个组成部分坐标映射到 Z9800 网格。
- `update-manifest.json`：文件大小、SHA-256、兼容版本和补丁范围。

## 来源与许可

来源：[UNESCO DataHub 世界遗产名录数据集 `whc001`](https://data.unesco.org/explore/dataset/whc001/)（数据集标注 CC BY-SA 4.0），官方名录：[UNESCO World Heritage List](https://whc.unesco.org/en/list/)。离线派生模块按 CC BY-SA 4.0 分发，署名 UNESCO；再分发或改编时需保留署名并按相同许可共享改编内容。该许可只适用于 UNESCO 数据模块，不代表程序或其他数据许可。
