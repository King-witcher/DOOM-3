Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System; using System.Runtime.InteropServices;
public class Win {
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
}
"@
# rightmost monitor (Ozone is the right physical display, X>=1920 here)
$right = ([System.Windows.Forms.Screen]::AllScreens | Sort-Object { $_.Bounds.X } -Descending)[0]
$wx = $right.Bounds.X + 80
$wy = $right.Bounds.Y + 60
$exe = "C:\code\id\DOOM-3\build\Win32\Release\DOOM3.exe"
$argv = @("+set","fs_basepath","C:\code\id\DOOM-3\run\q4","+set","fs_game","q4base","+set","fs_savepath","C:\code\id\DOOM-3\run\q4save","+set","si_pure","0","+set","fs_restrict","0","+set","com_allowConsole","1","+set","logFile","2","+set","r_fullscreen","0","+set","image_usePrecompressedTextures","0","+set","r_windowX","$wx","+set","r_windowY","$wy")
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
    # shove the window onto the right monitor so it never covers the middle one (SWP_NOSIZE=0x0001)
    [Win]::SetWindowPos($p.MainWindowHandle, [IntPtr]::Zero, $wx, $wy, 0, 0, 0x0001) | Out-Null
    Write-Output ("WINDOW appeared at {0:N1}s title=[{1}] moved to {2},{3}" -f $sw.Elapsed.TotalSeconds, $p.MainWindowTitle, $wx, $wy)
  }
}
Write-Output ("STILL ALIVE at {0:N0}s (window={1})" -f $sw.Elapsed.TotalSeconds, $lastWin)
try { $p.Kill() } catch {}
