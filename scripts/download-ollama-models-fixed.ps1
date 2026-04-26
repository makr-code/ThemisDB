<#
.SYNOPSIS
    Download and prepare Ollama models for ThemisDB LLM inferencing benchmarks.

.DESCRIPTION
    This script downloads models from Ollama, exports them to GGUF format,
    and stores them in .\models for use with ThemisDB's llama.cpp integration.

.PARAMETER ModelNames
    Array of Ollama model names to download (e.g., "llama3.2", "phi3", "mistral")

.PARAMETER OutputDir
    Directory to store the models (default: .\models)

.PARAMETER OllamaUrl
    Ollama API URL (default: http://localhost:11434)

.EXAMPLE
    .\download-ollama-models.ps1 -ModelNames @("llama3.2:1b", "phi3:mini")
    
.EXAMPLE
    .\download-ollama-models.ps1 -ModelNames @("mistral:7b") -OutputDir "C:\VCC\themis\models"
#>

param(
    [Parameter(Mandatory=$false)]
    [string[]]$ModelNames = @("llama3.2:1b", "phi3:mini", "tinyllama:1.1b"),
    
    [Parameter(Mandatory=$false)]
    [string]$OutputDir = ".\models",
    
    [Parameter(Mandatory=$false)]
    [string]$OllamaUrl = "http://localhost:11434"
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

# Check if Ollama is installed and running
function Test-OllamaAvailable {
    try {
        $response = Invoke-WebRequest -Uri "$OllamaUrl/api/tags" -Method Get -TimeoutSec 5 -ErrorAction Stop
        return $response.StatusCode -eq 200
    }
    catch {
        return $false
    }
}

# List available Ollama models
function Get-OllamaModels {
    try {
        $response = Invoke-RestMethod -Uri "$OllamaUrl/api/tags" -Method Get
        return $response.models
    }
    catch {
        Write-Error-Custom "Failed to get Ollama models: $_"
        return @()
    }
}

# Check if model is already available locally
function Test-OllamaModelExists {
    param(
        [string]$ModelName
    )
    
    try {
        $availableModels = Get-OllamaModels
        foreach ($model in $availableModels) {
            if ($model.name -eq $ModelName) {
                return $true
            }
        }
        return $false
    }
    catch {
        return $false
    }
}

# Pull a model from Ollama
function Get-OllamaModel {
    param(
        [string]$ModelName
    )
    
    # Check if model already exists locally
    if (Test-OllamaModelExists -ModelName $ModelName) {
        Write-Success "Model $ModelName already available locally (offline)"
        return $true
    }
    
    Write-Info "Pulling model: $ModelName (not found locally)"
    
    try {
        $body = @{ name = $ModelName } | ConvertTo-Json
        $uri = "$OllamaUrl/api/pull"
        
        # Stream the pull progress
        $webRequest = [System.Net.WebRequest]::Create($uri)
        $webRequest.Method = "POST"
        $webRequest.ContentType = "application/json"
        
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($body)
        $webRequest.ContentLength = $bytes.Length
        
        $requestStream = $webRequest.GetRequestStream()
        $requestStream.Write($bytes, 0, $bytes.Length)
        $requestStream.Close()
        
        $response = $webRequest.GetResponse()
        $reader = New-Object System.IO.StreamReader($response.GetResponseStream())
        
        $lastStatus = ""
        while (-not $reader.EndOfStream) {
            $line = $reader.ReadLine()
            if ($line) {
                $progress = $line | ConvertFrom-Json
                if ($progress.status -and $progress.status -ne $lastStatus) {
                    Write-Host "  $($progress.status)" -ForegroundColor Gray
                    $lastStatus = $progress.status
                }
            }
        }
        
        $reader.Close()
        $response.Close()
        
        Write-Success "Model $ModelName pulled successfully"
        return $true
    }
    catch {
        Write-Error-Custom "Failed to pull model $ModelName : $_"
        return $false
    }
}

# Export model to GGUF (Ollama stores models internally)
function Export-OllamaModelToGGUF {
    param(
        [string]$ModelName,
        [string]$OutputPath
    )
    
    Write-Info "Exporting model $ModelName to GGUF format..."
    
    # Ollama stores models in its own format
    # We need to find the model's blob storage location
    
    if ($IsWindows -or $env:OS -match "Windows") {
        $ollamaDir = "$env:USERPROFILE\.ollama\models"
    }
    else {
        $ollamaDir = "$env:HOME/.ollama/models"
    }
    
    Write-Info "Ollama model directory: $ollamaDir"
    
    # Parse model name and tag
    $parts = $ModelName -split ":"
    $modelBaseName = $parts[0]
    $modelTag = if ($parts.Count -gt 1) { $parts[1] } else { "latest" }
    
    # Find manifest file
    $manifestPath = Join-Path $ollamaDir "manifests\registry.ollama.ai\library\$modelBaseName\$modelTag"
    
    # Check if model exists locally first
    if (Test-OllamaModelExists -ModelName $ModelName) {
        Write-Info "Model found in local Ollama cache, copying directly..."
    }
    
    if (-not (Test-Path $manifestPath)) {
        Write-Warning-Custom "Manifest not found at $manifestPath"
        Write-Info "Attempting to locate model blobs by timestamp..."
        
        # Try to find the blob directly by looking for recently accessed/created files
        $blobsDir = Join-Path $ollamaDir "blobs"
        if (Test-Path $blobsDir) {
            # Get blobs sorted by write time (most recent first)
            $blobs = Get-ChildItem -Path $blobsDir -Filter "sha256-*" | 
                     Sort-Object LastWriteTime -Descending | 
                     Select-Object -First 10
            
            if ($blobs) {
                Write-Info "Found $($blobs.Count) recent blobs, selecting largest suitable one..."
                # Filter for blobs larger than 100MB (likely model weights, not config)
                $modelBlobs = $blobs | Where-Object { $_.Length -gt 100MB }
                
                if ($modelBlobs) {
                    $largestBlob = $modelBlobs | Sort-Object Length -Descending | Select-Object -First 1
                    
                    Write-Info "Selected blob: $($largestBlob.Name) ($([math]::Round($largestBlob.Length / 1MB, 2)) MB)"
                    Copy-Item -Path $largestBlob.FullName -Destination $OutputPath -Force
                    Write-Success "Exported model to: $OutputPath (offline copy)"
                    return $true
                }
                else {
                    Write-Warning-Custom "No suitable model blobs found (all < 100MB)"
                }
            }
        }
        
        return $false
    }
    
    try {
        $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
        
        # Find the model layer (usually the largest blob)
        $modelLayer = $manifest.layers | Sort-Object size -Descending | Select-Object -First 1
        
        if (-not $modelLayer) {
            Write-Error-Custom "No model layer found in manifest"
            return $false
        }
        
        $digest = $modelLayer.digest -replace ":", "-"
        $blobPath = Join-Path $ollamaDir "blobs\$digest"
        
        if (-not (Test-Path $blobPath)) {
            Write-Error-Custom "Model blob not found at: $blobPath"
            return $false
        }
        
        $blobSize = (Get-Item $blobPath).Length
        Write-Info "Copying model blob from: $blobPath ($([math]::Round($blobSize / 1MB, 2)) MB)"
        Copy-Item -Path $blobPath -Destination $OutputPath -Force
        
        Write-Success "Exported model to: $OutputPath (offline copy)"
        return $true
    }
    catch {
        Write-Error-Custom "Failed to export model: $_"
        return $false
    }
}

# Create model metadata file for ThemisDB
function New-ModelMetadata {
    param(
        [string]$ModelName,
        [string]$ModelPath,
        [long]$ModelSize
    )
    
    $metadataPath = "$ModelPath.json"
    
    $metadata = @{
        model_name = $ModelName
        model_type = "gguf"
        source = "ollama"
        download_date = (Get-Date).ToString("o")
        file_path = (Resolve-Path $ModelPath).Path
        file_size_bytes = $ModelSize
        file_size_mb = [math]::Round($ModelSize / 1MB, 2)
        themisdb_compatible = $true
        inference_backend = "llama.cpp"
    } | ConvertTo-Json -Depth 10
    
    $metadata | Out-File -FilePath $metadataPath -Encoding UTF8
    Write-Info "Created metadata file: $metadataPath"
}

# Create benchmark configuration
function New-BenchmarkConfig {
    param(
        [string]$OutputDir,
        [array]$Models
    )
    
    $configPath = Join-Path $OutputDir "benchmark_config.json"
    
    $config = @{
        benchmark_suite = "ThemisDB LLM Inferencing"
        created = (Get-Date).ToString("o")
        models = $Models | ForEach-Object {
            @{
                name = $_.name
                path = $_.path
                type = "gguf"
                backend = "llama.cpp"
            }
        }
        test_scenarios = @(
            @{
                name = "embedding_generation"
                description = "Generate embeddings and store in ThemisDB"
                benchmark_target = "LLMInferencingBench::EmbeddingGeneration_Store"
            },
            @{
                name = "rag_retrieval"
                description = "RAG search and retrieval (top-50)"
                benchmark_target = "LLMInferencingBench::RAG_Search_Retrieve_Top50"
            },
            @{
                name = "multi_query_expansion"
                description = "Multi-query expansion (5 queries)"
                benchmark_target = "LLMInferencingBench::MultiQueryExpansion_5Queries"
            }
        )
    } | ConvertTo-Json -Depth 10
    
    $config | Out-File -FilePath $configPath -Encoding UTF8
    Write-Success "Created benchmark config: $configPath"
}

# Main execution
function Main {
    Write-Host ""
    Write-Host "================================================" -ForegroundColor Magenta
    Write-Host "  ThemisDB Ollama Model Downloader" -ForegroundColor Magenta
    Write-Host "================================================" -ForegroundColor Magenta
    Write-Host ""
    
    # Check if Ollama is available
    Write-Info "Checking Ollama availability at $OllamaUrl..."
    if (-not (Test-OllamaAvailable)) {
        Write-Error-Custom "Ollama is not running or not accessible at $OllamaUrl"
        Write-Info "Please ensure Ollama is installed and running:"
        Write-Host "  - Windows: Download from https://ollama.ai" -ForegroundColor Gray
        Write-Host "  - Start Ollama service" -ForegroundColor Gray
        exit 1
    }
    Write-Success "Ollama is available"
    
    # Create output directory
    $resolvedOutputDir = Resolve-Path $OutputDir -ErrorAction SilentlyContinue
    if (-not $resolvedOutputDir) {
        New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
        $resolvedOutputDir = Resolve-Path $OutputDir
    }
    Write-Info "Output directory: $resolvedOutputDir"
    
    # List currently available models
    Write-Info "Fetching available Ollama models..."
    $availableModels = Get-OllamaModels
    if ($availableModels.Count -gt 0) {
        Write-Info "Found $($availableModels.Count) models in local Ollama registry"
        foreach ($model in ($availableModels | Select-Object -First 10)) {
            Write-Host "  - $($model.name)" -ForegroundColor Gray
        }
        if ($availableModels.Count -gt 10) {
            Write-Host "  ... (+$($availableModels.Count - 10) weitere)" -ForegroundColor DarkGray
        }
    }
    else {
        Write-Warning-Custom "No models currently registered in local Ollama cache"
    }

    Write-Host ""
    Write-Info "Starting download of $($ModelNames.Count) models..."
    Write-Host ""
    
    $exportedModels = @()
    
    foreach ($modelName in $ModelNames) {
        Write-Host "----------------------------------------" -ForegroundColor DarkGray
        Write-Info "Processing model: $modelName"
        
        # Pull model from Ollama
        $pullSuccess = Get-OllamaModel -ModelName $modelName
        
        if (-not $pullSuccess) {
            Write-Warning-Custom "Skipping $modelName due to pull failure"
            continue
        }
        
        # Export to GGUF
        $safeModelName = $modelName -replace "[:\\/]", "_"
        $outputPath = Join-Path $resolvedOutputDir "$safeModelName.gguf"
        
        $exportSuccess = Export-OllamaModelToGGUF -ModelName $modelName -OutputPath $outputPath
        
        if ($exportSuccess -and (Test-Path $outputPath)) {
            $fileInfo = Get-Item $outputPath
            New-ModelMetadata -ModelName $modelName -ModelPath $outputPath -ModelSize $fileInfo.Length
            
            $exportedModels += @{
                name = $modelName
                path = $outputPath
                size = $fileInfo.Length
            }
            
            Write-Success "Successfully processed: $modelName"
        }
        else {
            Write-Warning-Custom "Failed to export $modelName"
        }
        
        Write-Host ""
    }
    
    # Create benchmark configuration
    if ($exportedModels.Count -gt 0) {
        New-BenchmarkConfig -OutputDir $resolvedOutputDir -Models $exportedModels
    }
    
    # Summary
    Write-Host ""
    Write-Host "================================================" -ForegroundColor Magenta
    Write-Host "  Download Summary" -ForegroundColor Magenta
    Write-Host "================================================" -ForegroundColor Magenta
    Write-Host ""
    Write-Info "Total models requested: $($ModelNames.Count)"
    Write-Success "Successfully exported: $($exportedModels.Count)"
    Write-Host ""
    
    if ($exportedModels.Count -gt 0) {
        Write-Info "Exported models:"
        foreach ($model in $exportedModels) {
            $sizeMB = [math]::Round($model.size / 1MB, 2)
            Write-Host "  ✓ $($model.name) - $sizeMB MB" -ForegroundColor Green
            Write-Host "    -> $($model.path)" -ForegroundColor Gray
        }
        
        Write-Host ""
        Write-Info "Next steps:"
        Write-Host "  1. Build ThemisDB with LLM support:" -ForegroundColor Yellow
        Write-Host "     .\scripts\build-themis-server-llm.ps1" -ForegroundColor Gray
        Write-Host ""
        Write-Host "  2. Run benchmarks:" -ForegroundColor Yellow
        Write-Host "     cd build-msvc" -ForegroundColor Gray
        Write-Host "     .\Release\bench_comprehensive.exe --benchmark_filter=LLMInferencing" -ForegroundColor Gray
        Write-Host ""
        Write-Host "  3. Use in ThemisDB:" -ForegroundColor Yellow
        Write-Host '     themis_server --llm-model=models/yourmodel.gguf' -ForegroundColor Gray
    }
    
    Write-Host ""
}

# Run main function
Main
