# Load Testing Guide - Phase 5

## Übersicht

Dieses Dokument beschreibt die Load-Testing-Strategie für die Themis Ingestion Tool mit 1000+ Dateien unter realistischen Bedingungen.

## Testszenarien

### Scenario 1: Standard Load Test (500 Dateien)
**Ziel**: Baseline-Performance mit Cache-Warm-Up

```powershell
# 500 verschiedene PDF-Dateien, durchschnittlich 2MB
# MaxParallelFiles: 4
# EnableCaching: true
# EnableBatching: true
```

**Erwartungen**:
- Cache Hit Rate: 0-5% (erste Iteration)
- Avg Time/File: 2-3 Sekunden (LLM + Embedding)
- Memory: < 500MB
- CPU: 40-60%

### Scenario 2: High Load Test (1000 Dateien)
**Ziel**: Performance unter vollem Load mit Cache-Benefits

```powershell
# 1000 Dateien mit Duplikaten (80% unique, 20% duplicates)
# MaxParallelFiles: 8
# EnableCaching: true
# EnableBatching: true
```

**Erwartungen**:
- Cache Hit Rate: 15-25% (Duplikate führen zu Hits)
- Avg Time/File: 1.5-2.5 Sekunden
- Memory: < 800MB (LRU Eviction aktiv)
- CPU: 60-80%

### Scenario 3: Cache Efficiency Test (100 Dateien, 2x)
**Ziel**: Cache-Performance bei Wiederholungs-Läufen

```
Lauf 1: 100 verschiedene Dateien
Lauf 2: Gleiche 100 Dateien (alle aus Cache)
```

**Erwartungen**:
- Lauf 1: Avg 2.5 Sekunden/Datei
- Lauf 2: Avg 0.3 Sekunden/Datei (80-90% Cache Hits)
- Speedup: ~8x

### Scenario 4: Embedded Metadata Test (250 Dateien)
**Ziel**: Test mit aktiviertem Graph/Vector Metadata

```powershell
# MaxParallelFiles: 4
# EnableVectorMetadata: true
# EnableGraphMetadata: true
# StoreVectors: true
```

**Erwartungen**:
- Zusätzliche Embedding-Zeit: +20%
- Vector Index Growth: ~100MB pro 250 Dateien
- Memory Peak: ~600MB

### Scenario 5: Resilience & Failure Handling
**Ziel**: Test des Fehlerbehandlungsmechanismus

Szenarien:
1. Ollama Service stoppen nach 100 Dateien → Fallback zu Hash-Embeddings
2. ThemisDB nicht erreichbar → Graceful Degradation mit JSON Export
3. Polly Circuit Breaker Trigger → Fast-Fail verhindern weitere 5 Fehler

## Messmetriken

### Performance-Metriken
```
- Total Processing Time
- Avg Time per File
- Min/Max/Median Processing Time
- Throughput (Files/Second)
- Cache Hit Rate (Embeddings & LLM Responses)
```

### Resource-Metriken
```
- Memory Usage (Start, Peak, End)
- CPU Usage (Avg, Peak)
- Disk I/O (Reads/Writes)
- Vector Index Size
```

### Reliability-Metriken
```
- Successful Insertions
- Failed Insertions (with retry count)
- Circuit Breaker Triggers
- Fallback Usage Count
```

## Testdaten-Vorbereitung

### Generate Test Files
```powershell
# Script: generate-test-files.ps1
param([int]$Count = 500, [int]$SizeKB = 2000)

$testDir = "test-data"
New-Item -ItemType Directory -Path $testDir -Force | Out-Null

for ($i = 0; $i -lt $Count; $i++) {
    $content = "Test Document $i`n" + ("Lorem ipsum dolor sit amet... " * 1000)
    $path = Join-Path $testDir "document_$i.txt"
    [System.IO.File]::WriteAllText($path, $content)
    
    if ($i % 100 -eq 0) { Write-Host "Generated $i files..." }
}

Write-Host "✅ Generated $Count test files"
```

### Generate Duplicates
```powershell
# Script: generate-duplicates.ps1
param([int]$Original = 100, [int]$DuplicateRatio = 0.2)

$testDir = "test-data"
$duplicateCount = [int]($Original * $DuplicateRatio)

for ($i = 0; $i -lt $duplicateCount; $i++) {
    $sourceIndex = Get-Random -Minimum 0 -Maximum $Original
    $sourcePath = Join-Path $testDir "document_$sourceIndex.txt"
    $destPath = Join-Path $testDir "document_duplicate_$i.txt"
    
    Copy-Item $sourcePath $destPath
}

Write-Host "✅ Created $duplicateCount duplicate files"
```

## Testdurchführung

### Manual Testing
```powershell
# 1. Starten Sie den Application
& 'C:\VCC\themis\tools\Themis.IngestionTool\bin\Release\net8.0-windows\Themis.IngestionTool.exe'

# 2. Starten Sie Ollama (falls verfügbar)
ollama serve

# 3. Setzen Sie Test-Parameter
# - Source Folder: test-data
# - EnableCaching: true
# - MaxParallelFiles: 4

# 4. Klicken Sie "Start Ingestion"

# 5. Beobachten Sie:
# - Progress Bar
# - File Processing Rate
# - Memory Usage (Task Manager)
# - Cache Hit Rate (Dashboard)
```

### Automated Testing (Future)
```csharp
// Load Test Runner
var tester = new LoadTestRunner();

// Scenario 1: Standard Load
var result1 = await tester.RunLoadTestAsync(
    fileCount: 500,
    parallelism: 4,
    duration: TimeSpan.FromMinutes(30)
);
```

## Performance Baselines (Expected)

### Single Threaded (1 File)
```
Processing: 2.5 seconds
- LLM Analysis: 1.5s
- Embedding Generation: 0.8s
- DB Storage: 0.2s
```

### Parallel (4 Files)
```
Throughput: ~1.6 Files/Second
- File 1: 2.5s
- Files 2-4 (parallel): 2.5s each
- Total for 4: ~2.5s (parallel efficiency)
```

### With Cache
```
Hit: 0.1 seconds
Miss: 2.5 seconds
Average (20% hit rate): 2.0 seconds/file
```

## Tuning Parameters

### For Speed
```json
{
  "MaxParallelFiles": 8,
  "BatchSize": 20,
  "CacheMaxSize": 5000,
  "CacheTTLMinutes": 120
}
```

### For Stability
```json
{
  "MaxParallelFiles": 2,
  "MaxRetries": 5,
  "CircuitBreakerThreshold": 10,
  "CircuitBreakerDurationSeconds": 60
}
```

## Monitoring During Test

### Real-Time Metrics (Dashboard)
- Progress: X/Y files processed
- Current File: filename + processing time
- Cache Stats: Hits, Misses, Hit Rate
- Memory: Current usage + peak

### Logging
```
[INFO] Processing started: 500 files
[INFO] File 1/500: document_0.txt (2.3s)
[INFO] File 2/500: document_1.txt (2.4s)
[CACHE] Hit rate: 0/2 (0%)
...
[WARN] Embedding service unavailable, using fallback
[INFO] Circuit breaker opened after 5 failures
...
[INFO] Processing complete: 500/500 files, 1247s total
```

## Success Criteria

✅ **Performance**
- Throughput: >= 0.5 Files/Second (with cache)
- Cache Hit Rate: >= 80% on repeat runs
- Memory: < 1GB peak

✅ **Reliability**
- Success Rate: >= 95%
- Failed Insertions: < 5% with auto-retry
- Circuit Breaker prevents cascade failures

✅ **Resilience**
- Graceful degradation when Ollama unavailable
- Fallback to hash-based embeddings works
- Application doesn't crash on DB errors

## Reporting

### Test Report Template
```
Load Test Report - Phase 5
=========================

Date: 2026-01-01
Duration: 30 minutes
Files Processed: 1000

Performance:
- Avg Time/File: 1.8s
- Throughput: 0.55 files/sec
- Cache Hit Rate: 22%
- Memory Peak: 620MB

Reliability:
- Success Rate: 99.2%
- Failed Insertions: 8
- Circuit Breaker Triggers: 0

Recommendations:
- Increase MaxParallelFiles to 8 for better throughput
- Cache is working well - consider increasing CacheMaxSize
- Consider HNSW index optimization for vector searches
```

## Next Steps (Phase 6)

1. Implement automated load test runner
2. Add performance profiling (CPU, Memory, I/O)
3. Optimize hot paths based on profiling results
4. Test with different Ollama models (nomic vs all-minilm)
5. Benchmark vector similarity search performance
6. Load test graph traversal with 1000+ relationships
