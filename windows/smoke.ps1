# CI smoke test: run the screen saver and its options dialog, and take screenshots.
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
New-Item -ItemType Directory -Force shots | Out-Null

function Snap($name) {
  $b = [System.Windows.Forms.SystemInformation]::VirtualScreen
  $bmp = New-Object System.Drawing.Bitmap $b.Width, $b.Height
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($b.Left, $b.Top, 0, 0, $bmp.Size)
  $bmp.Save("shots\$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose()
  Write-Host "saved shots\$name.png ($($b.Width)x$($b.Height))"
}

$p = Start-Process .\BigAppleSam.scr -ArgumentList "/s" -PassThru
Start-Sleep -Seconds 20
if ($p.HasExited) { throw "screen saver exited early (code $($p.ExitCode))" }
Snap "run"
Get-ChildItem "$env:LOCALAPPDATA\Big Apple Sam\site" | Format-Table Name, Length
Stop-Process -Id $p.Id -Force

$c = Start-Process .\BigAppleSam.scr -ArgumentList "/c" -PassThru
Start-Sleep -Seconds 4
Snap "options"
Stop-Process -Id $c.Id -Force
