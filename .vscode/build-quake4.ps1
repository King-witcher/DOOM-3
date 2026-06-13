# build-quake4.ps1 -- build the engine (DOOM3.exe) and deploy Raven's RETAIL
# Quake 4 gamex86.dll next to it, so the engine loads the Quake 4 game logic via
# GetGameAPI (GAME_API_VERSION 37) instead of our DOOM 3 game DLL.
#
# Differences from build-doom3.ps1:
#   * Does NOT build game.vcxproj (we run Quake 4's retail game DLL, not ours) or
#     typeinfo.vcxproj (only a pre-build step of game.vcxproj).
#   * Deploys "<Q4Dir>\q4base\gamex86.dll" into the engine output dir. The engine's
#     FindDLL loads the exe-dir gamex86.dll first, so this guarantees the retail
#     v37 game DLL is the one that gets loaded.
#
# The engine is still launched with fs_basepath pointed at the Quake 4 install
# (see launch.json "Quake 4" configs) so it reads q4base/*.pk4 assets.
param(
    [string]$Root,
    [string]$Config = "Debug",
    # Quake 4 install root (Steam). Override with -Q4Dir if yours differs.
    [string]$Q4Dir  = "C:\Program Files (x86)\Steam\steamapps\common\Quake 4"
)
$ErrorActionPreference = "Stop"
if ( -not $Root ) { throw "build-quake4.ps1: -Root is required" }

$msb = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
$neo = Join-Path $Root "neo"

# Modern toolset + an installed Windows SDK (same as build-doom3.ps1).
$props = "/p:Configuration=$Config;Platform=Win32;PlatformToolset=v145;WindowsTargetPlatformVersion=10.0.26100.0"

# idlib + curllib are static libs the engine links; doomdll is DOOM3.exe.
$projects = @(
    "idlib.vcxproj",
    "curllib.vcxproj",
    "doomdll.vcxproj"
)

foreach ( $p in $projects ) {
    $full = Join-Path $neo $p
    Write-Host "==== building (Win32 $Config): $p ===="
    & $msb $full $props /m /nologo
    if ( $LASTEXITCODE -ne 0 ) { throw "build failed: $p" }
}

# Deploy Raven's retail Quake 4 game DLL beside the engine.
$outDir = Join-Path $Root "build\Win32\$Config"
$retail = Join-Path $Q4Dir "q4base\gamex86.dll"
if ( -not (Test-Path $retail) ) {
    throw "retail Quake 4 gamex86.dll not found at '$retail' -- pass -Q4Dir <Quake 4 install root>"
}
Copy-Item $retail (Join-Path $outDir "gamex86.dll") -Force
Write-Host "Quake 4 build complete -> $outDir\DOOM3.exe ; retail gamex86.dll deployed (GAME_API_VERSION 37)"
