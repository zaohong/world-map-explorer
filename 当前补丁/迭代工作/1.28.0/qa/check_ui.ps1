param([string]$Program='WorldMapExplorer-1.28.0.exe',[string[]]$Tests=@('SavedManagerUiChecks','DpiUiChecks','StartupUiChecks','SessionUiChecks','ExperienceUiChecks','ExperienceSearchChecks','BasemapMenuUiChecks','HistoryPlaybackRegression','EventSearchUiChecks','SearchInputUiChecks','HumanRegionPreview','RouteUiChecks','ExportChecks'),[string]$Run='acceptance-01',[switch]$WindowsScale)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$app=Join-Path (Split-Path (Split-Path $root -Parent) -Parent) ('WorldMapExplorer-1.28.0\'+$Program)
$output=Join-Path $PSScriptRoot ('development\'+$Run)
$bin=Join-Path $PSScriptRoot 'development\bin'
New-Item -ItemType Directory -Path $output,$bin -Force | Out-Null
foreach($name in $Tests){
  $folder=Join-Path $output $name
  if(Test-Path -LiteralPath $folder){throw "Use a new Run name for a fresh isolated profile: $folder"}
  New-Item -ItemType Directory -Path $folder -Force | Out-Null
  if($name -ne 'DpiUiChecks' -and !$WindowsScale){
    # Older input/pixel regressions run at a fixed UI size. Dedicated DPI checks exercise the production default and every scale.
    $profileFolder=Join-Path $folder 'profile\WorldMapExplorer'
    New-Item -ItemType Directory -Path $profileFolder -Force | Out-Null
    "[Preferences]`r`nUiScale=1`r`n" | Set-Content -LiteralPath (Join-Path $profileFolder 'settings.ini') -Encoding unicode
  }
  & 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe' /nologo /target:exe /r:System.Drawing.dll "/win32manifest:$root\native\world_map_explorer.manifest" "/out:$bin\$name.exe" (Join-Path $PSScriptRoot "$name.cs")
  if($LASTEXITCODE -ne 0){throw "UI check compilation failed: $name"}
  Write-Output ("Checking "+$name)
  & "$bin\$name.exe" $app $folder | Out-File -LiteralPath (Join-Path $output "$name.txt") -Encoding utf8
  $result=$LASTEXITCODE
  Get-Content -LiteralPath (Join-Path $output "$name.txt") -Tail 4
  if($result -ne 0){throw "UI checks failed: $name"}
}

