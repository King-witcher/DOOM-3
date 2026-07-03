# Launch retail Quake 4 MULTIPLAYER as a local listen server on this engine.
# The q4mp game dir supplies Raven's MPGame gamex86.dll (extracted from its paks).
param([string]$Map = "mp/q4dm1", [string]$GameType = "DM")
$exe = "C:\code\id\DOOM-3\build\Win32\Release\DOOM3.exe"
$argv = @(
    "+set","fs_basepath","C:\code\id\DOOM-3\run\q4",
    "+set","fs_game","q4mp",
    "+set","fs_savepath","C:\code\id\DOOM-3\run\q4save",
    "+set","si_pure","0","+set","fs_restrict","0",
    "+set","com_allowConsole","1","+set","logFile","2",
    "+set","r_fullscreen","0","+set","r_windowX","2000","+set","r_windowY","60",
    "+set","in_alwaysRun","1","+set","com_forceGenericSIMD","1",
    "+set","si_map",$Map,
    "+set","si_gameType",$GameType,
    "+set","si_maxPlayers","4","+set","si_usePass","0",
    "+spawnServer"
)
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $exe; $psi.WorkingDirectory = Split-Path $exe; $psi.UseShellExecute = $false
$psi.Arguments = ($argv -join ' ')
[System.Diagnostics.Process]::Start($psi) | Out-Null
Write-Host "Quake 4 MP listen server launched: $Map ($GameType)"
