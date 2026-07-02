# Quake 4 source reconstruction — original layout evidence

Raven's engine/game source root was named **`CodePC/`** (this repo's `neo/` plays
that role). Evidence from the retail binaries (assert + PDB strings):

- `e:\buildbot.quake4\Quake4_win32_bin\build\CodePC\framework\async\AsyncServer.cpp`
  (Quake4.exe, rescanSI assert)
- `e:\buildbot.quake4\Quake4_win32_bin\build\CodePC\game\Game_local.cpp` and
  `CodePC\game\MultiplayerGame.cpp` (retail gamex86.dll)
- `...\build\Win32\Release\SPGamex86.pdb` — the retail q4base/gamex86.dll was
  built as **SPGamex86** from `CodePC/game/`.

Original folder names were therefore `game/` and `mpgame/` directly under the
root. In this repo those names are taken by the DOOM 3 game code, so the Quake 4
trees live as `q4game/`, `q4mpgame/` and `q4idlib/` (internal structure
preserved exactly as in the official 1.4.2 SDK, which mirrors CodePC).
