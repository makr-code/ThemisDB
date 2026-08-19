param(
    [int]$Repetitions = 100,
    [string]$ExePath = 'build-msvc-windows-release\bin\test_self_rag_alce_focused.exe'
)

if (-not (Test-Path $ExePath)) { Write-Error "Executable not found: $ExePath"; exit 2 }

$ts = (Get-Date).ToString('yyyyMMdd_HHmmss')
$artifactDir = Join-Path 'artifacts' ("bench_release\$ts")
New-Item -ItemType Directory -Force -Path (Join-Path $artifactDir 'logs') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $artifactDir 'env') | Out-Null

# Set lightweight instrumentation env (no new PDBs)
$orig = @{}
$envVars = @('THEMIS_RAG_INSTRUMENT_RETRIEVAL','THEMIS_RAG_CAPTURE_STACK_ON_SLOW','THEMIS_RAG_RETRIEVAL_BENCH','THEMIS_RAG_RETRIEVAL_BENCH_COUNT')
foreach ($v in $envVars) { $orig[$v] = [Environment]::GetEnvironmentVariable($v); [Environment]::SetEnvironmentVariable($v, $null) }
[Environment]::SetEnvironmentVariable('THEMIS_RAG_INSTRUMENT_RETRIEVAL','1')
[Environment]::SetEnvironmentVariable('THEMIS_RAG_CAPTURE_STACK_ON_SLOW','1')
[Environment]::SetEnvironmentVariable('THEMIS_RAG_RETRIEVAL_BENCH','1')
[Environment]::SetEnvironmentVariable('THEMIS_RAG_RETRIEVAL_BENCH_COUNT','200')

for ($i=1; $i -le $Repetitions; $i++) {
    $log = Join-Path $artifactDir ("logs\run_{0:0000}.log" -f $i)
    Write-Host "Run #$i -> $log"
    & $ExePath --gtest_filter=SelfRAGALCETest.ALCE_01_LatencyRatioWithinBound --gtest_catch_exceptions=0 *>&1 | Tee-Object -FilePath $log
    Start-Sleep -Milliseconds 50
}

# Save environment snapshot
Get-ChildItem Env: | Sort-Object Name | Out-File (Join-Path $artifactDir 'env\env_snapshot.txt') -Encoding utf8

# Restore env
foreach ($k in $envVars) { [Environment]::SetEnvironmentVariable($k, $orig[$k]) }

Write-Host "Done. Artifacts: $artifactDir"
Write-Host "Compress with: Compress-Archive -Path $artifactDir -DestinationPath $artifactDir.zip"