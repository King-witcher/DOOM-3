$exe = "C:\code\id\DOOM-3\build\Win32\Release\DOOM3.exe"
$argv = @("+set","fs_basepath","C:\code\id\DOOM-3\run\q4","+set","fs_game","q4base","+set","fs_savepath","C:\code\id\DOOM-3\run\q4save","+set","si_pure","0","+set","fs_restrict","0","+set","com_allowConsole","1","+set","logFile","2","+set","r_fullscreen","0","+set","image_usePrecompressedTextures","0")
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $exe; $psi.WorkingDirectory = Split-Path $exe; $psi.UseShellExecute = $false
$psi.Arguments = ($argv -join ' ')
$sw = [System.Diagnostics.Stopwatch]::StartNew()
$p = [System.Diagnostics.Process]::Start($psi)
$max = [int]$args[0]
$lastWin = 0
while ($sw.Elapsed.TotalSeconds -lt $max) {
  Start-Sleep -Seconds 3
  $p.Refresh()
  if ($p.HasExited) {
    Write-Output ("EXITED after {0:N1}s  exitcode={1}" -f $sw.Elapsed.TotalSeconds, $p.ExitCode)
    exit
  }
  if ($p.MainWindowHandle -ne 0 -and $lastWin -eq 0) {
    $lastWin = 1
    Write-Output ("WINDOW appeared at {0:N1}s title=[{1}]" -f $sw.Elapsed.TotalSeconds, $p.MainWindowTitle)
  }
}
Write-Output ("STILL ALIVE at {0:N0}s (window={1})" -f $sw.Elapsed.TotalSeconds, $lastWin)
try { $p.Kill() } catch {}