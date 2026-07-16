# Performance Logger Framework
# Purpose: Collect, structure, and analyze benchmark metrics from all ThemisDB suites
# Version: 1.0
# Date: 2026-05-10

param(
    [Parameter(Mandatory=$false)]
    [string]$OutputDir = "./tmp",
    
    [Parameter(Mandatory=$false)]
    [string]$BuildDir = "./build/windows-release",
    
    [Parameter(Mandatory=$false)]
    [int]$Runs = 5,
    
    [Parameter(Mandatory=$false)]
    [int]$WarmupIters = 100,
    
    [Parameter(Mandatory=$false)]
    [switch]$IncludeInference
)

# ============================================================================
# Performance Logger Class
# ============================================================================

class PerformanceLogger {
    [string]$OutputDir
    [string]$BuildDir
    [int]$Runs
    [int]$WarmupIters
    [hashtable]$Results = @{}
    [System.Collections.ArrayList]$LogEntries = @()
    [string]$SessionId
    [datetime]$StartTime
    
    PerformanceLogger([string]$outDir, [string]$buildDir, [int]$runs, [int]$warmup) {
        $this.OutputDir = $outDir
        $this.BuildDir = $buildDir
        $this.Runs = $runs
        $this.WarmupIters = $warmup
        $this.SessionId = [System.DateTime]::Now.ToString("yyyyMMdd_HHmmss")
        $this.StartTime = [System.DateTime]::Now
        
        if (-not (Test-Path $outDir)) {
            New-Item -ItemType Directory -Path $outDir | Out-Null
        }
        
        $this.Log("Performance Logger initialized: SessionId=$($this.SessionId)")
    }
    
    [void]Log([string]$message) {
        $timestamp = [System.DateTime]::Now.ToString("yyyy-MM-dd HH:mm:ss.fff")
        $logEntry = @{
            Timestamp = $timestamp
            Message = $message
            SessionId = $this.SessionId
        }
        $this.LogEntries.Add($logEntry) | Out-Null
        Write-Host "[$timestamp] $message" -ForegroundColor Cyan
    }
    
    [void]LogError([string]$message) {
        $timestamp = [System.DateTime]::Now.ToString("yyyy-MM-dd HH:mm:ss.fff")
        Write-Host "[$timestamp] ERROR: $message" -ForegroundColor Red
        $this.LogEntries.Add(@{
            Timestamp = $timestamp
            Message = "ERROR: $message"
            SessionId = $this.SessionId
        }) | Out-Null
    }
    
    [void]LogSuccess([string]$message) {
        $timestamp = [System.DateTime]::Now.ToString("yyyy-MM-dd HH:mm:ss.fff")
        Write-Host "[$timestamp] ✅ $message" -ForegroundColor Green
        $this.LogEntries.Add(@{
            Timestamp = $timestamp
            Message = $message
            SessionId = $this.SessionId
        }) | Out-Null
    }
    
    [hashtable]RunBenchmarkSuite([string]$suiteName, [string]$binaryPath, [string]$filter) {
        $this.Log("Starting benchmark suite: $suiteName")
        
        if (-not (Test-Path $binaryPath)) {
            $this.LogError("Binary not found: $binaryPath")
            return @{Status = "FAILED"; Error = "Binary not found" }
        }
        
        # Set environment variables
        $env:THEMIS_BENCH_RUNS = $this.Runs
        $env:THEMIS_BENCH_WARMUP_ITERS = $this.WarmupIters
        
        $logFile = "$($this.OutputDir)/$($suiteName)_$($this.SessionId).txt"
        $metricsFile = "$($this.OutputDir)/$($suiteName)_$($this.SessionId).json"
        
        try {
            $methodStartTime = [System.DateTime]::Now
            
            # Run the benchmark and capture output
            $output = & $binaryPath --gtest_filter=$filter 2>&1 | Tee-Object -FilePath $logFile
            
            $methodEndTime = [System.DateTime]::Now
            $duration = ($methodEndTime - $methodStartTime).TotalSeconds
            
            # Parse output for metrics
            $result = @{
                SuiteName = $suiteName
                Status = "COMPLETED"
                Duration_Sec = $duration
                LogFile = $logFile
                MetricsFile = $metricsFile
                TestOutput = $output
            }
            
            # Extract key metrics
            if ($output -match "PASSED.*(\d+) tests") {
                $result.TestsPassed = [int]$matches[1]
            }
            if ($output -match "SKIPPED.*(\d+) tests") {
                $result.TestsSkipped = [int]$matches[1]
            }
            if ($output -match "Running (\d+) tests") {
                $result.TestsTotal = [int]$matches[1]
            }
            
            # Extract percentile values (e.g., "p95: 0.0005ms")
            $percentiles = @()
            $output | Select-String "p95|p99|percentile" -AllMatches | ForEach-Object {
                $percentiles += $_.Line
            }
            $result.PercentileMetrics = $percentiles
            
            # Save metrics to JSON
            $result | ConvertTo-Json | Out-File -FilePath $metricsFile -Encoding UTF8
            
            $this.LogSuccess("Benchmark suite completed: $suiteName (${duration}s)")
            $this.Results[$suiteName] = $result
            
            return $result
        }
        catch {
            $this.LogError("Exception during benchmark execution: $_")
            return @{Status = "FAILED"; Error = $_.Exception.Message }
        }
    }
    
    [void]GenerateSummaryReport() {
        $this.Log("Generating performance summary report")
        
        $reportFile = "$($this.OutputDir)/PERFORMANCE_SUMMARY_$($this.SessionId).md"
        $content = @"
# ThemisDB Performance Logging Report
**Session ID:** $($this.SessionId)
**Date:** $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
**Build Directory:** $($this.BuildDir)
**Output Directory:** $($this.OutputDir)
**Policy Configuration:** Runs=$($this.Runs), WarmupIterations=$($this.WarmupIters)

## Execution Summary

| Suite | Status | Tests | Duration (s) | Log File |
|-------|--------|-------|--------------|----------|
"@
        
        foreach ($suite in $this.Results.GetEnumerator()) {
            $name = $suite.Name
            $status = $suite.Value.Status
            $tests = if ($suite.Value.TestsTotal) { $suite.Value.TestsTotal } else { "N/A" }
            $duration = if ($suite.Value.Duration_Sec) { $suite.Value.Duration_Sec } else { "N/A" }
            $logFile = Split-Path -Leaf $suite.Value.LogFile
            
            $content += "`n| $name | $status | $tests | $duration | $logFile |"
        }
        
        $content += "`n`n## Detailed Results`n`n"
        
        foreach ($suite in $this.Results.GetEnumerator()) {
            $name = $suite.Name
            $result = $suite.Value
            
            $content += "### $name`n"
            $content += "- **Status:** $($result.Status)`n"
            $content += "- **Tests Passed:** $(if ($result.TestsPassed) { $result.TestsPassed } else { 'N/A' })`n"
            $content += "- **Tests Skipped:** $(if ($result.TestsSkipped) { $result.TestsSkipped } else { 'N/A' })`n"
            $content += "- **Duration:** $($result.Duration_Sec) seconds`n"
            $content += "- **Log File:** $($result.LogFile)`n`n"
            
            if ($result.PercentileMetrics) {
                $content += "**Percentile Metrics:**`n"
                $result.PercentileMetrics | ForEach-Object {
                    $content += "- $_`n"
                }
                $content += "`n"
            }
        }
        
        $content | Out-File -FilePath $reportFile -Encoding UTF8
        $this.LogSuccess("Summary report saved: $reportFile")
    }
    
    [void]ExportMetricsJSON() {
        $exportFile = "$($this.OutputDir)/METRICS_EXPORT_$($this.SessionId).json"
        
        $export = @{
            SessionId = $this.SessionId
            Timestamp = $this.StartTime
            Configuration = @{
                Runs = $this.Runs
                WarmupIterations = $this.WarmupIters
                BuildDirectory = $this.BuildDir
                OutputDirectory = $this.OutputDir
            }
            Results = $this.Results
        }
        
        $export | ConvertTo-Json -Depth 10 | Out-File -FilePath $exportFile -Encoding UTF8
        $this.LogSuccess("Metrics exported to JSON: $exportFile")
    }
    
    [void]ExportLogCSV() {
        $csvFile = "$($this.OutputDir)/LOG_ENTRIES_$($this.SessionId).csv"
        
        $this.LogEntries | ConvertTo-Csv -NoTypeInformation | Out-File -FilePath $csvFile -Encoding UTF8
        $this.LogSuccess("Log entries exported to CSV: $csvFile")
    }
}

# ============================================================================
# Main Execution
# ============================================================================

$logger = [PerformanceLogger]::new($OutputDir, $BuildDir, $Runs, $WarmupIters)

$logger.Log("=" * 80)
$logger.Log("ThemisDB Performance Logging - Full Suite Execution")
$logger.Log("=" * 80)

# Run all benchmark suites
$benchmarks = @(
    @{
        Name = "SchedulerBenchmark"
        Binary = "$BuildDir/bin/bench_llm_continuous_batch_scheduler.exe"
        Filter = "SchedulerBenchmark.*"
    },
    @{
        Name = "WireProtocolBenchmark"
        Binary = "$BuildDir/bin/test_wire_perf_benchmark.exe"
        Filter = "WirePerf*.*"
    },
    @{
        Name = "EthicsAIBenchmark"
        Binary = "$BuildDir/bin/test_ethics_ai_benchmark.exe"
        Filter = "EthicsAIBenchmarkTests.*"
    },
    @{
        Name = "AllocatorBenchmark"
        Binary = "$BuildDir/bin/themis_tests.exe"
        Filter = "PerformanceAllocatorTest.PerformanceBenchmark"
    }
)

if ($IncludeInference) {
    $benchmarks += @{
        Name = "InferencePerformance"
        Binary = "$BuildDir/bin/themis_tests.exe"
        Filter = "InferencePerformanceTest.*"
    }
}

foreach ($benchmark in $benchmarks) {
    $result = $logger.RunBenchmarkSuite($benchmark.Name, $benchmark.Binary, $benchmark.Filter)
    
    if ($result.Status -eq "FAILED") {
        $logger.LogError("Benchmark suite failed: $($benchmark.Name)")
    }
    else {
        $logger.LogSuccess("Benchmark suite completed successfully: $($benchmark.Name)")
    }
    
    Start-Sleep -Milliseconds 500
}

# Generate reports
$logger.Log("Finalizing performance logging session")
$logger.GenerateSummaryReport()
$logger.ExportMetricsJSON()
$logger.ExportLogCSV()

$logger.Log("=" * 80)
$logger.Log("Performance Logging Session Complete")
$logger.Log("Session ID: $($logger.SessionId)")
$logger.Log("Output Directory: $($logger.OutputDir)")
$logger.Log("=" * 80)

Write-Host "✅ Performance logging complete. Results in: $OutputDir" -ForegroundColor Green
