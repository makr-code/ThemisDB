<#
.SYNOPSIS
    Runs all available ThemisDB benchmark executables and aggregates results
    into a structured JSON + Markdown performance report.

.DESCRIPTION
    Discovers every bench_*.exe in the build directory, runs each one with
    Google Benchmark JSON output, and produces a per-module performance summary.
    A Markdown report is generated alongside the raw JSON files.

.PARAMETER BuildDir
    Root of the CMake build tree.
    Default: .\build\windows-bench-release

.PARAMETER Preset
    CMake preset name used when -BuildMissing is set.
    Default: windows-bench-release

.PARAMETER Filter
    Glob/regex applied to executable base names to restrict which benchmarks run.
    Example: "bench_transaction*" or "bench_(query|storage).*"
    Default: runs all.

.PARAMETER MinTime
    Value passed as --benchmark_min_time to each benchmark (e.g. "0.5s").
    Lower values make the run faster at the cost of accuracy.
    Default: 0.5s

.PARAMETER MaxTimePerBench
    Wall-clock timeout in seconds for a single benchmark executable.
    Benchmarks that exceed this limit are killed and marked as TIMEOUT.
    Default: 180

.PARAMETER BuildMissing
    When set, builds any bench_*.exe targets that are missing from the
    bin directory before running. Requires the CMake preset to be configured.

.PARAMETER BuildParallel
    Number of parallel jobs used when -BuildMissing is active.
    Default: 4

.PARAMETER OutputDir
    Directory where per-run JSON files and the aggregate report are written.
    A timestamped sub-folder is created automatically.
    Default: <BuildDir>\bench-results\batch_<timestamp>

.EXAMPLE
    # Run all already-built benchmarks with default settings
    .\scripts\run-bench-all.ps1

.EXAMPLE
    # Run only transaction and storage benchmarks, build if missing
    .\scripts\run-bench-all.ps1 -Filter "bench_(transaction|storage).*" -BuildMissing

.EXAMPLE
    # Quick smoke pass (1-second total budget per benchmark)
    .\scripts\run-bench-all.ps1 -MinTime 0.1s -MaxTimePerBench 30
#>

[CmdletBinding()]
param(
    [string]  $BuildDir       = ".\build\windows-bench-release",
    [string]  $Preset         = "windows-bench-release",
    [string]  $Filter         = "",
    [string]  $MinTime        = "0.5s",
    [int]     $MaxTimePerBench = 180,
    [switch]  $BuildMissing,
    [int]     $BuildParallel  = 4,
    [string]  $OutputDir      = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
function Write-Section ([string]$Msg) {
    Write-Host ""
    Write-Host "=== $Msg ===" -ForegroundColor Cyan
}

function Write-Ok ([string]$Msg)   { Write-Host "  [OK]  $Msg" -ForegroundColor Green  }
function Write-Warn ([string]$Msg) { Write-Host "  [!!]  $Msg" -ForegroundColor Yellow }
function Write-Fail ([string]$Msg) { Write-Host "  [XX]  $Msg" -ForegroundColor Red    }

# ---------------------------------------------------------------------------
# Module mapping: bench executable prefix -> src/ module name
# Used for grouping results in the report.
# ---------------------------------------------------------------------------
$ModuleMap = [ordered]@{
    bench_acceleration      = "acceleration"
    bench_active_vram       = "gpu"
    bench_adaptive_query    = "query"
    bench_advanced_patterns = "query"
    bench_api_endpoints     = "api"
    bench_approximate_radius= "search"
    bench_aql               = "aql"
    bench_arm_              = "core"
    bench_async_io          = "storage"
    bench_auth_             = "auth"
    bench_auto_buffers      = "storage"
    bench_backend_          = "gpu"
    bench_batch_insert      = "storage"
    bench_binary_quant      = "index"
    bench_blob_             = "storage"
    bench_branch_manager    = "replication"
    bench_cdc_              = "cdc"
    bench_chaos_            = "chaos"
    bench_changefeed        = "cdc"
    bench_compliance        = "governance"
    bench_comprehensive     = "core"
    bench_compression       = "storage"
    bench_config_           = "config"
    bench_content_          = "content"
    bench_continuous_query  = "query"
    bench_core_             = "core"
    bench_cross_functional  = "core"
    bench_crud              = "storage"
    bench_csv_export        = "exporters"
    bench_cuda_             = "gpu"
    bench_cycle_metrics     = "observability"
    bench_data_transfer     = "network"
    bench_delegate_         = "query"
    bench_di_logging        = "observability"
    bench_diff_engine       = "document"
    bench_distributed_coord = "replication"
    bench_distributed_know  = "distributed_knowledge"
    bench_docker_raid       = "storage"
    bench_edge_cases        = "core"
    bench_embedded_llm      = "llm"
    bench_embedding_cache   = "cache"
    bench_encryption        = "security"
    bench_ethics_ai         = "ethics_ai"
    bench_exporters         = "exporters"
    bench_extended_context  = "llm"
    bench_flash_attention   = "llm"
    bench_fused_kernels     = "gpu"
    bench_fused_lora        = "training"
    bench_geo_              = "geo"
    bench_gnn_              = "graph"
    bench_gorilla_codec     = "timeseries"
    bench_gossip_           = "replication"
    bench_governance_       = "governance"
    bench_gpu_backends      = "gpu"
    bench_gpu_erasure       = "gpu"
    bench_gpu_hardware      = "gpu"
    bench_gpu_module        = "gpu"
    bench_gpu_training      = "training"
    bench_gpu_vector_index  = "index"
    bench_gpu_vram          = "gpu"
    bench_graph_query       = "graph"
    bench_graph_traversal   = "graph"
    bench_hnsw_             = "index"
    bench_hot_reload        = "plugins"
    bench_hotspots_         = "performance"
    bench_hsm_              = "security"
    bench_hybrid_aql        = "aql"
    bench_hybrid_vector_geo = "geo"
    bench_image_analysis    = "onnx_clip"
    bench_importer_         = "importers"
    bench_index_rebuild     = "index"
    bench_ingestion_        = "ingestion"
    bench_insert_profiling  = "storage"
    bench_interval_tree     = "index"
    bench_knowledge_gap     = "distributed_knowledge"
    bench_latency_          = "core"
    bench_learned_quant     = "index"
    bench_legal_lora        = "training"
    bench_llama_cpp_        = "llama_cpp"
    bench_llm_inference     = "llm"
    bench_llm_infra         = "llm"
    bench_llm_judge         = "llm"
    bench_llm_raid          = "llm"
    bench_llm_real          = "llm"
    bench_llm_response      = "cache"
    bench_locality_         = "network"
    bench_lock_contention   = "transaction"
    bench_lora_auto         = "training"
    bench_lora_framework    = "training"
    bench_lora_gpu          = "training"
    bench_lora_inline       = "training"
    bench_lora_training     = "training"
    bench_lossy_vs          = "index"
    bench_metadata_         = "metadata"
    bench_metrics_          = "observability"
    bench_mixed_precision   = "gpu"
    bench_mmdb              = "geo"
    bench_module_load       = "plugins"
    bench_multi_gpu_lora    = "training"
    bench_multi_gpu_scaling = "gpu"
    bench_multi_lora_fusion = "training"
    bench_multithreading    = "core"
    bench_mvcc              = "transaction"
    bench_observability     = "observability"
    bench_olap_             = "analytics"
    bench_pagerank          = "graph"
    bench_parquet_export    = "exporters"
    bench_phase1_flash      = "llm"
    bench_pii_stream        = "security"
    bench_plugin_hot_plug   = "plugins"
    bench_plugin_system     = "plugins"
    bench_policy_evaluation = "governance"
    bench_postgres_e2e      = "rpc_grpc"
    bench_postgres_protocol = "network"
    bench_postgres_transact = "transaction"
    bench_process_import    = "process"
    bench_process_mining    = "process"
    bench_process_retrieval = "process"
    bench_product_quant     = "index"
    bench_prompt_engineer   = "prompt_engineering"
    bench_qlora_gpu         = "training"
    bench_query_lazy        = "query"
    bench_query             = "query"
    bench_rag_ethics        = "rag"
    bench_rag_evaluation    = "rag"
    bench_rag_hybrid        = "rag"
    bench_raid_lora         = "training"
    bench_random_access     = "storage"
    bench_replication_      = "replication"
    bench_residual_quant    = "index"
    bench_rotary_embed      = "llm"
    bench_saga_             = "transaction"
    bench_sanity            = "core"
    bench_scalability       = "core"
    bench_security          = "security"
    bench_shard_resource    = "sharding"
    bench_shard_routing     = "sharding"
    bench_sharding_         = "sharding"
    bench_simd_distance     = "index"
    bench_simple_insert     = "storage"
    bench_snapshot_manager  = "storage"
    bench_spatial_index     = "geo"
    bench_spatial_join      = "geo"
    bench_storage_perf      = "storage"
    bench_stream_protocol   = "network"
    bench_task_scheduler    = "scheduler"
    bench_temporal_         = "temporal"
    bench_tensor_fingerprint= "tensor"
    bench_text_extraction   = "ingestion"
    bench_themis_core       = "core"
    bench_thread_pool       = "core"
    bench_timeseries_       = "timeseries"
    bench_tpcc              = "transaction"
    bench_tpch              = "analytics"
    bench_transaction_      = "transaction"
    bench_update_pipeline   = "updates"
    bench_user_storage      = "user_storage_encrypted"
    bench_v1_               = "core"
    bench_vector_comp       = "index"
    bench_vector_prefilter  = "index"
    bench_vector_search     = "search"
    bench_video_            = "content"
    bench_voice_            = "voice"
    bench_vulkan_lora       = "training"
    bench_wal_              = "storage"
    bench_whisper_          = "whisper"
    bench_ycsb              = "storage"
}

function Resolve-Module ([string]$ExeName) {
    foreach ($prefix in $ModuleMap.Keys) {
        if ($ExeName -like "$prefix*") {
            return $ModuleMap[$prefix]
        }
    }
    return "other"
}

# ---------------------------------------------------------------------------
# Resolve paths
# ---------------------------------------------------------------------------
$RepoRoot  = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else {
    Join-Path $RepoRoot $BuildDir
}
$BinDir    = Join-Path $BuildPath "bin"

$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
if ($OutputDir -eq "") {
    $OutputDir = Join-Path $BuildPath "bench-results\batch_$Timestamp"
}
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

Write-Section "ThemisDB Benchmark Runner"
Write-Host "  Repo root   : $RepoRoot"
Write-Host "  Build dir   : $BuildPath"
Write-Host "  Bin dir     : $BinDir"
Write-Host "  Output dir  : $OutputDir"
Write-Host "  Min time    : $MinTime"
Write-Host "  Timeout/bench: ${MaxTimePerBench}s"

# ---------------------------------------------------------------------------
# Optionally build missing targets
# ---------------------------------------------------------------------------
if ($BuildMissing) {
    Write-Section "Building missing benchmark targets"
    Push-Location $RepoRoot
    try {
        # Get all bench target names from CMakeLists
        $cmakeListsPath = Join-Path $RepoRoot "benchmarks\CMakeLists.txt"
        $targetNames = Select-String -Path $cmakeListsPath -Pattern "add_executable\((\S+)" |
            ForEach-Object { $_.Matches[0].Groups[1].Value } |
            Where-Object { $_ -like "bench_*" -and $_ -notlike '*$*' } |
            Sort-Object -Unique

        $missing = @()
        foreach ($t in $targetNames) {
            $exePath = Join-Path $BinDir "$t.exe"
            if (-not (Test-Path $exePath)) {
                $missing += $t
            }
        }

        if ($missing.Count -eq 0) {
            Write-Ok "All targets already built."
        } else {
            Write-Host "  Building $($missing.Count) missing targets in parallel batches..."
            # Build in batches of BuildParallel to avoid overwhelming the linker
            $batchSize = [Math]::Max(1, $BuildParallel)
            for ($i = 0; $i -lt $missing.Count; $i += $batchSize) {
                $batch = $missing[$i .. [Math]::Min($i + $batchSize - 1, $missing.Count - 1)]
                $targetArgs = $batch | ForEach-Object { "--target", $_ }
                $buildArgs = @("--build", "--preset", $Preset) + $targetArgs + @("--parallel", $BuildParallel)
                Write-Host "  cmake $($buildArgs -join ' ')"
                $proc = Start-Process cmake -ArgumentList $buildArgs -Wait -PassThru -NoNewWindow
                if ($proc.ExitCode -ne 0) {
                    Write-Warn "Batch build failed (exit $($proc.ExitCode)); continuing with whatever was built."
                }
            }
        }
    } finally {
        Pop-Location
    }
}

# ---------------------------------------------------------------------------
# Discover benchmark executables
# ---------------------------------------------------------------------------
Write-Section "Discovering benchmark executables"

if (-not (Test-Path $BinDir)) {
    Write-Fail "Bin directory not found: $BinDir"
    Write-Host "  Run with -BuildMissing or build the preset first."
    exit 1
}

$allExes = @(Get-ChildItem $BinDir -Filter "bench_*.exe" | Sort-Object BaseName)

if ($Filter -ne "") {
    $allExes = @($allExes | Where-Object { $_.BaseName -match $Filter })
    Write-Host "  Filter '$Filter' -> $($allExes.Count) executables selected"
} else {
    Write-Host "  Found $($allExes.Count) bench_*.exe files"
}

if ($allExes.Count -eq 0) {
    Write-Fail "No benchmark executables found. Nothing to run."
    exit 1
}

# ---------------------------------------------------------------------------
# Run each benchmark
# ---------------------------------------------------------------------------
Write-Section "Running benchmarks"

$results   = [System.Collections.Generic.List[hashtable]]::new()
$succeeded = 0
$failed    = 0
$timedOut  = 0

foreach ($exe in $allExes) {
    $name    = $exe.BaseName
    $module  = Resolve-Module $name
    $outFile = Join-Path $OutputDir "${name}_${Timestamp}.json"

    Write-Host "  [$module] $name" -NoNewline

    $benchArgs = @(
        "--benchmark_out=$outFile",
        "--benchmark_out_format=json",
        "--benchmark_min_time=$MinTime",
        "--benchmark_color=false"
    )

    $startTime = Get-Date
    try {
        # Use a PowerShell job so we can enforce the wall-clock timeout reliably
        # while still capturing the exit code via $LASTEXITCODE in the job scope.
        $stdoutFile = "$outFile.stdout.txt"
        $stderrFile = "$outFile.stderr.txt"
        $envPath = $env:PATH
        $job = Start-Job -ScriptBlock {
            param($exePath, $argList, $workDir, $stdoutFile, $stderrFile, $pathEnv)
            # Reset error preference so native-command stderr doesn't abort the job
            $ErrorActionPreference = 'Continue'
            # Ensure DLLs in the build bin directory are findable
            $env:PATH = "$workDir;$pathEnv"
            Set-Location $workDir
            & $exePath @argList > $stdoutFile 2> $stderrFile
            return $LASTEXITCODE
        } -ArgumentList $exe.FullName, $benchArgs, $BinDir, $stdoutFile, $stderrFile, $envPath

        $completed = Wait-Job $job -Timeout $MaxTimePerBench
        $elapsed   = [math]::Round(((Get-Date) - $startTime).TotalSeconds, 1)

        if ($null -eq $completed) {
            Stop-Job  $job
            Remove-Job $job -Force
            Write-Host " TIMEOUT (${elapsed}s)" -ForegroundColor Yellow
            $timedOut++
            $results.Add(@{
                name    = $name
                module  = $module
                status  = "TIMEOUT"
                elapsed = $elapsed
                outFile = $outFile
                cases   = @()
            })
            continue
        }

        $jobResult = Receive-Job $job
        Remove-Job $job -Force
        $exitCode  = if ($null -ne $jobResult) { [int]$jobResult } else { 0 }

        if ($exitCode -ne 0) {
            # Some benchmarks crash during shutdown (e.g. AV in destructor) but still
            # write valid results to the JSON output file. Treat as success when the
            # output file exists and contains at least one benchmark entry.
            $hasOutput = $false
            if (Test-Path $outFile) {
                try {
                    $probe = Get-Content $outFile -Raw | ConvertFrom-Json
                    $hasOutput = $null -ne $probe.benchmarks -and $probe.benchmarks.Count -gt 0
                } catch { }
            }
            if (-not $hasOutput) {
                $stderr = Get-Content $stderrFile -Raw -ErrorAction SilentlyContinue
                Write-Host " FAILED (exit $exitCode, ${elapsed}s)" -ForegroundColor Red
                if ($stderr) {
                    $firstLine = ($stderr -split "`n")[0].Trim()
                    if ($firstLine) { Write-Host "    $firstLine" -ForegroundColor DarkRed }
                }
                $failed++
                $results.Add(@{
                    name    = $name
                    module  = $module
                    status  = "FAILED"
                    elapsed = $elapsed
                    outFile = $outFile
                    cases   = @()
                })
                continue
            }
            # Fall through to successful JSON parsing below (exit on cleanup, not on run)
            Write-Host " OK+AV_EXIT (exit $exitCode, ${elapsed}s)" -ForegroundColor DarkYellow -NoNewline
        }

        # Parse JSON output
        $cases = @()
        if (Test-Path $outFile) {
            try {
                $benchData = Get-Content $outFile -Raw | ConvertFrom-Json
                $cases = @($benchData.benchmarks | Where-Object {
                    $_.name -like "*_mean" -or
                    ($null -ne $_.run_type -and $_.run_type -eq "iteration")
                } | ForEach-Object {
                    @{
                        case      = $_.name
                        real_time = [math]::Round($_.real_time, 3)
                        cpu_time  = [math]::Round($_.cpu_time, 3)
                        unit      = $_.time_unit
                        iters     = $_.iterations
                    }
                })
            } catch {
                Write-Warn "Could not parse JSON output for $name"
            }
        }

        Write-Host " OK (${elapsed}s, $($cases.Count) cases)" -ForegroundColor Green
        $succeeded++
        $results.Add(@{
            name    = $name
            module  = $module
            status  = "OK"
            elapsed = $elapsed
            outFile = $outFile
            cases   = $cases
        })

    } catch {
        $elapsed = [math]::Round(((Get-Date) - $startTime).TotalSeconds, 1)
        Write-Host " ERROR: $_" -ForegroundColor Red
        $failed++
        $results.Add(@{
            name    = $name
            module  = $module
            status  = "ERROR"
            elapsed = $elapsed
            outFile = $outFile
            cases   = @()
        })
    }
}

# ---------------------------------------------------------------------------
# Aggregate results by module
# ---------------------------------------------------------------------------
Write-Section "Aggregating results"

$byModule = @{}
foreach ($r in $results) {
    $mod = $r.module
    if (-not $byModule.ContainsKey($mod)) {
        $byModule[$mod] = @{ benches = @(); ok = 0; failed = 0; timeout = 0 }
    }
    $byModule[$mod].benches += $r
    switch ($r.status) {
        "OK"      { $byModule[$mod].ok++      }
        "FAILED"  { $byModule[$mod].failed++  }
        "TIMEOUT" { $byModule[$mod].timeout++ }
        "ERROR"   { $byModule[$mod].failed++  }
    }
}

# Build aggregate JSON
$aggregate = [ordered]@{
    run_timestamp = $Timestamp
    preset        = $Preset
    min_time      = $MinTime
    total_benches = $results.Count
    succeeded     = $succeeded
    failed        = $failed
    timed_out     = $timedOut
    modules       = [ordered]@{}
    results       = @($results)
}

foreach ($mod in ($byModule.Keys | Sort-Object)) {
    $entry = $byModule[$mod]
    # Collect all mean measurements across all benchmarks in this module
    $allCases = @($entry.benches | ForEach-Object { $_.cases } | Where-Object { $_ })
    $aggregate.modules[$mod] = [ordered]@{
        ok        = $entry.ok
        failed    = $entry.failed
        timeout   = $entry.timeout
        cases     = $allCases.Count
        benchmarks = @($entry.benches | Select-Object -Property name, status, elapsed)
    }
}

$aggregateFile = Join-Path $OutputDir "bench_aggregate_$Timestamp.json"
$aggregate | ConvertTo-Json -Depth 10 | Set-Content $aggregateFile -Encoding UTF8
Write-Ok "Aggregate JSON: $aggregateFile"

# ---------------------------------------------------------------------------
# Generate Markdown performance report
# ---------------------------------------------------------------------------
$reportFile = Join-Path $OutputDir "PERF_REPORT_$Timestamp.md"

$md = [System.Text.StringBuilder]::new()
[void]$md.AppendLine("# ThemisDB Performance Report")
[void]$md.AppendLine("")
[void]$md.AppendLine("**Date:** $(Get-Date -Format 'yyyy-MM-dd HH:mm')")
[void]$md.AppendLine("**Preset:** $Preset")
[void]$md.AppendLine("**Min time per case:** $MinTime")
[void]$md.AppendLine("")
[void]$md.AppendLine("## Summary")
[void]$md.AppendLine("")
[void]$md.AppendLine("| Metric | Value |")
[void]$md.AppendLine("|--------|-------|")
[void]$md.AppendLine("| Total benchmarks run | $($results.Count) |")
[void]$md.AppendLine("| Succeeded | $succeeded |")
[void]$md.AppendLine("| Failed | $failed |")
[void]$md.AppendLine("| Timed out (>${MaxTimePerBench}s) | $timedOut |")
[void]$md.AppendLine("| Modules covered | $($byModule.Keys.Count) |")
[void]$md.AppendLine("")

# Coverage table
[void]$md.AppendLine("## Module Coverage")
[void]$md.AppendLine("")
[void]$md.AppendLine("| Module | Benches | OK | Failed | Timeout | Cases |")
[void]$md.AppendLine("|--------|---------|----|--------|---------|-------|")
foreach ($mod in ($byModule.Keys | Sort-Object)) {
    $entry = $byModule[$mod]
    $total = $entry.ok + $entry.failed + $entry.timeout
    $allCases = @($entry.benches | ForEach-Object { $_.cases } | Where-Object { $_ })
    $statusIcon = if ($entry.failed -gt 0 -or $entry.timeout -gt 0) { "WARN" } else { "OK" }
    [void]$md.AppendLine("| $mod | $total | $($entry.ok) | $($entry.failed) | $($entry.timeout) | $($allCases.Count) |")
}
[void]$md.AppendLine("")

# Per-module detail sections
[void]$md.AppendLine("## Detailed Results by Module")
[void]$md.AppendLine("")

foreach ($mod in ($byModule.Keys | Sort-Object)) {
    $entry = $byModule[$mod]
    [void]$md.AppendLine("### $mod")
    [void]$md.AppendLine("")

    foreach ($r in $entry.benches) {
        $statusLabel = switch ($r.status) {
            "OK"      { "OK" }
            "FAILED"  { "FAILED" }
            "TIMEOUT" { "TIMEOUT" }
            default   { $r.status }
        }
        [void]$md.AppendLine("**$($r.name)** - $statusLabel ($($r.elapsed)s)")
        [void]$md.AppendLine("")

        if ($r.cases.Count -gt 0) {
            [void]$md.AppendLine("| Benchmark Case | Real Time | CPU Time | Unit | Iterations |")
            [void]$md.AppendLine("|----------------|-----------|----------|------|------------|")
            foreach ($c in $r.cases) {
                $caseName = $c.case -replace "_mean$", ""
                [void]$md.AppendLine("| $caseName | $($c.real_time) | $($c.cpu_time) | $($c.unit) | $($c.iters) |")
            }
            [void]$md.AppendLine("")
        }
    }
}

# Modules with NO benchmark results yet
$allSrcModules = @(
    "acceleration","ai","analytics","api","aql","auth","base","cache","cdc","chaos",
    "chimera","config","content","core","distributed_knowledge","document","ethics_ai",
    "exporters","failover","geo","governance","gpu","graph","importers","index",
    "ingestion","llama_cpp","llm","maintenance","metadata","network","observability",
    "onnx_clip","performance","plugins","process","projects","prompt_engineering",
    "query","rag","replication","rpc_grpc","scheduler","scraper","search","security",
    "server","sharding","stable_diffusion","storage","temporal","tensor","themis",
    "timeseries","toolbox","training","transaction","updates","user_storage_encrypted",
    "utils","voice","whisper"
)
$notCovered = @($allSrcModules | Where-Object { -not $byModule.ContainsKey($_) })

if ($notCovered.Count -gt 0) {
    [void]$md.AppendLine("## Modules Without Benchmark Coverage")
    [void]$md.AppendLine("")
    [void]$md.AppendLine("The following src/ modules had no benchmark executable run in this batch:")
    [void]$md.AppendLine("")
    foreach ($m in $notCovered) {
        [void]$md.AppendLine("- $m")
    }
    [void]$md.AppendLine("")
}

$md.ToString() | Set-Content $reportFile -Encoding UTF8
Write-Ok "Markdown report : $reportFile"

# ---------------------------------------------------------------------------
# Final summary
# ---------------------------------------------------------------------------
Write-Section "Done"
Write-Host ""
Write-Host "  Succeeded : $succeeded"  -ForegroundColor Green
if ($failed  -gt 0) { Write-Host "  Failed    : $failed"   -ForegroundColor Red    }
if ($timedOut -gt 0) { Write-Host "  Timed out : $timedOut" -ForegroundColor Yellow }
Write-Host ""
Write-Host "  Output dir: $OutputDir"
Write-Host "  Report    : $reportFile"
Write-Host ""
