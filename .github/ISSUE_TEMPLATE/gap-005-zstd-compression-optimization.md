---
name: ZSTD Compression & Optimization (GAP-005)
about: Implementation template for ZSTD compression optimization features from GAP-005
title: '[ZSTD] '
labels: ['enhancement', 'future', 'content-pipeline', 'performance']
assignees: ''
---

# ZSTD-Komprimierung & Optimierung

**Quelle:** GAP-005-Future-Issues-Template.md - Gruppe 1  
**Kontext:** Die ZSTD-Komprimierung ist bereits vollständig implementiert mit Streaming-Support. Diese Issues fokussieren auf weitere Optimierungen.

**Gesamtaufwand Gruppe:** 6-9 Tage

---

## Issue-Auswahl

Bitte wähle aus, welches der folgenden Issues du implementieren möchtest:

- [ ] **Issue 1:** Dictionary-basierte ZSTD-Kompression für ähnliche Inhalte
- [ ] **Issue 2:** Pipeline-Kompressionsstatistiken und -Metriken
- [ ] **Issue 3:** Batch-Kompression-Optimierung

---

## Issue 1: Dictionary-basierte ZSTD-Kompression für ähnliche Inhalte

### Beschreibung
Implementierung von ZSTD-Dictionary-Training für Content-Sets mit ähnlichen Daten, um höhere Kompressionsraten zu erreichen.

### Labels
- `enhancement`
- `future`
- `content-pipeline`
- `performance`

### Priorität
🟢 Niedrig

### Lösungsansatz

#### Funktionale Anforderungen
1. **Analyse von Content-Sets**
   - Identifikation ähnlicher Daten in Content-Collections
   - Gruppierung von Inhalten nach Similarität
   - Statistiken über Datenähnlichkeit sammeln

2. **Dictionary-Training**
   - Integration von `ZSTD_trainFromBuffer()` API
   - Training auf repräsentativen Datensamples
   - Optimierung der Dictionary-Größe
   - Dictionary-Qualitäts-Metriken

3. **Dictionary-Management**
   - Dictionary-Versionierung und -Storage
   - Lazy Loading von Dictionaries
   - Cache-Management für häufig genutzte Dictionaries
   - Dictionary-Selection basierend auf Content-Type

4. **Validation & Testing**
   - A/B-Testing für Kompressionsraten mit/ohne Dictionary
   - Benchmark-Suite für verschiedene Content-Typen
   - Performance-Messung (Compression Speed vs. Ratio)

#### Implementierungsplan

**Phase 1: Dictionary-Training-Integration** (1-2 Tage)
- [ ] Integration der `ZSTD_trainFromBuffer()` API
- [ ] Implementierung einer TrainingDataCollector-Klasse
- [ ] Sample-Selection-Strategie für Training
- [ ] Dictionary-Generation und -Validierung
- [ ] Unit-Tests für Training-Funktionalität

**Phase 2: Dictionary-Management** (1 Tag)
- [ ] Dictionary-Storage-Schema (Filesystem/DB)
- [ ] DictionaryManager-Klasse implementieren
- [ ] Versionierung und Metadaten-Tracking
- [ ] Dictionary-Loading und -Caching
- [ ] Tests für Dictionary-Lifecycle

**Phase 3: ZstdCompression-Integration** (1 Tag)
- [ ] Erweitern der ZstdCompression-Klasse
- [ ] Dictionary-basierte Compression/Decompression
- [ ] Fallback auf Standard-Compression ohne Dictionary
- [ ] API für Dictionary-Selection
- [ ] Integration-Tests

**Phase 4: Benchmarking und Optimierung** (0.5-1 Tag)
- [ ] A/B-Test-Framework aufsetzen
- [ ] Benchmarks für verschiedene Content-Typen
- [ ] Kompressionsraten-Analyse
- [ ] Performance-Optimierungen
- [ ] Dokumentation der Ergebnisse

#### API-Design

```cpp
// Dictionary Training API
class ZstdDictionaryTrainer {
public:
    // Train dictionary from samples
    Status TrainDictionary(
        const std::vector<std::string_view>& samples,
        size_t dict_size,
        ZstdDictionary* output);
    
    // Train from file samples
    Status TrainFromFiles(
        const std::vector<std::string>& file_paths,
        const TrainingConfig& config,
        ZstdDictionary* output);
    
    // Validate dictionary quality
    DictionaryMetrics EvaluateDictionary(
        const ZstdDictionary& dict,
        const std::vector<std::string_view>& test_samples);
};

// Dictionary Management API
class ZstdDictionaryManager {
public:
    // Store trained dictionary
    Status StoreDictionary(
        const std::string& dict_id,
        const ZstdDictionary& dict,
        const DictionaryMetadata& metadata);
    
    // Load dictionary by ID or content-type
    Status LoadDictionary(
        const std::string& dict_id,
        ZstdDictionary* output);
    
    // Get dictionary for content type
    Status GetDictionaryForContentType(
        const std::string& content_type,
        ZstdDictionary* output);
    
    // List available dictionaries
    std::vector<DictionaryMetadata> ListDictionaries();
    
    // Delete old/unused dictionaries
    Status PurgeDictionary(const std::string& dict_id);
};

// Enhanced ZstdCompression with Dictionary Support
class ZstdCompression {
public:
    // Compress with dictionary
    Status CompressWithDictionary(
        std::string_view input,
        const ZstdDictionary& dict,
        std::string* output);
    
    // Decompress with dictionary
    Status DecompressWithDictionary(
        std::string_view compressed,
        const ZstdDictionary& dict,
        std::string* output);
    
    // Auto-select dictionary for content
    Status CompressAuto(
        std::string_view input,
        const std::string& content_type,
        std::string* output);
};
```

#### Konfigurationsbeispiel

```yaml
zstd_dictionary:
  enabled: true
  training:
    min_samples: 100           # Minimum samples for training
    max_samples: 10000         # Maximum samples to use
    dictionary_size: 110KB     # Target dictionary size
    sample_size: 1MB           # Size of each sample
  management:
    cache_size: 10             # Number of dictionaries to cache
    storage_path: "/var/lib/themisdb/dictionaries"
    auto_cleanup: true         # Auto-delete unused dictionaries
    cleanup_threshold_days: 90 # Delete if not used for 90 days
  content_types:
    - type: "text/plain"
      dictionary_id: "dict_text_v1"
      enabled: true
    - type: "application/json"
      dictionary_id: "dict_json_v1"
      enabled: true
```

#### Test-Anforderungen

**Unit-Tests:**
```cpp
TEST(ZstdDictionaryTrainer, TrainFromSamples)
TEST(ZstdDictionaryTrainer, TrainInsufficientData)
TEST(ZstdDictionaryManager, StoreAndLoadDictionary)
TEST(ZstdDictionaryManager, CacheManagement)
TEST(ZstdCompression, CompressWithDictionary)
TEST(ZstdCompression, DecompressWithDictionary)
TEST(ZstdCompression, FallbackWithoutDictionary)
```

**Benchmark-Tests:**
- Kompressionsrate: Standard vs. Dictionary
- Kompressionsgeschwindigkeit: Standard vs. Dictionary
- Speicher-Overhead für Dictionary-Caching
- Training-Zeit für verschiedene Sample-Größen

#### Erfolgskriterien
- [ ] Dictionary-Training funktioniert mit verschiedenen Content-Sets
- [ ] Kompressionsrate-Verbesserung von mindestens 10-30% für ähnliche Inhalte
- [ ] Dictionary-Management mit Versionierung implementiert
- [ ] A/B-Test-Ergebnisse dokumentiert
- [ ] Performance-Impact < 5% für Compression/Decompression
- [ ] Umfassende Unit-Tests (> 85% Coverage)
- [ ] Benchmark-Suite implementiert
- [ ] Dokumentation vollständig

**Aufwand:** 3-4 Tage

---

## Issue 2: Pipeline-Kompressionsstatistiken und -Metriken

### Beschreibung
Erweiterung der ZstdCompression-Klasse um detaillierte Statistiken (Kompressionsraten, Durchsatz, Zeitverbrauch) für Performance-Monitoring.

### Labels
- `enhancement`
- `future`
- `content-pipeline`
- `monitoring`

### Priorität
🟢 Niedrig

### Lösungsansatz

#### Funktionale Anforderungen
1. **Metriken-Sammlung**
   - Kompressionsraten (ratio, savings)
   - Durchsatz (bytes/second)
   - Zeitverbrauch (latency, p50, p95, p99)
   - Erfolgs-/Fehlerrate
   - Memory-Usage

2. **Prometheus-Integration**
   - Metriken-Export über Prometheus-Exporter
   - Counter, Gauges, Histograms
   - Labeling (content-type, compression-level)
   - `/metrics` Endpoint

3. **Statistik-Aggregation**
   - Aggregation über Zeitfenster (1m, 5m, 1h, 24h)
   - Historische Trends
   - Moving Averages
   - Anomalie-Detection

4. **Grafana-Dashboards**
   - Visualisierung der Key-Metriken
   - Alerting-Rules
   - Dashboard-Templates

#### Implementierungsplan

**Phase 1: Metriken-Interface** (0.5 Tag)
- [ ] CompressionMetrics-Struktur definieren
- [ ] Metriken-Collection-Points in ZstdCompression
- [ ] Thread-safe Metriken-Aggregation
- [ ] Unit-Tests für Metriken-Collection

**Phase 2: Prometheus-Integration** (0.5-1 Tag)
- [ ] Prometheus-Client-Library-Integration
- [ ] Metriken-Registrierung (Counters, Histograms, Gauges)
- [ ] `/metrics` HTTP-Endpoint
- [ ] Label-Schema für Metriken
- [ ] Integration-Tests

**Phase 3: Statistik-Aggregation** (0.5 Tag)
- [ ] Time-Window-basierte Aggregation
- [ ] Historische Daten-Speicherung
- [ ] Statistik-API für Abfragen
- [ ] Tests für Aggregation-Logik

**Phase 4: Grafana-Dashboards** (0.5 Tag)
- [ ] Dashboard-Templates erstellen
- [ ] Wichtige Visualisierungen (Compression Ratio, Throughput)
- [ ] Alerting-Rules definieren
- [ ] Dashboard-Dokumentation

#### API-Design

```cpp
// Compression Metrics Structure
struct CompressionMetrics {
    // Size metrics
    size_t input_bytes;
    size_t output_bytes;
    double compression_ratio;
    
    // Performance metrics
    std::chrono::microseconds duration;
    double throughput_mbps;
    
    // Context
    std::string content_type;
    int compression_level;
    bool dictionary_used;
    
    // Timestamp
    std::chrono::system_clock::time_point timestamp;
};

// Metrics Collector Interface
class CompressionMetricsCollector {
public:
    // Record compression operation
    void RecordCompression(const CompressionMetrics& metrics);
    
    // Get aggregated statistics
    AggregatedMetrics GetStatistics(
        std::chrono::seconds window) const;
    
    // Export for Prometheus
    std::string ExportPrometheus() const;
    
    // Get recent metrics
    std::vector<CompressionMetrics> GetRecentMetrics(
        size_t count) const;
};

// Enhanced ZstdCompression with Metrics
class ZstdCompression {
private:
    std::shared_ptr<CompressionMetricsCollector> metrics_collector_;
    
public:
    // Existing methods now collect metrics automatically
    Status Compress(std::string_view input, std::string* output);
    
    // Get metrics collector
    std::shared_ptr<CompressionMetricsCollector> GetMetricsCollector();
};
```

#### Prometheus-Metriken

```prometheus
# Compression operations total
themisdb_compression_operations_total{type="compress",content_type="text",level="3"} 1234

# Compression ratio histogram
themisdb_compression_ratio{content_type="text"} 0.35

# Compression duration histogram (seconds)
themisdb_compression_duration_seconds_bucket{le="0.001"} 100
themisdb_compression_duration_seconds_bucket{le="0.01"} 500
themisdb_compression_duration_seconds_bucket{le="0.1"} 1000

# Compression throughput (bytes/sec)
themisdb_compression_throughput_bytes_per_second{content_type="text"} 50000000

# Input/Output bytes total
themisdb_compression_input_bytes_total 1000000000
themisdb_compression_output_bytes_total 350000000

# Errors total
themisdb_compression_errors_total{error_type="buffer_overflow"} 5
```

#### Konfigurationsbeispiel

```yaml
compression_metrics:
  enabled: true
  collection:
    sample_rate: 1.0           # Collect 100% of operations
    buffer_size: 10000         # Buffer for metrics
  prometheus:
    enabled: true
    endpoint: "/metrics"
    port: 9090
  aggregation:
    windows: [60, 300, 3600]   # 1m, 5m, 1h
    retention: 86400           # Keep 24h of data
  grafana:
    dashboard_path: "/etc/themisdb/dashboards"
```

#### Test-Anforderungen

**Unit-Tests:**
```cpp
TEST(CompressionMetrics, RecordCompression)
TEST(CompressionMetrics, AggregateStatistics)
TEST(CompressionMetrics, PrometheusExport)
TEST(CompressionMetrics, ThreadSafety)
TEST(ZstdCompression, MetricsCollection)
```

**Integration-Tests:**
- Prometheus-Scraping funktioniert
- Metriken sind korrekt formatiert
- Grafana-Dashboard zeigt Daten korrekt an
- Alerting-Rules funktionieren

#### Erfolgskriterien
- [ ] Metriken-Interface in ZstdCompression integriert
- [ ] Prometheus-Integration funktioniert
- [ ] Metriken werden korrekt aggregiert
- [ ] Grafana-Dashboard erstellt und getestet
- [ ] Performance-Overhead < 1% für Metriken-Collection
- [ ] Unit-Tests vollständig (> 90% Coverage)
- [ ] Dokumentation vollständig (Setup, Metriken-Bedeutung)

**Aufwand:** 1-2 Tage

---

## Issue 3: Batch-Kompression-Optimierung

### Beschreibung
Optimierung der Kompression für mehrere gleichartige Dateien durch gemeinsame Dictionary-Nutzung und parallele Verarbeitung.

### Labels
- `enhancement`
- `future`
- `content-pipeline`
- `performance`

### Priorität
🟢 Niedrig

### Lösungsansatz

#### Funktionale Anforderungen
1. **Batch-API**
   - Batch-Compression für mehrere Dateien
   - Async/Parallel-Verarbeitung
   - Progress-Tracking
   - Error-Handling für einzelne Dateien

2. **Shared Dictionary**
   - Dictionary-Training auf Batch-Daten
   - Gemeinsame Dictionary-Nutzung für alle Batch-Items
   - Dictionary-Caching während Batch-Verarbeitung

3. **Parallelisierung**
   - Thread-Pool für parallele Kompression
   - Work-Stealing-Scheduler
   - CPU-Core-basierte Thread-Count
   - Load-Balancing

4. **Memory-Pool**
   - Buffer-Reuse zwischen Kompressionen
   - Memory-Pool für Compression-Contexts
   - Memory-Limit-Enforcement

#### Implementierungsplan

**Phase 1: Batch-API** (1 Tag)
- [ ] BatchCompressionRequest-Struktur
- [ ] BatchCompressor-Klasse implementieren
- [ ] Async-Batch-Verarbeitung
- [ ] Progress-Callback-Mechanismus
- [ ] Error-Handling und Partial-Success
- [ ] Unit-Tests

**Phase 2: Shared Dictionary-Support** (0.5 Tag)
- [ ] Dictionary-Training auf Batch-Sample
- [ ] Dictionary-Sharing während Batch
- [ ] Dictionary-Cache für Batch-Session
- [ ] Tests für Shared-Dictionary-Logik

**Phase 3: Parallelisierung** (0.5-1 Tag)
- [ ] Thread-Pool-Integration
- [ ] Work-Queue für Batch-Items
- [ ] Task-Scheduling mit Priorities
- [ ] Thread-Pool-Size-Konfiguration
- [ ] Performance-Tests

**Phase 4: Memory-Pool-Optimierung** (0.5-1 Tag)
- [ ] Buffer-Pool implementieren
- [ ] Compression-Context-Pool
- [ ] Memory-Limiting
- [ ] Buffer-Reuse-Tests
- [ ] Memory-Leak-Tests

#### API-Design

```cpp
// Batch Compression Request
struct BatchCompressionItem {
    std::string id;                    // Unique identifier
    std::string_view input;            // Input data
    std::string content_type;          // Content type
    std::optional<int> compression_level;
};

struct BatchCompressionResult {
    std::string id;                    // Item identifier
    Status status;                     // Success/Error
    std::string output;                // Compressed data
    CompressionMetrics metrics;        // Metrics for this item
};

// Progress Callback
using BatchProgressCallback = std::function<void(
    size_t completed,
    size_t total,
    const BatchCompressionResult& latest_result)>;

// Batch Compressor
class BatchZstdCompressor {
public:
    // Compress multiple items with shared dictionary
    std::vector<BatchCompressionResult> CompressBatch(
        const std::vector<BatchCompressionItem>& items,
        const BatchCompressionConfig& config,
        BatchProgressCallback progress_cb = nullptr);
    
    // Async batch compression
    std::future<std::vector<BatchCompressionResult>> CompressBatchAsync(
        const std::vector<BatchCompressionItem>& items,
        const BatchCompressionConfig& config,
        BatchProgressCallback progress_cb = nullptr);
    
    // Compress batch with auto-trained dictionary
    std::vector<BatchCompressionResult> CompressBatchWithAutoDict(
        const std::vector<BatchCompressionItem>& items,
        const BatchCompressionConfig& config);
};

// Memory Pool for Compression
class CompressionMemoryPool {
public:
    // Get buffer from pool
    std::unique_ptr<Buffer> AcquireBuffer(size_t size);
    
    // Return buffer to pool
    void ReleaseBuffer(std::unique_ptr<Buffer> buffer);
    
    // Get compression context
    ZSTD_CCtx* AcquireContext();
    
    // Return compression context
    void ReleaseContext(ZSTD_CCtx* ctx);
    
    // Pool statistics
    MemoryPoolStats GetStats() const;
};
```

#### Konfigurationsbeispiel

```yaml
batch_compression:
  enabled: true
  parallelization:
    thread_pool_size: 0        # 0 = auto (CPU cores)
    max_concurrent_items: 16   # Max items in flight
    work_stealing: true        # Enable work stealing
  shared_dictionary:
    enabled: true
    train_on_batch: true       # Train dict from batch samples
    sample_count: 100          # Samples for training
  memory_pool:
    buffer_pool_size: 32       # Number of buffers to pool
    context_pool_size: 16      # Number of contexts to pool
    max_memory_mb: 512         # Max memory for batch operations
  progress_reporting:
    enabled: true
    interval_ms: 1000          # Report progress every 1s
```

#### Test-Anforderungen

**Unit-Tests:**
```cpp
TEST(BatchCompressor, CompressBatch)
TEST(BatchCompressor, CompressBatchAsync)
TEST(BatchCompressor, SharedDictionary)
TEST(BatchCompressor, ParallelExecution)
TEST(BatchCompressor, ProgressCallback)
TEST(BatchCompressor, ErrorHandling)
TEST(MemoryPool, BufferReuse)
TEST(MemoryPool, ContextReuse)
TEST(MemoryPool, MemoryLimits)
```

**Performance-Tests:**
- Batch-Throughput: Einzeln vs. Batch vs. Batch+Dict
- Parallelisierungs-Speedup: 1, 2, 4, 8 Threads
- Memory-Pool-Overhead vs. Allocation
- Shared-Dictionary-Impact auf Kompressionsrate

**Integration-Tests:**
- Batch mit 100+ Dateien
- Batch mit Mixed-Content-Types
- Batch mit Error-Handling
- Async-Batch mit Cancellation

#### Erfolgskriterien
- [ ] Batch-API implementiert und getestet
- [ ] Parallelisierung zeigt signifikanten Speedup (3-5x bei 8 Cores)
- [ ] Shared Dictionary verbessert Kompressionsrate um 10-20%
- [ ] Memory-Pool reduziert Allocation-Overhead um > 50%
- [ ] Progress-Tracking funktioniert korrekt
- [ ] Error-Handling robust (einzelne Fehler brechen Batch nicht ab)
- [ ] Unit-Tests vollständig (> 85% Coverage)
- [ ] Performance-Benchmarks dokumentiert
- [ ] Dokumentation vollständig

**Aufwand:** 2-3 Tage

---

## Allgemeine Anforderungen

### Dependencies
<!-- Was muss vorher implementiert sein? -->
- **Requires**: Vollständige ZstdCompression-Implementierung mit Streaming-Support (bereits vorhanden)
- **Blocks**: Content-Pipeline Production-Readiness
- **Related Issues**: 
  - GAP-005 Content Pipeline Implementation
  - Storage-Tiering-Features
  - Bulk-Upload-Optimierungen

### Integration Points
<!-- Mit welchen Systemen integriert dies? -->
- [ ] ZstdCompression-Klasse (src/content/compression/)
- [ ] ContentManager (src/content/)
- [ ] AsyncBulkUploader (src/content/)
- [ ] Prometheus-Exporter (src/monitoring/)
- [ ] Storage-Layer (src/storage/)

### Documentation Requirements
- [ ] API-Dokumentation (Doxygen-Comments)
- [ ] Nutzungs-Guide für neue Features
- [ ] Konfigurations-Guide
- [ ] Benchmark-Ergebnisse dokumentieren
- [ ] Upgrade-Guide (falls breaking changes)

### Code Review Checklist
- [ ] Code folgt Projekt-Style-Guide (.clang-format)
- [ ] Alle Tests erfolgreich (Unit + Integration)
- [ ] Performance-Benchmarks zeigen Verbesserungen
- [ ] Keine Memory-Leaks (Valgrind/ASAN)
- [ ] Thread-Safety gewährleistet
- [ ] Error-Handling robust
- [ ] Logging angemessen
- [ ] Dokumentation vollständig

---

## Label-Schema

**Empfohlene Labels für diese Issues:**

**Haupt-Labels:**
- `enhancement` - Neue Features/Verbesserungen
- `future` - Geplante zukünftige Features
- `content-pipeline` - Content-Pipeline-bezogen

**Technische Labels:**
- `performance` - Performance-Optimierungen
- `monitoring` - Monitoring/Observability
- `storage` - Storage-bezogen

**Prioritäts-Labels:**
- `priority:low` - Niedrige Priorität (alle Issues dieser Gruppe)

**Status-Labels (während Bearbeitung):**
- `status:in-progress` - In Bearbeitung
- `status:review` - Code Review
- `status:testing` - Testing-Phase

---

## References
<!-- Relevante Dokumentation und Links -->

- **GAP-005 Dokument**: `docs/de/development/GAP-005-Future-Issues-Template.md`
- **ZSTD Dokumentation**: https://facebook.github.io/zstd/
- **ZSTD API Reference**: https://facebook.github.io/zstd/zstd_manual.html
- **Dictionary Training**: https://github.com/facebook/zstd/blob/dev/programs/zstd.1.md#dictionary-builder
- **Prometheus Client**: https://prometheus.io/docs/instrumenting/clientlibs/
- **Content Pipeline Design**: `docs/architecture/content-pipeline.md`

---

**Checklist:**
- [ ] Ich habe das Issue ausgewählt, das ich bearbeiten möchte
- [ ] Ich habe die funktionalen Anforderungen verstanden
- [ ] Ich habe den Lösungsansatz gelesen
- [ ] Ich habe die Erfolgskriterien zur Kenntnis genommen
- [ ] Ich habe bestehende Issues geprüft (keine Duplikate)
- [ ] Ich habe die Dependencies identifiziert
