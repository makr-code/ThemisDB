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
$SERVER_HOST = "127.0.0.1"
$DEMO_COLLECTION = "demo_articles"
$DEMO_GRAPH = "demo_knowledge_graph"
$DEMO_VECTORS = "demo_embeddings"
$LLM_TIMEOUT_SECONDS = 180
$DEMO_NO_PAUSE = ($env:THEMIS_DEMO_NO_PAUSE -eq "1")

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

function Write-Request {
    param(
        [string]$CommandText,
        [string]$Body = ""
    )

    Write-Host "[REQUEST] $CommandText" -ForegroundColor Magenta
    if (-not [string]::IsNullOrWhiteSpace($Body)) {
        Write-Host "          body: $Body" -ForegroundColor DarkMagenta
    }
    Write-Host ""
}

function Pause-DemoStep {
    param([string]$NextSection)

    if ($DEMO_NO_PAUSE) {
        if ([string]::IsNullOrWhiteSpace($NextSection)) {
            Write-Host "[AUTO] Continuing without key pause (THEMIS_DEMO_NO_PAUSE=1)." -ForegroundColor DarkGray
        } else {
            Write-Host "[AUTO] Continuing without key pause (next: $NextSection)." -ForegroundColor DarkGray
        }
        Write-Host ""
        return
    }

    if ([string]::IsNullOrWhiteSpace($NextSection)) {
        Write-Host "Press any key to continue..." -ForegroundColor DarkGray
    } else {
        Write-Host "Press any key to continue... (next: $NextSection)" -ForegroundColor DarkGray
    }

    try {
        [void][System.Console]::ReadKey($true)
    } catch {
        # Fallback for hosts where ReadKey is not available.
        Read-Host | Out-Null
    }
    Write-Host ""
}

function Test-FeatureReadiness {
    param(
        [string]$Name,
        [scriptblock]$Probe
    )

    Write-Host "[PRECHECK] $Name" -ForegroundColor Cyan
    $probeOutput = & $Probe 2>&1
    $ok = ($LASTEXITCODE -eq 0)

    if ($ok) {
        Write-Host "[PRECHECK] OK: $Name" -ForegroundColor Green
    } else {
        Write-Host "[PRECHECK] FAIL: $Name" -ForegroundColor Yellow
        if ($probeOutput) {
            $probeOutput | Select-Object -First 8 | ForEach-Object { Write-Host "  $_" -ForegroundColor DarkYellow }
        }
    }

    Write-Host ""
    return $ok
}

function Resolve-DemoLlmModelPath {
    if (-not [string]::IsNullOrWhiteSpace($env:THEMIS_DEMO_LLM_MODEL_PATH)) {
        if (Test-Path $env:THEMIS_DEMO_LLM_MODEL_PATH) {
            return $env:THEMIS_DEMO_LLM_MODEL_PATH
        }
        Write-Host "[PRECHECK] WARN: THEMIS_DEMO_LLM_MODEL_PATH gesetzt, aber Datei nicht gefunden: $($env:THEMIS_DEMO_LLM_MODEL_PATH)" -ForegroundColor Yellow
        return $null
    }

    $modelRoot = ".\models"
    if (Test-Path $modelRoot) {
        # Prefer known-good phi4 model for demo reliability.
        $preferred = Get-ChildItem -Path $modelRoot -Filter "*phi4*.gguf" -File -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($preferred) {
            Write-Host "[PRECHECK] INFO: Auto-selected default LLM model: $($preferred.FullName)" -ForegroundColor Cyan
            return $preferred.FullName
        }

        $fallback = Get-ChildItem -Path $modelRoot -Filter "*.gguf" -File -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($fallback) {
            Write-Host "[PRECHECK] INFO: Auto-selected fallback LLM model: $($fallback.FullName)" -ForegroundColor Cyan
            return $fallback.FullName
        }
    }

    Write-Host "[PRECHECK] INFO: Kein lokales GGUF-Modell gefunden (erwartet z. B. .\\models\\phi4.gguf)." -ForegroundColor DarkYellow

    return $null
}

function Try-AutoLoadLlmDefaultModel {
    param([string]$ModelPath)

    if ([string]::IsNullOrWhiteSpace($ModelPath)) {
        Write-Host "[PRECHECK] INFO: Kein lokales GGUF-Modell fuer Auto-Load gefunden." -ForegroundColor DarkYellow
        return $false
    }

    $body = @{ model_id = "default"; path = $ModelPath } | ConvertTo-Json -Compress
    Write-Host "[PRECHECK] Attempting auto-load of default LLM model..." -ForegroundColor Cyan
    Write-Host "[REQUEST] $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/llm/models/load" -ForegroundColor Magenta
    Write-Host "          body: $body" -ForegroundColor DarkMagenta
    Write-Host ""

    $body | & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/llm/models/load --stdin --content-type application/json
    $ok = ($LASTEXITCODE -eq 0)
    if ($ok) {
        Write-Host "[PRECHECK] OK: model auto-load succeeded." -ForegroundColor Green
    } else {
        Write-Host "[PRECHECK] FAIL: model auto-load failed." -ForegroundColor Yellow
    }
    Write-Host ""
    return $ok
}

function Resolve-DocsDatabasePath {
    $candidates = @(
        ".\data\docs_artifact.json",
        ".\data\docs_database.json",
        ".\data\docs.db",
        ".\docs_artifact.json",
        ".\docs_database.json",
        ".\docs.db"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

# ============================================================================
# PRE-FLIGHT: Server starten und Demo-Daten laden
# ============================================================================

$THEMIS_SERVER  = Resolve-ThemisBinary -BinaryName "themis_server.exe"
$SERVER_DB_PATH = ".\demo\data\themis_db"
$SERVER_PORT    = 8765
$serverProcess  = $null
$demoWarnings = New-Object System.Collections.Generic.List[string]

function Test-ServerRunning {
    try {
        $out = & $THEMISCTL --host $SERVER_HOST --port $SERVER_PORT health 2>&1
        return ($LASTEXITCODE -eq 0)
    } catch {
        return $false
    }
}

function Test-DemoDataPresent {
    $keys = @(
        "demo_articles:art_0001",
        "demo_embeddings:vec_0001",
        "demo_knowledge_graph:node_0001"
    )

    foreach ($key in $keys) {
        & $THEMISCTL --host $SERVER_HOST --port $SERVER_PORT get $key 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            return $false
        }
    }

    return $true
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
        & $setupScript -ThemisctlPath $THEMISCTL -ServerHost $SERVER_HOST -ServerPort $SERVER_PORT -DataDir ".\demo\data"
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[ABORT] setup_demo_data.ps1 failed." -ForegroundColor Red
            if ($serverProcess -and -not $serverProcess.HasExited) { $serverProcess.Kill() }
            exit 1
        }
    } else {
        Write-Host "[WARN] setup_demo_data.ps1 nicht gefunden – Demo laeuft ohne Beispieldaten." -ForegroundColor Yellow
    }
    Write-Host ""
} else {
    Write-Host "[PRE-FLIGHT] Server bereits erreichbar unter ${SERVER_HOST}:$SERVER_PORT." -ForegroundColor Green
    if (-not (Test-DemoDataPresent)) {
        Write-Host "[PRE-FLIGHT] Demo-Daten fehlen. Starte setup_demo_data.ps1..." -ForegroundColor Yellow
        $setupScript = ".\demo\setup\setup_demo_data.ps1"
        if (Test-Path $setupScript) {
            & $setupScript -ThemisctlPath $THEMISCTL -ServerHost $SERVER_HOST -ServerPort $SERVER_PORT -DataDir ".\demo\data"
            if ($LASTEXITCODE -ne 0) {
                Write-Host "[ABORT] setup_demo_data.ps1 failed." -ForegroundColor Red
                exit 1
            }
        } else {
            Write-Host "[ABORT] setup_demo_data.ps1 nicht gefunden." -ForegroundColor Red
            exit 1
        }
    }
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

$section6GraphExplainBody = '{"query_type":"k_hop","start_vertex":"demo_knowledge_graph:node_0001","max_depth":1}'
$llmInferenceBody = '{"prompt":"Summarize the impact of ACID transactions for distributed databases in two sentences.","max_tokens":64,"temperature":0.2}'
$ragQueryBody = '{"query":"What are the latest papers on quantum computing by MIT?","collection":"demo_articles","top_k":3,"max_tokens":96,"temperature":0.2}'
$docsHelpQueryBody = '{"query":"How do I configure sharding and RAG safely in ThemisDB?","user_id":"demo","max_tokens":64,"temperature":0.2}'

Write-Header "ThemisDB Demo - Runtime Pre-Checks"

$llmInferenceReady = Test-FeatureReadiness -Name "Section 5 LLM inference endpoint" -Probe {
    $llmInferenceBody | & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/llm/inference --stdin --content-type application/json
}

if (-not $llmInferenceReady) {
    $autoModelPath = Resolve-DemoLlmModelPath
    $autoLoadOk = Try-AutoLoadLlmDefaultModel -ModelPath $autoModelPath
    if ($autoLoadOk) {
        $llmInferenceReady = Test-FeatureReadiness -Name "Section 5 LLM inference endpoint (after auto-load)" -Probe {
            $llmInferenceBody | & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/llm/inference --stdin --content-type application/json
        }
    }
}

$section6ProbeReady = Test-FeatureReadiness -Name "Section 6 graph query explain endpoint" -Probe {
    $section6GraphExplainBody | & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/graph/query/explain --stdin --content-type application/json
}

$ragReady = Test-FeatureReadiness -Name "Section 7 RAG endpoint" -Probe {
    $ragQueryBody | & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/llm/rag --stdin --content-type application/json
}

$docsHelpReady = Test-FeatureReadiness -Name "Section 8 docs.db help endpoint" -Probe {
    $docsHelpQueryBody | & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/llm/docs/query --stdin --content-type application/json
}

if (-not $docsHelpReady) {
    $docsDbPath = Resolve-DocsDatabasePath
    if ([string]::IsNullOrWhiteSpace($docsDbPath)) {
        Write-Host "[PRECHECK] INFO: keine docs-Datenbank gefunden (erwartet z. B. .\\data\\docs_artifact.json oder .\\data\\docs_database.json)." -ForegroundColor DarkYellow
    } else {
        Write-Host "[PRECHECK] INFO: docs-Datenbank gefunden unter $docsDbPath, aber Endpoint bleibt nicht ready." -ForegroundColor DarkYellow
    }
    Write-Host ""
}

Write-Header "ThemisDB Multi-Model Database - Live Demo"

# ============================================================================
# SECTION 1: System Status & Schema
# ============================================================================
Write-Section "[1] Checking ThemisDB Server Status..."
Write-Request -CommandText "$THEMISCTL --host $SERVER_HOST --port $SERVER_PORT schema"
& $THEMISCTL --host $SERVER_HOST --port $SERVER_PORT schema 2>$null | Select-Object -First 20
Write-Info "Server is running and responding to queries."
Pause-DemoStep -NextSection "[2] Document Read-Back"

# ============================================================================
# SECTION 2: Document Read-Back (Compatibility Mode)
# ============================================================================
Write-Section "[2] Document Read-Back - Sample Entity"
Write-Host "Compatibility mode: reading imported demo document via primary key." -ForegroundColor Cyan
Write-Request -CommandText "$THEMISCTL --host $SERVER_HOST --port $SERVER_PORT get demo_articles:art_0001"
& $THEMISCTL --host $SERVER_HOST --port $SERVER_PORT get "demo_articles:art_0001"
if ($LASTEXITCODE -ne 0) {
    $demoWarnings.Add("Section 2 failed: demo_articles sample not readable")
}
Write-Host ""
Pause-DemoStep -NextSection "[3] Vector Payload Read-Back"

# ============================================================================
# SECTION 3: Vector Payload Read-Back (Compatibility Mode)
# ============================================================================
Write-Section "[3] Vector Payload Read-Back"
Write-Host "Compatibility mode: reading imported vector payload via key." -ForegroundColor Cyan
Write-Host ""
Write-Request -CommandText "$THEMISCTL --host $SERVER_HOST --port $SERVER_PORT get demo_embeddings:vec_0001"
& $THEMISCTL --host $SERVER_HOST --port $SERVER_PORT get "demo_embeddings:vec_0001"
if ($LASTEXITCODE -ne 0) {
    $demoWarnings.Add("Section 3 failed: demo_embeddings sample not readable")
}
Write-Host ""
Pause-DemoStep -NextSection "[4] Graph Node Read-Back"

# ============================================================================
# SECTION 4: Graph Node Read-Back (Compatibility Mode)
# ============================================================================
Write-Section "[4] Graph Node Read-Back"
Write-Host "Compatibility mode: reading imported graph node via key." -ForegroundColor Cyan
Write-Host ""
Write-Request -CommandText "$THEMISCTL --host $SERVER_HOST --port $SERVER_PORT get demo_knowledge_graph:node_0001"
& $THEMISCTL --host $SERVER_HOST --port $SERVER_PORT get "demo_knowledge_graph:node_0001"
if ($LASTEXITCODE -ne 0) {
    $demoWarnings.Add("Section 4 failed: demo_knowledge_graph sample not readable")
}
Write-Host ""
Pause-DemoStep -NextSection "[5] LLM Inference Probe"

# ============================================================================
# SECTION 5: LLM Inference Probe
# ============================================================================
Write-Section "[5] LLM Inference Probe"
Write-Host "Calling direct LLM inference endpoint with a short summarization task." -ForegroundColor Cyan
Write-Host ""
if (-not $llmInferenceReady) {
    Write-Host "[SKIP] Section 5 skipped by pre-check (LLM inference not ready)." -ForegroundColor Yellow
    $demoWarnings.Add("Section 5 skipped by pre-check: LLM inference endpoint unavailable on current build/runtime")
} else {
    Write-Request -CommandText "$THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/llm/inference" -Body $llmInferenceBody
    $llmInferenceBody | & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/llm/inference --stdin --content-type application/json
    if ($LASTEXITCODE -ne 0) {
        $demoWarnings.Add("Section 5 warning: LLM inference endpoint unavailable on current build/runtime")
        Write-Host "[INFO] Fetching LLM health diagnostics..." -ForegroundColor Yellow
        Write-Request -CommandText "$THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api GET /api/v1/llm/health"
        & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api GET /api/v1/llm/health
    }
}
Write-Host ""
Pause-DemoStep -NextSection "[6] Complex Query Probe"

# ============================================================================
# SECTION 6: Complex Query Probe
# ============================================================================
Write-Section "[6] Complex Query Probe"
Write-Host "Executing graph query planning probe via /api/v1/graph/query/explain (k_hop)." -ForegroundColor Cyan
Write-Host ""
if (-not $section6ProbeReady) {
    Write-Host "[SKIP] Section 6 skipped by pre-check (graph query explain endpoint not ready)." -ForegroundColor Yellow
    $demoWarnings.Add("Section 6 skipped by pre-check: graph query explain endpoint unavailable on current build/runtime")
} else {
    Write-Request -CommandText "$THEMISCTL --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/graph/query/explain" -Body $section6GraphExplainBody
    $section6GraphExplainBody | & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/graph/query/explain --stdin --content-type application/json
    if ($LASTEXITCODE -ne 0) {
        $demoWarnings.Add("Section 6 warning: graph query explain endpoint unavailable on current build/runtime")
    }
}
Write-Host ""
Pause-DemoStep -NextSection "[7] RAG Capability Probe"

# ============================================================================
# SECTION 7: RAG Capability Probe
# ============================================================================
Write-Section "[7] RAG Capability Probe"
Write-Host "Checking whether RAG endpoint is currently available on this build." -ForegroundColor Cyan
Write-Host ""
if (-not $ragReady) {
    Write-Host "[SKIP] Section 7 skipped by pre-check (RAG endpoint not ready)." -ForegroundColor Yellow
    $demoWarnings.Add("Section 7 skipped by pre-check: RAG endpoint unavailable on current build/runtime")
} else {
    Write-Request -CommandText "$THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/llm/rag" -Body $ragQueryBody
    $ragQueryBody | & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT api POST /api/v1/llm/rag --stdin --content-type application/json
    if ($LASTEXITCODE -ne 0) {
        $demoWarnings.Add("Section 7 warning: RAG endpoint unavailable on current build/runtime")
    }
}
Write-Host ""
Pause-DemoStep -NextSection "[8] Themis Help Probe"

# ============================================================================
# SECTION 8: Themis Help Probe (docs.db)
# ============================================================================
Write-Section "[8] Themis Help Probe (docs.db)"
Write-Host "Running ThemisDB-specific help mode (RAG/LLM/LoRA) backed by compiled docs.db." -ForegroundColor Cyan
Write-Host ""
if (-not $docsHelpReady) {
    Write-Host "[SKIP] Section 8 skipped by pre-check (docs.db help endpoint unavailable)." -ForegroundColor Yellow
    $demoWarnings.Add("Section 8 skipped by pre-check: docs.db help mode (lora) unavailable on current build/runtime")
} else {
    Write-Request -CommandText "$THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT help --mode lora \"How do I configure sharding and RAG safely in ThemisDB?\""
    & $THEMISCTL --timeout $LLM_TIMEOUT_SECONDS --host $SERVER_HOST --port $SERVER_PORT help --mode lora "How do I configure sharding and RAG safely in ThemisDB?"
    if ($LASTEXITCODE -ne 0) {
        $demoWarnings.Add("Section 8 warning: docs.db help mode (lora) unavailable on current build/runtime")
    }
}
Write-Host ""
Pause-DemoStep -NextSection "[9] CRUD Consistency Check"

# ============================================================================
# SECTION 9: CRUD Consistency Check
# ============================================================================
Write-Section "[9] CRUD Consistency Check"
Write-Host "Write a probe entity and read it back (compatibility check)." -ForegroundColor Cyan
Write-Host ""
$runtimeProbeBody = @{ blob = '{"title":"Runtime Probe","content":"Compatibility mode"}' } | ConvertTo-Json -Compress
Write-Request -CommandText "$THEMISCTL --host $SERVER_HOST --port $SERVER_PORT put demo_articles:runtime_probe" -Body $runtimeProbeBody
& $THEMISCTL --host $SERVER_HOST --port $SERVER_PORT put "demo_articles:runtime_probe" $runtimeProbeBody
if ($LASTEXITCODE -ne 0) {
    $demoWarnings.Add("Section 9 failed: runtime_probe write failed")
}
Write-Request -CommandText "$THEMISCTL --host $SERVER_HOST --port $SERVER_PORT get demo_articles:runtime_probe"
& $THEMISCTL --host $SERVER_HOST --port $SERVER_PORT get "demo_articles:runtime_probe"
if ($LASTEXITCODE -ne 0) {
    $demoWarnings.Add("Section 9 failed: runtime_probe readback failed")
}
Write-Host ""
Pause-DemoStep -NextSection "[10] System Performance Metrics"

# ============================================================================
# SECTION 10: Performance & Statistics
# ============================================================================
Write-Section "[10] System Performance Metrics"
Write-Request -CommandText "$THEMISCTL --host $SERVER_HOST --port $SERVER_PORT admin stats"
& $THEMISCTL --host $SERVER_HOST --port $SERVER_PORT admin stats | Select-String -Pattern "queries|throughput|latency|cache"
Write-Host ""
Pause-DemoStep -NextSection "[11] Automatic Index Recommendation"

# ============================================================================
# SECTION 11: Index Recommendations
# ============================================================================
Write-Section "[11] Automatic Index Recommendation"
Write-Host "ThemisDB analyzes query patterns and recommends optimizations:" -ForegroundColor Yellow
Write-Host ""
Write-Request -CommandText "$THEMISCTL --host $SERVER_HOST --port $SERVER_PORT index recommend $DEMO_COLLECTION"
& $THEMISCTL --host $SERVER_HOST --port $SERVER_PORT index recommend $DEMO_COLLECTION
Write-Host ""
Pause-DemoStep -NextSection "Closing Summary"

# ============================================================================
# CLOSING
# ============================================================================
Write-Header "Demo Complete!"
Write-Host "Key Features Demonstrated:" -ForegroundColor Green
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Document read-back via entity keys"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Vector payload read-back"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Graph payload read-back"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " RAG capability probe"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " LLM inference probe"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Graph query planning probe"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " docs.db help mode (RAG/LLM/LoRA)"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " CRUD consistency check"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Performance Analytics"
Write-Host "  " -NoNewline
Write-Host "✓" -ForegroundColor Green -NoNewline
Write-Host " Automatic Index Recommendations"
Write-Host ""
if ($demoWarnings.Count -eq 0) {
    Write-Host "ThemisDB demo checks passed. System appears operational for this scenario." -ForegroundColor Green
} else {
    $onlyDocsDbCaveat = ($demoWarnings.Count -eq 1 -and $demoWarnings[0].Contains("Section 8 skipped by pre-check"))
    Write-Host "Demo completed with caveats:" -ForegroundColor Yellow
    foreach ($w in $demoWarnings) {
        Write-Host "  - $w" -ForegroundColor Yellow
    }
    if ($onlyDocsDbCaveat) {
        Write-Host "Primary remaining blocker: docs database artifact for Section 8 (help mode)." -ForegroundColor Yellow
    }
    Write-Host 'ThemisDB is partially operational for this scenario. See warnings above.' -ForegroundColor Yellow
}
Write-Host ""
