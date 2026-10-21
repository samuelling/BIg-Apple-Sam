# CI smoke test: run the screen saver and its options dialog, and take screenshots.
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
New-Item -ItemType Directory -Force shots | Out-Null

# Launch directly: going through the shell would add /S (the .scr "open" verb).
function Launch($arg) {
  $si = New-Object System.Diagnostics.ProcessStartInfo (Resolve-Path .\BigAppleSam.scr).Path, $arg
  $si.UseShellExecute = $false
  [System.Diagnostics.Process]::Start($si)
}

function Snap($name) {
  $b = [System.Windows.Forms.SystemInformation]::VirtualScreen
  $bmp = New-Object System.Drawing.Bitmap $b.Width, $b.Height
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($b.Left, $b.Top, 0, 0, $bmp.Size)
  $bmp.Save("shots\$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose()
  Write-Host "saved shots\$name.png ($($b.Width)x$($b.Height))"
}

$p = Launch "/s"
Start-Sleep -Seconds 20
if ($p.HasExited) { throw "screen saver exited early (code $($p.ExitCode))" }
Snap "run"
Get-ChildItem "$env:LOCALAPPDATA\Big Apple Sam\site" | Format-Table Name, Length
Stop-Process -Id $p.Id -Force
Start-Sleep -Seconds 2

$c = Launch "/c"
Start-Sleep -Seconds 4
Snap "options"
Stop-Process -Id $c.Id -Force
