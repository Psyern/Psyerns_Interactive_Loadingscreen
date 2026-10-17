# Packs Psyerns_Interactive_Loadingscreen into a PBO with the DayZ Tools AddonBuilder.
# Usage:  powershell -ExecutionPolicy Bypass -File build_pbo.ps1 [-OutDir <folder>] [-SignKey <path to .biprivatekey>]
param(
	[string]$OutDir = "",
	[string]$SignKey = "",
	[string]$AddonBuilder = "C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe"
)

$tools = $PSScriptRoot
$repo = Split-Path (Split-Path $tools -Parent) -Parent
$source = Join-Path $repo "Psyerns_Interactive_Loadingscreen"
if ($OutDir -eq "") { $OutDir = Join-Path $repo "@Psyerns_Interactive_Loadingscreen\Addons" }

New-Item -ItemType Directory -Force $OutDir | Out-Null

$abArgs = @($source, $OutDir, "-clear", "-packonly", "-prefix=Psyerns_Interactive_Loadingscreen", "-include=$(Join-Path $tools 'include.txt')")
if ($SignKey -ne "") { $abArgs += "-sign=$SignKey" }

& $AddonBuilder @args
if ($LASTEXITCODE -ne 0) { Write-Error "AddonBuilder failed ($LASTEXITCODE)"; exit $LASTEXITCODE }

Copy-Item (Join-Path $source "mod.cpp") (Split-Path $OutDir -Parent) -Force
Write-Host "PBO written to $OutDir"
