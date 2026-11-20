param(
    [string]$OutputName = 'WorldMapExplorer-candidate.exe',
    [switch]$RunChecks
)
$ErrorActionPreference = 'Stop'
if ($OutputName -notmatch '^[A-Za-z0-9._-]+\.exe$') { throw 'OutputName must be an executable filename, without a directory.' }
$workDir = $PSScriptRoot
$releaseDir = Join-Path (Split-Path (Split-Path $workDir -Parent) -Parent) 'WorldMapExplorer-1.28.0'
$qaDir = Join-Path $workDir 'qa\development'
$binDir = Join-Path $qaDir 'bin'
New-Item -ItemType Directory -Path $binDir -Force | Out-Null
$vsDevCmd = 'C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat'
if (!(Test-Path -LiteralPath $vsDevCmd)) { throw "Visual Studio environment not found: $vsDevCmd" }
$environmentLines = & $env:ComSpec /d /s /c "call `"$vsDevCmd`" -no_logo -arch=x64 && set"
if ($LASTEXITCODE -ne 0) { throw 'Visual Studio environment initialization failed' }
foreach ($line in $environmentLines) {
    $separator = $line.IndexOf('=')
    if ($separator -gt 0) {
        $name = $line.Substring(0,$separator)
        if ($name -match '^[A-Za-z_][A-Za-z0-9_]*$') { [Environment]::SetEnvironmentVariable($name,$line.Substring($separator+1),'Process') }
    }
}
Push-Location $workDir
try {
    & rc.exe /nologo "/fo$binDir\app.res" 'native\world_map_explorer.rc'
    if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed' }
    $flags = @('/nologo','/std:c++17','/O2','/EHsc','/utf-8','/DUNICODE','/D_UNICODE','/DWIN32','/D_WINDOWS')
    $libraries = @('gdiplus.lib','comctl32.lib','comdlg32.lib','user32.lib','gdi32.lib')
    & cl.exe @flags "/Fe:$releaseDir\$OutputName" "/Fo:$binDir\app.obj" "/Fd:$binDir\app.pdb" 'native\world_map_explorer.cpp' "$binDir\app.res" /link /SUBSYSTEM:WINDOWS @libraries
    if ($LASTEXITCODE -ne 0) { throw 'Application compilation failed' }
    if ($RunChecks) {
        & (Join-Path $workDir 'qa\check_native.ps1')
        & (Join-Path $workDir 'qa\check_vectors.ps1')
        & node.exe (Join-Path $workDir 'qa\audit_sea_connections.cjs')
        if ($LASTEXITCODE -ne 0) { throw 'Sea-connection audit failed' }
    }
} finally { Pop-Location }
