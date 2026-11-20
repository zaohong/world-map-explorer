$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path $PSScriptRoot 'development'
$vs='C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat'
$lines=& $env:ComSpec /d /s /c "call `"$vs`" -no_logo -arch=x64 && set"
foreach($line in $lines){$p=$line.IndexOf('=');if($p -gt 0 -and $line.Substring(0,$p) -match '^[A-Za-z_][A-Za-z0-9_]*$'){[Environment]::SetEnvironmentVariable($line.Substring(0,$p),$line.Substring($p+1),'Process')}}
$bin=Join-Path $output 'bin'
New-Item -ItemType Directory -Path $bin -Force | Out-Null
& node.exe (Join-Path $PSScriptRoot 'make_vector_fixture.cjs')
if($LASTEXITCODE -ne 0){throw 'Synthetic fixture build failed'}
& cl.exe /nologo /std:c++17 /O2 /EHsc /utf-8 /DUNICODE /D_UNICODE "/Fe:$bin\HistoryVectorChecks.exe" "/Fo:$bin\HistoryVectorChecks.obj" (Join-Path $PSScriptRoot 'HistoryVectorChecks.cpp') /link gdiplus.lib user32.lib gdi32.lib
if($LASTEXITCODE -ne 0){throw 'Vector checks failed to compile'}
$archive=Join-Path (Split-Path (Split-Path $root -Parent) -Parent) 'WorldMapExplorer-1.28.0\data\historical-boundaries.wmh2'
& "$bin\HistoryVectorChecks.exe" $archive $output | Tee-Object -FilePath (Join-Path $output 'history-vector-checks.txt')
if($LASTEXITCODE -ne 0){throw 'Vector checks failed'}
