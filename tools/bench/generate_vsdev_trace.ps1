# Generate VsDevCmd debug trace into artifacts/vsdevcmd_trace.txt
$vs = Get-ChildItem -Path 'C:\Program Files\Microsoft Visual Studio\2022' -Recurse -Filter 'VsDevCmd.bat' -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $vs) { Write-Host 'VsDevCmd.bat not found under C:\Program Files\Microsoft Visual Studio\2022'; exit 3 }
$vsPath = $vs.FullName
$trace = 'artifacts\vsdevcmd_trace.txt'
New-Item -ItemType Directory -Force -Path artifacts | Out-Null
$traceFull = Join-Path (Get-Location).Path $trace
$cmd = "set VSCMD_DEBUG=3 && `"$vsPath`" > `"$traceFull`" 2>&1"
Write-Host "Running: cmd.exe /c $cmd"
cmd.exe /c $cmd
Write-Host "Trace written to $traceFull"