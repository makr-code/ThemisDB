param(
    [Parameter(Mandatory=$true)]
    [string]$Root
)

$ErrorActionPreference = 'Stop'

$build = Join-Path $Root 'build-msvc'
if (-not (Test-Path $build)) {
    New-Item -ItemType Directory -Path $build | Out-Null
}

$vcpkgRoot = Join-Path $Root 'vcpkg'
if (-not (Test-Path (Join-Path $vcpkgRoot 'vcpkg.exe'))) {
    # Fallback to root workspace vcpkg if the worktree lacks its own copy
    $vcpkgRoot = 'C:\VCC\themis\vcpkg'
}
$env:VCPKG_ROOT = $vcpkgRoot

$env:VCPKG_INSTALLED_DIR = Join-Path $Root 'vcpkg_installed'
if (-not (Test-Path $env:VCPKG_INSTALLED_DIR)) {
    # Fallback to the main workspace cache if a fresh worktree is used
    $env:VCPKG_INSTALLED_DIR = 'C:\VCC\themis\vcpkg_installed'
}

$tool = Join-Path $vcpkgRoot 'scripts\buildsystems\vcpkg.cmake'

# Ensure required dependencies exist in the selected vcpkg tree
& (Join-Path $vcpkgRoot 'vcpkg.exe') install --triplet x64-windows

Push-Location $Root
$prefix = Join-Path $env:VCPKG_INSTALLED_DIR 'x64-windows'
$opensslRoot = $prefix
$rocksdbDir = Join-Path $prefix 'share\rocksdb'
$curlDir = Join-Path $prefix 'share\curl'
$cmakeArgs = @(
    '-S', '.',
    '-B', $build,
    '-G', 'Visual Studio 17 2022',
    '-A', 'x64',
    "-DCMAKE_TOOLCHAIN_FILE=$tool",
    '-DVCPKG_TARGET_TRIPLET=x64-windows',
    '-DBUILD_SHARED_LIBS=ON',
    '-DTHEMIS_ENABLE_LLM=OFF',
    "-DOPENSSL_ROOT_DIR=$opensslRoot",
    "-DRocksDB_DIR=$rocksdbDir",
    "-DCURL_DIR=$curlDir",
    "-DCMAKE_PREFIX_PATH=$prefix"
)
cmake @cmakeArgs

$targets = @(
    @{ Name = 'bench_core_performance'; OutSuffix = 'core_perf'; Args = @('--benchmark_min_time=0.3s') },
    @{ Name = 'bench_comprehensive'; OutSuffix = 'comprehensive_llm_aql'; Args = @('--benchmark_filter=LLMInferencingBench|AQLQueryBench|AQLJoinBench', '--benchmark_min_time=0.15s') },
    @{ Name = 'bench_transaction_throughput'; OutSuffix = 'txn_throughput'; Args = @('--benchmark_min_time=0.3s') },
    @{ Name = 'bench_hybrid_aql_sugar'; OutSuffix = 'hybrid_aql'; Args = @('--benchmark_min_time=0.15s') }
)

$builtTargets = @()
foreach ($t in $targets) {
    $proj = Join-Path $build "$($t.Name).vcxproj"
    if (-not (Test-Path $proj)) {
        continue
    }
    cmake --build $build --config Release --target $($t.Name) --parallel 4
    $builtTargets += $t
}

$rel = Join-Path $build 'Release'
$out = 'C:\VCC\themis\msvc_bench_results'
if (-not (Test-Path $out)) {
    New-Item -ItemType Directory -Path $out | Out-Null
}

$tagName = Split-Path $Root -Leaf

foreach ($t in $builtTargets) {
    $exe = Join-Path $rel "$($t.Name).exe"
    if (-not (Test-Path $exe)) {
        continue
    }
    $outFile = Join-Path $out "$tagName`_$($t.OutSuffix).json"
    & $exe --benchmark_out=$outFile --benchmark_out_format=json @($t.Args)
}

Pop-Location
