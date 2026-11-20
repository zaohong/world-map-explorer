param([Parameter(Mandatory=$true)][string]$Folder)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath $Folder).Path
$manifest=Join-Path $root 'FILES.sha256'
if(!(Test-Path -LiteralPath $manifest)){throw 'FILES.sha256 not found in the extracted runtime'}
$checked=0
foreach($line in Get-Content -LiteralPath $manifest -Encoding UTF8){
 if(!$line -or $line.StartsWith('#')){continue}
 if($line -notmatch '^([0-9A-Fa-f]{64})  (.+)$'){throw 'Invalid manifest line'}
 $expected=$Matches[1];$relative=$Matches[2]
 if($relative -match '(^|/)\.\.(/|$)' -or [IO.Path]::IsPathRooted($relative)){throw 'Unsafe manifest path'}
 $file=[IO.Path]::GetFullPath((Join-Path $root $relative.Replace('/','\')))
 if(!$file.StartsWith($root.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Manifest path escapes runtime'}
 if(!(Test-Path -LiteralPath $file -PathType Leaf)){throw ('Missing: '+$relative)}
 if((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $expected){throw ('Hash mismatch: '+$relative)}
 $checked++
}
Write-Output ("PASS: "+$checked+" runtime files match FILES.sha256")
