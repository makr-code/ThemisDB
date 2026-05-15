# ThemisDB Demo Setup - Vereinfacht fuer Kickstarter Video
# Erstellt Sample-Daten und Query-Dateien

$DemoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$DataDir = "$DemoRoot\data"
$QueryDir = "$DemoRoot\queries"
$VideoDir = "$DemoRoot\videos"

Write-Host "Initialisiere ThemisDB Demo..." -ForegroundColor Cyan

# Erstelle data/ Verzeichnis
New-Item -ItemType Directory -Path $DataDir -Force | Out-Null
New-Item -ItemType Directory -Path $VideoDir\screenshots -Force | Out-Null

# 1. CUSTOMERS DATA
$csv = @"
customer_id,name,email,country,signup_date
1,Alice Johnson,alice@example.com,DE,2024-01-15
2,Bob Schmidt,bob@example.com,AT,2024-02-20
3,Carol Chen,carol@example.com,CH,2024-03-10
4,David Mueller,david@example.com,DE,2024-03-25
5,Emma Rossi,emma@example.com,IT,2024-04-05
"@

$csv | Out-File "$DataDir\customers.csv" -Encoding UTF8 -Force
Write-Host "[OK] customers.csv" -ForegroundColor Green

# 2. PRODUCTS DATA
$json = @"
[
  {"product_id":"P001","name":"Solar Panel 400W","description":"High-efficiency monocrystalline solar panel with 400W peak power. Perfect for residential installations.","price":249.99,"category":"Solar Energy","stock":150},
  {"product_id":"P002","name":"Wind Turbine Compact","description":"Portable 1kW wind turbine for small renewable energy generation. Ideal for rural areas.","price":899.99,"category":"Wind Energy","stock":25},
  {"product_id":"P003","name":"Battery Storage 10kWh","description":"Lithium-ion battery storage 10kWh with BMS. Enables energy independence and backup power.","price":4999.00,"category":"Energy Storage","stock":40},
  {"product_id":"P004","name":"Smart Energy Monitor","description":"Real-time energy monitoring device. Track consumption, optimize usage, reduce bills with ML insights.","price":199.00,"category":"Smart Grid","stock":300},
  {"product_id":"P005","name":"EV Charging Station DC","description":"Fast DC charging station for electric vehicles. Supports CHAdeMO and CCS protocols.","price":5499.00,"category":"EV Charging","stock":15}
]
"@

$json | Out-File "$DataDir\products.json" -Encoding UTF8 -Force
Write-Host "[OK] products.json" -ForegroundColor Green

# 3. ORDERS DATA
$orders = @"
order_id,customer_id,product_id,quantity,order_date,total_amount,status
O001,1,P001,2,2024-05-01,499.98,delivered
O002,2,P003,1,2024-05-02,4999.00,shipped
O003,1,P004,1,2024-05-03,199.00,pending
O004,3,P002,1,2024-05-04,899.99,processing
O005,4,P005,1,2024-05-05,5499.00,pending
O006,5,P001,3,2024-05-06,749.97,delivered
"@

$orders | Out-File "$DataDir\orders.csv" -Encoding UTF8 -Force
Write-Host "[OK] orders.csv" -ForegroundColor Green

# 4. CREATE QUERY FILES
Write-Host ""
Write-Host "Erstelle Query-Dateien..." -ForegroundColor Cyan

# SQL Join Query
@"
SELECT o.order_id, c.name, p.name, o.quantity, o.total_amount, o.status
FROM orders o
JOIN customers c ON o.customer_id = c.customer_id
JOIN products p ON o.product_id = p.product_id
WHERE c.country = 'DE'
ORDER BY o.order_date DESC;
"@ | Out-File "$QueryDir\1_relational_join.sql" -Encoding UTF8 -Force
Write-Host "[OK] 1_relational_join.sql" -ForegroundColor Green

# Vector Search Query
@"
SELECT product_id, name, description, price, similarity_score
FROM products_vector_search(
  query='renewable energy battery storage',
  top_k=5,
  min_similarity=0.6
)
ORDER BY similarity_score DESC;
"@ | Out-File "$QueryDir\2_vector_search.sql" -Encoding UTF8 -Force
Write-Host "[OK] 2_vector_search.sql" -ForegroundColor Green

# Graph Query
@"
MATCH (c:Customer)-[:PLACED]->(o:Order)-[:CONTAINS]->(p:Product)
WHERE c.country IN ['DE', 'AT']
RETURN c.name, COUNT(o.order_id) AS order_count, 
       SUM(o.total_amount) AS total_spent,
       COLLECT(DISTINCT p.category) AS categories
GROUP BY c.customer_id
ORDER BY total_spent DESC;
"@ | Out-File "$QueryDir\3_graph_relationships.aql" -Encoding UTF8 -Force
Write-Host "[OK] 3_graph_relationships.aql" -ForegroundColor Green

# Time-Series Query
@"
SELECT DATE_TRUNC('week', order_date) AS week_start,
       COUNT(*) AS order_count,
       ROUND(AVG(total_amount), 2) AS avg_order_value,
       SUM(total_amount) AS total_revenue
FROM orders
GROUP BY DATE_TRUNC('week', order_date)
ORDER BY week_start DESC;
"@ | Out-File "$QueryDir\4_timeseries_aggregation.sql" -Encoding UTF8 -Force
Write-Host "[OK] 4_timeseries_aggregation.sql" -ForegroundColor Green

# LLM Query
@"
SELECT llm_generate(
  model='llama2-7b',
  prompt='Write marketing copy for: Solar Panel 400W',
  max_tokens=50,
  temperature=0.7
) AS marketing_pitch;
"@ | Out-File "$QueryDir\5_llm_inference.aql" -Encoding UTF8 -Force
Write-Host "[OK] 5_llm_inference.aql" -ForegroundColor Green

# RAG Query
@"
SELECT rag_answer(
  question='What energy solutions do we offer?',
  context=(SELECT STRING_AGG(description, ' | ') 
           FROM products 
           WHERE category IN ('Energy Storage','Smart Grid')),
  check_hallucination=true,
  confidence_threshold=0.75
) AS answer;
"@ | Out-File "$QueryDir\6_rag_hallucination_detection.aql" -Encoding UTF8 -Force
Write-Host "[OK] 6_rag_hallucination_detection.aql" -ForegroundColor Green

# 5. CREATE RECORDING CHECKLIST
Write-Host ""
Write-Host "Erstelle Video-Anleitung..." -ForegroundColor Cyan

$checklist = @"
KICKSTARTER VIDEO RECORDING CHECKLIST
Single-Take Demo: 4-5 Minutes

BEFORE RECORDING:
- Close all windows (no notifications)
- Enlarge terminal font (14-16pt)
- Prepare demo commands
- Test microphone
- Open OBS Studio or screen recorder

RECORDING FLOW (Timeline):
[0:00-0:30] INTRO
  Say: "Welcome to ThemisDB. I show you a live demo."

[0:30-1:30] SCENE 1: SQL + Relational
  Command: SELECT * FROM orders WHERE customer_id=1;
  Show: Relational data with multiple tables

[1:30-2:30] SCENE 2: Vector Search
  Command: vector_search('renewable energy battery', top_k=5)
  Show: Semantic search results with similarity scores

[2:30-3:30] SCENE 3: Graph Queries
  Command: MATCH (c:Customer)-[:PLACED]->(o:Order)...
  Show: Relationships and network analysis

[3:30-4:30] SCENE 4: LLM Inference (LOCAL!)
  Command: llm_generate(prompt='Market copy for Solar Panel')
  Say: Native LLM inference, no cloud calls!

[4:30-5:00] SUMMARY
  Say: "Multi-model queries in <100ms. One ACID database replaces 7 systems."

TIPS:
- Speak slowly and clearly
- Explain while executing
- One error is OK (authentic)
- No editing allowed (must be single-take)

OUTPUT:
- Save as: themisdb-demo-video.mp4
- Minimum: 720p HD, H.264 codec
- Location: demo/videos/themisdb-demo-video.mp4
"@

$checklist | Out-File "$VideoDir\VIDEO_RECORDING_CHECKLIST.txt" -Encoding UTF8 -Force
Write-Host "[OK] VIDEO_RECORDING_CHECKLIST.txt" -ForegroundColor Green

# 6. CREATE SCREENSHOT GUIDE
$screenshots = @"
KICKSTARTER SCREENSHOT GUIDE
6 Screenshots for Product Demonstration

After recording the video, create these 6 screenshots:

SCREENSHOT 1: SQL Query Results
Command: SELECT * FROM orders WHERE customer_id=1;
Save: 01-sql-query-results.png
Caption: "Relational queries with ACID transactions"

SCREENSHOT 2: Vector Search Output
Command: vector_search('renewable energy', top_k=5)
Save: 02-vector-search-results.png
Caption: "Semantic vector search on product embeddings"

SCREENSHOT 3: Graph Visualization
Command: MATCH (c:Customer)-[:PLACED]->(o:Order)...
Save: 03-graph-relationships.png
Caption: "Graph queries for customer relationships"

SCREENSHOT 4: LLM Inference Output
Command: llm_generate(prompt='...')
Save: 04-llm-inference-local.png
Caption: "Native LLM inference (local, no cloud)"

SCREENSHOT 5: Performance Metrics
Command: SHOW QUERY STATS;
Save: 05-performance-metrics.png
Caption: "Real-time performance monitoring"

SCREENSHOT 6: Full Architecture Stack
Display: Terminal with all 6 query types
Save: 06-multi-model-architecture.png
Caption: "Multi-model core: SQL, Graph, Vector, TimeSeries, LLM, RAG, Document"

REQUIREMENTS:
- Minimum 1280x720 (HD)
- PNG format (lossless)
- Clear and readable
- Include query text
- Save to: demo/videos/screenshots/

FILES TO CREATE:
01-sql-query-results.png
02-vector-search-results.png
03-graph-relationships.png
04-llm-inference-local.png
05-performance-metrics.png
06-multi-model-architecture.png
"@

$screenshots | Out-File "$VideoDir\SCREENSHOT_GUIDE.txt" -Encoding UTF8 -Force
Write-Host "[OK] SCREENSHOT_GUIDE.txt" -ForegroundColor Green

Write-Host ""
Write-Host "======================================================" -ForegroundColor Green
Write-Host "DEMO SETUP COMPLETE!" -ForegroundColor Green
Write-Host "======================================================" -ForegroundColor Green
Write-Host ""
Write-Host "Files created:" -ForegroundColor Cyan
Write-Host "  demo/data/" -ForegroundColor Gray
Write-Host "    - customers.csv" -ForegroundColor Green
Write-Host "    - products.json" -ForegroundColor Green
Write-Host "    - orders.csv" -ForegroundColor Green
Write-Host ""
Write-Host "  demo/queries/" -ForegroundColor Gray
Write-Host "    - 1_relational_join.sql" -ForegroundColor Green
Write-Host "    - 2_vector_search.sql" -ForegroundColor Green
Write-Host "    - 3_graph_relationships.aql" -ForegroundColor Green
Write-Host "    - 4_timeseries_aggregation.sql" -ForegroundColor Green
Write-Host "    - 5_llm_inference.aql" -ForegroundColor Green
Write-Host "    - 6_rag_hallucination_detection.aql" -ForegroundColor Green
Write-Host ""
Write-Host "  demo/videos/" -ForegroundColor Gray
Write-Host "    - VIDEO_RECORDING_CHECKLIST.txt" -ForegroundColor Green
Write-Host "    - SCREENSHOT_GUIDE.txt" -ForegroundColor Green
Write-Host "    - screenshots/ (create 6 PNG files here)" -ForegroundColor Yellow
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Read demo/videos/VIDEO_RECORDING_CHECKLIST.txt" -ForegroundColor Cyan
Write-Host "  2. Start ThemisDB server" -ForegroundColor Cyan
Write-Host "  3. Record live demo (4-5 min, single-take)" -ForegroundColor Cyan
Write-Host "  4. Create 6 screenshots (see SCREENSHOT_GUIDE.txt)" -ForegroundColor Cyan
Write-Host "  5. Upload to Kickstarter" -ForegroundColor Cyan
Write-Host ""
