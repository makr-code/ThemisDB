# Run VsDevCmd with a forced VCToolsVersion and capture trace
$vcVersion = '14.44.35207'
$vs = Get-ChildItem -Path 'C:\Program Files\Microsoft Visual Studio\2022' -Recurse -Filter 'VsDevCmd.bat' -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $vs) { Write-Host 'VsDevCmd.bat not found'; exit 3 }
$vsPath = $vs.FullName
$trace = 'artifacts\vsdevcmd_trace_fixed.txt'
New-Item -ItemType Directory -Force -Path artifacts | Out-Null
$traceFull = Join-Path (Get-Location).Path $trace
$cmd = "set VCToolsVersion=$vcVersion && set VSCMD_DEBUG=3 && `"$vsPath`" > `"$traceFull`" 2>&1"
Write-Host "Running: cmd.exe /c $cmd"
cmd.exe /c $cmd
Write-Host "Trace written to $traceFull"