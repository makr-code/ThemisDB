# Batch-Processing und Kompression: Weitere Optimierungsmöglichkeiten

**Stand:** 15. Dezember 2025  
**Version:** 1.0.0  
**Status:** Analyse  
**Kategorie:** Technical Analysis

---

## Überblick

Diese Analyse identifiziert weitere Stellen in ThemisDB, wo **Batch-Verarbeitung** und **Single-Point-to-Batch** mit Kompression fehlen oder sinnvoll wären, basierend auf dem erfolgreichen Muster von `TSAutoBuffer` für Time-Series-Daten.

## Bereits implementiert: TSAutoBuffer ✅

**Komponente:** Time Series (TSStore)  
**Pattern:** Auto-Batching mit Gorilla-Kompression  
**Implementierung:** `include/timeseries/ts_auto_buffer.h`

**Features:**
- Automatisches Buffering einzelner Datenpunkte
- Multi-Threshold Flush (Größe, Zeit, Speicher)
- Per-metric:entity Gruppierung
- 10-20x Kompression

---

## 1. Vector Index (VectorIndexManager) 🔴 **HIGH PRIORITY**

### Aktuelle Situation

**Vorhanden:**
- `addBatch()`, `updateBatch()`, `removeBatch()` für Bulk-Operationen
- Direkte `addEntity()`, `updateEntity()` für Einzelpunkte

**Problem:**
```cpp
// Datei: include/index/vector_index.h, Zeile 61-70
Status addEntity(const BaseEntity& e, std::string_view vectorField = "embedding");

// HTTP-API nutzt einzelne addEntity() Calls
// Keine automatische Pufferung für Streaming-Ingestion
```

### Fehlende Features

1. **Kein Auto-Buffer für Vector-Inserts**
   - Streaming-Embedding-Generierung kann nicht von Batch-Operationen profitieren
   - Jeder `addEntity()` Call führt zu direktem RocksDB-Write + HNSW-Update

2. **Keine Kompression für Embedding-Vektoren**
   - Float32-Arrays (z.B. 768 dim × 4 bytes = 3 KB pro Vektor)
   - Bei Millionen von Vektoren: Hunderte GB Speicher
   - Potenzielle Kompression:
     - Quantization (Float32 → Int8): 4x Reduktion
     - Sparse Encoding für Transformers: 10-50% Einsparung
     - Vector Compression (Product Quantization): 10-32x

### Empfohlene Lösung: VectorAutoBuffer

**Analog zu TSAutoBuffer:**

```cpp
// include/index/vector_auto_buffer.h
class VectorAutoBuffer {
public:
    struct Config {
        size_t max_vectors_per_buffer = 1000;
        std::chrono::milliseconds flush_interval{5000};
        size_t max_memory_bytes = 500 * 1024 * 1024;  // 500 MB
        
        // Compression options
        enum class Compression {
            None,
            Quantization,      // Float32 → Int8/Int16
            ProductQuantization,  // PQ for HNSW
            Sparse              // Sparse vector encoding
        };
        Compression compression = Compression::None;
    };
    
    Status add(const BaseEntity& entity, std::string_view vectorField = "embedding");
    size_t flush();
};
```

**Vorteile:**
- **Batch-HNSW-Updates:** HNSW-Index-Updates können in Batches effizienter sein
- **Kompression:** Speicherreduktion um 4-32x
- **Throughput:** 10-50x höherer Durchsatz
- **Identisches Pattern** wie TSAutoBuffer → Wiederverwendung

**Use Cases:**
- Embedding-Generierung für große Dokument-Sammlungen
- Real-time RAG-Ingestion
- Knowledge Graph Embeddings

---

## 2. Changefeed (CDC) 🟡 **MEDIUM PRIORITY**

### Aktuelle Situation

**Vorhanden:**
- Event-basierte CDC mit `putEvent()`
- Sequence-Numbers für Ordering

**Problem:**
```cpp
// Datei: include/cdc/changefeed.h
// Einzelne Events werden direkt geschrieben
Status putEvent(const ChangeEvent& event);

// Keine Batch-Operationen
// Keine Kompression für Event-Payloads
```

### Fehlende Features

1. **Kein Batch-Event-Ingestion**
   - Transaktionen mit vielen Änderungen schreiben Events einzeln
   - Hohe Latenz bei Bulk-Updates

2. **Keine Event-Kompression**
   - JSON-Events mit großen Payloads (z.B. geänderte Dokumente)
   - Potenzielle Kompression: Zstd/Gorilla für Timestamps

### Empfohlene Lösung: ChangefeedBuffer

```cpp
class ChangefeedBuffer {
public:
    struct Config {
        size_t max_events_per_batch = 500;
        std::chrono::milliseconds flush_interval{1000};  // 1s für niedrige Latenz
        bool compress_payloads = true;  // Zstd compression
    };
    
    Status addEvent(const ChangeEvent& event);
    Status addEventsBatch(const std::vector<ChangeEvent>& events);
    size_t flush();
};
```

**Vorteile:**
- **Niedrigere Latenz** für Bulk-Transaktionen
- **Kompression:** JSON-Payloads mit Zstd (3-5x)
- **Höherer Durchsatz** für CDC-Konsumenten

**Use Cases:**
- Transaction-Commit mit vielen Änderungen
- Materialized View Updates
- Audit-Log-Streaming

---

## 3. Metrics Collector 🟡 **MEDIUM PRIORITY**

### Aktuelle Situation

**Vorhanden:**
- Direktes Recording einzelner Metriken

**Problem:**
```cpp
// Datei: include/observability/metrics_collector.h, Zeile 33
void recordTSStoreWrite(const std::string& metric, size_t batch_size, double latency_ms);

// Jeder Aufruf akquiriert Mutex
// Keine Aggregation vor Prometheus-Export
```

### Fehlende Features

1. **Kein Lock-Free Buffering**
   - Mutex-Contention bei High-Throughput-Metriken
   - Jeder `recordXXX()` Call ist ein Mutex-Lock

2. **Keine Vor-Aggregation**
   - Rohdaten werden gespeichert
   - Prometheus-Export berechnet Aggregationen zur Laufzeit

### Empfohlene Lösung: Metrics Ring Buffer

```cpp
// Lock-free Ring Buffer für High-Throughput Metrics
class MetricsRingBuffer {
private:
    struct MetricEvent {
        uint64_t timestamp_ns;
        std::string metric_name;
        double value;
    };
    
    // Lock-free circular buffer (Boost.Lockfree oder std::atomic)
    std::vector<std::atomic<MetricEvent>> buffer_;
    std::atomic<size_t> write_index_{0};
    std::atomic<size_t> read_index_{0};
    
public:
    void record(const std::string& metric, double value) {
        // Lock-free write
        size_t idx = write_index_.fetch_add(1) % buffer_.size();
        buffer_[idx].store({now(), metric, value}, std::memory_order_release);
    }
    
    // Background-Thread aggregiert periodisch
    void aggregate();
};
```

**Vorteile:**
- **Lock-free:** Keine Mutex-Contention
- **Höherer Throughput:** 10-100x schnelleres Recording
- **Vor-Aggregation:** Weniger Speicher, schnellerer Export

---

## 4. Content Import 🟢 **LOW PRIORITY** (bereits gut)

### Aktuelle Situation

**Vorhanden:**
- Batch-Import via `/content/import` Endpoint
- `ImportOptions.batch_size = 1000`

**Assessment:**
```cpp
// Datei: include/importers/importer_interface.h, Zeile 53
size_t batch_size = 1000;  // Already configurable
```

✅ **Bereits gut implementiert!**

**Mögliche Verbesserungen:**
- Auto-Tuning der `batch_size` basierend auf Speicher/CPU
- Kompression für Import-Payloads (Zstd)

---

## 5. Graph Operations 🟡 **MEDIUM PRIORITY**

### Aktuelle Situation

**Vorhanden:**
- `addNodesBatch()`, `addEdgesBatch()` für Property Graphs
- WriteBatch für Atomizität

**Problem:**
```cpp
// Datei: include/index/property_graph.h, Zeile 174-177
Status addNodesBatch(const std::vector<BaseEntity>& nodes, ...);
Status addEdgesBatch(const std::vector<BaseEntity>& edges, ...);

// Aber: Keine Auto-Buffer für Streaming-Ingestion
// Kein Kompression für große Property-Payloads
```

### Fehlende Features

1. **Kein Auto-Buffer für Streaming Graph Construction**
   - Knowledge Graph Ingestion aus Streams
   - Real-time Graph Updates

2. **Keine Property-Kompression**
   - JSON-Properties können groß sein (z.B. Dokument-Metadaten)
   - Zstd-Kompression möglich

### Empfohlene Lösung: GraphAutoBuffer

```cpp
class GraphAutoBuffer {
public:
    struct Config {
        size_t max_nodes_per_buffer = 1000;
        size_t max_edges_per_buffer = 1000;
        std::chrono::milliseconds flush_interval{5000};
        bool compress_properties = true;  // Zstd für JSON
    };
    
    Status addNode(const BaseEntity& node);
    Status addEdge(const BaseEntity& edge);
    size_t flush();
};
```

**Use Cases:**
- Knowledge Graph Construction
- Social Network Ingestion
- Real-time Relationship Mining

---

## 6. Replication (WAL Shipping) 🔴 **HIGH PRIORITY**

### Aktuelle Situation

**Vorhanden:**
- `WALShipper` mit Batch-Konfiguration
- `batch_size = 100`

**Problem:**
```cpp
// Datei: include/sharding/wal_shipper.h, Zeile 49-50
size_t batch_size = 100;
size_t max_batch_bytes = 1024 * 1024;  // 1 MB

// Aber: Keine Kompression für WAL-Batches
// Keine adaptive Batch-Größe basierend auf Netzwerkbedingungen
```

### Fehlende Features

1. **Keine WAL-Kompression**
   - WAL-Entries enthalten oft redundante Daten
   - Zstd/LZ4-Kompression: 3-10x Reduktion
   - Kritisch für Geo-Replikation

2. **Keine adaptive Batching**
   - Feste `batch_size` unabhängig von:
     - Netzwerklatenz
     - Bandbreite
     - Replikations-Lag

### Empfohlene Lösung: Compressed WAL Shipping

```cpp
class CompressedWALShipper : public WALShipper {
public:
    struct Config : WALShipper::Config {
        enum class Compression {
            None,
            LZ4,    // Schnell, 2-4x
            Zstd    // Langsamer, 3-10x
        };
        Compression compression = Compression::LZ4;
        
        // Adaptive batching
        bool adaptive_batch_size = true;
        size_t min_batch_size = 10;
        size_t max_batch_size = 1000;
    };
    
    bool shipBatch(const std::vector<WALEntry>& entries) override;
};
```

**Vorteile:**
- **Netzwerk-Bandbreite:** 3-10x Reduktion
- **Geo-Replikation:** Schnellere Synchronisation
- **Kosten:** Weniger Daten-Transfer

---

## 7. GNN Embeddings 🟢 **LOW PRIORITY** (bereits gut)

### Aktuelle Situation

**Vorhanden:**
- `generateNodeEmbeddingsBatch()` mit `batch_size = 32`
- `generateEdgeEmbeddingsBatch()`

**Assessment:**
```cpp
// Datei: include/index/gnn_embeddings.h, Zeile 248-252
Status generateNodeEmbeddingsBatch(
    const std::vector<std::string>& node_ids,
    const std::string& model_name,
    size_t batch_size = 32
);
```

✅ **Bereits gut implementiert!**

**Mögliche Verbesserungen:**
- Embedding-Caching mit TTL
- Lazy Embedding-Generation

---

## Zusammenfassung & Priorisierung

### 🔴 HIGH PRIORITY

1. **VectorAutoBuffer** (Vector Index)
   - **Impact:** Sehr hoch (RAG, Semantic Search)
   - **Aufwand:** Mittel (Analog zu TSAutoBuffer)
   - **ROI:** 10-50x Durchsatz, 4-32x Speicher

2. **Compressed WAL Shipping** (Replication)
   - **Impact:** Hoch (Geo-Replikation)
   - **Aufwand:** Niedrig (Nur Kompression hinzufügen)
   - **ROI:** 3-10x Bandbreite

### 🟡 MEDIUM PRIORITY

3. **ChangefeedBuffer** (CDC)
   - **Impact:** Mittel (Bulk-Transaktionen)
   - **Aufwand:** Niedrig
   - **ROI:** 3-5x Kompression, niedrigere Latenz

4. **GraphAutoBuffer** (Property Graphs)
   - **Impact:** Mittel (Knowledge Graphs)
   - **Aufwand:** Mittel
   - **ROI:** 2-5x Durchsatz

5. **MetricsRingBuffer** (Observability)
   - **Impact:** Mittel (High-Throughput-Metriken)
   - **Aufwand:** Hoch (Lock-free Design)
   - **ROI:** 10-100x Recording-Throughput

### 🟢 LOW PRIORITY (bereits gut)

6. Content Import ✅
7. GNN Embeddings ✅

---

## Gemeinsame Design-Patterns

Alle vorgeschlagenen Lösungen folgen dem **TSAutoBuffer-Pattern**:

```cpp
template<typename T>
class AutoBuffer {
public:
    struct Config {
        size_t max_items_per_buffer;
        std::chrono::milliseconds flush_interval;
        size_t max_memory_bytes;
        CompressionType compression;
    };
    
    Status add(const T& item);
    size_t flush();
    
private:
    std::map<std::string, std::deque<T>> buffers_;  // Per-key buffering
    std::thread flush_thread_;
    std::mutex buffers_mutex_;
};
```

**Wiederverwendbare Komponenten:**
- Threading-Pattern (Background-Flush)
- Multi-Threshold-Logik (Size, Time, Memory)
- Statistiken (Counters, Latencies)
- Konfiguration (Config-Structs)

---

## Nächste Schritte

### Phase 1: VectorAutoBuffer (Q1 2026)
1. Header-Design (`include/index/vector_auto_buffer.h`)
2. Implementation mit Quantization-Support
3. Integration mit HTTP-API (`/vectors/add/buffered`)
4. Benchmarks vs. direktem Insert

### Phase 2: Compressed WAL Shipping (Q1 2026)
1. LZ4/Zstd-Integration in `WALShipper`
2. Adaptive Batch-Sizing
3. Performance-Tests (Geo-Replikation)

### Phase 3: ChangefeedBuffer & GraphAutoBuffer (Q2 2026)
1. Implementation analog zu TSAutoBuffer
2. Integration mit Transaction-Manager
3. End-to-End-Tests

### Phase 4: MetricsRingBuffer (Q2 2026)
1. Lock-free Ring Buffer
2. Background-Aggregation
3. Prometheus-Export-Optimierung

---

## Bibliotheken & Dependencies

**Bestehend (keine neuen Dependencies):**
- Standard C++ (`std::deque`, `std::thread`, `std::mutex`)
- RocksDB (`WriteBatch`)
- Zstd (bereits in vcpkg)
- LZ4 (bereits in vcpkg via RocksDB)

**Optional (für Vector-Kompression):**
- [Faiss](https://github.com/facebookresearch/faiss) (Product Quantization) - bereits vorhanden!
- [ScaNN](https://github.com/google-research/google-research/tree/master/scann) (Vector Quantization)

---

## Metriken & KPIs

### Success Metrics

| Komponente | Metrik | Ziel | Aktuell |
|------------|--------|------|---------|
| VectorAutoBuffer | Insert-Throughput | 50,000 vec/s | ~1,000 vec/s |
| VectorAutoBuffer | Speicher-Reduktion | 10x (via PQ) | 1x |
| WAL Shipping | Bandbreite | -70% (via Zstd) | Baseline |
| ChangefeedBuffer | Event-Latenz | <100ms | ~500ms |
| MetricsRingBuffer | Record-Latenz | <10ns | ~1µs |

### Testing Strategy

- **Unit-Tests:** Jeder AutoBuffer separat
- **Integration-Tests:** End-to-End mit HTTP-API
- **Performance-Tests:** Benchmarks vs. Direct-Insert
- **Memory-Tests:** Valgrind/AddressSanitizer
- **Stress-Tests:** Multi-Threading, Buffer-Overflow

---

## Referenzen

- [TSAutoBuffer Implementation](../timeseries/AUTO_BUFFER.md)
- [Gorilla Compression](https://www.vldb.org/pvldb/vol8/p1816-teller.pdf)
- [Product Quantization](https://ieeexplore.ieee.org/document/5432202)
- [LZ4 Compression](https://github.com/lz4/lz4)
- [Zstd Compression](https://facebook.github.io/zstd/)

---

**Autor:** ThemisDB Team  
**Datum:** 15. Dezember 2025  
**Review:** Erforderlich vor Implementation
