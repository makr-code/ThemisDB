<#
.SYNOPSIS
    Run ThemisDB LLM inferencing benchmarks with downloaded models.

.DESCRIPTION
    This script runs comprehensive benchmarks for LLM inferencing in ThemisDB
    using models downloaded from Ollama.

.PARAMETER ModelPath
    Path to the GGUF model file (default: auto-detect from .\models)

.PARAMETER BenchmarkFilter
    Google Benchmark filter pattern (default: "LLMInferencing")

.PARAMETER BuildDir
    Build directory containing the benchmark executables (default: .\build-msvc)

.PARAMETER Iterations
    Number of benchmark iterations (default: 100)

.PARAMETER OutputReport
    Path to save the benchmark report (default: .\benchmark_llm_report.json)

.EXAMPLE
    .\run-llm-benchmarks.ps1 -ModelPath ".\models\llama3.2_1b.gguf"
    
.EXAMPLE
    .\run-llm-benchmarks.ps1 -BenchmarkFilter "LLMInferencing.*RAG" -Iterations 50
#>

param(
    [Parameter(Mandatory=$false)]
    [string]$ModelPath = "",
    
    [Parameter(Mandatory=$false)]
    [string]$BenchmarkFilter = "LLMInferencing",
    
    [Parameter(Mandatory=$false)]
    [string]$BuildDir = ".\build-msvc",
    
    [Parameter(Mandatory=$false)]
    [int]$Iterations = 100,
    
    [Parameter(Mandatory=$false)]
    [string]$OutputReport = ".\benchmark_llm_report.json"
)

$ErrorActionPreference = "Stop"

# Color output functions
function Write-Info { 
    param([string]$Message)
    Write-Host "INFO: $Message" -ForegroundColor Cyan 
}

function Write-Success { 
    param([string]$Message)
    Write-Host "SUCCESS: $Message" -ForegroundColor Green 
}

function Write-Warning-Custom { 
    param([string]$Message)
    Write-Host "WARNING: $Message" -ForegroundColor Yellow 
}

function Write-Error-Custom { 
    param([string]$Message)
    Write-Host "ERROR: $Message" -ForegroundColor Red 
}

# Auto-detect model if not specified
function Find-ModelFile {
    $modelsDir = ".\models"
    
    if (-not (Test-Path $modelsDir)) {
        return $null
    }
    
    $models = Get-ChildItem -Path $modelsDir -Filter "*.gguf" -File | Sort-Object Length
    
    if ($models.Count -eq 0) {
        return $null
    }
    
    # Prefer smaller models for faster benchmarking
    return $models[0].FullName
}

# Check if build exists
function Test-BuildExists {
    param([string]$BuildDir)
    
    $benchExe = Join-Path $BuildDir "Release\bench_comprehensive.exe"
    
    if (-not (Test-Path $benchExe)) {
        $benchExe = Join-Path $BuildDir "Release\bench_v1_3_0_features.exe"
    }
    
    if (-not (Test-Path $benchExe)) {
        return $null
    }
    
    return $benchExe
}

# Parse benchmark results
function ConvertFrom-BenchmarkOutput {
    param([string]$Output)
    
    $results = @()
    $lines = $Output -split "`n"
    
    $inResults = $false
    foreach ($line in $lines) {
        if ($line -match "^Benchmark\s+Time\s+CPU") {
            $inResults = $true
            continue
        }
        
        if ($inResults -and $line -match "^(\S+)\s+(\d+)\s+(\w+)\s+(\d+)\s+(\w+)") {
            $results += @{
                name = $matches[1]
                time = "$($matches[2]) $($matches[3])"
                cpu = "$($matches[4]) $($matches[5])"
            }
        }
    }
    
    return $results
}

# Generate HTML report
function New-HtmlReport {
    param(
        [array]$Results,
        [string]$ModelName,
        [string]$OutputPath
    )
    
    $html = @"
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>ThemisDB LLM Benchmark Report</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background: #f5f5f5;
        }
        h1 {
            color: #2c3e50;
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px;
        }
        .info-box {
            background: white;
            padding: 15px;
            margin: 20px 0;
            border-radius: 5px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        table {
            width: 100%;
            border-collapse: collapse;
            background: white;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        th, td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }
        th {
            background: #3498db;
            color: white;
            font-weight: bold;
        }
        tr:hover {
            background: #f5f5f5;
        }
        .metric {
            font-weight: bold;
            color: #27ae60;
        }
        .footer {
            margin-top: 30px;
            text-align: center;
            color: #7f8c8d;
            font-size: 0.9em;
        }
    </style>
</head>
<body>
    <h1>🚀 ThemisDB LLM Inferencing Benchmark Report</h1>
    
    <div class="info-box">
        <h2>📊 Test Configuration</h2>
        <p><strong>Model:</strong> $ModelName</p>
        <p><strong>Benchmark Suite:</strong> $BenchmarkFilter</p>
        <p><strong>Date:</strong> $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")</p>
        <p><strong>Iterations:</strong> $Iterations</p>
    </div>
    
    <div class="info-box">
        <h2>📈 Benchmark Results</h2>
        <table>
            <thead>
                <tr>
                    <th>Benchmark</th>
                    <th>Time</th>
                    <th>CPU</th>
                </tr>
            </thead>
            <tbody>
"@
    
    foreach ($result in $Results) {
        $html += @"
                <tr>
                    <td>$($result.name)</td>
                    <td class="metric">$($result.time)</td>
                    <td class="metric">$($result.cpu)</td>
                </tr>
"@
    }
    
    $html += @"
            </tbody>
        </table>
    </div>
    
    <div class="footer">
        <p>Generated by ThemisDB LLM Benchmark Suite</p>
        <p>ThemisDB v1.3.0+ with llama.cpp integration</p>
    </div>
</body>
</html>
"@
    
    $html | Out-File -FilePath $OutputPath -Encoding UTF8
}

# Main execution
function Main {
    Write-Host ""
    Write-Host "================================================" -ForegroundColor Magenta
    Write-Host "  ThemisDB LLM Inferencing Benchmarks" -ForegroundColor Magenta
    Write-Host "================================================" -ForegroundColor Magenta
    Write-Host ""
    
    # Find model
    if ([string]::IsNullOrEmpty($ModelPath)) {
        Write-Info "Auto-detecting model from .\models..."
        $ModelPath = Find-ModelFile
        
        if (-not $ModelPath) {
            Write-Error-Custom "No model found. Please run download-ollama-models.ps1 first"
            Write-Info "Example: .\scripts\download-ollama-models.ps1 -ModelNames @('llama3.2:1b')"
            exit 1
        }
        
        Write-Success "Found model: $ModelPath"
    }
    
    if (-not (Test-Path $ModelPath)) {
        Write-Error-Custom "Model file not found: $ModelPath"
        exit 1
    }
    
    $modelInfo = Get-Item $ModelPath
    $modelName = $modelInfo.BaseName
    $modelSizeMB = [math]::Round($modelInfo.Length / 1MB, 2)
    
    Write-Info "Model: $modelName ($modelSizeMB MB)"
    
    # Check build
    Write-Info "Checking for benchmark executables..."
    $benchExe = Test-BuildExists -BuildDir $BuildDir
    
    if (-not $benchExe) {
        Write-Error-Custom "Benchmark executable not found in $BuildDir"
        Write-Info "Please build ThemisDB with LLM support first:"
        Write-Host "  .\scripts\build-themis-server-llm.ps1" -ForegroundColor Yellow
        exit 1
    }
    
    Write-Success "Found benchmark executable: $benchExe"
    
    # Set environment variable for model path
    $env:THEMIS_LLM_MODEL_PATH = $ModelPath
    Write-Info "Set THEMIS_LLM_MODEL_PATH=$ModelPath"
    
    # Run benchmarks
    Write-Host ""
    Write-Info "Running benchmarks..."
    Write-Host "  Filter: $BenchmarkFilter" -ForegroundColor Gray
    Write-Host "  Iterations: $Iterations" -ForegroundColor Gray
    Write-Host ""
    
    $benchArgs = @(
        "--benchmark_filter=$BenchmarkFilter",
        "--benchmark_repetitions=$Iterations",
        "--benchmark_format=json",
        "--benchmark_out=$OutputReport"
    )
    
    Write-Host "Executing: $benchExe $($benchArgs -join ' ')" -ForegroundColor DarkGray
    Write-Host ""
    
    try {
        # Run benchmark and capture output
        $output = & $benchExe @benchArgs 2>&1 | Out-String
        
        Write-Host $output
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error-Custom "Benchmark execution failed with exit code: $LASTEXITCODE"
            exit $LASTEXITCODE
        }
        
        Write-Success "Benchmarks completed successfully"
        
        # Parse and display results
        if (Test-Path $OutputReport) {
            Write-Info "Benchmark results saved to: $OutputReport"
            
            # Generate HTML report
            $htmlReport = $OutputReport -replace "\.json$", ".html"
            $results = ConvertFrom-BenchmarkOutput -Output $output
            
            if ($results.Count -gt 0) {
                New-HtmlReport -Results $results -ModelName $modelName -OutputPath $htmlReport
                Write-Success "HTML report generated: $htmlReport"
            }
        }
        
    }
    catch {
        Write-Error-Custom "Failed to run benchmarks: $_"
        exit 1
    }
    
    # Summary
    Write-Host ""
    Write-Host "================================================" -ForegroundColor Magenta
    Write-Host "  Benchmark Summary" -ForegroundColor Magenta
    Write-Host "================================================" -ForegroundColor Magenta
    Write-Host ""
    Write-Success "✓ Model: $modelName"
    Write-Success "✓ Results: $OutputReport"
    if (Test-Path ($OutputReport -replace "\.json$", ".html")) {
        Write-Success "✓ HTML Report: $($OutputReport -replace '\.json$', '.html')"
    }
    Write-Host ""
    
    Write-Info "To view detailed results:"
    Write-Host "  cat $OutputReport | ConvertFrom-Json | Format-List" -ForegroundColor Gray
    Write-Host ""
}

# Run main function
Main
