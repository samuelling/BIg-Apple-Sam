# Builds BigAppleSam.scr with MSVC. Run from a Developer PowerShell (CI uses ilammy/msvc-dev-cmd).
# Needs the WebView2 SDK in .\packages:  nuget install Microsoft.Web.WebView2 -Version 1.0.4258.31 -OutputDirectory packages
param([string]$Version = "1.0.0")
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$sdk = Get-ChildItem packages -Directory -Filter "Microsoft.Web.WebView2*" | Select-Object -First 1
if (-not $sdk) { throw "WebView2 SDK not found in .\packages" }
$inc = Join-Path $sdk.FullName "build\native\include"
$lib = Join-Path $sdk.FullName "build\native\x64\WebView2LoaderStatic.lib"

$parts = ($Version -split "\.") + @("0", "0", "0", "0") | Select-Object -First 4
$num = ($parts | ForEach-Object { [int]$_ }) -join ","

Set-Content version.h "#define BAS_VERSION_NUM $num`n#define BAS_VERSION_STR `"$Version`"" -Encoding ascii
rc /nologo /fo BigAppleSam.res BigAppleSam.rc
if ($LASTEXITCODE) { throw "rc failed" }

cl /nologo /EHsc /O2 /MT /std:c++17 /W3 /DUNICODE /D_UNICODE /I"$inc" BigAppleSam.cpp BigAppleSam.res `
   /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTINPUT:app.manifest /MANIFESTUAC:NO `
   "$lib" user32.lib gdi32.lib ole32.lib oleaut32.lib shell32.lib shlwapi.lib advapi32.lib comctl32.lib version.lib `
   /OUT:BigAppleSam.scr
if ($LASTEXITCODE) { throw "cl failed" }
Get-Item BigAppleSam.scr | Format-List Name, Length
