<#
.SYNOPSIS
    Complete workflow: Download Ollama models and run ThemisDB benchmarks.

.DESCRIPTION
    This script automates the entire process:
    1. Check prerequisites (Ollama, ThemisDB build)
    2. Download specified models from Ollama
    3. Run comprehensive LLM inferencing benchmarks
    4. Generate reports

.PARAMETER Models
    Array of model names to download and benchmark (default: small models for testing)

.PARAMETER SkipDownload
    Skip model download if models already exist

.PARAMETER SkipBuild
    Skip build verification/build step

.PARAMETER BenchmarkIterations
    Number of iterations for each benchmark (default: 50)

.EXAMPLE
    # Quick test with small model
    .\setup-llm-benchmarks.ps1
    
.EXAMPLE
    # Custom models with more iterations
    .\setup-llm-benchmarks.ps1 -Models @("phi3:mini", "llama3.2:3b") -BenchmarkIterations 100
    
.EXAMPLE
    # Use existing models
    .\setup-llm-benchmarks.ps1 -SkipDownload
#>

param(
    [Parameter(Mandatory=$false)]
    [string[]]$Models = @("tinyllama:1.1b"),
    
    [Parameter(Mandatory=$false)]
    [switch]$SkipDownload = $false,
    
    [Parameter(Mandatory=$false)]
    [switch]$SkipBuild = $false,
    
    [Parameter(Mandatory=$false)]
    [int]$BenchmarkIterations = 50
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir

# Color output
function Write-Step { 
    param([string]$Message)
    Write-Host ""
    Write-Host "═══════════════════════════════════════════════" -ForegroundColor Magenta
    Write-Host "  $Message" -ForegroundColor Cyan
    Write-Host "═══════════════════════════════════════════════" -ForegroundColor Magenta
    Write-Host ""
}

function Write-Info { 
    param([string]$Message)
    Write-Host "➜ $Message" -ForegroundColor Cyan 
}

function Write-Success { 
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green 
}

function Write-Error-Custom { 
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor Red 
}

# Check Ollama
function Test-OllamaInstalled {
    try {
        $null = Get-Command ollama -ErrorAction Stop
        return $true
    }
    catch {
        Write-Warning "Ollama not available: $_"
        return $false
    }
}

# Check ThemisDB build
function Test-ThemisDBBuild {
    $buildPaths = @(
        "$RootDir\build-msvc\Release\themis_server.exe",
        "$RootDir\build-msvc\Release\bench_comprehensive.exe",
        "$RootDir\build-msvc\Release\bench_v1_3_0_features.exe"
    )
    
    foreach ($path in $buildPaths) {
        if (Test-Path $path) {
            return $true
        }
    }
    
    return $false
}

# Main workflow
Write-Host ""
Write-Host "╔══════════════════════════════════════════════════════════╗" -ForegroundColor Magenta
Write-Host "║  ThemisDB LLM Benchmark Setup & Execution Workflow      ║" -ForegroundColor Magenta
Write-Host "╚══════════════════════════════════════════════════════════╝" -ForegroundColor Magenta
Write-Host ""

# Step 1: Prerequisites check
Write-Step "STEP 1: Checking Prerequisites"

Write-Info "Checking Ollama installation..."
if (-not (Test-OllamaInstalled)) {
    Write-Error-Custom "Ollama is not installed"
    Write-Host ""
    Write-Host "Please install Ollama:" -ForegroundColor Yellow
    Write-Host "  Windows: https://ollama.ai/download" -ForegroundColor Gray
    Write-Host "  Or: choco install ollama" -ForegroundColor Gray
    exit 1
}
Write-Success "Ollama is installed"

Write-Info "Checking Ollama service..."
$ollamaRunning = $false
try {
    $response = Invoke-WebRequest -Uri "http://localhost:11434/api/tags" -TimeoutSec 3 -ErrorAction Stop
    $ollamaRunning = $response.StatusCode -eq 200
}
catch {
    Write-Error-Custom "Ollama service is not running"
    Write-Host ""
    Write-Host "Starting Ollama service..." -ForegroundColor Yellow
    Start-Process "ollama" -ArgumentList "serve" -WindowStyle Hidden
    Start-Sleep -Seconds 3
    
    try {
        $response = Invoke-WebRequest -Uri "http://localhost:11434/api/tags" -TimeoutSec 3 -ErrorAction Stop
        $ollamaRunning = $response.StatusCode -eq 200
    }
    catch {
        Write-Error-Custom "Failed to start Ollama service"
        Write-Host "Please start Ollama manually: ollama serve" -ForegroundColor Yellow
        exit 1
    }
}

if ($ollamaRunning) {
    Write-Success "Ollama service is running"
}

if (-not $SkipBuild) {
    Write-Info "Checking ThemisDB build..."
    if (-not (Test-ThemisDBBuild)) {
        Write-Error-Custom "ThemisDB build not found"
        Write-Host ""
        Write-Host "Building ThemisDB with LLM support..." -ForegroundColor Yellow
        
        $buildScript = Join-Path $ScriptDir "build-themis-server-llm.ps1"
        if (Test-Path $buildScript) {
            & $buildScript
            
            if ($LASTEXITCODE -ne 0) {
                Write-Error-Custom "Build failed"
                exit 1
            }
        }
        else {
            Write-Error-Custom "Build script not found: $buildScript"
            Write-Host "Please build manually:" -ForegroundColor Yellow
            Write-Host "  cmake -S . -B build-msvc -DTHEMIS_ENABLE_LLM=ON" -ForegroundColor Gray
            Write-Host "  cmake --build build-msvc --config Release" -ForegroundColor Gray
            exit 1
        }
    }
    Write-Success "ThemisDB build is ready"
}

# Step 2: Download models
if (-not $SkipDownload) {
    Write-Step "STEP 2: Downloading Models from Ollama"
    
    $downloadScript = Join-Path $ScriptDir "download-ollama-models.ps1"
    
    if (-not (Test-Path $downloadScript)) {
        Write-Error-Custom "Download script not found: $downloadScript"
        exit 1
    }
    
    Write-Info "Models to download: $($Models -join ', ')"
    
    & $downloadScript -ModelNames $Models -OutputDir "$RootDir\models"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Custom "Model download failed"
        exit 1
    }
    
    Write-Success "Models downloaded successfully"
}
else {
    Write-Step "STEP 2: Skipping Model Download"
    Write-Info "Using existing models in .\models"
}

# Step 3: Run benchmarks
Write-Step "STEP 3: Running LLM Inferencing Benchmarks"

$benchmarkScript = Join-Path $ScriptDir "run-llm-benchmarks.ps1"

if (-not (Test-Path $benchmarkScript)) {
    Write-Error-Custom "Benchmark script not found: $benchmarkScript"
    exit 1
}

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$reportPath = "$RootDir\benchmark_llm_report_$timestamp.json"

Write-Info "Running benchmarks (this may take several minutes)..."
Write-Host "  Iterations: $BenchmarkIterations" -ForegroundColor Gray
Write-Host "  Report: $reportPath" -ForegroundColor Gray

& $benchmarkScript `
    -BenchmarkFilter "LLMInferencing" `
    -Iterations $BenchmarkIterations `
    -OutputReport $reportPath

if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Benchmark execution failed"
    exit 1
}

Write-Success "Benchmarks completed successfully"

# Step 4: Summary
Write-Step "STEP 4: Summary & Results"

Write-Success "✓ Prerequisites verified"
Write-Success "✓ Models ready in .\models\"
Write-Success "✓ Benchmarks executed"

if (Test-Path $reportPath) {
    Write-Success "✓ Report generated: $reportPath"
    
    $htmlReport = $reportPath -replace "\.json$", ".html"
    if (Test-Path $htmlReport) {
        Write-Success "✓ HTML report: $htmlReport"
        Write-Host ""
        Write-Info "Opening HTML report in browser..."
        Start-Process $htmlReport
    }
}

Write-Host ""
Write-Host "═══════════════════════════════════════════════" -ForegroundColor Green
Write-Host "  Workflow Completed Successfully! 🎉" -ForegroundColor Green
Write-Host "═══════════════════════════════════════════════" -ForegroundColor Green
Write-Host ""

Write-Info "Next steps:"
Write-Host "  • View results: cat $reportPath | ConvertFrom-Json" -ForegroundColor Gray
Write-Host "  • Run more benchmarks: .\scripts\run-llm-benchmarks.ps1" -ForegroundColor Gray
Write-Host "  • Try different models: .\scripts\download-ollama-models.ps1 -ModelNames @('phi3:mini')" -ForegroundColor Gray
Write-Host ""
