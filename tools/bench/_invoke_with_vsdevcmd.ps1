# Helper: find VsDevCmd.bat and run the benchmark inside its environment
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File tools/bench/_invoke_with_vsdevcmd.ps1

Write-Host "Locating VsDevCmd.bat under 'C:\Program Files\Microsoft Visual Studio\2022'..."
$vs = Get-ChildItem -Path 'C:\Program Files\Microsoft Visual Studio\2022' -Recurse -Filter 'VsDevCmd.bat' -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $vs) {
    Write-Error "VsDevCmd.bat not found under C:\Program Files\Microsoft Visual Studio\2022."
    exit 3
}
$vsPath = $vs.FullName
Write-Host "Found: $vsPath"

# Build the cmd line: run VsDevCmd.bat in cmd, then call PowerShell to run the benchmark script with same environment
$benchmarkScript = "tools\bench\self_rag_benchmark.ps1"
$reps = 100

# Create a temporary batch file to run VsDevCmd and then the PowerShell benchmark
$fullBenchmark = Resolve-Path -LiteralPath $benchmarkScript -ErrorAction SilentlyContinue
if (-not $fullBenchmark) { $fullBenchmark = Join-Path (Get-Location) $benchmarkScript }
$tmpBat = Join-Path $env:TEMP ("run_vs_build_{0}.bat" -f (Get-Random))
$batContent = @"
@echo off
"$vsPath"
powershell -NoProfile -ExecutionPolicy Bypass -File "$fullBenchmark" -Repetitions $reps -RunNow
"@
Write-Host "Writing temporary batch: $tmpBat"
Set-Content -Path $tmpBat -Value $batContent -Encoding ASCII
Write-Host "Executing batch: $tmpBat"
$proc = Start-Process -FilePath cmd.exe -ArgumentList '/c', $tmpBat -NoNewWindow -Wait -PassThru
# Clean up
Remove-Item -Force -LiteralPath $tmpBat -ErrorAction SilentlyContinue
exit $proc.ExitCode
