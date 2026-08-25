<#
PowerShell helper to build RelWithDebInfo and run focused Self-RAG benchmark.

Usage examples:
  # Dry-run configure+build and run 100 iterations (safe):
  .\self_rag_benchmark.ps1 -Repetitions 100

  # Run and keep artifacts:
  .\self_rag_benchmark.ps1 -Repetitions 200 -RunNow

Notes:
- Requires CMake + Ninja + MSVC toolchain available in PATH (vcvars64 or Developer Command Prompt).
- Script creates build directory `build-msvc-relwithdebinfo`.
- Artifacts (logs + PDBs) are saved under `artifacts/bench_self_rag/<timestamp>/`.
#>
param(
    [int]$Repetitions = 100,
    [string]$BuildDir = "build-msvc-relwithdebinfo",
    [string]$Target = "test_self_rag_alce_focused",
    [string]$ExeRelPath = "bin\test_self_rag_alce_focused.exe",
    [switch]$RunNow
)

function Require-Command($name) {
    $p = Get-Command $name -ErrorAction SilentlyContinue
    if (-not $p) { Write-Error "Required command '$name' not found in PATH."; exit 2 }
}

Write-Host "Self-RAG benchmark script: repetitions=$Repetitions buildDir=$BuildDir"

# Preconditions
Require-Command cmake
Require-Command ninja

# Configure
# Clear platform hints that break Ninja generator when present in the env
Remove-Item Env:CMAKE_GENERATOR_PLATFORM -ErrorAction SilentlyContinue
Remove-Item Env:Platform -ErrorAction SilentlyContinue

$cmakeArgs = @(
    '-S', '.',
    '-B', $BuildDir,
    '-G', 'Ninja',
    '-DCMAKE_BUILD_TYPE=RelWithDebInfo',
    '-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=/Zi',
    '-DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO=/DEBUG',
    '-DTHEMIS_BUILD_TESTS=ON'
)
Write-Host "Running: cmake $($cmakeArgs -join ' ')"
$cfgBuildDir = Resolve-Path -LiteralPath $BuildDir -ErrorAction SilentlyContinue
if ($cfgBuildDir) {
    Write-Host "Removing existing build dir: $BuildDir"
    Remove-Item -Recurse -Force -LiteralPath $BuildDir
}

$cfg = & cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { Write-Error "CMake configure failed (exit $LASTEXITCODE). See output above."; exit $LASTEXITCODE }

# Build target
Write-Host "Building target: $Target"
$procCount = [int][Environment]::ProcessorCount
$buildArgs = @('--build', $BuildDir, '--config', 'RelWithDebInfo', '--target', $Target, '--', '-j', [math]::Max(1, $procCount))
$rc = & cmake @buildArgs
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed (exit $LASTEXITCODE)."; exit $LASTEXITCODE }

# Prepare artifacts dir
$ts = (Get-Date).ToString('yyyyMMdd_HHmmss')
$artifactDir = Join-Path -Path 'artifacts' -ChildPath "bench_self_rag\$ts"
New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $artifactDir 'pdbs') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $artifactDir 'logs') | Out-Null

$exePath = Join-Path $BuildDir $ExeRelPath
if (-not (Test-Path $exePath)) { Write-Error "Executable not found: $exePath"; exit 3 }

if (-not $RunNow) {
    Write-Host "Prepared build and artifacts directory: $artifactDir"
    Write-Host "Run the script with -RunNow to execute $Repetitions runs and collect PDBs/logs. Example: .\self_rag_benchmark.ps1 -Repetitions 100 -RunNow"
    exit 0
}

Write-Host "Executing $Repetitions runs and collecting outputs..."
# Ensure deterministic env for the runs
$origEnv = @{}
$envVars = @('THEMIS_RAG_INSTRUMENT_RETRIEVAL','THEMIS_RAG_CAPTURE_STACK_ON_SLOW','THEMIS_RAG_RETRIEVAL_BENCH')
foreach ($v in $envVars) { $origEnv[$v] = [Environment]::GetEnvironmentVariable($v) }

# Enable instrumentation
[Environment]::SetEnvironmentVariable('THEMIS_RAG_INSTRUMENT_RETRIEVAL','1')
[Environment]::SetEnvironmentVariable('THEMIS_RAG_CAPTURE_STACK_ON_SLOW','1')
[Environment]::SetEnvironmentVariable('THEMIS_RAG_RETRIEVAL_BENCH','1')
[Environment]::SetEnvironmentVariable('THEMIS_RAG_RETRIEVAL_BENCH_COUNT','200')

for ($i = 1; $i -le $Repetitions; $i++) {
    $log = Join-Path $artifactDir ("logs\run_{0:0000}.log" -f $i)
    Write-Host "Run #$i -> $log"
    # Run and capture stdout+stderr
    & $exePath --gtest_filter=SelfRAGALCETest.ALCE_01_LatencyRatioWithinBound --gtest_catch_exceptions=0 *>&1 | Tee-Object -FilePath $log
    Start-Sleep -Milliseconds 50
}

# Collect PDBs for the executable and any nearby DLLs
Write-Host "Collecting PDBs into $artifactDir\pdbs"
$binFolder = Split-Path -Parent $exePath
Get-ChildItem -Path $binFolder -Filter *.pdb -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item -Path $_.FullName -Destination (Join-Path $artifactDir 'pdbs') -Force
}

# Also collect PDBs under build dir
Get-ChildItem -Path $BuildDir -Filter *.pdb -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item -Path $_.FullName -Destination (Join-Path $artifactDir 'pdbs') -Force
}

# Save a small manifest
$manifest = @{ timestamp = $ts; repetitions = $Repetitions; exe = $exePath; buildDir = $BuildDir }
$manifest | ConvertTo-Json | Out-File -FilePath (Join-Path $artifactDir 'manifest.json') -Encoding utf8

# Restore env
foreach ($k in $envVars) { [Environment]::SetEnvironmentVariable($k, $origEnv[$k]) }

Write-Host "Done. Artifacts saved to: $artifactDir"
Write-Host "You can compress the folder for upload: Compress-Archive -Path $artifactDir -DestinationPath $artifactDir.zip"
