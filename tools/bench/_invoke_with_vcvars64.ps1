# Fallback invoker: run vcvars64.bat then the benchmark script
$vcvarsCandidates = @(
    'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat'
)
$vc = $vcvarsCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $vc) { Write-Error "vcvars64.bat not found in common locations."; exit 4 }
Write-Host "Using vcvars: $vc"

$benchmarkScript = Resolve-Path -LiteralPath 'tools\bench\self_rag_benchmark.ps1'
$reps = 100
$tmpBat = Join-Path $env:TEMP ("run_vcvars_{0}.bat" -f (Get-Random))
$batContent = @"
@echo off
"$vc"
powershell -NoProfile -ExecutionPolicy Bypass -File "$benchmarkScript" -Repetitions $reps -RunNow
"@

Set-Content -Path $tmpBat -Value $batContent -Encoding ASCII
Write-Host "Running batch: $tmpBat"
$proc = Start-Process -FilePath cmd.exe -ArgumentList '/c', $tmpBat -NoNewWindow -Wait -PassThru
Remove-Item -Force -LiteralPath $tmpBat -ErrorAction SilentlyContinue
exit $proc.ExitCode
