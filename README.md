# Building and running DOOM 3 (Windows, modern toolchain)

This fork builds the 2011 GPL DOOM 3 source with a current Visual Studio
toolchain. It is x86-only (every configuration is Win32) and renders with
OpenGL. The legacy DirectX SDK (June 2010) is **not** required: DirectInput8 and
DirectSound come from the modern Windows SDK, `DxErr` is replaced by a local
shim, and the EAX GUIDs are defined in-tree.

## Prerequisites

- Visual Studio 2026 with the "Desktop development with C++" workload, including:
  - MSVC v145 toolset
  - C++ MFC for latest build tools (x86 & x64)
  - A Windows 10/11 SDK (10.0.26100.0 is assumed; see Notes to change it)
- VS Code with the C/C++ extension (`ms-vscode.cpptools`) to build/debug with F5.
- Retail DOOM 3 game data (e.g. Steam app 9050). The source ships no assets.

## 1. Point the engine at the game data

The launch profile loads assets from `run\base`. Link it to your retail install
(the folder that contains `base`):

```powershell
# Developer Mode or an elevated shell is required to create a symlink.
New-Item -ItemType SymbolicLink -Path run\base -Target "D:\SteamLibrary\steamapps\common\Doom 3\base"
```

Copying the `base` folder into `run\base` works too.

## 2. Build

In VS Code, run the build task (`msbuild: doom3 (Debug)`), or from a shell:

```powershell
powershell -ExecutionPolicy Bypass -File .vscode\build-doom3.ps1 -Root . -Config Debug
```

This builds `idlib`, `curllib`, `typeinfo`, `game`, and `doomdll` (it skips
`MayaImport`, which needs the Maya SDK), producing:

- `build\Win32\Debug\DOOM3.exe` — the engine
- `build\Win32\Debug\gamex86.dll` — the game logic, also deployed to `run\dev`

Use `-Config Release` for an optimized build.

## 3. Run

Press F5 (`DOOM 3 — Debug` / `DOOM 3 — Release`). The engine launches with
`fs_basepath=run` and `fs_game=dev`, so `run\dev\gamex86.dll` (your freshly built
DLL) overrides the retail one. There is no QVM/bytecode in DOOM 3; game logic is
always this native DLL.

## Notes

- The `.vcxproj` files have no `PlatformToolset`, so the build script overrides it
  to v145 on the command line. No project files need editing.
- `.vscode\build-doom3.ps1` hardcodes the MSBuild path (VS 2026 Community) and
  `WindowsTargetPlatformVersion=10.0.26100.0`. Edit both if your VS edition,
  install path, or installed SDK differs.
- The `TypeInfo.exe` pre-build codegen is disabled: it is only used by the
  `ID_DEBUG_MEMORY` configuration and requires game data to run.
