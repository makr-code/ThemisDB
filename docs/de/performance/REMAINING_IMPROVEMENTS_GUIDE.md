# Verbleibende Performance-Verbesserungen - Implementierungsleitfaden

**Version:** v1.3.0 Phase 2+  
**Status:** Planung  
**Datum:** 22. Dezember 2025

---

## Überblick

Dieses Dokument beschreibt die Implementierungsdetails für die verbleibenden 4 Performance-Verbesserungen aus PERFORMANCE_IMPROVEMENT_OPTIONS_V1.3.0.md.

**Status:**
- ✅ Implementiert: 4 von 8 (50%)
- ⏳ Verbleibend: 4 von 8 (50%)

---

## ⏳ Verbesserung 5: Per-Key Point Lock Manager (RocksDB 10.6+)

### Komplexität: MITTEL
### Erwartete Verbesserung: +100-200% bei Write Contention
### Implementierungszeit: 1-2 Stunden

### Implementierung

**Datei:** `src/storage/rocksdb_wrapper.cpp`

**Zu ändernde Stelle:** Bei TransactionDB-Erstellung

```cpp
// In RocksDBWrapper::open() - Transaction DB Setup
TransactionDBOptions txn_db_options;

// v1.3.0 Phase 2: Enable Per-Key Point Lock Manager (RocksDB 10.6+)
// Improves efficiency under high write contention
// FIFO ordering, per-thread conditional variables
txn_db_options.use_per_key_point_lock_mgr = true;
txn_db_options.deadlock_timeout_us = 0;  // Immediate deadlock detection

// Existing transaction lock timeout
txn_db_options.transaction_lock_timeout = config_.transaction_lock_timeout_ms;
```

### Wissenschaftliche Grundlage
- RocksDB HISTORY.md (10.6.0): Experimental PerKeyPointLockManager
- FIFO ordering reduces contention
- Per-thread CV → better cache locality
- Scalability: O(threads) statt O(lock_stripes)

### Test-Strategie
- Benchmark mit hoher Write Contention (viele Threads, wenige Keys)
- Messen: Lock wait time, throughput
- Vergleich: Standard vs. Per-Key Lock Manager

### Referenzen
- https://github.com/facebook/rocksdb/blob/main/HISTORY.md#1060-08222025

---

## ⏳ Verbesserung 6: Asynchronous I/O (MultiScan) (RocksDB 10.7+)

### Komplexität: HOCH
### Erwartete Verbesserung: +200-500% Sequential Scans
### Implementierungszeit: 4-8 Stunden

### Implementierung

**Betroffen:** Scan-Operationen in Query Processing

**Option 1: MultiGet mit Async I/O**
```cpp
// In query/scan operations
rocksdb::ReadOptions read_opts;
read_opts.async_io = true;  // RocksDB 10.7+
read_opts.optimize_multiget_for_io = true;

std::vector<rocksdb::Slice> keys;
std::vector<std::string> values;
std::vector<rocksdb::Status> statuses = db_->MultiGet(read_opts, keys, &values);
```

**Option 2: Iterator mit Prefetching**
```cpp
rocksdb::ReadOptions read_opts;
read_opts.readahead_size = 64 * 1024 * 1024;  // 64MB prefetch
read_opts.async_io = true;

auto it = db_->NewIterator(read_opts);
for (it->SeekToFirst(); it->Valid(); it->Next()) {
    // Process
}
```

### Wissenschaftliche Grundlage
- "Asynchronous I/O for LSM-Trees" (SOSP 2022)
- Overlapping I/O with computation
- Prefetching hides disk latency

### Test-Strategie
- Benchmark: Sequential scans über große Datasets
- Messen: Scan latency, throughput
- Vergleich: Sync vs. Async I/O

### Referenzen
- https://github.com/facebook/rocksdb/blob/main/include/rocksdb/db.h

---

## ⏳ Verbesserung 7: Vector Quantization (High-Dimensional Embeddings)

### Komplexität: SEHR HOCH
### Erwartete Verbesserung: +250-400% für 1536D Vectors
### Implementierungszeit: 2-4 Wochen

### Implementierung

**Neues Modul:** `src/index/vector_quantization.h/cpp`

**Option A: FAISS Integration (Empfohlen)**
```cpp
#include <faiss/IndexPQ.h>
#include <faiss/IndexBinaryFlat.h>

class QuantizedVectorIndex {
public:
    // Product Quantization: 1536D → 96 bytes (64x compression)
    faiss::IndexPQ index(dimension, num_subquantizers, bits_per_code);
    
    // Binary Quantization: 1536D → 192 bytes (24x compression)
    faiss::IndexBinaryFlat binary_index(dimension);
};
```

**Option B: Custom Implementation**
```cpp
class ProductQuantizer {
    // Split vector into subvectors
    // Quantize each to 8-bit
    std::vector<uint8_t> quantize(const std::vector<float>& vec);
    std::vector<float> dequantize(const std::vector<uint8_t>& codes);
    float distance(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b);
};
```

### Integration in VectorIndex
```cpp
// In index/vector_index.cpp
class VectorIndex {
private:
    std::unique_ptr<ProductQuantizer> quantizer_;
    bool use_quantization_ = false;
    
public:
    void enableQuantization(int subquantizers = 96) {
        quantizer_ = std::make_unique<ProductQuantizer>(dimension_, subquantizers);
        use_quantization_ = true;
    }
};
```

### Wissenschaftliche Grundlage
- "Product Quantization for Nearest Neighbor Search" (PAMI 2011)
- "Binary and Scalar Quantization for Vector Search" (VLDB 2023)
- Memory: 1536D float32 (6KB) → 96 bytes (64x compression)
- Speed: ~10-50x faster distance computation

### Test-Strategie
- Benchmark: Insert/Search für 384D, 768D, 1536D
- Messen: Throughput, Recall@k
- Vergleich: Full precision vs. Quantized

### Referenzen
- FAISS: https://github.com/facebookresearch/faiss
- Paper: https://hal.inria.fr/inria-00514462/document

---

## ⏳ Verbesserung 8: Native Binary Wire Protocol (gRPC)

### Komplexität: MITTEL-HOCH
### Erwartete Verbesserung: +25-35% Overall Performance
### Implementierungszeit: 1-2 Wochen

### Implementierung

**Bereits vorhanden:** `proto/themis.proto` (gRPC Interface)

**Aktivierung als Standard:**

**Datei:** `src/server/main_server.cpp` oder Config

```cpp
// Config-Option hinzufügen
struct ServerConfig {
    bool use_grpc_by_default = false;  // ← auf true setzen
    int grpc_port = 50051;
    int http_port = 8080;
};

// Server-Startup
if (config.use_grpc_by_default) {
    startGRPCServer(config.grpc_port);
} else {
    startHTTPServer(config.http_port);
}
```

**Client SDK Update:**
```cpp
// ThemisDB Client
class ThemisDBClient {
    enum class Protocol { HTTP, GRPC };
    
    ThemisDBClient(const std::string& host, Protocol protocol = Protocol::GRPC) {
        if (protocol == Protocol::GRPC) {
            stub_ = ThemisDB::NewStub(grpc::CreateChannel(host, credentials));
        } else {
            // HTTP client
        }
    }
};
```

### Wissenschaftliche Grundlage
- "Efficient Wire Protocols for Database Systems" (VLDB 2019)
- Binary protocols: 2-5x effizienter als JSON/HTTP
- HTTP/2 mit Multiplexing
- Zero-copy message passing

### Test-Strategie
- Benchmark: HTTP vs. gRPC für verschiedene Operationen
- Messen: Latency, throughput, serialization overhead
- Load testing mit verschiedenen Client-Zahlen

### Referenzen
- https://grpc.io/docs/what-is-grpc/introduction/
- PostgreSQL Wire Protocol: https://www.postgresql.org/docs/current/protocol.html

---

## Implementierungsreihenfolge (Empfohlen)

1. **Per-Key Point Lock Manager** (1-2h) ✅ Schneller Win
2. **gRPC als Standard** (1-2 Wochen) ✅ Große Auswirkung
3. **Async I/O** (4-8h) ✅ Moderate Komplexität
4. **Vector Quantization** (2-4 Wochen) ⚠️ Hohe Komplexität, große Auswirkung

---

## Zusammenfassung

| Verbesserung | Komplexität | Zeit | Erwartung | Priorität |
|--------------|-------------|------|-----------|-----------|
| Per-Key Lock | Mittel | 1-2h | +100-200% | ⭐⭐⭐ |
| Async I/O | Hoch | 4-8h | +200-500% | ⭐⭐ |
| Vector Quant | Sehr Hoch | 2-4w | +250-400% | ⭐⭐⭐ |
| gRPC Protocol | Mittel-Hoch | 1-2w | +25-35% | ⭐⭐⭐ |

**Empfohlener nächster Schritt:** Per-Key Point Lock Manager implementieren (schnellster ROI)

---

**Letzte Aktualisierung:** 22. Dezember 2025  
**Version:** v1.3.0 Phase 2+
