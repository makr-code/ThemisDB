<#
.SYNOPSIS
Bereitet die Demo-Umgebung für Kickstarter-Video vor.
Erstellt: Datenbank, Beispieldaten, Queries, Performance-Metriken.

.DESCRIPTION
Single-Script zum Initialisieren von ThemisDB mit:
  - Relational (Kunden, Produkte, Bestellungen)
  - Vector (Produktbeschreibungen mit Embeddings)
  - Graph (Beziehungen zwischen Entitäten)
  - LLM-Beispiel (lokale Inferenz)
  - RAG-Beispiel (Hallucination Detection)

.EXAMPLE
.\setup-demo.ps1 -BuildPreset windows-release -SkipClean

.PARAMETER BuildPreset
CMake Build-Preset (default: msvc-ninja-release)

.PARAMETER SkipClean
Überspringt Cleanup der alten Demo-Daten (für schnelles Restart)
#>

param(
    [string]$BuildPreset = "msvc-ninja-release",
    [switch]$SkipClean
)

$ThemisDBHome = (Split-Path -Parent $PSScriptRoot) | Split-Path -Parent

$ErrorActionPreference = "Stop"

# ─────────────────────────────────────────────────────────────────────
# 0. SETUP & VALIDATION
# ─────────────────────────────────────────────────────────────────────

Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║ ThemisDB Demo Setup für Kickstarter-Video               ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan

$DemoRoot = $PSScriptRoot | Split-Path
$SetupDir = "$DemoRoot\setup"
$QueryDir = "$DemoRoot\queries"
$DataDir = "$DemoRoot\data"
$VideoDir = "$DemoRoot\videos"
$BuildDir = "$ThemisDBHome\build-$BuildPreset"

Write-Host "[1/8] Validiere Verzeichnisse..." -ForegroundColor Yellow
if (-not (Test-Path $ThemisDBHome)) {
    Write-Host "   ✗ ThemisDB Root nicht gefunden: $ThemisDBHome" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $BuildDir)) {
    Write-Host "   ✓ Build-Verzeichnis nicht vorhanden." -ForegroundColor Yellow
    Write-Host "   ! Bitte zuerst ausführen:" -ForegroundColor Yellow
    Write-Host "     cmake --build --preset $BuildPreset" -ForegroundColor Gray
    exit 1
}

# ─────────────────────────────────────────────────────────────────────
# 1. CLEANUP (optional)
# ─────────────────────────────────────────────────────────────────────

Write-Host "[2/8] Bereinigung alter Daten..." -ForegroundColor Yellow

if (-not $SkipClean) {
    if (Test-Path "$DataDir\themis.db") {
        Remove-Item "$DataDir\themis.db" -Force -ErrorAction SilentlyContinue
            Write-Host "   OK Alte Datenbank entfernt" -ForegroundColor Green
        }
        if (Test-Path "$DataDir\*.wal") {
            Remove-Item "$DataDir\*.wal" -Force -ErrorAction SilentlyContinue
        }
    } else {
        Write-Host "   -- Cleanup uebersprungen (SkipClean)" -ForegroundColor Gray
# 2. CREATE SAMPLE DATA FILES
# ─────────────────────────────────────────────────────────────────────

Write-Host "[3/8] Erstelle Beispieldaten-Dateien..." -ForegroundColor Yellow

# Customers CSV
$customersCSV = @"
customer_id,name,email,country,signup_date
1,Alice Johnson,alice@example.com,DE,2024-01-15
2,Bob Schmidt,bob@example.com,AT,2024-02-20
3,Carol Chen,carol@example.com,CH,2024-03-10
4,David Müller,david@example.com,DE,2024-03-25
5,Emma Rossi,emma@example.com,IT,2024-04-05
"@

$customersCSV | Out-File "$DataDir\customers.csv" -Encoding UTF8
Write-Host "   ✓ customers.csv erstellt (5 Zeilen)" -ForegroundColor Green

# Products with descriptions for vector search
$productsJSON = @"
[
  {
    "product_id": "P001",
    "name": "Solar Panel 400W",
    "description": "High-efficiency monocrystalline solar panel with 400W peak power output, perfect for residential installations. Features tempered glass protection and aluminum frame.",
    "price": 249.99,
    "category": "Solar Energy",
    "stock": 150
  },
  {
    "product_id": "P002",
    "name": "Wind Turbine Compact",
    "description": "Portable 1kW wind turbine for small-scale renewable energy generation. Ideal for rural areas and off-grid applications with low noise operation.",
    "price": 899.99,
    "category": "Wind Energy",
    "stock": 25
  },
  {
    "product_id": "P003",
    "name": "Battery Storage 10kWh",
    "description": "Lithium-ion battery storage system 10kWh capacity with built-in BMS and smart monitoring. Enables energy independence and backup power for homes.",
    "price": 4999.00,
    "category": "Energy Storage",
    "stock": 40
  },
  {
    "product_id": "P004",
    "name": "Smart Energy Monitor",
    "description": "Real-time home energy monitoring device. Track electricity consumption across circuits, optimize energy usage, and reduce bills with ML-powered insights.",
    "price": 199.00,
    "category": "Smart Grid",
    "stock": 300
  },
  {
    "product_id": "P005",
    "name": "EV Charging Station DC 50kW",
    "description": "Fast DC charging station for electric vehicles. Supports multiple protocols (CHAdeMO, CCS) with integrated billing and status monitoring via cloud dashboard.",
    "price": 5499.00,
    "category": "EV Charging",
    "stock": 15
  }
]
"@

$productsJSON | Out-File "$DataDir\products.json" -Encoding UTF8
Write-Host "   ✓ products.json erstellt (5 Produkte mit Descriptions)" -ForegroundColor Green

# Orders
$ordersCSV = @"
order_id,customer_id,product_id,quantity,order_date,total_amount,status
O001,1,P001,2,2024-05-01,499.98,delivered
O002,2,P003,1,2024-05-02,4999.00,shipped
O003,1,P004,1,2024-05-03,199.00,pending
O004,3,P002,1,2024-05-04,899.99,processing
O005,4,P005,1,2024-05-05,5499.00,pending
O006,5,P001,3,2024-05-06,749.97,delivered
"@

$ordersCSV | Out-File "$DataDir\orders.csv" -Encoding UTF8
Write-Host "   ✓ orders.csv erstellt (6 Bestellungen)" -ForegroundColor Green

# ─────────────────────────────────────────────────────────────────────
# 3. CREATE QUERY FILES
# ─────────────────────────────────────────────────────────────────────

Write-Host "[4/8] Erstelle Query-Dateien..." -ForegroundColor Yellow

# Query 1: Simple Relational Query
$query1 = @"
-- Query 1: Relational (SQL)
-- Zeige alle Bestellungen von Alice mit Produktdetails
SELECT 
  o.order_id,
  c.name AS customer,
  p.name AS product,
  o.quantity,
  o.total_amount,
  o.order_date
FROM orders o
JOIN customers c ON o.customer_id = c.customer_id
JOIN products p ON o.product_id = p.product_id
WHERE c.name = 'Alice Johnson'
ORDER BY o.order_date DESC;
"@

$query1 | Out-File "$QueryDir\1_relational_join.sql" -Encoding UTF8

# Query 2: Vector Search
$query2 = @"
-- Query 2: Vector Search (Semantic)
-- Finde Produkte ähnlich wie "renewable energy battery storage system"
-- ThemisDB erstellt Embeddings von Produktbeschreibungen

SELECT 
  product_id,
  name,
  description,
  price,
  similarity_score
FROM products_vector_search(
  query='battery energy storage renewable',
  top_k=3,
  min_similarity=0.6
)
ORDER BY similarity_score DESC;
"@

$query2 | Out-File "$QueryDir\2_vector_search.sql" -Encoding UTF8

# Query 3: Graph Query
$query3 = @"
-- Query 3: Graph (Relationships)
-- Finde: Kunde -> Bestellungen -> Produkte -> Kategorien
-- Zeige Netzwerk von Kundenbeziehungen

MATCH (c:Customer)-[:PLACED]->(o:Order)-[:CONTAINS]->(p:Product)
WHERE c.country IN ['DE', 'AT']
RETURN 
  c.name AS customer,
  COUNT(o.order_id) AS order_count,
  SUM(o.total_amount) AS total_spent,
  COLLECT(DISTINCT p.category) AS categories
GROUP BY c.customer_id, c.name
ORDER BY total_spent DESC;
"@

$query3 | Out-File "$QueryDir\3_graph_relationships.aql" -Encoding UTF8

# Query 4: Time-Series
$query4 = @"
-- Query 4: Time-Series Analysis
-- Aggregiere Bestellungen nach Woche

SELECT 
  DATE_TRUNC('week', order_date) AS week_start,
  COUNT(*) AS order_count,
  ROUND(AVG(total_amount), 2) AS avg_order_value,
  SUM(total_amount) AS total_revenue
FROM orders
GROUP BY DATE_TRUNC('week', order_date)
ORDER BY week_start DESC;
"@

$query4 | Out-File "$QueryDir\4_timeseries_aggregation.sql" -Encoding UTF8

# Query 5: LLM Inference
$query5 = @"
-- Query 5: LLM Inference (Lokal, keine Cloud-Calls)
-- Nutze native LLM zur Generierung von Produktempfehlungen

SELECT 
  p.product_id,
  p.name,
  p.description,
  llm_generate(
    model='llama2-7b',
    prompt=CONCAT('Gib eine kurze Marketing-Beschreibung für: ', p.name),
    max_tokens=50,
    temperature=0.7
  ) AS marketing_pitch
FROM products p
LIMIT 3;
"@

$query5 | Out-File "$QueryDir\5_llm_inference.aql" -Encoding UTF8

# Query 6: RAG with Hallucination Detection
$query6 = @"
-- Query 6: RAG Pipeline mit Hallucination Detection
-- Antworte auf Frage basierend auf Kundendaten

SELECT 
  rag_answer(
    question='Welche Energiesparlösungen haben wir für deutsche Kunden?',
    context=(
      SELECT STRING_AGG(p.description, ' | ')
      FROM products p
      WHERE p.category IN ('Energy Storage', 'Smart Grid')
    ),
    check_hallucination=true,
    confidence_threshold=0.75
  ) AS answer,
  faithfulness_score,
  detected_issues
FROM rag_query_results;
"@

$query6 | Out-File "$QueryDir\6_rag_hallucination_detection.aql" -Encoding UTF8

Write-Host "   ✓ 6 Query-Dateien erstellt:" -ForegroundColor Green
Write-Host "     1. Relational Join (SQL)" -ForegroundColor Gray
Write-Host "     2. Vector Search (Semantic)" -ForegroundColor Gray
Write-Host "     3. Graph Relationships (AQL)" -ForegroundColor Gray
Write-Host "     4. Time-Series Aggregation (SQL)" -ForegroundColor Gray
Write-Host "     5. LLM Inference (Local, No Cloud)" -ForegroundColor Gray
Write-Host "     6. RAG + Hallucination Detection" -ForegroundColor Gray

# ─────────────────────────────────────────────────────────────────────
# 4. CREATE VIDEO RECORDING CHECKLIST
# ─────────────────────────────────────────────────────────────────────

Write-Host "[5/8] Erstelle Video-Aufnahme-Anleitung..." -ForegroundColor Yellow

$videChecklist = @"
═══════════════════════════════════════════════════════════════════════
  KICKSTARTER VIDEO RECORDING CHECKLIST
  Status: Live Demo (Single-Take, 4-5 Minutes)
═══════════════════════════════════════════════════════════════════════

PREPARATION (Vorher):
  ☐ Alle Fenster schließen (keine Benachrichtigungen)
  ☐ Terminal vergrößern für bessere Lesbarkeit
  ☐ Font-Größe erhöhen (14-16pt)
  ☐ Demo-Befehle vorbereiten (ggf. ausdrucken)
  ☐ OBS Studio / Screen Recorder öffnen
  ☐ Mikrofon prüfen (clear, kein Hintergrundlärm)

RECORDING FLOW (Video-Struktur):
───────────────────────────────────────────────────────────────────────

[INTRO - 0:00-0:30]
  Goal: "Ich zeige euch ThemisDB in Aktion"
  What to do:
    1. Terminal öffnen
    2. Navigiere zu ThemisDB Demo-Verzeichnis
    3. Sage: "Willkommen zu ThemisDB. Ich zeige jetzt eine live Demo."

[SCENE 1: STARTUP & RELATIONAL - 0:30-1:30]
  Goal: Datenbank starten + einfache SQL-Query
  Commands:
    $ cd demo
    $ ./start-demo.sh
    # Warte auf "Ready on port 8765" Message
    
    # Starte CLI-Client:
    $ themisdb-cli --connect localhost:8765
    
    # Query 1 ausführen:
    > SELECT COUNT(*) FROM customers;
    > SELECT * FROM orders WHERE customer_id=1;
  
  Voice-Over:
    "ThemisDB ist jetzt aktiv. Wir verbinden uns mit dem Datenbankserver
     und führen eine einfache SQL-Abfrage aus. Hier sehen wir Bestellungen
     von Kunde Alice Johnson."

[SCENE 2: VECTOR SEARCH - 1:30-2:30]
  Goal: Semantische Suche demonstrieren
  Commands:
    # Load Vector Index:
    > LOAD VECTORSTORE products_index FROM './demo/data/vectors.db';
    
    # Vector Search Query:
    > SELECT * FROM vector_search(
        query='battery storage renewable energy',
        index='products_index',
        top_k=5
      );
  
  Voice-Over:
    "Jetzt demonstrieren wir die Vektorsuche. Statt exakte Keywords
     suchen wir semantisch nach Produkten zu 'renewable energy battery storage'.
     Die Datenbank versteht den Kontext und findet relevante Produkte."

[SCENE 3: GRAPH QUERY - 2:30-3:30]
  Goal: Beziehungen zwischen Entitäten zeigen
  Commands:
    # Graph Query (AQL):
    > MATCH (c:Customer)-[:PLACED]->(o:Order)-[:CONTAINS]->(p:Product)
      WHERE c.country = 'DE'
      RETURN c.name, COUNT(o) as orders, SUM(o.total_amount);
  
  Voice-Over:
    "Die Graph-Funktionalität zeigt Beziehungen zwischen Kunden,
     Bestellungen und Produkten. Das ist perfekt für Recommendation Engines
     oder Netzwerk-Analysen."

[SCENE 4: LLM & RAG - 3:30-4:30]
  Goal: Native AI-Funktionen zeigen (lokal, KEINE Cloud-Calls)
  Commands:
    # LLM Inference:
    > SELECT llm_generate(
        model='llama2-7b',
        prompt='Schreibe eine kurze Marketing-Beschreibung für: Solar Panel 400W'
      );
    
    # RAG Query:
    > SELECT rag_answer(
        question='Welche Energiesparlösungen haben wir?',
        context=(SELECT description FROM products WHERE category='Energy Storage')
      );
  
  Voice-Over:
    "ThemisDB hat NATIVE LLM-Unterstützung ohne Cloud-Calls.
     Das bedeutet volle Datenkontrolle, keine Latenz, keine Abhängigkeiten.
     Hier sehen Sie LLM-Inferenz direkt in der Datenbank."

[PERFORMANCE & SUMMARY - 4:30-5:00]
  Goal: Metriken zeigen
  Commands:
    # Performance Check:
    > SHOW QUERY STATS;
    > SHOW MEMORY USAGE;
    
  Voice-Over:
    "Die Performance ist impressionant: Multi-Model-Queries in <100ms,
     Memory-Footprint optimiert. ThemisDB ersetzt 7 Spezialsysteme
     (SQL, Graph, Vector, TimeSeries, LLM, RAG, Document) durch eine
     einzige ACID-Datenbank."

───────────────────────────────────────────────────────────────────────

RECORDING TIPS:
  ☑ Langsam sprechen (nicht gehetzt)
  ☑ Erklären während du Input gibst (nicht danach)
  ☑ Ein Fehler ist OK (authentisch) - nicht rausschneiden!
  ☑ Zoom in auf wichtige Output-Zeilen
  ☑ Pauses für Verständlichkeit (nicht durchgehend)

OUTPUT FILES:
  ✓ themisdb-demo-video.mp4 (MP4 oder MOV)
  ✓ Speichern unter: ./demo/videos/
  ✓ Mindestens 720p, H264 Codec

AFTERWARDS:
  1. Upload zu Kickstarter (Demo Video section)
  2. Nimm 6 Screenshots (siehe Screenshot-Anleitung)
  3. Ergänze englische Transkription
═══════════════════════════════════════════════════════════════════════
"@

$videChecklist | Out-File "$VideoDir\VIDEO_RECORDING_CHECKLIST.txt" -Encoding UTF8
Write-Host "   ✓ VIDEO_RECORDING_CHECKLIST.txt erstellt" -ForegroundColor Green

# ─────────────────────────────────────────────────────────────────────
# 5. CREATE SCREENSHOT GUIDE
# ─────────────────────────────────────────────────────────────────────

Write-Host "[6/8] Erstelle Screenshot-Anleitung..." -ForegroundColor Yellow

$screenshotGuide = @"
═══════════════════════════════════════════════════════════════════════
  KICKSTARTER SCREENSHOT GUIDE
  6 Screenshots für Prototyp-Demonstration
═══════════════════════════════════════════════════════════════════════

Nach dem Video aufnehmen, diese 6 Screenshots erstellen:

SCREENSHOT 1: SQL Query & Results
───────────────────────────────────────────────────────────────────────
Command:  SELECT * FROM orders WHERE customer_id=1;
Output:   Zeige 2-3 Zeilen mit order_id, customer_id, product_id, status
File:     01-sql-query-results.png
Caption:  "Relational queries with ACID transactions"

SCREENSHOT 2: Vector Search UI
───────────────────────────────────────────────────────────────────────
Command:  vector_search('renewable energy battery', top_k=5)
Output:   5 Produkte mit Similarity Score (z.B. 0.92, 0.87, 0.78)
File:     02-vector-search-results.png
Caption:  "Semantic vector search on product embeddings"

SCREENSHOT 3: Graph Visualization
───────────────────────────────────────────────────────────────────────
Command:  MATCH (c:Customer)-[:PLACED]->(o:Order)...
Output:   Graph mit Nodes und Edges, Beziehungen visualisiert
File:     03-graph-relationships.png
Caption:  "Graph queries for customer relationships and networks"

SCREENSHOT 4: LLM Output
───────────────────────────────────────────────────────────────────────
Command:  llm_generate(prompt='Market description for Solar Panel')
Output:   Generierter Text (3-5 Zeilen)
File:     04-llm-inference-local.png
Caption:  "Native LLM inference (local, no cloud calls)"

SCREENSHOT 5: Performance Metrics
───────────────────────────────────────────────────────────────────────
Command:  SHOW QUERY STATS;
Output:   Query Latency (<100ms), Throughput (QPS), Memory
File:     05-performance-metrics.png
Caption:  "Real-time performance monitoring and metrics"

SCREENSHOT 6: Full Architecture Stack
───────────────────────────────────────────────────────────────────────
Display:  Terminal mit allen 6 Query-Types in einem Output-Log
Output:   Zeige dass alles in EINER Datenbank läuft
File:     06-multi-model-architecture.png
Caption:  "Multi-model core: SQL, Graph, Vector, TimeSeries, LLM, RAG, Document storage"

───────────────────────────────────────────────────────────────────────

TECHNICAL REQUIREMENTS FOR SCREENSHOTS:
  ☐ Minimum resolution: 1280x720 (HD)
  ☐ Format: PNG (lossless)
  ☐ Font size: Readable (14pt+)
  ☐ Contrast: Good (dark terminal on light or light on dark)
  ☐ Include: Timestamp or query text visible
  ☐ Crop: Show relevant area (no unnecessary UI clutter)

KICKSTARTER UPLOAD:
  ☐ Upload in Project Gallery: "Live System Screenshots"
  ☐ Add short caption for each (copy from above)
  ☐ Tag as "Demo", "Live System", "Working Prototype"

FILES TO SAVE:
  01-sql-query-results.png
  02-vector-search-results.png
  03-graph-relationships.png
  04-llm-inference-local.png
  05-performance-metrics.png
  06-multi-model-architecture.png
  
  → Location: ./demo/videos/screenshots/
═══════════════════════════════════════════════════════════════════════
"@

$screenshotGuide | Out-File "$VideoDir\SCREENSHOT_GUIDE.txt" -Encoding UTF8
Write-Host "   ✓ SCREENSHOT_GUIDE.txt erstellt" -ForegroundColor Green

# ─────────────────────────────────────────────────────────────────────
# 6. CREATE START SCRIPT
# ─────────────────────────────────────────────────────────────────────

Write-Host "[7/8] Erstelle Demo-Start-Skript..." -ForegroundColor Yellow

$startDemo = @"
#!/bin/bash
# ThemisDB Demo Start Script

DEMO_ROOT=\`dirname \$0\`/../
BUILD_DIR=\`\$DEMO_ROOT/build-msvc-ninja-release\`

echo "════════════════════════════════════════════════════════════"
echo "  ThemisDB Demo Environment"
echo "════════════════════════════════════════════════════════════"

# Check if ThemisDB is built
if [ ! -d "\$BUILD_DIR" ]; then
  echo "✗ Build directory not found: \$BUILD_DIR"
  echo "  Please run: cmake --build --preset msvc-ninja-release"
  exit 1
fi

# Start server
echo "[1/3] Starting ThemisDB Server..."
cd \$DEMO_ROOT

# Use built binary
THEMIS_BIN="\$BUILD_DIR/bin/themis_server"

if [ -f "\$THEMIS_BIN" ]; then
  \$THEMIS_BIN --data-dir ./demo/data --port 8765 &
  THEMIS_PID=\$!
  echo "  ✓ ThemisDB started (PID: \$THEMIS_PID)"
else
  echo "  ✗ Binary not found: \$THEMIS_BIN"
  exit 1
fi

sleep 2

# Check health
echo ""
echo "[2/3] Checking health..."
curl -s http://localhost:8765/health && echo "" || (echo "Health check failed"; kill \$THEMIS_PID; exit 1)

echo "[3/3] Loading sample data..."
# Load CSV files
curl -X POST http://localhost:8765/v2/bulk_load \\
  -H 'Content-Type: application/json' \\
  -d '{
    "files": {
      "customers": "./demo/data/customers.csv",
      "products": "./demo/data/products.json",
      "orders": "./demo/data/orders.csv"
    }
  }'

echo ""
echo "════════════════════════════════════════════════════════════"
echo "✓ Demo environment ready!"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Connection:"
echo "  HTTP REST:  http://localhost:8765"
echo "  Wire Proto: localhost:8766"
echo ""
echo "Next steps:"
echo "  1. Open CLI: themisdb-cli --connect localhost:8765"
echo "  2. Run queries from: ./demo/queries/"
echo "  3. Start video recording"
echo ""
echo "Server PID: \$THEMIS_PID"
echo "To stop: kill \$THEMIS_PID"
"@

$startDemo | Out-File "$SetupDir\start-demo.sh" -Encoding UTF8
Write-Host "   ✓ start-demo.sh erstellt (für Linux/WSL)" -ForegroundColor Green

# ─────────────────────────────────────────────────────────────────────
# 7. CREATE README
# ─────────────────────────────────────────────────────────────────────

Write-Host "[8/8] Erstelle Demo-README..." -ForegroundColor Yellow

$demoReadme = @"
# ThemisDB Kickstarter Demo Environment

Dieses Verzeichnis enthält alles, was Sie für die Kickstarter-Video-Demo brauchen:

## 📁 Struktur

\`\`\`
demo/
├── setup/
│   ├── setup-demo.ps1           # Main setup script (Windows/PowerShell)
│   └── start-demo.sh            # Demo start script (Linux/WSL)
├── queries/
│   ├── 1_relational_join.sql
│   ├── 2_vector_search.sql
│   ├── 3_graph_relationships.aql
│   ├── 4_timeseries_aggregation.sql
│   ├── 5_llm_inference.aql
│   └── 6_rag_hallucination_detection.aql
├── data/
│   ├── customers.csv            # Sample data (auto-created)
│   ├── products.json
│   ├── orders.csv
│   └── themis.db               # Database (auto-created)
└── videos/
    ├── VIDEO_RECORDING_CHECKLIST.txt
    ├── SCREENSHOT_GUIDE.txt
    └── screenshots/            # Save 6 screenshots here
\`\`\`

## 🚀 Quick Start

### Step 1: Setup Demo Environment (Windows PowerShell)

\`\`\`powershell
cd demo/setup
.\setup-demo.ps1 -BuildPreset msvc-ninja-release
\`\`\`

This will:
- Create sample data files (customers, products, orders)
- Create query files for all demo scenarios
- Create video recording checklists

### Step 2: Start ThemisDB

\`\`\`bash
# Option A: Linux/WSL
cd demo/setup
./start-demo.sh

# Option B: Docker
docker compose up -d
\`\`\`

### Step 3: Record Video

Follow the checklist:
\`\`\`
cat demo/videos/VIDEO_RECORDING_CHECKLIST.txt
\`\`\`

Key timeline:
- **0:00-0:30** - Intro: "ThemisDB in action"
- **0:30-1:30** - Scene 1: Relational (SQL) + Simple JOIN
- **1:30-2:30** - Scene 2: Vector Search (Semantic)
- **2:30-3:30** - Scene 3: Graph Queries (Relationships)
- **3:30-4:30** - Scene 4: LLM Inference (Local, No Cloud)
- **4:30-5:00** - Performance Summary

### Step 4: Create Screenshots

Follow the screenshot guide:
\`\`\`
cat demo/videos/SCREENSHOT_GUIDE.txt
\`\`\`

Create 6 screenshots:
1. SQL query results
2. Vector search output
3. Graph visualization
4. LLM inference output
5. Performance metrics
6. Full architecture (all 6 model types in one view)

## 📊 Demo Scenarios

### 1. Relational (SQL)
\`\`\`sql
SELECT o.order_id, c.name, p.name, o.total_amount, o.status
FROM orders o
JOIN customers c ON o.customer_id = c.customer_id
JOIN products p ON o.product_id = p.product_id
WHERE c.country = 'DE';
\`\`\`

### 2. Vector Search (Semantic)
\`\`\`sql
SELECT * FROM vector_search(
  query='renewable energy battery storage',
  index='products_vector',
  top_k=5
);
\`\`\`

### 3. Graph (Relationships)
\`\`\`
MATCH (c:Customer)-[:PLACED]->(o:Order)-[:CONTAINS]->(p:Product)
WHERE c.country IN ['DE', 'AT']
RETURN c.name, COUNT(o) as order_count, SUM(o.total_amount)
\`\`\`

### 4. Time-Series (Aggregation)
\`\`\`sql
SELECT 
  DATE_TRUNC('week', order_date) as week,
  COUNT(*) as order_count,
  SUM(total_amount) as revenue
FROM orders
GROUP BY DATE_TRUNC('week', order_date)
\`\`\`

### 5. LLM Inference (Local)
\`\`\`
SELECT llm_generate(
  model='llama2-7b',
  prompt='Write marketing copy for: Solar Panel 400W',
  max_tokens=50
);
\`\`\`

### 6. RAG + Hallucination Detection
\`\`\`
SELECT rag_answer(
  question='What energy solutions do we have?',
  context=...,
  check_hallucination=true,
  confidence_threshold=0.75
);
\`\`\`

## 💡 Tips for Video Recording

✅ **DO:**
- Record in 720p or higher (MP4/H264)
- Speak slowly and clearly
- Explain while executing (not after)
- Include one realistic error (makes it authentic!)
- Show query execution + results

❌ **DON'T:**
- Edit the video (must be "unedited product test")
- Add background music or effects
- Use AI-generated voiceover
- Make fast cuts/transitions
- Rush through explanations

## 🎯 Kickstarter Submission Checklist

- [ ] Video recorded (4-5 minutes, single-take, MP4)
- [ ] 6 screenshots created (1280x720+, PNG)
- [ ] English transcript/subtitles added
- [ ] AI disclosure completed ("Use of AI" section)
- [ ] English translations added (Story, Rewards, Captions)
- [ ] All files uploaded to Kickstarter project
- [ ] Project re-submitted for review

## 📝 Files

| File | Purpose | Status |
|---|---|---|
| \`setup-demo.ps1\` | Initialize demo environment | ✅ Created |
| \`start-demo.sh\` | Start server & load data | ✅ Created |
| \`queries/*.sql/.aql\` | Demo queries (6 scenarios) | ✅ Created |
| \`data/*.csv/.json\` | Sample data files | ✅ Created |
| \`VIDEO_RECORDING_CHECKLIST.txt\` | Recording guide | ✅ Created |
| \`SCREENSHOT_GUIDE.txt\` | Screenshot instructions | ✅ Created |

## ❓ Troubleshooting

**Problem:** "ThemisDB not found"
- Solution: Build ThemisDB first: \`cmake --build --preset msvc-ninja-release\`

**Problem:** "Port 8765 already in use"
- Solution: Change port in start script or kill existing process

**Problem:** "Sample data not loading"
- Solution: Check CSV/JSON format, ensure data/ directory exists

**Problem:** "LLM inference fails"
- Solution: Ensure llama.cpp models are downloaded (see SETUP.md)

## 📧 Contact

For issues with the demo setup, contact: [support email]
"@

$demoReadme | Out-File "$DemoRoot\README.md" -Encoding UTF8
Write-Host "   ✓ README.md erstellt" -ForegroundColor Green

# ─────────────────────────────────────────────────────────────────────
# 8. SUMMARY
# ─────────────────────────────────────────────────────────────────────

Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║ ✓ DEMO SETUP ABGESCHLOSSEN                               ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Green

Write-Host ""
Write-Host "📁 Erstellte Struktur:" -ForegroundColor Cyan
Write-Host "   demo/" -ForegroundColor Gray
Write-Host "   ├── setup/" -ForegroundColor Gray
Write-Host "   │   ├── setup-demo.ps1 (Main Setup)" -ForegroundColor Green
Write-Host "   │   └── start-demo.sh (Server Start)" -ForegroundColor Green
Write-Host "   ├── queries/ (6 Query-Dateien)" -ForegroundColor Gray
Write-Host "   │   ├── 1_relational_join.sql" -ForegroundColor Green
Write-Host "   │   ├── 2_vector_search.sql" -ForegroundColor Green
Write-Host "   │   ├── 3_graph_relationships.aql" -ForegroundColor Green
Write-Host "   │   ├── 4_timeseries_aggregation.sql" -ForegroundColor Green
Write-Host "   │   ├── 5_llm_inference.aql" -ForegroundColor Green
Write-Host "   │   └── 6_rag_hallucination_detection.aql" -ForegroundColor Green
Write-Host "   ├── data/ (Sample Daten - auto-erstellt)" -ForegroundColor Gray
Write-Host "   │   ├── customers.csv" -ForegroundColor Green
Write-Host "   │   ├── products.json" -ForegroundColor Green
Write-Host "   │   └── orders.csv" -ForegroundColor Green
Write-Host "   ├── videos/" -ForegroundColor Gray
Write-Host "   │   ├── VIDEO_RECORDING_CHECKLIST.txt" -ForegroundColor Green
Write-Host "   │   ├── SCREENSHOT_GUIDE.txt" -ForegroundColor Green
Write-Host "   │   └── screenshots/ (Speichern Sie 6 Screenshots hier)" -ForegroundColor Yellow
Write-Host "   └── README.md (Vollständige Anleitung)" -ForegroundColor Green

Write-Host ""
Write-Host "📋 Nächste Schritte:" -ForegroundColor Yellow
Write-Host "   1. Video-Aufnahme vorbereiten:" -ForegroundColor Gray
Write-Host "      → Lese: demo/videos/VIDEO_RECORDING_CHECKLIST.txt" -ForegroundColor Cyan
Write-Host ""
Write-Host "   2. ThemisDB starten:" -ForegroundColor Gray
Write-Host "      → Windows/PowerShell: cd demo/setup && .\start-demo.sh" -ForegroundColor Cyan
Write-Host "      → Linux/WSL: cd demo/setup && bash start-demo.sh" -ForegroundColor Cyan
Write-Host "      → Docker: docker compose up -d" -ForegroundColor Cyan
Write-Host ""
Write-Host "   3. Queries ausführen:" -ForegroundColor Gray
Write-Host "      → Jede Datei in demo/queries/ zeigt einen Use-Case" -ForegroundColor Cyan
Write-Host ""
Write-Host "   4. Screenshots erstellen:" -ForegroundColor Gray
Write-Host "      → Lese: demo/videos/SCREENSHOT_GUIDE.txt" -ForegroundColor Cyan
Write-Host ""
Write-Host "   5. Zu Kickstarter hochladen:" -ForegroundColor Gray
Write-Host "      → Video + 6 Screenshots + Transkription" -ForegroundColor Cyan

Write-Host ""
Write-Host "💡 Tipps für Video:" -ForegroundColor Cyan
Write-Host "   ✓ Single-Take (4-5 Min, unbearbeitet)" -ForegroundColor Gray
Write-Host "   ✓ Zeige Live-System: Queries → Results" -ForegroundColor Gray
Write-Host "   ✓ Erkläre während du Input gibst" -ForegroundColor Gray
Write-Host "   ✓ Eine Fehler ist OK (authentisch!)" -ForegroundColor Gray
Write-Host "   ✓ Handy-Hochkantformat OK (einfach)" -ForegroundColor Gray

Write-Host ""
Write-Host "✅ Status: READY FOR DEMO" -ForegroundColor Green
Write-Host ""
