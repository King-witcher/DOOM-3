# build-doom3.ps1 -- build the Doom 3 engine + base game DLL and leave them in
# neo\..\build\Win32\<Config>\ (DOOM3.exe + gamex86.dll).
#
# Why a script instead of building neo\doom.sln directly:
#   * The solution also contains MayaImport.vcxproj, which needs the Autodesk
#     Maya SDK and would fail on a normal machine. We build only the projects
#     required to run the game.
#   * The original .vcxproj files have NO <PlatformToolset> (they default to
#     v100 / VS2010, which isn't installed). We override it to v145 on the
#     command line, exactly like the Quake III setup in the sibling repo.
#   * Doom 3 is x86-only: every solution config is |Win32. There is no x64
#     config, so Platform is hard-pinned to Win32.
#
# Build order matters: TypeInfo.exe is produced by typeinfo.vcxproj and run as a
# pre-build step of game.vcxproj (it generates GameTypeInfo.h).
param(
    [string]$Root,
    [string]$Config = "Debug"
)
$ErrorActionPreference = "Stop"
if ( -not $Root ) { throw "build-doom3.ps1: -Root is required" }

$msb = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
$neo = Join-Path $Root "neo"

# Modern toolset + an installed Windows SDK. 10.0.26100.0 is what is present on
# this machine; change it if your installed SDK differs (VS Installer shows it).
$props = "/p:Configuration=$Config;Platform=Win32;PlatformToolset=v145;WindowsTargetPlatformVersion=10.0.26100.0"

# idlib + curllib are static libs the engine links; typeinfo builds the tool the
# game DLL's pre-build step runs; game is gamex86.dll; doomdll is DOOM3.exe.
$projects = @(
    "idlib.vcxproj",
    "curllib.vcxproj",
    "typeinfo.vcxproj",
    "game.vcxproj",
    "doomdll.vcxproj"
)

foreach ( $p in $projects ) {
    $full = Join-Path $neo $p
    Write-Host "==== building (Win32 $Config): $p ===="
    & $msb $full $props /m /nologo
    if ( $LASTEXITCODE -ne 0 ) { throw "build failed: $p" }
}

# Deploy our freshly built game DLL into run\dev. Launched with fs_game=dev, the
# engine searches run\dev before run\base, so this overrides the retail
# base\gamex86.dll with our build. Doom 3 has NO QVM/bytecode -- game logic is
# always this native DLL -- so this is the only "mod" artifact to deploy. The
# .pdb goes along so the game DLL is debuggable from run\dev too.
$outDir = Join-Path $Root "build\Win32\$Config"
$dev    = Join-Path $Root "run\dev"
New-Item -ItemType Directory -Force -Path $dev | Out-Null
Copy-Item (Join-Path $outDir "gamex86.dll") $dev -Force
$pdb = Join-Path $outDir "gamex86.pdb"
if ( Test-Path $pdb ) { Copy-Item $pdb $dev -Force }

Write-Host "Doom 3 build complete -> build\Win32\$Config\DOOM3.exe ; game DLL deployed to run\dev"
