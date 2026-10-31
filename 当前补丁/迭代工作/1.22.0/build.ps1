param([switch]$SkipHistoricalGeneration)
$ErrorActionPreference = 'Stop'
$workDir = $PSScriptRoot
$patchRoot = Split-Path (Split-Path $workDir -Parent) -Parent
$releaseDir = Join-Path $patchRoot 'WorldMapExplorer-1.22.0'
$qaDir = Join-Path $workDir 'qa'
$source = Join-Path $workDir 'native\world_map_explorer.cpp'
$moduleBuilder = Join-Path $workDir 'native\build_world_2026.cpp'
$historyBuilder = Join-Path $workDir 'native\build_historical_worlds.js'
$historySourceDir = Join-Path $workDir 'source-data\history-atlas'
$countryShape = Join-Path $workDir 'source-data\natural-earth\ne_10m_admin_0_countries.shp'
$countryDbf = Join-Path $workDir 'source-data\natural-earth\ne_10m_admin_0_countries.dbf'
$testSource = Join-Path $qaDir 'fine_grid_logic_tests.cpp'
$uiSmokeSource = Join-Path $qaDir 'WorldMapExplorerUiSmoke.cs'
$historyChecks = Join-Path $qaDir 'historical_module_checks.js'
$eventChecks = Join-Path $qaDir 'world_event_checks.js'
$assetChecks = Join-Path $qaDir 'asset_hash_checks.js'
$iconGenerator = Join-Path $workDir 'native\create_app_icon.js'
$iconResource = Join-Path $workDir 'native\world_map_explorer.rc'
$iconFile = Join-Path $workDir 'native\world_map_explorer.ico'
$vsDevCmd = 'C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat'
$csharpCompiler = 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe'

if (!(Test-Path -LiteralPath $releaseDir)) { throw "运行目录不存在：$releaseDir" }
if (!(Test-Path -LiteralPath $source)) { throw "源码不存在：$source" }
if (!(Test-Path -LiteralPath $moduleBuilder)) { throw "世界2026数据构建源码不存在：$moduleBuilder" }
if (!(Test-Path -LiteralPath $historyBuilder)) { throw "历史年份模块构建源码不存在：$historyBuilder" }
if (!(Test-Path -LiteralPath $historyChecks)) { throw "历史模块抽样检查不存在：$historyChecks" }
if (!(Test-Path -LiteralPath $eventChecks)) { throw "历史事件模块检查不存在：$eventChecks" }
if (!(Test-Path -LiteralPath $assetChecks)) { throw "底图及素材完整性检查不存在：$assetChecks" }
if (!(Test-Path -LiteralPath $iconGenerator) -or !(Test-Path -LiteralPath $iconResource)) { throw '缺少应用图标生成器或 Windows 图标资源文件' }
if (!(Test-Path -LiteralPath (Join-Path $releaseDir 'data\world-events.tsv'))) { throw '缺少离线历史事件模块 data\world-events.tsv' }
if (!(Test-Path -LiteralPath (Join-Path $historySourceDir 'CLIOPATRIA_LICENSE.md')) -or !(Test-Path -LiteralPath (Join-Path $historySourceDir 'HISTORY_ATLAS_README.md'))) { throw '历史数据来源说明或许可文件不完整' }
if (!(Test-Path -LiteralPath $countryShape) -or !(Test-Path -LiteralPath $countryDbf)) { throw '缺少 Natural Earth 10m 国家面数据，请先恢复迭代归档中的 source-data 文件' }
if (!(Test-Path -LiteralPath $vsDevCmd)) { throw "找不到 Visual Studio 编译环境：$vsDevCmd" }
if (!(Test-Path -LiteralPath $csharpCompiler)) { throw "找不到 C# 界面检查工具：$csharpCompiler" }

$environmentLines = & $env:ComSpec /d /s /c "call `"$vsDevCmd`" -no_logo -arch=x64 && set"
if ($LASTEXITCODE -ne 0) { throw '初始化 Visual Studio 编译环境失败' }
foreach ($line in $environmentLines) {
    $separator = $line.IndexOf('=')
    if ($separator -gt 0) {
        $name = $line.Substring(0, $separator)
        if ($name -match '^[A-Za-z_][A-Za-z0-9_]*$') {
            [Environment]::SetEnvironmentVariable($name, $line.Substring($separator + 1), 'Process')
        }
    }
}
$compiler = (Get-Command cl.exe -ErrorAction Stop).Source
$resourceCompiler = (Get-Command rc.exe -ErrorAction Stop).Source
$node = (Get-Command node.exe -ErrorAction Stop).Source

Push-Location $workDir
try {
    $licenseDir = Join-Path $releaseDir 'licenses'
    New-Item -ItemType Directory -Path $licenseDir -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $historySourceDir 'CLIOPATRIA_LICENSE.md') -Destination (Join-Path $licenseDir 'CLIOPATRIA_LICENSE.md') -Force
    Copy-Item -LiteralPath (Join-Path $historySourceDir 'HISTORY_ATLAS_README.md') -Destination (Join-Path $licenseDir 'HISTORY_ATLAS_DATA_SOURCE.md') -Force

    & $compiler /nologo /std:c++17 /O2 /EHsc /utf-8 `
        "/Fe:$qaDir\build_world_2026.exe" "/Fo:$qaDir\build_world_2026.obj" `
        "/Fd:$qaDir\build_world_2026.pdb" $moduleBuilder
    if ($LASTEXITCODE -ne 0) { throw "世界2026数据构建工具编译失败，退出码 $LASTEXITCODE" }
    & "$qaDir\build_world_2026.exe" $countryShape $countryDbf (Join-Path $releaseDir 'data\world-2026.json') | Tee-Object -FilePath (Join-Path $qaDir 'world-2026-data-build.stdout.txt')
    if ($LASTEXITCODE -ne 0) { throw "世界2026离线数据生成失败，退出码 $LASTEXITCODE" }

    if (!$SkipHistoricalGeneration) {
        & $node $historyBuilder | Tee-Object -FilePath (Join-Path $qaDir 'historical-data-build.stdout.txt')
        if ($LASTEXITCODE -ne 0) { throw "历史世界离线数据生成失败，退出码 $LASTEXITCODE" }
    }

    & $node $historyChecks (Join-Path $releaseDir 'data') | Tee-Object -FilePath (Join-Path $qaDir 'historical-data-checks.stdout.txt')
    if ($LASTEXITCODE -ne 0) { throw "历史世界数据抽样检查失败，退出码 $LASTEXITCODE" }

    & $node $eventChecks (Join-Path $releaseDir 'data') (Join-Path $workDir 'native\history_years.generated.h') | Tee-Object -FilePath (Join-Path $qaDir 'world-event-checks.stdout.txt')
    if ($LASTEXITCODE -ne 0) { throw "历史事件数据检查失败，退出码 $LASTEXITCODE" }

    & $node $assetChecks $releaseDir (Join-Path $patchRoot 'WorldMapExplorer-1.18.0') | Tee-Object -FilePath (Join-Path $qaDir 'asset-hash-checks.stdout.txt')
    if ($LASTEXITCODE -ne 0) { throw "底图或运行素材校验失败，退出码 $LASTEXITCODE" }

    & $node $iconGenerator $iconFile
    if ($LASTEXITCODE -ne 0) { throw "应用图标生成失败，退出码 $LASTEXITCODE" }
    $iconResourceOutput = Join-Path $qaDir 'world_map_explorer.res'
    & $resourceCompiler /nologo "/fo$iconResourceOutput" $iconResource
    if ($LASTEXITCODE -ne 0) { throw "Windows 图标资源编译失败，退出码 $LASTEXITCODE" }

    & $compiler /nologo /std:c++17 /O2 /EHsc /utf-8 /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS `
        "/Fe:$releaseDir\WorldMapExplorer.exe" "/Fo:$qaDir\world_map_explorer.obj" `
        "/Fd:$qaDir\world_map_explorer.pdb" $source $iconResourceOutput /link /SUBSYSTEM:WINDOWS gdiplus.lib comctl32.lib user32.lib gdi32.lib
    if ($LASTEXITCODE -ne 0) { throw "主程序编译失败，退出码 $LASTEXITCODE" }

    & $compiler /nologo /std:c++17 /O2 /EHsc /utf-8 /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS `
        "/Fe:$qaDir\fine_grid_logic_tests.exe" "/Fo:$qaDir\fine_grid_logic_tests.obj" `
        "/Fd:$qaDir\fine_grid_logic_tests.pdb" $testSource /link gdiplus.lib comctl32.lib user32.lib gdi32.lib
    if ($LASTEXITCODE -ne 0) { throw "精度与逻辑检查编译失败，退出码 $LASTEXITCODE" }

    & "$qaDir\fine_grid_logic_tests.exe" | Tee-Object -FilePath (Join-Path $qaDir 'logic-precision.stdout.txt')
    if ($LASTEXITCODE -ne 0) { throw "精度与逻辑检查失败，退出码 $LASTEXITCODE" }

    & $csharpCompiler /nologo /target:exe /platform:x64 /reference:System.Drawing.dll "/out:$qaDir\WorldMapExplorerUiSmoke.exe" $uiSmokeSource
    if ($LASTEXITCODE -ne 0) { throw "界面检查程序编译失败，退出码 $LASTEXITCODE" }
    & "$qaDir\WorldMapExplorerUiSmoke.exe" (Join-Path $releaseDir 'WorldMapExplorer.exe') $qaDir | Tee-Object -FilePath (Join-Path $qaDir 'ui-smoke.stdout.txt')
    if ($LASTEXITCODE -ne 0) { throw "界面运行检查失败，退出码 $LASTEXITCODE" }
} finally {
    Pop-Location
}

$temporaryBuildFiles = @(
    'build_world_2026.exe','build_world_2026.obj','build_world_2026.pdb',
    'fine_grid_logic_tests.exe','fine_grid_logic_tests.obj','fine_grid_logic_tests.pdb','world_map_explorer.res',
    'world_map_explorer.obj','world_map_explorer.pdb','WorldMapExplorerUiSmoke.exe'
)
foreach ($name in $temporaryBuildFiles) {
    $path = Join-Path $qaDir $name
    if (Test-Path -LiteralPath $path -PathType Leaf) { Remove-Item -LiteralPath $path -Force }
}
