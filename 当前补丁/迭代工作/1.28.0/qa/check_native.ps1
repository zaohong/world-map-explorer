param([string[]]$Tests=@('SavedStoreChecks','ViewportChecks','SessionStateChecks','ExperienceModelChecks','EventCatalogChecks','RouteAccuracyChecks','route_geometry_checks','fine_grid_logic_tests'))
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path $PSScriptRoot 'development'
$bin=Join-Path $output 'bin'
New-Item -ItemType Directory -Path $bin -Force | Out-Null
$vs='C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat'
$lines=& $env:ComSpec /d /s /c "call `"$vs`" -no_logo -arch=x64 && set"
foreach($line in $lines){$p=$line.IndexOf('=');if($p -gt 0 -and $line.Substring(0,$p) -match '^[A-Za-z_][A-Za-z0-9_]*$'){[Environment]::SetEnvironmentVariable($line.Substring(0,$p),$line.Substring($p+1),'Process')}}
foreach($name in $Tests){
  & cl.exe /nologo /std:c++17 /O2 /EHsc /utf-8 /DUNICODE /D_UNICODE "/Fe:$bin\$name.exe" "/Fo:$bin\$name.obj" (Join-Path $PSScriptRoot "$name.cpp") /link gdiplus.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib shell32.lib
  if($LASTEXITCODE -ne 0){throw "Native check compilation failed: $name"}
  $arguments=@()
  if($name -eq 'SavedStoreChecks'){$arguments=@((Join-Path $output 'saved-store-model'))}
  if($name -eq 'SessionStateChecks'){$arguments=@((Join-Path $output 'session-model'))}
  if($name -eq 'ExperienceModelChecks'){$arguments=@((Join-Path (Split-Path (Split-Path $root -Parent) -Parent) 'WorldMapExplorer-1.28.0\data\place-search.tsv'),(Join-Path $output 'experience-model'))}
  if($name -eq 'RouteAccuracyChecks'){$arguments=@((Join-Path $output 'route-geometry.json'))}
  if($name -eq 'EventCatalogChecks'){$arguments=@((Join-Path (Split-Path (Split-Path $root -Parent) -Parent) 'WorldMapExplorer-1.28.0\data\world-events-v2.tsv'))}
  if($name -eq 'fine_grid_logic_tests'){$arguments=@((Join-Path (Split-Path (Split-Path $root -Parent) -Parent) 'WorldMapExplorer-1.28.0\data\world-2026.json'))}
  & "$bin\$name.exe" @arguments | Tee-Object -FilePath (Join-Path $output "$name.txt")
  if($LASTEXITCODE -ne 0){throw "Native checks failed: $name"}
}

