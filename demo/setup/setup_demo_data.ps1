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
    [string]$ThemisctlPath = "",
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

function Resolve-ThemisctlPath {
    $candidates = @(
        ".\build-msvc-windows-release\bin\themisctl.exe",
        ".\build\windows-release\bin\themisctl.exe",
        ".\build\msvc-ninja-release\bin\themisctl.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Import-JsonlViaBatchInsert {
    param(
        [string]$Collection,
        [string]$FilePath,
        [switch]$Edges
    )

    if (-not (Test-Path $FilePath)) {
        throw "File not found: $FilePath"
    }

    $lineCount = (Get-Content $FilePath | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Measure-Object).Count
    if ($lineCount -eq 0) {
        return 0
    }

    $maxRetries = 8
    $attempt = 0
    while ($attempt -lt $maxRetries) {
        $attempt++

        if ($Edges) {
            $batchOutput = Get-Content $FilePath | & $ThemisctlPath --host $ServerHost --port $ServerPort batch-insert --collection $Collection --edges --batch-size 200 2>&1
        } else {
            $batchOutput = Get-Content $FilePath | & $ThemisctlPath --host $ServerHost --port $ServerPort batch-insert --collection $Collection --batch-size 200 2>&1
        }

        if ($LASTEXITCODE -eq 0) {
            return $lineCount
        }

        $msg = ($batchOutput | Out-String).Trim()
        if ($msg -match 'HTTP 429|Too Many Requests') {
            $retryAfter = 1
            if ($msg -match 'retry_after_seconds"\s*:\s*(\d+)') {
                $retryAfter = [Math]::Max([int]$Matches[1], 1)
            }
            Start-Sleep -Seconds ($retryAfter + 1)
            continue
        }

        if ([string]::IsNullOrWhiteSpace($msg)) {
            throw "batch-insert failed for collection $Collection"
        }

        throw "batch-insert failed for collection ${Collection}: $msg"
    }

    throw "batch-insert failed for collection $Collection after $maxRetries retries"
}

function Test-KeyAccessible {
    param(
        [string]$Key
    )

    $maxRetries = 10
    $attempt = 0

    while ($attempt -lt $maxRetries) {
        $attempt++
        $out = & $ThemisctlPath --host $ServerHost --port $ServerPort get $Key 2>&1
        if ($LASTEXITCODE -eq 0) {
            return $true
        }

        $msg = ($out | Out-String).Trim()
        if ($msg -match 'HTTP 429|Too Many Requests') {
            $retryAfter = 1
            if ($msg -match 'retry_after_seconds"\s*:\s*(\d+)') {
                $retryAfter = [Math]::Max([int]$Matches[1], 1)
            }
            Start-Sleep -Seconds ($retryAfter + 1)
            continue
        }

        break
    }

    return $false
}

# =============================================================================
# SETUP
# =============================================================================

Write-Header "ThemisDB Demo Data Setup"

if ([string]::IsNullOrWhiteSpace($ThemisctlPath)) {
    $ThemisctlPath = Resolve-ThemisctlPath
}

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

$articlesImported = 0
$embeddingsImported = 0
$nodesImported = 0
$edgesImported = 0

# Import Articles
Write-Info "  → Importing demo_articles collection..."
$articlesFile = "$DataDir\demo_articles.jsonl"
if (Test-Path $articlesFile) {
    try {
        $imported = Import-JsonlViaBatchInsert -Collection "demo_articles" -FilePath $articlesFile
        $articlesImported = $imported
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
    try {
        $imported = Import-JsonlViaBatchInsert -Collection "demo_embeddings" -FilePath $embeddingsFile
        $embeddingsImported = $imported
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
    try {
        $imported = Import-JsonlViaBatchInsert -Collection "demo_knowledge_graph" -FilePath $nodesFile
        $nodesImported = $imported
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
    try {
        $imported = Import-JsonlViaBatchInsert -Collection "demo_knowledge_graph" -FilePath $edgesFile -Edges
        $edgesImported = $imported
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
    if (Test-KeyAccessible -Key "demo_articles:art_0001") {
        Write-Success "    ✓ demo_articles collection is accessible"
    } else {
        throw "demo_articles sample lookup failed"
    }

    Write-Info "  → Reading demo_embeddings sample..."
    if (Test-KeyAccessible -Key "demo_embeddings:vec_0001") {
        Write-Success "    ✓ demo_embeddings collection is accessible"
    } else {
        throw "demo_embeddings sample lookup failed"
    }

    Write-Info "  → Reading demo_knowledge_graph sample..."
    if (Test-KeyAccessible -Key "demo_knowledge_graph:node_0001") {
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
Write-Host "  • demo_articles ($articlesImported administrative case documents)" -ForegroundColor Gray
Write-Host "  • demo_embeddings ($embeddingsImported vectors, 128 dimensions)" -ForegroundColor Gray
Write-Host "  • demo_knowledge_graph ($nodesImported nodes + $edgesImported edges)" -ForegroundColor Gray
Write-Host ""
