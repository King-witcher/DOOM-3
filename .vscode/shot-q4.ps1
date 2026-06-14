Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public struct RECT { public int L,T,R,B; }
public class Cap {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
 [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
}
"@
$exe = "C:\code\id\DOOM-3\build\Win32\Release\DOOM3.exe"
$argv = @("+set","fs_basepath","C:\code\id\DOOM-3\run\q4","+set","fs_game","q4base","+set","fs_savepath","C:\code\id\DOOM-3\run\q4save","+set","si_pure","0","+set","fs_restrict","0","+set","com_allowConsole","1","+set","logFile","2","+set","r_fullscreen","0")
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $exe; $psi.WorkingDirectory = Split-Path $exe; $psi.UseShellExecute = $false
$psi.Arguments = ($argv -join ' ')
$p = [System.Diagnostics.Process]::Start($psi)
Start-Sleep -Seconds $([int]$args[0])
$p.Refresh()
$shot = "C:\code\id\DOOM-3\.vscode\q4menu.png"
if (-not $p.HasExited -and $p.MainWindowHandle -ne 0) {
  $h = $p.MainWindowHandle
  Write-Output ("MainWindowTitle: [" + $p.MainWindowTitle + "]")
  [Cap]::ShowWindow($h,9) | Out-Null; [Cap]::BringWindowToTop($h) | Out-Null; [Cap]::SetForegroundWindow($h) | Out-Null
  Start-Sleep -Milliseconds 800
  $r = New-Object RECT; [Cap]::GetWindowRect($h,[ref]$r) | Out-Null
  $w = $r.R - $r.L; $ht = $r.B - $r.T
  Write-Output "window rect ${w}x${ht} at $($r.L),$($r.T)"
  if ($w -gt 0 -and $ht -gt 0) {
    $bmp = New-Object System.Drawing.Bitmap $w, $ht
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.L, $r.T, 0, 0, (New-Object System.Drawing.Size($w,$ht)))
    $bmp.Save($shot, [System.Drawing.Imaging.ImageFormat]::Png)
    Write-Output "SHOT saved: $shot"
  }
} else { Write-Output "no window (exited=$($p.HasExited))" }
if (-not $p.HasExited) { try { $p.Kill() } catch {} }