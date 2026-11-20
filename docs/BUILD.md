# 构建与目录

当前 Windows 源码保留在 `当前补丁/迭代工作/1.28.0/`。`native/` 包含 C++17 程序、界面模块、资源清单和已生成的目录头文件；`qa/` 包含当前版本的模型、几何与实际窗口检查。

## 重建桌面程序

1. 下载 v1.28.0 的完整运行包，将解压后的 `WorldMapExplorer-1.28.0` 文件夹放到仓库的 `当前补丁/` 下。
2. 安装 Visual Studio C++ 桌面开发工具（含 x64 编译器、Windows SDK）及 Node.js。界面检查还使用 Windows 自带 .NET Framework C# 编译器。现有构建脚本默认使用 Visual Studio 2019 Professional 的安装位置；使用其它版本时，将 `build-app.ps1`、`qa/check_native.ps1`、`qa/check_vectors.ps1` 中的 VsDevCmd 路径改为实际安装位置。
3. 在 `当前补丁/迭代工作/1.28.0/` 执行：

```powershell
./build-app.ps1 -OutputName WorldMapExplorer-candidate.exe -RunChecks
```

候选程序输出到同一运行目录，可以直接使用已下载的数据。编译不需要联网服务，也不需要下载可选原始输入。不同编译时间或工具链可能产生不同的 EXE 哈希。

## 检查

```powershell
./qa/check_native.ps1
./qa/check_vectors.ps1
./qa/check_ui.ps1 -Program WorldMapExplorer-candidate.exe -Run local-check-01
```

检查程序使用独立配置，不读取个人收藏。界面检查应顺序运行，避免输入焦点互相影响；每次使用新的 `-Run` 名称。默认部分既有像素检查固定界面 100%，专门的 DPI 检查验证其它比例；`-WindowsScale` 可用于跟随系统的检查。

若只编译已发布版本，保留生成头文件即可。需要重建来源数据时，先按 [SUPPLEMENTAL-DATA.md](../SUPPLEMENTAL-DATA.md) 恢复输入；`build_history_vectors.cjs`、`build_place_catalog.cjs`、`build_events.cjs` 等对应当前数据流程。`build_historical_worlds.js` 是旧栅格数据构建工具，不是当前矢量历史的重建入口。

日常运行的数据和全部细节瓦片位于 Release；大型原始输入单独提供。仓库不包含个人浏览配置、收藏、私钥或本机维护日志。
