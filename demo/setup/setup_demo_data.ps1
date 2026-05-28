#!/usr/bin/env pwsh
<#
.SYNOPSIS
    ThemisDB Kickstarter Demo Data Setup Script
    
.DESCRIPTION
    Generates comprehensive demo datasets and imports them into ThemisDB
    
.EXAMPLE
    .\setup_demo_data.ps1
#>

param(
    [string]$ThemisctlPath = ".\build\windows-release\bin\themisctl.exe",
    [string]$ServerHost = "127.0.0.1",
    [int]$ServerPort = 8765,
    [string]$DataDir = ".\demo\data"
)

$ErrorActionPreference = "Stop"

# Colors
function Write-Header {
    Write-Host "`n" -NoNewline
    Write-Host "===========================================" -ForegroundColor Blue
    Write-Host $args[0] -ForegroundColor Blue
    Write-Host "===========================================" -ForegroundColor Blue
    Write-Host ""
}

function Write-Section {
    Write-Host $args[0] -ForegroundColor Green
}

function Write-Info {
    Write-Host $args[0] -ForegroundColor Yellow
}

function Write-Success {
    Write-Host $args[0] -ForegroundColor Green
}

function Write-Error-Custom {
    Write-Host "ERROR: $($args[0])" -ForegroundColor Red
}

function Import-JsonlViaPut {
    param(
        [string]$Collection,
        [string]$FilePath,
        [string]$KeyPrefix
    )

    if (-not (Test-Path $FilePath)) {
        throw "File not found: $FilePath"
    }

    $lineNumber = 0
    foreach ($line in Get-Content $FilePath) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }

        $lineNumber++
        $key = "{0}:{1}{2:d4}" -f $Collection, $KeyPrefix, $lineNumber
        $payload = '{"blob":' + (ConvertTo-Json $line -Compress) + '}'

        & $ThemisctlPath --host $ServerHost --port $ServerPort put $key $payload 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "put failed for key $key"
        }
    }

    return $lineNumber
}

# =============================================================================
# SETUP
# =============================================================================

Write-Header "ThemisDB Demo Data Setup"

# Check themisctl exists
if (-not (Test-Path $ThemisctlPath)) {
    Write-Error-Custom "themisctl not found at: $ThemisctlPath"
    Write-Host "Build it first with: cmake --build --preset windows-release --target themisctl"
    exit 1
}

Write-Success "✓ themisctl found at $ThemisctlPath"

# Check server is running
Write-Section "[1] Checking ThemisDB Server..."
try {
    & $ThemisctlPath --host $ServerHost --port $ServerPort health 2>&1 | Out-Null
    Write-Success "✓ Server is running at ${ServerHost}:$ServerPort"
} catch {
    Write-Error-Custom "Cannot connect to server at ${ServerHost}:$ServerPort"
    Write-Host "Start the server first with: themisctl server --port 8765"
    exit 1
}

# =============================================================================
# GENERATE DATA
# =============================================================================

Write-Section "[2] Generating Demo Data..."

$pythonScript = ".\demo\setup\generate_demo_data.py"
if (-not (Test-Path $pythonScript)) {
    Write-Error-Custom "Python script not found: $pythonScript"
    exit 1
}

try {
    python $pythonScript
    Write-Success "✓ Demo data generated successfully"
} catch {
    Write-Error-Custom "Failed to generate demo data: $_"
    exit 1
}

# =============================================================================
# IMPORT DATA
# =============================================================================

Write-Section "[3] Importing Data into ThemisDB..."

# Import Articles
Write-Info "  → Importing demo_articles collection..."
$articlesFile = "$DataDir\demo_articles.jsonl"
if (Test-Path $articlesFile) {
    $articleCount = @(Get-Content $articlesFile | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count
    try {
        $imported = Import-JsonlViaPut -Collection "demo_articles" -FilePath $articlesFile -KeyPrefix "art_"
        Write-Success "    ✓ Imported $imported articles"
    } catch {
        Write-Error-Custom "Failed to import articles: $_"
    }
} else {
    Write-Error-Custom "Articles file not found: $articlesFile"
}

# Import Embeddings
Write-Info "  → Importing demo_embeddings collection..."
$embeddingsFile = "$DataDir\demo_embeddings.jsonl"
if (Test-Path $embeddingsFile) {
    $embeddingCount = @(Get-Content $embeddingsFile | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count
    try {
        $imported = Import-JsonlViaPut -Collection "demo_embeddings" -FilePath $embeddingsFile -KeyPrefix "vec_"
        Write-Success "    ✓ Imported $imported embeddings"
    } catch {
        Write-Error-Custom "Failed to import embeddings: $_"
    }
} else {
    Write-Error-Custom "Embeddings file not found: $embeddingsFile"
}

# Import Graph Nodes
Write-Info "  → Importing demo_knowledge_graph collection (nodes)..."
$nodesFile = "$DataDir\demo_knowledge_graph_nodes.jsonl"
if (Test-Path $nodesFile) {
    $nodeCount = @(Get-Content $nodesFile | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count
    try {
        $imported = Import-JsonlViaPut -Collection "demo_knowledge_graph" -FilePath $nodesFile -KeyPrefix "node_"
        Write-Success "    ✓ Imported $imported nodes"
    } catch {
        Write-Error-Custom "Failed to import graph nodes: $_"
    }
} else {
    Write-Error-Custom "Graph nodes file not found: $nodesFile"
}

# Import Graph Edges
Write-Info "  → Importing demo_knowledge_graph collection (edges)..."
$edgesFile = "$DataDir\demo_knowledge_graph_edges.jsonl"
if (Test-Path $edgesFile) {
    $edgeCount = @(Get-Content $edgesFile | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count
    try {
        $imported = Import-JsonlViaPut -Collection "demo_knowledge_graph" -FilePath $edgesFile -KeyPrefix "edge_"
        Write-Success "    ✓ Imported $imported edges"
    } catch {
        Write-Error-Custom "Failed to import graph edges: $_"
    }
} else {
    Write-Error-Custom "Graph edges file not found: $edgesFile"
}

# =============================================================================
# VERIFICATION
# =============================================================================

Write-Section "[4] Verifying Data..."
$verificationFailed = $false

try {
    Write-Info "  → Reading demo_articles sample..."
    & $ThemisctlPath --host $ServerHost --port $ServerPort get "demo_articles:art_0001" 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Success "    ✓ demo_articles collection is accessible"
    } else {
        throw "demo_articles sample lookup failed"
    }

    Write-Info "  → Reading demo_embeddings sample..."
    & $ThemisctlPath --host $ServerHost --port $ServerPort get "demo_embeddings:vec_0001" 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Success "    ✓ demo_embeddings collection is accessible"
    } else {
        throw "demo_embeddings sample lookup failed"
    }

    Write-Info "  → Reading demo_knowledge_graph sample..."
    & $ThemisctlPath --host $ServerHost --port $ServerPort get "demo_knowledge_graph:node_0001" 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Success "    ✓ demo_knowledge_graph collection is accessible"
    } else {
        throw "demo_knowledge_graph sample lookup failed"
    }
} catch {
    Write-Error-Custom "Verification failed: $_"
    $verificationFailed = $true
}

if ($verificationFailed) {
    Write-Error-Custom "Setup finished with verification errors"
    exit 1
}

# =============================================================================
# SUMMARY
# =============================================================================

Write-Header "Setup Complete!"
Write-Success "✓ All demo data has been generated and imported"
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Run the demo: .\demo\kickstarter_demo_script.ps1" -ForegroundColor Gray
Write-Host "2. Record your screen while the demo runs" -ForegroundColor Gray
Write-Host "3. See QUICKSTART.md for complete instructions" -ForegroundColor Gray
Write-Host ""
Write-Host "Data files location:" -ForegroundColor Cyan
Write-Host "  $DataDir" -ForegroundColor Gray
Write-Host ""
Write-Host "Collections created:" -ForegroundColor Cyan
Write-Host "  • demo_articles (13 research articles)" -ForegroundColor Gray
Write-Host "  • demo_embeddings (128-dimensional vectors)" -ForegroundColor Gray
Write-Host "  • demo_knowledge_graph (researchers, papers, conferences + relationships)" -ForegroundColor Gray
Write-Host ""
