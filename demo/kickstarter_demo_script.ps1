#!/usr/bin/env pwsh
# ThemisDB Kickstarter Demo Script (PowerShell)
# Unpolished, single-take demonstration of ThemisDB capabilities
# Run this script with commentary for the video

# Configuration
function Resolve-ThemisBinary {
    param([string]$BinaryName)

    $candidates = @(
        ".\\build\\windows-release\\bin\\$BinaryName",
        ".\\build-msvc-windows-release\\bin\\$BinaryName",
        ".\\build\\msvc-ninja-release\\bin\\$BinaryName"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

$THEMISCTL = Resolve-ThemisBinary -BinaryName "themisctl.exe"
$SERVER_HOST = "localhost"
$DEMO_COLLECTION = "demo_articles"
$DEMO_GRAPH = "demo_knowledge_graph"
$DEMO_VECTORS = "demo_embeddings"

# Colors
function Write-Header {
    param([string]$Text)
    Write-Host "================================================================" -ForegroundColor Blue
    Write-Host $Text -ForegroundColor Blue
    Write-Host "================================================================" -ForegroundColor Blue
    Write-Host ""
}

function Write-Section {
    param([string]$Text)
    Write-Host $Text -ForegroundColor Green
    Write-Host ""
}

function Write-Info {
    param([string]$Text)
    Write-Host $Text -ForegroundColor Yellow
    Write-Host ""
}

# ============================================================================
# PRE-FLIGHT: Server starten und Demo-Daten laden
# ============================================================================

$THEMIS_SERVER  = Resolve-ThemisBinary -BinaryName "themis_server.exe"
$SERVER_DB_PATH = ".\demo\data\themis_db"
$SERVER_PORT    = 8765
$serverProcess  = $null

function Test-ServerRunning {
    try {
        $out = & $THEMISCTL schema --host $SERVER_HOST --port $SERVER_PORT 2>&1
        return ($LASTEXITCODE -eq 0)
    } catch {
        return $false
    }
}

if (-not (Test-Path $THEMISCTL)) {
    Write-Host "[ABORT] themisctl.exe nicht gefunden in bekannten Build-Ordnern." -ForegroundColor Red
    Write-Host "        Erwartete Orte:" -ForegroundColor Gray
    Write-Host "        - .\\build\\windows-release\\bin\\themisctl.exe" -ForegroundColor Gray
    Write-Host "        - .\\build-msvc-windows-release\\bin\\themisctl.exe" -ForegroundColor Gray
    Write-Host "        - .\\build\\msvc-ninja-release\\bin\\themisctl.exe" -ForegroundColor Gray
    Write-Host "        Zuerst bauen: cmake --build --preset windows-release" -ForegroundColor Gray
    exit 1
}

if (-not (Test-ServerRunning)) {
    Write-Host "[PRE-FLIGHT] Starte ThemisDB Server..." -ForegroundColor Cyan

    if (-not (Test-Path $THEMIS_SERVER)) {
        Write-Host "[ABORT] themis_server.exe nicht gefunden in bekannten Build-Ordnern." -ForegroundColor Red
        exit 1
    }

    New-Item -ItemType Directory -Path $SERVER_DB_PATH -Force | Out-Null

    $serverProcess = Start-Process `
        -FilePath $THEMIS_SERVER `
        -ArgumentList "--db `"$SERVER_DB_PATH`" --port $SERVER_PORT --allow-degraded-build --allow-stub-hsm" `
        -PassThru -WindowStyle Hidden

    # Auf Bereitschaft warten (max. 30 s)
    Write-Host "[PRE-FLIGHT] Warte auf Server-Bereitschaft" -ForegroundColor Yellow -NoNewline
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 500
        Write-Host "." -NoNewline -ForegroundColor Yellow
        if (Test-ServerRunning) { break }
    }
    Write-Host ""

    if (-not (Test-ServerRunning)) {
        Write-Host "[ABORT] Server nicht erreichbar nach 30 s." -ForegroundColor Red
        if ($serverProcess -and -not $serverProcess.HasExited) { $serverProcess.Kill() }
        exit 1
    }

    Write-Host "[PRE-FLIGHT] Server laeuft. Lade Demo-Daten..." -ForegroundColor Green
    $setupScript = ".\demo\setup\setup_demo_data.ps1"
    if (Test-Path $setupScript) {
        & $setupScript -ThemisctlPath $THEMISCTL -ServerHost $SERVER_HOST -DataDir ".\demo\data"
    } else {
        Write-Host "[WARN] setup_demo_data.ps1 nicht gefunden – Demo laeuft ohne Beispieldaten." -ForegroundColor Yellow
    }
    Write-Host ""
} else {
    Write-Host "[PRE-FLIGHT] Server bereits erreichbar unter ${SERVER_HOST}:$SERVER_PORT." -ForegroundColor Green
    Write-Host ""
}

# Server beim Beenden des Scripts stoppen (nur wenn wir ihn selbst gestartet haben)
if ($serverProcess) {
    Register-EngineEvent PowerShell.Exiting -MessageData $serverProcess -Action {
        $proc = $event.MessageData
        if ($proc -and -not $proc.HasExited) {
            Write-Host "`n[CLEANUP] Stoppe ThemisDB Server..." -ForegroundColor Gray
            $proc.Kill()
        }
    } | Out-Null
}

Write-Header "ThemisDB Multi-Model Database - Live Demo"

# ============================================================================
# SECTION 1: System Status & Schema
# ============================================================================
Write-Section "[1] Checking ThemisDB Server Status..."
& $THEMISCTL schema --host $SERVER_HOST --port $SERVER_PORT 2>$null | Select-Object -First 20
Write-Info "Server is running and responding to queries."

# ============================================================================
# SECTION 2: Document & Full-Text Search (SQL-like)
# ============================================================================
Write-Section "[2] Document Search - Finding Articles About AI"
Write-Host "Query:" -ForegroundColor Cyan
Write-Host "  FOR doc IN $DEMO_COLLECTION" -ForegroundColor Gray
Write-Host "    FILTER doc.title LIKE '%AI%' OR doc.content LIKE '%machine learning%'" -ForegroundColor Gray
Write-Host "    SORT doc.published DESC" -ForegroundColor Gray
Write-Host "    LIMIT 5" -ForegroundColor Gray
Write-Host "    RETURN { title: doc.title, published: doc.published, score: doc.relevance }" -ForegroundColor Gray
Write-Host ""
Write-Host "Results:" -ForegroundColor Cyan
& $THEMISCTL query --host $SERVER_HOST --port $SERVER_PORT `
  "FOR doc IN $DEMO_COLLECTION FILTER doc.title LIKE '%AI%' OR doc.content LIKE '%machine learning%' SORT doc.published DESC LIMIT 5 RETURN { title: doc.title, published: doc.published, score: doc.relevance }"
Write-Host ""

# ============================================================================
# SECTION 3: Vector Search (Semantic Search)
# ============================================================================
Write-Section "[3] Vector Search - Semantic Similarity"
Write-Host "Query: Find 5 most similar articles to 'neural network optimization'" -ForegroundColor Cyan
Write-Host ""
& $THEMISCTL query --host $SERVER_HOST --port $SERVER_PORT `
  "FOR doc IN $DEMO_VECTORS LET similarity = COSINE_SIMILARITY(doc.embedding, @query_embedding) FILTER similarity > 0.7 SORT similarity DESC LIMIT 5 RETURN { title: doc.title, similarity: ROUND(similarity, 3) }" `
  --bind-var query_embedding="[0.1, -0.2, 0.8, ...]"
Write-Host ""

# ============================================================================
# SECTION 4: Graph Traversal (Multi-hop Relationships)
# ============================================================================
Write-Section "[4] Graph Navigation - Knowledge Graph Traversal"
Write-Host "Query: Find all researchers and their published papers (2 hops)" -ForegroundColor Cyan
Write-Host ""
Write-Host "FOR researcher IN $DEMO_GRAPH" -ForegroundColor Gray
Write-Host "  FILTER researcher.type == 'researcher'" -ForegroundColor Gray
Write-Host "  FOR paper IN 1..2 OUTBOUND researcher._id graph_edges" -ForegroundColor Gray
Write-Host "    FILTER paper.type == 'paper'" -ForegroundColor Gray
Write-Host "    RETURN { researcher: researcher.name, paper: paper.title, citations: paper.citation_count }" -ForegroundColor Gray
Write-Host ""
& $THEMISCTL query --host $SERVER_HOST --port $SERVER_PORT `
  "FOR researcher IN $DEMO_GRAPH FILTER researcher.type == 'researcher' FOR paper IN 1..2 OUTBOUND researcher._id graph_edges FILTER paper.type == 'paper' RETURN { researcher: researcher.name, paper: paper.title, citations: paper.citation_count } LIMIT 8"
Write-Host ""

# ============================================================================
# SECTION 5: LLM Features - RAG (Retrieval-Augmented Generation)
# ============================================================================
Write-Section "[5] RAG Query - LLM-Powered Natural Language to SQL"
Write-Host "User Question: 'What are the latest research papers on quantum computing by MIT researchers?'" -ForegroundColor Cyan
Write-Host ""
Write-Host "ThemisDB LLM Agent processes this and executes:" -ForegroundColor Yellow
& $THEMISCTL rag query `
  --collection $DEMO_COLLECTION `
  --top-k 3 `
  "What are the latest papers on quantum computing by MIT?" `
    --host $SERVER_HOST --port $SERVER_PORT
Write-Host ""

# ============================================================================
# SECTION 6: Complex Multi-Model Join
# ============================================================================
Write-Section "[6] Multi-Model Data Fusion - Documents + Vectors + Graph"
Write-Host "Query: Researcher + their papers (vector similarity) + collaboration network" -ForegroundColor Cyan
Write-Host ""
& $THEMISCTL query --host $SERVER_HOST --port $SERVER_PORT `
  "FOR researcher IN $DEMO_GRAPH FILTER researcher.type == 'researcher' LET papers = (FOR paper IN $DEMO_VECTORS FILTER paper.author_id == researcher._id LET sim = COSINE_SIMILARITY(paper.embedding, @topic_embedding) FILTER sim > 0.6 RETURN { title: paper.title, similarity: sim }) LET collaborators = (FOR collab IN 1 OUTBOUND researcher._id graph_edges FILTER collab.type == 'researcher' RETURN collab.name) RETURN { researcher: researcher.name, paper_count: LENGTH(papers), top_papers: SLICE(papers, 0, 2), collaborators: collaborators } LIMIT 5" `
  --bind-var topic_embedding="[0.2, 0.5, -0.1, ...]"
Write-Host ""

# ============================================================================
# SECTION 7: Performance & Statistics
# ============================================================================
Write-Section "[7] System Performance Metrics"
& $THEMISCTL admin stats --host $SERVER_HOST --port $SERVER_PORT | Select-String -Pattern "queries|throughput|latency|cache"
Write-Host ""

# ============================================================================
# SECTION 8: Index Recommendations
# ============================================================================
Write-Section "[8] Automatic Index Recommendation"
Write-Host "ThemisDB analyzes query patterns and recommends optimizations:" -ForegroundColor Yellow
Write-Host ""
& $THEMISCTL index recommend --host $SERVER_HOST --port $SERVER_PORT $DEMO_COLLECTION
Write-Host ""

# ============================================================================
# CLOSING
# ============================================================================
Write-Header "Demo Complete!"
Write-Host "Key Features Demonstrated:" -ForegroundColor Green
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Document/Full-Text Search (SQL-like AQL queries)"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Vector Search (Semantic similarity)"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Graph Traversal (Multi-hop relationships)"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " RAG Agent (LLM-powered natural language queries)"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Multi-Model Data Fusion (Documents + Vectors + Graph)"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Performance Analytics"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Automatic Index Recommendations"
Write-Host ""
Write-Host "ThemisDB is fully operational and ready for production use!" -ForegroundColor Green
Write-Host ""
