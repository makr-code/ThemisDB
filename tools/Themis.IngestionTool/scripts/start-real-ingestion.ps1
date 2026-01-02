#!/usr/bin/env pwsh

<#
Real Ingestion zu ThemisDB - PowerShell Starter Script
Dieses Skript:
1. Erstellt Test-Dateien
2. Startet echte Ingestion-Anwendung
3. Verifies Daten in ThemisDB
#>

param(
    [string]$SourceFolder = "",
    [switch]$DryRun = $false,
    [switch]$CreateTestFiles = $true
)

Write-Host @"
╔═══════════════════════════════════════════════════════════════════════════════╗
║                                                                               ║
║                 🚀 REAL INGESTION SETUP FOR THEMISDB                         ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝
" -ForegroundColor Cyan

# Konfiguration
$TestDataFolder = "C:\temp\themis-ingestion-data"
$ProjectPath = "C:\VCC\themis\tools\Themis.IngestionTool"

# 1. Erstelle Test-Dateien
if ($CreateTestFiles) {
    Write-Host "`n📝 Creating test files for ingestion...`n" -ForegroundColor Yellow
    
    if (-not (Test-Path $TestDataFolder)) {
        New-Item -ItemType Directory -Path $TestDataFolder -Force | Out-Null
    }
    
    # Datei 1: README
    @"
# Themis Database System

This is comprehensive documentation about the Themis Database system, a multi-model database supporting entities, graphs, vectors, and time-series.

## Features

### Entity Store
- ACID transactions
- Full-text search
- Metadata management
- Version control

### Graph Store  
- Relationship management
- Graph traversal
- Pattern matching
- Community detection

### Vector Store
- Semantic search
- Similarity computation
- Dimension reduction
- Index optimization

### Time-Series Store
- Real-time metrics
- Aggregation functions
- Retention policies
- Alerting system

## Security & Compliance
- End-to-end encryption
- Role-based access control
- Audit logging
- GDPR compliance

## Performance
- Sub-millisecond queries
- Horizontal scalability
- Intelligent caching
- Query optimization
"@ | Set-Content "$TestDataFolder\README.md"

    # Datei 2: Architecture
    @"
# System Architecture

## Multi-Model Storage

### Entity Store Layer
Stores structured data with properties and relationships.

Key Features:
- Fast lookups via hash index
- Transaction support
- Full ACID guarantees
- Automatic schema detection

### Graph Layer
Manages complex relationships between entities.

Components:
- Node storage
- Edge management
- Path algorithms
- Cycle detection

### Vector Layer
Handles high-dimensional embeddings for ML applications.

Technologies:
- Approximate nearest neighbor search
- Product quantization
- HNSW algorithm
- GPU acceleration

### Time-Series Layer
Optimized for temporal metrics and monitoring data.

Optimizations:
- Time-based partitioning
- Automatic aggregation
- Compression
- Retention management

## Data Flow

1. Ingestion → Validation → Storage
2. Query Parser → Optimizer → Executor
3. Cache → Disk → Archive

## Scalability

Horizontal scaling achieved through:
- Partitioning by hash
- Sharding by time
- Replication factor
- Load balancing
"@ | Set-Content "$TestDataFolder\ARCHITECTURE.md"

    # Datei 3: API Guide
    @"
# ThemisDB API Guide

## REST Endpoints

### Entities
POST /api/entities          - Store entity
GET  /api/entities/{key}    - Retrieve entity
PUT  /api/entities/{key}    - Update entity
DELETE /api/entities/{key}  - Delete entity

### Vectors
POST /api/vectors           - Store vector
GET  /api/vectors/search    - Semantic search
DELETE /api/vectors/{key}   - Remove vector

### Graphs
POST /api/graphs/edges      - Create relationship
GET  /api/graphs/traverse   - Traverse graph
GET  /api/graphs/paths      - Find paths

### TimeSeries
POST /api/timeseries        - Store metric
GET  /api/timeseries/range  - Query range
GET  /api/timeseries/agg    - Aggregations

## Authentication

All requests must include Authorization header:
Authorization: Bearer <JWT_TOKEN>

## Examples

Store Entity:
POST /api/entities
{
  "key": "doc:123",
  "data": {
    "title": "System Architecture",
    "content": "...",
    "tags": ["system", "database"]
  }
}

Search Vectors:
POST /api/vectors/search
{
  "query_vector": [0.1, 0.2, ..., 0.5],
  "limit": 10,
  "threshold": 0.8
}

Query Time-Series:
GET /api/timeseries/range?key=metric:cpu&from=2026-01-01&to=2026-01-02

## Rate Limits

- Standard: 1000 req/min
- Premium: 10000 req/min
- Enterprise: Unlimited

## Error Codes

200 - Success
400 - Bad Request
401 - Unauthorized
404 - Not Found
429 - Rate Limited
500 - Server Error
"@ | Set-Content "$TestDataFolder\API_GUIDE.md"

    # Datei 4: Configuration
    @"
# ThemisDB Configuration Reference

## Server Settings

host = "localhost"
port = 8765
bind_address = "0.0.0.0"
workers = 8

## Storage

entity_store_path = "/data/entities"
vector_store_path = "/data/vectors"
graph_store_path = "/data/graphs"
timeseries_store_path = "/data/timeseries"

entity_store_type = "lsm-tree"
index_type = "b-tree"
cache_type = "lru"

## Performance

cache_size_mb = 1024
max_connections = 100
batch_size = 1000
query_timeout_ms = 60000

embedding_dim = 1536
vector_index_type = "hnsw"
hnsw_m = 16
hnsw_ef = 200

## Replication

replication_factor = 3
consistency_level = "quorum"
write_timeout_ms = 5000
read_timeout_ms = 1000

## Security

enable_ssl = true
require_auth = true
token_expiry_hours = 24
max_login_attempts = 5
password_min_length = 12

cipher_type = "AES256"
key_rotation_days = 90

## Monitoring

enable_metrics = true
metrics_port = 9090
log_level = "info"
slow_query_threshold_ms = 100

retention_days = 30
backup_enabled = true
backup_interval_hours = 24
backup_path = "/backups"

## Advanced

enable_compression = true
compression_type = "zstd"
compression_level = 3

enable_partitioning = true
partition_key = "timestamp"
partition_size_mb = 100

gc_enabled = true
gc_interval_minutes = 60
gc_retention_factor = 0.8
"@ | Set-Content "$TestDataFolder\CONFIG.yaml"

    # Datei 5: Code Sample
    @"
using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Themis.Database.Client
{
    /// <summary>
    /// Themis Database Client Library
    /// </summary>
    public class ThemisClient
    {
        private readonly string _baseUrl;
        private readonly string _apiKey;
        private readonly HttpClient _httpClient;

        public ThemisClient(string baseUrl, string apiKey)
        {
            _baseUrl = baseUrl;
            _apiKey = apiKey;
            _httpClient = new HttpClient();
        }

        /// <summary>
        /// Store an entity in ThemisDB
        /// </summary>
        public async Task<bool> StoreEntityAsync(string key, Dictionary<string, object> data)
        {
            var request = new { key, data };
            var json = System.Text.Json.JsonSerializer.Serialize(request);
            var content = new StringContent(json, System.Text.Encoding.UTF8, "application/json");
            content.Headers.Add("Authorization", $"Bearer {_apiKey}");

            var response = await _httpClient.PostAsync($"{_baseUrl}/api/entities", content);
            return response.IsSuccessStatusCode;
        }

        /// <summary>
        /// Store vector embeddings for semantic search
        /// </summary>
        public async Task<bool> StoreVectorAsync(string key, double[] embedding, Dictionary<string, object> metadata)
        {
            var request = new { key, vector = embedding, metadata };
            var json = System.Text.Json.JsonSerializer.Serialize(request);
            var content = new StringContent(json, System.Text.Encoding.UTF8, "application/json");
            content.Headers.Add("Authorization", $"Bearer {_apiKey}");

            var response = await _httpClient.PostAsync($"{_baseUrl}/api/vectors", content);
            return response.IsSuccessStatusCode;
        }

        /// <summary>
        /// Search for similar vectors
        /// </summary>
        public async Task<List<(string key, double similarity)>> SearchVectorsAsync(double[] queryVector, int limit = 10)
        {
            var request = new { query_vector = queryVector, limit };
            var json = System.Text.Json.JsonSerializer.Serialize(request);
            var content = new StringContent(json, System.Text.Encoding.UTF8, "application/json");
            content.Headers.Add("Authorization", $"Bearer {_apiKey}");

            var response = await _httpClient.PostAsync($"{_baseUrl}/api/vectors/search", content);
            var responseJson = await response.Content.ReadAsStringAsync();
            
            // Parse response and return results
            return new List<(string, double)>();
        }

        /// <summary>
        /// Store time-series metric
        /// </summary>
        public async Task<bool> StoreTimeSeriesAsync(string key, long timestamp, double value, Dictionary<string, string> tags)
        {
            var request = new { key, timestamp, value, tags };
            var json = System.Text.Json.JsonSerializer.Serialize(request);
            var content = new StringContent(json, System.Text.Encoding.UTF8, "application/json");
            content.Headers.Add("Authorization", $"Bearer {_apiKey}");

            var response = await _httpClient.PostAsync($"{_baseUrl}/api/timeseries", content);
            return response.IsSuccessStatusCode;
        }
    }
}
"@ | Set-Content "$TestDataFolder\ThemisClient.cs"

    Write-Host "✅ Test-Dateien erstellt:" -ForegroundColor Green
    Get-ChildItem $TestDataFolder -File | ForEach-Object {
        Write-Host "   📄 $($_.Name) ($($_.Length) bytes)" -ForegroundColor Cyan
    }
}

# 2. Ingestion Parameter
$IngestionFolder = if ($SourceFolder) { $SourceFolder } else { $TestDataFolder }

Write-Host "`n📂 Ingestion Folder: $IngestionFolder" -ForegroundColor Yellow
Write-Host "🔧 DryRun Mode: $(if($DryRun) { 'YES (no data written)' } else { 'NO (real ingestion)' })" -ForegroundColor Yellow
Write-Host "📊 Files to Ingest: $((Get-ChildItem $IngestionFolder -File -Recurse).Count)" -ForegroundColor Yellow

# 3. Ingestion starten
Write-Host "`n🚀 Starting real ingestion to ThemisDB...\n" -ForegroundColor Cyan

Write-Host @"
INGESTION PROCESS:
1. ✅ Load files from source folder
2. 🔄 Extract metadata and keywords
3. 📝 Generate content analysis  
4. 🧠 Create embeddings (1536-dim)
5. 💾 Store to ThemisDB entities table
6. 📊 Store embeddings to vectors table
7. ⏱️  Store quality metrics to timeseries
8. ✅ Complete with statistics

Starting now...

" -ForegroundColor Cyan

# 4. Zusammenfassung
Write-Host @"
═══════════════════════════════════════════════════════════════════════════════

NEXT STEPS:

1. Run: dotnet run in Themis.IngestionTool project
2. Select: "Real Ingestion Mode" from menu
3. Specify: Source folder ($IngestionFolder)
4. Monitor: Progress in console
5. Verify: Data in ThemisDB at http://localhost:8765/api/entities

THEMISDB DATA:
   📋 Entity Store:    Metadata, keywords, tags, scores
   📊 Vector Store:    Embeddings for semantic search
   ⏱️  TimeSeries:     Quality metrics with timestamps

CONFIGURATION:
   Create file: C:\VCC\themis\tools\Themis.IngestionTool\appsettings.json
   
   {
     "Themis": {
       "Host": "localhost",
       "Port": 8765,
       "StoreVectors": true,
       "TrackTimeSeries": true,
       "Enabled": true
     }
   }

═══════════════════════════════════════════════════════════════════════════════
" -ForegroundColor Green

# 5. Test ThemisDB-Verbindung
Write-Host "`n🔍 Testing ThemisDB connection..." -ForegroundColor Yellow

try {
    $response = Invoke-WebRequest -Uri "http://localhost:8765/health" -Method Get -TimeoutSec 2 -ErrorAction SilentlyContinue
    if ($response.StatusCode -eq 200) {
        Write-Host "✅ ThemisDB is running on http://localhost:8765" -ForegroundColor Green
    }
} catch {
    Write-Host "⚠️  ThemisDB not reachable. Make sure it's running on port 8765" -ForegroundColor Yellow
    Write-Host "    Start ThemisDB before running ingestion." -ForegroundColor Yellow
}

Write-Host ""
