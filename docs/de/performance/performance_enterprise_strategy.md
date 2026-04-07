# Enterprise Scalability Strategy: Massive Parallel Access & Batch Operations

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** ⚡ Performance

---

## 📑 Table of Contents

- [Status](#status-draft)
- [Overview](#overview)
- [Strategy](#scalability-strategy)

---

**Status:** Draft  
**Version:** 1.0  
**Date:** 2025-11-28  
**Target:** ThemisDB Enterprise Features

---

## Executive Summary

ThemisDB hat bereits solide Grundlagen für parallele Verarbeitung (TBB, WriteBatch, MultiGet). Für Enterprise-Einsätze mit massiven parallelen Zugriffen (>1000 concurrent clients, >100k req/s) sind jedoch weitere Optimierungen notwendig.

**Aktuelle Stärken:**
- ✅ TBB-basierte parallele Query-Ausführung (`PARALLEL_THRESHOLD=100`)
- ✅ RocksDB WriteBatch für atomare Bulk-Writes
- ✅ MultiGet für effizientes Batch-Loading von Entities
- ✅ HNSW Vector-Batch-Insert (500+ Vektoren < 1s)
- ✅ Worker-Thread-Pool im HTTP-Server (konfigurierbar via `num_threads`)

**Enterprise Gaps (zu schließen):**
- ⚠️ Kein Connection Pooling für externe Services (DB-Shards, Embedding-APIs)
- ⚠️ Rate Limiting nur rudimentär (100 req/min global, kein Burst-Token-Bucket)
- ⚠️ Batch-Endpoints nicht vollständig REST-konform (z.B. fehlt `/entities/batch`)
- ⚠️ Keine adaptive Load-Shedding bei Überlast
- ⚠️ Bulk-Import limitiert durch sequentielle Embedding-API-Calls

---

## 1. Connection Pooling & Circuit Breaker

### 1.1 Problem
Externe API-Calls (OpenAI Embedding, Remote Shards, PKI/HSM) verwenden keine Pools → TCP-Overhead, häufige Reconnects.

### 1.2 Lösung

**HTTP-Client-Pool (für Embedding-APIs):**
```cpp
// include/utils/http_client_pool.h
class HTTPClientPool {
public:
    struct Config {
        size_t max_connections = 50;
        std::chrono::seconds idle_timeout{30};
        std::chrono::seconds connect_timeout{5};
        bool enable_keepalive = true;
    };
    
    std::future<HTTPResponse> post(const std::string& url, const json& body);
    
private:
    asio::io_context ioc_;
    std::vector<std::unique_ptr<HTTPClient>> pool_;
    std::mutex mutex_;
};
```

**Circuit Breaker Pattern (für Remote-Shards/HSM):**
```cpp
class CircuitBreaker {
public:
    enum class State { CLOSED, OPEN, HALF_OPEN };
    
    struct Config {
        size_t failure_threshold = 5;          // Failures before opening
        std::chrono::seconds timeout{30};      // Duration before HALF_OPEN
        size_t success_threshold_half_open = 2; // Successes to close
    };
    
    template<typename Func>
    std::optional<typename std::invoke_result<Func>::type> 
    execute(Func&& func);
    
private:
    std::atomic<State> state_{State::CLOSED};
    std::atomic<size_t> failure_count_{0};
    std::chrono::steady_clock::time_point last_failure_time_;
};
```

**Integration:**
```cpp
// src/vector/embedding_provider.cpp
HTTPClientPool embedding_pool_;

std::vector<std::vector<float>> EmbeddingProvider::batchEmbed(
    const std::vector<std::string>& texts
) {
    // Pool aus 50 Connections, Keep-Alive
    auto future = embedding_pool_.post("/v1/embeddings", {
        {"input", texts},
        {"model", "text-embedding-3-small"}
    });
    
    auto response = future.get(); // Async/Await
    return parseEmbeddings(response.body);
}
```

**Benefit:** ~30% Reduktion bei Embedding-API-Latenz (TCP-Setup entfällt), robuste Fehlerbehandlung.

---

## 2. Advanced Rate Limiting & Admission Control

### 2.1 Problem
Aktuelles Rate Limiting: Global 100 req/min (hardcoded), kein Burst, keine Priorisierung.

### 2.2 Token-Bucket-Algorithmus

```cpp
// include/server/rate_limiter_v2.h
class TokenBucketRateLimiter {
public:
    struct Config {
        size_t capacity = 1000;           // Max tokens (burst)
        size_t refill_rate = 100;          // Tokens per second
        bool enable_priority_lanes = true; // VIP/Standard/Batch lanes
    };
    
    enum class Priority { HIGH, NORMAL, LOW };
    
    bool tryAcquire(size_t tokens = 1, Priority prio = Priority::NORMAL);
    
private:
    std::atomic<size_t> tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    std::mutex mutex_;
    
    // Separate buckets for priority lanes
    std::unordered_map<Priority, size_t> priority_tokens_;
};
```

**HTTP-Middleware:**
```cpp
void HttpServer::setupRateLimiting() {
    auto limiter = std::make_shared<TokenBucketRateLimiter>(
        TokenBucketRateLimiter::Config{
            .capacity = 10000,      // 10k burst
            .refill_rate = 1000     // 1k/s sustained
        }
    );
    
    router_.use([limiter](auto req, auto res, auto next) {
        auto prio = extractPriority(req); // Via JWT claims
        
        if (!limiter->tryAcquire(1, prio)) {
            return res->status(429)
                      ->json({{"error", "Rate limit exceeded"}});
        }
        next();
    });
}
```

**Per-Client Limits (via Redis/Memory):**
```cpp
class PerClientRateLimiter {
public:
    bool allowRequest(const std::string& client_id) {
        auto& bucket = client_buckets_[client_id];
        return bucket.tryAcquire();
    }
    
private:
    std::unordered_map<std::string, TokenBucketRateLimiter> client_buckets_;
    std::mutex mutex_;
};
```

**Benefit:** Burst-Traffic (z.B. 5000 Requests in 1s) wird geglättet; VIP-Clients werden priorisiert.

---

## 3. Batch-API-Endpoints

### 3.1 Fehlende Enterprise-Endpoints

Derzeit: `/vector/batch_insert`, `/transaction` (bulk).  
Fehlend: `/entities/batch`, `/query/batch`, `/graph/batch_traverse`.

### 3.2 Implementierung

**Batch CRUD:**
```cpp
// POST /entities/batch
{
  "operations": [
    {"op": "put", "table": "users", "pk": "u1", "fields": {...}},
    {"op": "put", "table": "users", "pk": "u2", "fields": {...}},
    {"op": "delete", "table": "orders", "pk": "o123"}
  ]
}

// Response:
{
  "succeeded": 2,
  "failed": [
    {"index": 1, "error": "Duplicate key"}
  ]
}
```

**Implementation:**
```cpp
void HttpServer::handleBatchEntities(const Request& req, Response& res) {
    auto ops = req.json["operations"];
    auto batch = db_->createWriteBatch();
    
    std::vector<json> errors;
    size_t succeeded = 0;
    
    for (size_t i = 0; i < ops.size(); ++i) {
        const auto& op = ops[i];
        try {
            if (op["op"] == "put") {
                auto entity = BaseEntity::fromJson(op["pk"], op["fields"]);
                batch->put(makeKey(op["table"], op["pk"]), entity.serialize());
                secIdx_->put(op["table"], entity, *batch);
                ++succeeded;
            } else if (op["op"] == "delete") {
                // ... deletion logic
            }
        } catch (const std::exception& e) {
            errors.push_back({{"index", i}, {"error", e.what()}});
        }
    }
    
    batch->commit();
    
    res->json({
        {"succeeded", succeeded},
        {"failed", errors}
    });
}
```

**Batch Query (Parallel Execution):**
```cpp
// POST /query/batch
{
  "queries": [
    {"table": "users", "predicates": [{"column": "age", "op": "=", "value": 25}]},
    {"table": "orders", "rangePredicates": [...]}
  ]
}
```

```cpp
void HttpServer::handleBatchQuery(const Request& req, Response& res) {
    auto queries = req.json["queries"];
    std::vector<json> results(queries.size());
    
    tbb::parallel_for(size_t(0), queries.size(), [&](size_t i) {
        auto q = ConjunctiveQuery::fromJson(queries[i]);
        auto [st, entities] = query_engine_->executeAndEntities(q);
        
        if (st.ok) {
            results[i] = {{"data", entitiesToJson(entities)}};
        } else {
            results[i] = {{"error", st.message}};
        }
    });
    
    res->json({{"results", results}});
}
```

**Benefit:** ~10x Durchsatz-Steigerung für Batch-Workloads (1 Request statt 100).

---

## 4. Adaptive Batch-Sizing & Load Shedding

### 4.1 Problem
Feste `BATCH_SIZE=50` ist suboptimal: Bei Low-Load zu klein (Overhead), bei High-Load zu groß (Latenz-Spikes).

### 4.2 Adaptive Batching

```cpp
class AdaptiveBatchConfig {
public:
    size_t getBatchSize() const {
        auto load = getCurrentLoad(); // CPU/Memory/Queue-Depth
        
        if (load < 0.3) return 100;       // Low load: large batches
        else if (load < 0.7) return 50;   // Medium load
        else return 25;                   // High load: reduce batch size
    }
    
private:
    double getCurrentLoad() const {
        return (cpu_usage_ + memory_usage_ + queue_depth_ratio_) / 3.0;
    }
    
    std::atomic<double> cpu_usage_{0.0};
    std::atomic<double> memory_usage_{0.0};
    std::atomic<double> queue_depth_ratio_{0.0};
};
```

**Load Shedding (bei Überlast):**
```cpp
class LoadShedder {
public:
    bool shouldReject(const Request& req) {
        if (getCurrentLoad() > 0.95) {
            // Reject low-priority requests (keep VIP/Health checks)
            return req.priority == Priority::LOW;
        }
        return false;
    }
};
```

**HTTP-Middleware:**
```cpp
router_.use([shedder](auto req, auto res, auto next) {
    if (shedder->shouldReject(req)) {
        return res->status(503)
                  ->json({{"error", "Service overloaded. Retry later."}});
    }
    next();
});
```

---

## 5. RocksDB MultiGet Optimizations

### 5.1 Aktueller Stand
`db_.multiGet(keys)` wird bereits verwendet (Graph-Queries, Batch-Loading). Optimierungen:

**Prefetching:**
```cpp
// src/storage/rocksdb_wrapper.cpp
std::vector<std::optional<std::vector<uint8_t>>> 
RocksDBWrapper::multiGet(const std::vector<std::string>& keys) {
    std::vector<rocksdb::Slice> key_slices;
    key_slices.reserve(keys.size());
    for (const auto& k : keys) {
        key_slices.emplace_back(k);
    }
    
    // Enable prefetching for sequential I/O
    rocksdb::ReadOptions read_opts;
    read_opts.fill_cache = true;
    read_opts.async_io = true;              // NEW: Async I/O
    read_opts.optimize_multiget_for_io = true; // NEW: RocksDB 7.0+
    
    std::vector<rocksdb::PinnableSlice> values(keys.size());
    std::vector<rocksdb::Status> statuses(keys.size());
    
    txn_db_->MultiGet(read_opts, default_cf_, keys.size(), 
                      key_slices.data(), values.data(), statuses.data());
    
    // ... convert to optional<vector<uint8_t>>
}
```

**Benefit:** ~40% schneller bei 100+ Keys (async I/O, prefetching).

---

## 6. Write-Ahead-Log (WAL) Tuning

### 6.1 Bulk-Import-Optimierung

Für Bulk-Imports (>10k Entities):

```cpp
Status BulkImporter::importEntities(const std::vector<BaseEntity>& entities) {
    // Disable WAL für Bulk-Import
    rocksdb::WriteOptions write_opts;
    write_opts.disableWAL = true;
    
    auto batch = db_->createWriteBatch();
    for (const auto& e : entities) {
        batch->put(makeKey(e.getPrimaryKey()), e.serialize());
    }
    
    batch->commit(write_opts);
    
    // Flush nach Import (WAL-los → manueller Flush nötig)
    db_->flush();
    
    return Status::OK();
}
```

**WAL-Komprimierung (RocksDB 7.0+):**
```cpp
config.wal_compression = "zstd"; // WAL compression (reduces I/O)
```

---

## 7. Metrics & Monitoring (Enterprise-Grade)

### 7.1 Performance Counters

```cpp
class PerformanceMetrics {
public:
    struct Snapshot {
        uint64_t requests_total;
        uint64_t requests_per_sec;
        uint64_t p50_latency_ms;
        uint64_t p95_latency_ms;
        uint64_t p99_latency_ms;
        double cpu_usage_percent;
        uint64_t memory_used_mb;
        uint64_t active_connections;
    };
    
    void recordRequest(std::chrono::milliseconds latency);
    Snapshot getSnapshot() const;
    
    // Prometheus-Export
    std::string prometheusFormat() const;
};
```

**HTTP-Endpoint:**
```cpp
// GET /metrics (Prometheus format)
router_.get("/metrics", [metrics](auto req, auto res) {
    res->contentType("text/plain")
       ->send(metrics->prometheusFormat());
});
```

**Grafana-Dashboard:**
- Throughput (req/s)
- Latency Percentiles (p50/p95/p99)
- Error Rate (5xx/4xx)
- Queue Depth
- RocksDB Stats (Compaction, Cache Hit Rate)

---

## 8. Implementation Roadmap

| Phase | Feature | Priority | Effort | Timeline |
|-------|---------|----------|--------|----------|
| **Phase 1** | Token-Bucket Rate Limiter | HIGH | 2d | Week 1 |
| | Batch CRUD Endpoint (`/entities/batch`) | HIGH | 3d | Week 1-2 |
| | HTTP Client Pool (Embedding APIs) | MEDIUM | 3d | Week 2 |
| **Phase 2** | Circuit Breaker (Shards/HSM) | MEDIUM | 2d | Week 3 |
| | Adaptive Batch Sizing | LOW | 2d | Week 3 |
| | MultiGet Async I/O | MEDIUM | 1d | Week 3 |
| **Phase 3** | Prometheus Metrics Export | HIGH | 3d | Week 4 |
| | Load Shedding Middleware | LOW | 2d | Week 4 |
| | WAL Compression (Config) | LOW | 1d | Week 4 |

**Total Effort:** ~19 Tage (≈4 Wochen)

---

## 9. Performance Targets (Post-Implementation)

| Metric | Current | Target | Improvement |
|--------|---------|--------|-------------|
| Max Concurrent Clients | 100 | 1000 | 10x |
| Throughput (reads/s) | 5k | 50k | 10x |
| Throughput (writes/s) | 2k | 20k | 10x |
| Batch Insert (1000 entities) | 500ms | 100ms | 5x |
| P99 Latency (Query) | 200ms | 50ms | 4x |
| Embedding API Latency | 300ms | 200ms | 1.5x |

---

## 10. Testing Strategy

### 10.1 Load Testing

**Tool:** k6 (https://k6.io)

```javascript
// load_test.js
import http from 'k6/http';
import { check } from 'k6';

export let options = {
  stages: [
    { duration: '1m', target: 100 },   // Ramp-up to 100 users
    { duration: '5m', target: 100 },   // Stay at 100 for 5 min
    { duration: '1m', target: 1000 },  // Spike to 1000
    { duration: '3m', target: 1000 },  // Stay at 1000
    { duration: '1m', target: 0 },     // Ramp-down to 0
  ],
};

export default function () {
  let res = http.post('http://localhost:18765/entities/batch', JSON.stringify({
    operations: [/* ... 100 ops */]
  }), {
    headers: { 'Content-Type': 'application/json' },
  });
  
  check(res, {
    'status is 200': (r) => r.status === 200,
    'response time < 500ms': (r) => r.timings.duration < 500,
  });
}
```

**Run:**
```bash
k6 run load_test.js
```

### 10.2 Chaos Engineering

**Tool:** Pumba (https://github.com/alexei-led/pumba)

```bash
# Simulate 100ms network latency
pumba netem --duration 5m delay --time 100 themisdb

# Kill random container replicas (test failover)
pumba kill --interval 30s --random themisdb
```

---

## 11. Cost-Benefit Analysis

**Investment:**
- Engineering: ~19 Tage (~€20k @ €1k/Tag)
- Infrastructure: +20% (Load-Balancer, Monitoring)

**ROI:**
- **10x Throughput** → Support 10x mehr Kunden ohne neue Hardware
- **4x Latenz-Reduktion** → Bessere UX → Höhere Conversion
- **Reliability (99.9% → 99.99%)** → Weniger Incidents → Geringere Support-Kosten

**Break-Even:** 3 Monate (bei 10 neuen Enterprise-Kunden @ €5k/Monat)

---

## 12. References

- [RocksDB Tuning Guide](https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide)
- [Token Bucket Algorithm](https://en.wikipedia.org/wiki/Token_bucket)
- [Circuit Breaker Pattern](https://martinfowler.com/bliki/CircuitBreaker.html)
- [Google SRE Book: Load Shedding](https://sre.google/sre-book/handling-overload/)
- [Intel TBB Documentation](https://spec.oneapi.io/versions/latest/elements/oneTBB/source/nested-index.html)

---

**Next Steps:**
1. Review mit Team (Priorisierung Phase 1)
2. Spike: Token-Bucket Prototyp (2d)
3. Load-Test Setup (k6 + Docker Compose)
4. Metrics-Dashboard (Grafana Template)

**Contact:** Architecture Team  
**Status:** Ready for Implementation
