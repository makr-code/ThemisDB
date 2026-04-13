# 📊 ThemisDB Benchmark-Dokumentation & Test-Runbook

**Version:** 2.0.0  
**Status:** ✅ Produktionsreif  
**Letzte Aktualisierung:** 2026-02-02  
**Sprache:** Deutsch

> **Zentrale Referenz** für alle Benchmark- und Performance-Tests in ThemisDB

---

## 📑 Inhaltsverzeichnis

1. [Übersicht](#übersicht)
2. [Benchmark-Typen im Vergleich](#benchmark-typen-im-vergleich)
3. [Benchmark-Szenarien](#benchmark-szenarien)
4. [Hardware-Anforderungen](#hardware-anforderungen)
5. [Beispielausgaben](#beispielausgaben)
6. [Fehlerquellen & Troubleshooting](#fehlerquellen--troubleshooting)
7. [Test-Runbook für neue Benchmarks](#test-runbook-für-neue-benchmarks)
8. [Best Practices](#best-practices)
9. [Referenzen](#referenzen)

---

## 🎯 Übersicht

Diese Dokumentation bietet einen umfassenden Überblick über alle Benchmark- und Performance-Tests in ThemisDB. Sie dient als zentrale Referenz für Entwickler, QA-Ingenieure und Performance-Analysten.

### Zweck

- **Standardisierung**: Einheitliche Ausführung und Bewertung von Benchmarks
- **Transparenz**: Klare Dokumentation von Anforderungen und Erwartungen
- **Qualitätssicherung**: Früherkennung von Performance-Regressionen
- **Wissenstransfer**: Onboarding neuer Team-Mitglieder

### Benchmark-Framework

ThemisDB verwendet mehrere Benchmark-Frameworks:

- **CHIMERA Suite**: Wissenschaftlich validiertes Multi-Model-Benchmark-Framework
- **Google Benchmark**: C++ Mikro-Benchmarks für einzelne Komponenten
- **Python Benchmarks**: HTTP/REST API Performance-Tests
- **Docker Benchmarks**: End-to-End Integrationstests

### Projektstruktur

```
benchmarks/
├── bench_*.cpp                 # C++ Mikro-Benchmarks (80+ Dateien)
├── *_benchmark.py              # Python Integration Benchmarks
├── chimera/                    # CHIMERA Suite
├── docker-compose.*.yml        # Docker Benchmark Definitionen
├── run_docker_benchmarks.py    # Docker Orchestrierung
├── README.md                   # Benchmark-Übersicht
├── QUICKSTART.md               # Schnelleinstieg
└── BENCHMARK_BEST_PRACTICES.md # Best Practices
```

---

## 📊 Benchmark-Typen im Vergleich

### 1. Mikro-Benchmarks (C++)

**Beschreibung**: Testen einzelne Funktionen oder kleine Code-Abschnitte isoliert mit nanosekunden-genauer Messung.

**Framework**: Google Benchmark (`google/benchmark`)

**Vorteile**:
- ✅ Präzise Messung auf Nanosekunden-Ebene
- ✅ Komplexitätsanalyse (O(n), O(log n))
- ✅ Compiler-Optimierungen sichtbar machen
- ✅ Minimaler Overhead
- ✅ Automatische Iterationsanpassung

**Nachteile**:
- ❌ Keine End-to-End Validierung
- ❌ Kann reale Workloads nicht vollständig simulieren
- ❌ Setup-Overhead muss manuell ausgeschlossen werden

**Anwendungsfälle**:
- Vektor-Operationen (HNSW, FAISS)
- Index-Lookups (Secondary, Graph, Fulltext, Geo)
- Komprimierungsalgorithmen (LZ4, ZSTD, Binary Quantization)
- Graph-Traversierungen (BFS, DFS, Dijkstra, PageRank)
- Kryptografische Operationen (AES, SHA-256)
- Transaktions-Manager (MVCC, Isolation, Conflict Resolution)
- LoRA Inferenz und Adapter Management

**Verfügbare Benchmarks** (Auswahl):
```
benchmarks/bench_vector_search.cpp           # Vector Search (HNSW/FAISS)
benchmarks/bench_crud.cpp                     # CRUD mit allen Indextypen
benchmarks/bench_graph_traversal.cpp          # Graph-Algorithmen
benchmarks/bench_compression.cpp              # Komprimierung
benchmarks/bench_transaction_throughput.cpp   # ACID Transaktionen
benchmarks/bench_llm_inference_performance.cpp # LLM Inferenz
benchmarks/bench_lora_inline.cpp              # LoRA Adapter
benchmarks/bench_sharding_performance.cpp     # Distributed Sharding
benchmarks/bench_graph_query_optimizer.cpp    # Query Optimization
benchmarks/bench_snapshot_manager.cpp         # Snapshot Isolation
```

**Ausführung**:
```bash
# Einzelner Benchmark
cd build
./benchmarks/bench_vector_search

# Mit Filtern
./benchmarks/bench_crud --benchmark_filter=Insert.*

# JSON Output für Automation
./benchmarks/bench_transaction_throughput \
  --benchmark_format=json \
  --benchmark_out=results.json

# Erweiterte Optionen
./benchmarks/bench_graph_traversal \
  --benchmark_min_time=5.0 \
  --benchmark_repetitions=10 \
  --benchmark_report_aggregates_only=true
```

---

### 2. Integration Benchmarks (Python)

**Beschreibung**: Testen die gesamte System-Pipeline über HTTP/REST APIs mit realistischen Workloads.

**Framework**: Custom Python Scripts mit `httpx` und `time.perf_counter()`

**Vorteile**:
- ✅ Realistische End-to-End Tests
- ✅ Einfache Automatisierung in CI/CD
- ✅ Simuliert echte Client-Interaktionen
- ✅ Multi-Protokoll-Support (HTTP, gRPC, WebSocket, PostgreSQL Wire)

**Nachteile**:
- ❌ Netzwerk-Latenz beeinflusst Messungen
- ❌ Weniger präzise als Mikro-Benchmarks
- ❌ Schwieriger zu isolieren bei Problemen

**Anwendungsfälle**:
- REST API Performance
- Multi-Shard Operationen
- Transaktions-Durchsatz über Netzwerk
- Concurrent Client Tests
- Enterprise Features (LDAP, Kerberos, RBAC)
- LLM/NLP Integration Tests

**Verfügbare Benchmarks**:
```python
benchmarks/complete_benchmark_suite.py        # CHIMERA Suite (YCSB, TPC-C, TPC-H)
benchmarks/comprehensive_crud_benchmark.py    # CRUD über HTTP/gRPC
benchmarks/enterprise_comparison_suite.py     # Enterprise-Features
benchmarks/specialized_comparative_benchmarks.py # Vergleich mit anderen DBs
benchmarks/llm_nlp_integration_test_suite.py  # LLM/NLP Features
benchmarks/multi_protocol_support.py          # Multi-Protokoll Tests
```

**Ausführung**:
```bash
# Vollständige Suite
cd benchmarks
python3 complete_benchmark_suite.py \
  --mode full \
  --databases ThemisDB PostgreSQL MongoDB \
  --output-dir results/complete

# YCSB Workloads
python3 complete_benchmark_suite.py \
  --mode ycsb \
  --workloads A B C \
  --duration 300

# Enterprise Benchmarks
python3 enterprise_comparison_suite.py \
  --features all \
  --protocols http grpc \
  --concurrent-clients 50
```

---

### 3. Docker Benchmarks

**Beschreibung**: Vollständige Deployment-Tests mit Docker-Containern für realistische Produktionsszenarien.

**Framework**: Docker Compose + Python Orchestration

**Vorteile**:
- ✅ Produktions-nahe Umgebung
- ✅ Reproduzierbare Tests auf verschiedenen Systemen
- ✅ Multi-Node Tests möglich
- ✅ Isolation von Host-System
- ✅ Einfache Netzwerk-Simulation
- ✅ RAID/Sharding Tests

**Nachteile**:
- ❌ Höherer Overhead durch Container
- ❌ Längere Ausführungszeit
- ❌ Erfordert Docker-Installation
- ❌ Mehr Speicherplatz benötigt

**Anwendungsfälle**:
- RAID-Konfigurationen testen (MIRROR, STRIPE, PARITY)
- Multi-Shard Performance
- Netzwerk-Latenz simulieren
- Failover-Szenarien
- Cross-Shard Queries
- High-Availability Tests

**Verfügbare Benchmarks**:
```
benchmarks/run_docker_benchmarks.py            # Docker Orchestrierung
benchmarks/docker_benchmarks_unified.py        # Unified Docker Tests
benchmarks/raid_sharding_test_suite.py         # RAID Testing
benchmarks/run_multi_shard_raid_benchmark.py   # Multi-Shard RAID
```

**Docker Compose Dateien**:
```
benchmarks/docker-compose.benchmark.yml        # Standard Benchmark
benchmarks/docker-compose.multi-shard-raid.yml # RAID Setup
benchmarks/docker-compose.llm-integration.yml  # LLM Features
benchmarks/docker-compose.raid-phase1.yml      # RAID Phase 1
benchmarks/docker-compose.raid-phase2.yml      # RAID Phase 2
```

**Ausführung**:
```bash
# Docker Benchmarks starten
cd benchmarks
python3 run_docker_benchmarks.py

# RAID Tests
python3 raid_sharding_test_suite.py \
  --shards 4 \
  --raid-mode MIRROR \
  --replication-factor 2

# Multi-Shard Benchmark
python3 run_multi_shard_raid_benchmark.py \
  --shard-configs "2,4,8,16" \
  --workload mixed
```

---

### 4. CHIMERA Suite

**Beschreibung**: Wissenschaftlich validiertes, vendor-neutrales Benchmark-Framework für Multi-Model Datenbanken mit AI/LLM Integration.

**Framework**: CHIMERA (Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment)

**Standards**:
- IEEE/ACM Konformität
- TPC-C / TPC-H (Transaction & Analytics)
- YCSB Workloads (A-F)
- LDBC (Linked Data Benchmark Council)
- ANN-Benchmarks (Vector Search)

**Vorteile**:
- ✅ Vendor-neutral und wissenschaftlich validiert
- ✅ Statistische Signifikanz (t-tests, Mann-Whitney, Cohen's d)
- ✅ Farbblind-freundliche Visualisierungen (Okabe-Ito, Paul Tol)
- ✅ Vergleichbar mit anderen Datenbanken
- ✅ Multi-Format Reports (HTML, CSV, PDF)
- ✅ Confidence Intervals und Effect Sizes

**Nachteile**:
- ❌ Komplexere Konfiguration
- ❌ Längere Ausführungszeit (10-30 Minuten)
- ❌ Erfordert mehr Dokumentation
- ❌ Mehr Python-Dependencies (scipy, numpy, pandas)

**Anwendungsfälle**:
- Wissenschaftliche Publikationen
- Wettbewerbsvergleiche
- Marketing-Material
- Performance-Zertifizierung
- Produktionsreife-Validierung
- ACM Artifact Badging

**Verfügbare Benchmarks**:
```
benchmarks/chimera/benchmark_runner.py        # CHIMERA Runner
benchmarks/scientific_benchmark_runner.py     # Scientific Protocol
benchmarks/scientific_crud_benchmark.py       # CRUD mit Statistik
benchmarks/scientific_enterprise_integration.py # Enterprise Features
benchmarks/test_scientific_compliance.py      # Compliance Tests
```

**Ausführung**:
```bash
# CHIMERA Suite
cd benchmarks
python3 complete_benchmark_suite.py \
  --mode full \
  --scientific \
  --databases ThemisDB PostgreSQL MongoDB Neo4j \
  --output-dir results/chimera

# Mit Statistik
python3 scientific_benchmark_runner.py \
  --workloads ycsb tpcc tpch \
  --repetitions 10 \
  --confidence-level 0.95
```

### Vergleichstabelle

| Eigenschaft | Mikro | Integration | Docker | CHIMERA |
|-------------|-------|-------------|--------|---------|
| **Präzision** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Setup-Zeit** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐ |
| **Aussagekraft** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **CI/CD Eignung** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| **Ausführungszeit** | < 1 min | 5-10 min | 10-20 min | 10-30 min |
| **Komplexität** | Niedrig | Mittel | Mittel | Hoch |
| **Isolation** | Hoch | Mittel | Hoch | Mittel |
| **Reproduzierbarkeit** | Hoch | Mittel | Hoch | Sehr Hoch |

---

## 🔬 Benchmark-Szenarien

### Szenario 1: Vector Search Performance

**Ziel**: Validierung der HNSW/FAISS Index Performance für Similarity Search.

**Benchmark**: `benchmarks/bench_vector_search.cpp`

**Workload**:
- **Dimensionen**: 384D (Standard Embeddings), 1536D (LLM), 4096D (Vision)
- **Dataset-Größen**: 10K, 100K, 1M, 10M Vektoren
- **Abfragen**: KNN (k=10, k=50, k=100)
- **Index-Typen**: HNSW, FAISS-IVF, Brute-Force
- **Metriken**: Cosine, Euclidean, Dot Product

**Erwartete Performance** (AMD Ryzen 9 7950X, NVMe SSD):
```
384D HNSW Insert:              > 400K vectors/s
384D HNSW Search (k=10):       > 50K queries/s  (Recall >= 0.95)
384D HNSW Search (k=50):       > 20K queries/s  (Recall >= 0.95)
1536D HNSW Insert:             > 100K vectors/s
1536D HNSW Search (k=10):      > 20K queries/s  (Recall >= 0.95)
4096D HNSW Insert:             > 50K vectors/s
4096D HNSW Search (k=10):      > 10K queries/s  (Recall >= 0.95)
```

**Code-Beispiel**:
```cpp
BENCHMARK_DEFINE_F(VectorSearchFixture, HNSWInsert384D)(benchmark::State& state) {
    std::vector<std::vector<float>> vectors = generateVectors(state.range(0), 384);
    
    for (auto _ : state) {
        state.PauseTiming();
        auto batch = getNextBatch(vectors, 100);
        state.ResumeTiming();
        
        vector_index_->insertBatch("embeddings", batch);
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(VectorSearchFixture, HNSWInsert384D)
    ->Range(1000, 1000000)
    ->Complexity();
```

---

### Szenario 2: CRUD Operationen

**Ziel**: Baseline Performance für Create, Read, Update, Delete Operationen mit verschiedenen Index-Typen.

**Benchmark**: `benchmarks/bench_crud.cpp`

**Workload**:
- **Entitäten**: 1K, 10K, 100K, 1M
- **Blob-Größen**: 100B, 1KB, 10KB, 100KB
- **Operationen**: PUT, GET, UPDATE, DELETE
- **Indizes**: Secondary, Range, Fulltext, Geo, TTL, Sparse
- **Concurrency**: 1, 4, 8, 16 Threads

**Erwartete Performance**:
```
PUT (1KB blob, no index):      > 50K ops/s
PUT (1KB blob, with indexes):  > 40K ops/s
GET (1KB blob):                > 100K ops/s
UPDATE (1KB blob):             > 35K ops/s
DELETE:                        > 80K ops/s
Secondary Index Lookup:        > 200K ops/s
Range Scan (100 items):        > 50K ops/s
Fulltext Search:               > 100K ops/s
Geo Query (radius):            > 80K ops/s
```

**Code-Beispiel**:
```cpp
BENCHMARK_DEFINE_F(CRUDFixture, InsertWithAllIndexes)(benchmark::State& state) {
    size_t counter = 0;
    
    for (auto _ : state) {
        state.PauseTiming();
        BaseEntity entity("person_" + std::to_string(counter++));
        entity.setField("email", generateRandomString(20));
        entity.setField("age", static_cast<int64_t>(25 + (counter % 50)));
        entity.setField("bio", generateRandomString(200));
        state.ResumeTiming();
        
        secondary_->put("Person", entity);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(CRUDFixture, InsertWithAllIndexes)
    ->Unit(benchmark::kMillisecond);
```

---

### Szenario 3: Graph Traversierung

**Ziel**: Performance von Graph-Algorithmen (BFS, DFS, Dijkstra, PageRank).

**Benchmark**: `benchmarks/bench_graph_traversal.cpp`

**Workload**:
- **Graph-Größen**: 1K Knoten, 10K Knoten, 100K Knoten
- **Kantentypen**: Sparse (Avg. Degree 5), Dense (Avg. Degree 50)
- **Algorithmen**: BFS, DFS, Dijkstra, A*, PageRank
- **Traversal-Tiefen**: 3, 5, 10 Hops

**Erwartete Performance**:
```
BFS (10K nodes, depth 5):      > 5M traversals/s
DFS (10K nodes, depth 5):      > 4M traversals/s
Dijkstra (10K nodes):          > 100K path computations/s
PageRank (10K nodes):          > 1K iterations/s
Neighbor Query:                > 8M queries/s
Edge Lookup:                   > 10M queries/s
```

**Code-Beispiel**:
```cpp
BENCHMARK_DEFINE_F(GraphFixture, BFS_10K_Depth5)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        std::string start_node = getRandomNode();
        state.ResumeTiming();
        
        auto result = graph_index_->breadthFirstSearch(start_node, 5);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(GraphFixture, BFS_10K_Depth5)
    ->Unit(benchmark::kMicrosecond);
```

---


### Szenario 4: Transaktions-Durchsatz

**Ziel**: ACID-Transaktionen mit MVCC, Isolation Levels und Conflict Resolution.

**Benchmark**: `benchmarks/bench_transaction_throughput.cpp`

**Workload**:
- **Transaktionstypen**: Read-Only, Write-Heavy, Mixed (70:30)
- **Isolation Levels**: Read Uncommitted, Read Committed, Repeatable Read, Serializable
- **Concurrency**: 1, 4, 8, 16, 32 Threads
- **Datenbankgröße**: 10K, 100K, 1M Entitäten
- **Konfliktrate**: Low (5%), Medium (20%), High (50%)

**Erwartete Performance** (AMD Ryzen 9 7950X):
```
Read-Only TX (1 Thread):           > 150K tx/s
Read-Only TX (16 Threads):         > 800K tx/s
Write-Heavy TX (1 Thread):         > 40K tx/s
Write-Heavy TX (16 Threads):       > 250K tx/s
Mixed Workload (16 Threads):       > 400K tx/s
Serializable (Low Conflict):       > 150K tx/s
Serializable (High Conflict):      > 30K tx/s
Snapshot Isolation:                > 500K tx/s
```

**Code-Beispiel**:
```cpp
BENCHMARK_DEFINE_F(TransactionFixture, WriteMixedWorkload)(benchmark::State& state) {
    const int num_threads = state.range(0);
    std::atomic<size_t> tx_count{0};
    
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        state.ResumeTiming();
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                auto tx = tx_manager_->beginTransaction(IsolationLevel::SERIALIZABLE);
                
                // 70% Reads, 30% Writes
                for (int j = 0; j < 10; ++j) {
                    if (j < 7) {
                        auto entity = tx->get("Person", "person_" + std::to_string(rand() % 10000));
                    } else {
                        BaseEntity new_entity("person_" + std::to_string(tx_count++));
                        new_entity.setField("balance", static_cast<int64_t>(1000));
                        tx->put("Person", new_entity);
                    }
                }
                
                tx->commit();
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_threads * 10);
}
BENCHMARK_REGISTER_F(TransactionFixture, WriteMixedWorkload)
    ->Arg(1)->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->Unit(benchmark::kMillisecond);
```

**Diagnose-Commands**:
```bash
# Transaction Manager Statistiken
./benchmarks/bench_transaction_throughput \
  --benchmark_filter=WriteMixedWorkload \
  --benchmark_counters_tabular=true

# Mit Profiling
perf record -g ./benchmarks/bench_transaction_throughput
perf report --stdio > tx_profile.txt

# Lock Contention analysieren
./benchmarks/bench_lock_contention --benchmark_repetitions=5
```

---

### Szenario 5: LLM/LoRA Integration

**Ziel**: Validierung der LLM-Inferenz und LoRA Adapter Performance.

**Benchmark**: 
- `benchmarks/bench_llm_inference_performance.cpp`
- `benchmarks/bench_lora_inline.cpp`
- `benchmarks/bench_lora_auto_binding.cpp`

**Workload**:
- **Modelle**: TinyLlama (1.1B), Phi-2 (2.7B), Llama-2-7B
- **Adapter**: 0, 1, 4, 8 aktive LoRA Adapter
- **Batch-Größen**: 1, 4, 16, 32
- **Kontext-Längen**: 128, 512, 2048 Tokens
- **Quantisierung**: FP16, INT8, INT4 (GGUF)

**Erwartete Performance** (NVIDIA RTX 4090):
```
TinyLlama (FP16, Batch=1):         > 150 tokens/s
TinyLlama (INT8, Batch=1):         > 280 tokens/s
TinyLlama (INT4, Batch=1):         > 450 tokens/s
Phi-2 (INT8, Batch=1):             > 120 tokens/s
Llama-2-7B (INT4, Batch=1):        > 60 tokens/s

LoRA Adapter Switch:               < 5ms
LoRA Adapter Load:                 < 50ms
LoRA Inference Overhead:           < 2% vs. Base Model
Multi-LoRA Fusion (4 adapters):    > 80 tokens/s
```

**Code-Beispiel**:
```cpp
BENCHMARK_DEFINE_F(LoRAFixture, InferenceWithAdapter)(benchmark::State& state) {
    const size_t batch_size = state.range(0);
    
    // Load LoRA adapter
    lora_manager_->loadAdapter("sentiment-analysis", 
                               "./models/lora/sentiment_r8_alpha16.safetensors");
    lora_manager_->activateAdapter("sentiment-analysis");
    
    std::vector<std::string> prompts = generateBatchPrompts(batch_size);
    
    for (auto _ : state) {
        state.PauseTiming();
        auto input_tokens = tokenize(prompts);
        state.ResumeTiming();
        
        auto output = llm_engine_->generate(input_tokens, 
                                           /*max_tokens=*/50,
                                           /*temperature=*/0.7);
        benchmark::DoNotOptimize(output);
    }
    
    double tokens_per_sec = (state.iterations() * batch_size * 50) / 
                           (state.iterations() * state.seconds_elapsed());
    state.counters["tokens_per_sec"] = benchmark::Counter(
        tokens_per_sec, benchmark::Counter::kIsRate);
}
BENCHMARK_REGISTER_F(LoRAFixture, InferenceWithAdapter)
    ->Arg(1)->Arg(4)->Arg(16)->Arg(32)
    ->Unit(benchmark::kMillisecond);
```

**Python Integration Test**:
```python
# benchmarks/llm_nlp_integration_test_suite.py
import asyncio
import httpx
from typing import List

async def benchmark_llm_inference(
    base_url: str,
    model_name: str,
    batch_size: int,
    num_iterations: int
) -> dict:
    """Benchmark LLM inference over HTTP API."""
    
    async with httpx.AsyncClient(timeout=30.0) as client:
        prompts = [
            f"Analyze sentiment: {generate_random_review()}"
            for _ in range(batch_size)
        ]
        
        start = time.perf_counter()
        
        for _ in range(num_iterations):
            response = await client.post(
                f"{base_url}/llm/generate",
                json={
                    "model": model_name,
                    "prompts": prompts,
                    "max_tokens": 50,
                    "temperature": 0.7
                }
            )
            response.raise_for_status()
        
        elapsed = time.perf_counter() - start
        total_tokens = num_iterations * batch_size * 50
        
        return {
            "total_time": elapsed,
            "tokens_per_second": total_tokens / elapsed,
            "latency_ms": (elapsed / num_iterations) * 1000,
            "throughput_rps": num_iterations / elapsed
        }

# Ausführung
results = asyncio.run(benchmark_llm_inference(
    base_url="http://localhost:8080",
    model_name="TinyLlama-1.1B-Chat-v1.0-Q4_K_M",
    batch_size=4,
    num_iterations=100
))

print(f"Tokens/s: {results['tokens_per_second']:.2f}")
print(f"Latency: {results['latency_ms']:.2f} ms")
```

---

### Szenario 6: Multi-Shard RAID

**Ziel**: Distributed Sharding mit RAID-Konfigurationen (MIRROR, STRIPE, PARITY).

**Benchmark**: 
- `benchmarks/bench_sharding_performance.cpp`
- `benchmarks/run_multi_shard_raid_benchmark.py`
- `benchmarks/raid_sharding_test_suite.py`

**Workload**:
- **Shard-Anzahl**: 2, 4, 8, 16 Shards
- **RAID-Modi**: NONE, MIRROR, STRIPE, PARITY
- **Replication Factor**: 1, 2, 3
- **Query-Typen**: Single-Shard, Cross-Shard, Broadcast
- **Datenmenge**: 10M Entitäten, 100GB Daten

**Erwartete Performance** (4-Node Cluster, 10GbE):
```
Single-Shard Query:                > 100K ops/s
Cross-Shard Query (2 shards):      > 60K ops/s
Cross-Shard Query (8 shards):      > 30K ops/s
Broadcast Query (all shards):      > 20K ops/s

RAID MIRROR (RF=2):
  - Write Throughput:              50% of single node
  - Read Throughput:               180% of single node
  - Failover Time:                 < 100ms

RAID STRIPE:
  - Write Throughput:              350% of single node
  - Read Throughput:               380% of single node
  - Shard Rebalancing:             > 5 GB/s

RAID PARITY (4+1):
  - Write Throughput:              280% of single node
  - Read Throughput:               420% of single node
  - Rebuild Time (1TB):            < 10 minutes
```

**Code-Beispiel** (C++):
```cpp
BENCHMARK_DEFINE_F(ShardingFixture, CrossShardQuery)(benchmark::State& state) {
    const int num_shards = state.range(0);
    
    // Setup sharding coordinator
    auto coordinator = std::make_unique<ShardingCoordinator>(num_shards);
    for (int i = 0; i < num_shards; ++i) {
        coordinator->addShard(i, "node_" + std::to_string(i), 8080 + i);
    }
    
    for (auto _ : state) {
        state.PauseTiming();
        // Query spans multiple shards
        auto query = AQLQuery(R"(
            FOR doc IN Person
            FILTER doc.age > 25 AND doc.age < 35
            RETURN doc
        )");
        state.ResumeTiming();
        
        auto result = coordinator->executeQuery(query);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["shards"] = num_shards;
}
BENCHMARK_REGISTER_F(ShardingFixture, CrossShardQuery)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16)
    ->Unit(benchmark::kMillisecond);
```

**Python Docker Test**:
```python
# benchmarks/run_multi_shard_raid_benchmark.py
import docker
import time
from typing import List

def run_raid_benchmark(
    num_shards: int,
    raid_mode: str,
    replication_factor: int
) -> dict:
    """Run multi-shard RAID benchmark in Docker."""
    
    client = docker.from_env()
    
    # Start shard containers
    containers = []
    for i in range(num_shards):
        container = client.containers.run(
            "themisdb/themisdb:latest",
            name=f"themis-shard-{i}",
            environment={
                "THEMIS_SHARD_ID": str(i),
                "THEMIS_RAID_MODE": raid_mode,
                "THEMIS_REPLICATION_FACTOR": str(replication_factor)
            },
            ports={f"808{i}/tcp": 8080 + i},
            detach=True,
            network="themis-raid-net"
        )
        containers.append(container)
    
    # Wait for cluster formation
    time.sleep(10)
    
    # Run benchmark
    results = {
        "write_ops_per_sec": 0,
        "read_ops_per_sec": 0,
        "cross_shard_latency_ms": 0
    }
    
    try:
        # Insert 100K entities
        start = time.perf_counter()
        for i in range(100000):
            shard_id = i % num_shards
            response = requests.post(
                f"http://localhost:{8080 + shard_id}/api/v1/entity",
                json={"id": f"person_{i}", "name": f"User {i}", "age": 25 + (i % 50)}
            )
        elapsed = time.perf_counter() - start
        results["write_ops_per_sec"] = 100000 / elapsed
        
        # Cross-shard queries
        start = time.perf_counter()
        for _ in range(1000):
            response = requests.post(
                f"http://localhost:8080/api/v1/aql",
                json={"query": "FOR doc IN Person FILTER doc.age > 30 RETURN doc"}
            )
        elapsed = time.perf_counter() - start
        results["cross_shard_latency_ms"] = (elapsed / 1000) * 1000
        
    finally:
        # Cleanup
        for container in containers:
            container.stop()
            container.remove()
    
    return results

# Ausführung
for shards in [2, 4, 8]:
    for raid_mode in ["MIRROR", "STRIPE", "PARITY"]:
        print(f"\n=== {shards} Shards, RAID {raid_mode} ===")
        results = run_raid_benchmark(shards, raid_mode, replication_factor=2)
        print(f"Write: {results['write_ops_per_sec']:.0f} ops/s")
        print(f"Cross-Shard Latency: {results['cross_shard_latency_ms']:.2f} ms")
```

**Docker Compose Konfiguration**:
```yaml
# benchmarks/docker-compose.multi-shard-raid.yml
version: '3.8'

services:
  themis-shard-0:
    image: themisdb/themisdb:latest
    container_name: themis-shard-0
    environment:
      THEMIS_SHARD_ID: "0"
      THEMIS_TOTAL_SHARDS: "4"
      THEMIS_RAID_MODE: "MIRROR"
      THEMIS_REPLICATION_FACTOR: "2"
      THEMIS_COORDINATOR_URL: "http://themis-coordinator:9090"
    ports:
      - "8080:8080"
    networks:
      - themis-raid-net
    volumes:
      - shard0-data:/var/lib/themisdb

  themis-shard-1:
    image: themisdb/themisdb:latest
    container_name: themis-shard-1
    environment:
      THEMIS_SHARD_ID: "1"
      THEMIS_TOTAL_SHARDS: "4"
      THEMIS_RAID_MODE: "MIRROR"
      THEMIS_REPLICATION_FACTOR: "2"
      THEMIS_COORDINATOR_URL: "http://themis-coordinator:9090"
    ports:
      - "8081:8080"
    networks:
      - themis-raid-net
    volumes:
      - shard1-data:/var/lib/themisdb

  # ... shard-2, shard-3 ähnlich ...

  themis-coordinator:
    image: themisdb/coordinator:latest
    container_name: themis-coordinator
    ports:
      - "9090:9090"
    networks:
      - themis-raid-net
    environment:
      THEMIS_COORDINATOR_MODE: "true"
      THEMIS_SHARDS: "themis-shard-0:8080,themis-shard-1:8080,themis-shard-2:8080,themis-shard-3:8080"

networks:
  themis-raid-net:
    driver: bridge

volumes:
  shard0-data:
  shard1-data:
  shard2-data:
  shard3-data:
```

**Ausführung**:
```bash
# Docker Compose starten
cd benchmarks
docker-compose -f docker-compose.multi-shard-raid.yml up -d

# Warten auf Cluster-Formation
sleep 15

# RAID Benchmark ausführen
python3 run_multi_shard_raid_benchmark.py \
  --shard-configs "2,4,8" \
  --raid-modes "MIRROR,STRIPE,PARITY" \
  --duration 300

# Logs prüfen
docker-compose -f docker-compose.multi-shard-raid.yml logs -f

# Cleanup
docker-compose -f docker-compose.multi-shard-raid.yml down -v
```

---

## ⚙️ Hardware-Anforderungen

### Mindestanforderungen

**Für Mikro-Benchmarks (C++)**:
```
CPU:      4 Kerne / 8 Threads (z.B. Intel i5-10400 / AMD Ryzen 5 3600)
RAM:      8 GB DDR4
Storage:  50 GB SSD (SATA)
OS:       Linux (Ubuntu 22.04+), macOS (12.0+), Windows 10/11
Compiler: GCC 11+, Clang 14+, MSVC 19.30+
```

**Für Integration Benchmarks (Python)**:
```
CPU:      4 Kerne / 8 Threads
RAM:      16 GB DDR4
Storage:  100 GB SSD (NVMe empfohlen)
Network:  1 Gbps
Python:   3.10+
```

**Für Docker Benchmarks**:
```
CPU:      8 Kerne / 16 Threads
RAM:      32 GB DDR4
Storage:  200 GB SSD (NVMe)
Network:  10 Gbps (für Multi-Node Tests)
Docker:   20.10+
```

**Für CHIMERA Suite**:
```
CPU:      16 Kerne / 32 Threads (z.B. AMD Ryzen 9 5950X)
RAM:      64 GB DDR4
Storage:  500 GB NVMe SSD
Network:  10 Gbps
GPU:      Optional (für LLM Benchmarks: NVIDIA RTX 3090+, 24GB VRAM)
```

### Empfohlene Konfiguration

**High-Performance Benchmark System**:
```
CPU:      AMD Ryzen 9 7950X (16C/32T, 5.7 GHz Boost)
          oder Intel Core i9-13900K (24C/32T, 5.8 GHz Boost)
RAM:      128 GB DDR5-5600 (2x 64GB)
Storage:  2TB NVMe Gen4 SSD (Samsung 990 Pro, 7000 MB/s Read)
          + 4TB NVMe Gen4 für Daten (WD Black SN850X)
Network:  10 Gbps Ethernet (für Multi-Node)
          oder 25 Gbps (für große RAID-Cluster)
GPU:      NVIDIA RTX 4090 (24GB VRAM) für LLM Benchmarks
          oder NVIDIA A100 (40GB/80GB) für Production
OS:       Ubuntu 24.04 LTS Server
          Kernel 6.5+ mit Performance Governor
```

**Cluster Setup** (für Multi-Shard RAID):
```
Nodes:    4-16 Nodes (identische Hardware)
Network:  10 Gbps Ethernet mit RDMA
          oder InfiniBand (100 Gbps)
Storage:  Distributed (Ceph / GlusterFS)
          oder Local NVMe + RAID 10
Latency:  < 1ms zwischen Nodes
```

### Umgebungsvariablen

**Build-Zeit Konfiguration**:
```bash
# CMake Build-Typen
export CMAKE_BUILD_TYPE=Release           # Produktions-Build
export CMAKE_BUILD_TYPE=RelWithDebInfo    # Mit Debug-Symbolen
export CMAKE_BUILD_TYPE=Debug             # Debug-Build (langsam)

# Compiler-Optimierungen
export CMAKE_CXX_FLAGS="-march=native -mtune=native"  # CPU-spezifisch
export CMAKE_CXX_FLAGS="-O3 -flto"                    # Link-Time Optimization

# Google Benchmark
export BENCHMARK_ENABLE_LTO=ON            # Link-Time Optimization
export BENCHMARK_ENABLE_TESTING=OFF       # Keine Benchmark-Tests
```

**Laufzeit-Konfiguration**:
```bash
# ThemisDB Konfiguration
export THEMIS_DB_PATH=/mnt/nvme/themisdb  # Datenbank-Pfad
export THEMIS_MEMTABLE_SIZE_MB=256        # RocksDB MemTable
export THEMIS_BLOCK_CACHE_SIZE_MB=4096    # Block Cache (4GB)
export THEMIS_MAX_OPEN_FILES=1000000      # File Handles
export THEMIS_COMPRESSION=zstd            # Kompression (none/lz4/zstd)
export THEMIS_WRITE_BUFFER_COUNT=4        # Write Buffers

# Performance Tuning
export THEMIS_DISABLE_WAL=false           # Write-Ahead Log (für Benchmarks: true)
export THEMIS_ALLOW_MMAP_WRITES=true      # Memory-Mapped Writes
export THEMIS_COMPACTION_THREADS=8        # Compaction Threads
export THEMIS_BLOOM_FILTER_BITS=10        # Bloom Filter (10 bits/key)

# Sharding & RAID
export THEMIS_SHARD_ID=0                  # Shard ID (0-N)
export THEMIS_TOTAL_SHARDS=4              # Anzahl Shards
export THEMIS_RAID_MODE=MIRROR            # NONE/MIRROR/STRIPE/PARITY
export THEMIS_REPLICATION_FACTOR=2        # Replikationsfaktor
export THEMIS_COORDINATOR_URL=http://localhost:9090

# LLM/LoRA Konfiguration
export THEMIS_LLM_MODEL_PATH=/models/tinyllama-1.1b-chat-q4_k_m.gguf
export THEMIS_LORA_CACHE_SIZE_MB=2048     # LoRA Adapter Cache
export THEMIS_GPU_DEVICE=0                # GPU Device ID
export THEMIS_GPU_LAYERS=32               # Anzahl GPU Layers

# Logging & Monitoring
export THEMIS_LOG_LEVEL=info              # trace/debug/info/warn/error
export THEMIS_METRICS_PORT=9091           # Prometheus Metrics
export THEMIS_ENABLE_PROFILING=false      # CPU Profiling (perf)

# System Tuning (Linux)
export OMP_NUM_THREADS=16                 # OpenMP Threads
export MKL_NUM_THREADS=16                 # Intel MKL Threads
export OPENBLAS_NUM_THREADS=16            # OpenBLAS Threads
```

**CPU Governor** (Linux):
```bash
# Performance Mode (maximale Leistung)
sudo cpupower frequency-set -g performance

# Turbo Boost aktivieren
echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo

# CPU Affinity für Benchmarks
taskset -c 0-15 ./benchmarks/bench_vector_search
```

**Memory & Filesystem**:
```bash
# Swappiness reduzieren
sudo sysctl vm.swappiness=10

# Transparent Huge Pages
echo always | sudo tee /sys/kernel/mm/transparent_hugepage/enabled

# Filesystem Mount-Optionen (fstab)
/dev/nvme0n1p1 /mnt/nvme ext4 noatime,nodiratime,discard,data=ordered 0 2

# I/O Scheduler (für NVMe)
echo none | sudo tee /sys/block/nvme0n1/queue/scheduler
```

**Docker Konfiguration**:
```bash
# Docker Ressourcen
export DOCKER_MEMORY=16G                  # Memory Limit pro Container
export DOCKER_CPUS=8                      # CPU Limit pro Container
export DOCKER_STORAGE_DRIVER=overlay2     # Storage Driver

# Docker Compose
export COMPOSE_PARALLEL_LIMIT=8           # Parallele Container
```

### Hardware-Monitoring während Benchmarks

**CPU & Memory**:
```bash
# CPU Auslastung
htop

# Memory Usage
watch -n 1 free -h

# Per-Process Memory
ps aux --sort=-%mem | head -n 20

# CPU Frequency
watch -n 1 "cat /proc/cpuinfo | grep MHz"
```

**Storage**:
```bash
# Disk I/O
iostat -x 1

# NVMe Stats
nvme smart-log /dev/nvme0n1

# Disk Latency
ioping /mnt/nvme

# Throughput
dd if=/dev/zero of=/mnt/nvme/testfile bs=1M count=10000
```

**Network** (für Multi-Shard):
```bash
# Bandwidth
iperf3 -c node2 -t 60

# Latency
ping -c 100 node2

# Packet Loss
mtr node2
```

**GPU** (für LLM Benchmarks):
```bash
# GPU Usage
nvidia-smi -l 1

# VRAM Usage
nvidia-smi --query-gpu=memory.used,memory.total --format=csv -l 1

# GPU Temperature
nvidia-smi --query-gpu=temperature.gpu --format=csv -l 1
```

**System-weites Profiling**:
```bash
# CPU Profiling mit perf
perf record -g -F 99 ./benchmarks/bench_vector_search
perf report --stdio > profile.txt

# Memory Profiling mit Valgrind
valgrind --tool=massif ./benchmarks/bench_crud
ms_print massif.out.* > memory_profile.txt

# Call Graph
valgrind --tool=callgrind ./benchmarks/bench_graph_traversal
kcachegrind callgrind.out.*
```

---

## 📈 Beispielausgaben

### 1. Mikro-Benchmark Ausgabe (C++)

**Beispiel**: `./benchmarks/bench_vector_search`

```
Run on (32 X 5704.57 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x16)
  L1 Instruction 32 KiB (x16)
  L2 Unified 512 KiB (x16)
  L3 Unified 65536 KiB (x2)
Load Average: 1.23, 0.98, 0.76
--------------------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------
HNSW_Insert_384D/1000           2.34 ms         2.31 ms          302 items_per_second=432.9k/s complexity_n=1000
HNSW_Insert_384D/10000          26.8 ms         26.5 ms           26 items_per_second=377.4k/s complexity_n=10000
HNSW_Insert_384D/100000          312 ms          308 ms            2 items_per_second=324.7k/s complexity_n=100000
HNSW_Insert_384D/1000000        3.89 s          3.84 s             1 items_per_second=260.4k/s complexity_n=1000000
HNSW_Insert_384D_BigO         4.21 NlogN      4.16 NlogN

HNSW_Search_384D_k10/1000       18.2 us         18.0 us        38764 items_per_second=55.6k/s recall=0.972
HNSW_Search_384D_k10/10000      21.6 us         21.3 us        32854 items_per_second=46.9k/s recall=0.968
HNSW_Search_384D_k10/100000     25.8 us         25.4 us        27543 items_per_second=39.4k/s recall=0.965
HNSW_Search_384D_k10/1000000    31.2 us         30.8 us        22738 items_per_second=32.5k/s recall=0.961
HNSW_Search_384D_k10_BigO      0.01 logN       0.01 logN

HNSW_Search_384D_k50/1000       42.3 us         41.8 us        16745 items_per_second=23.9k/s recall=0.964
HNSW_Search_384D_k50/10000      48.7 us         48.1 us        14552 items_per_second=20.8k/s recall=0.961
HNSW_Search_384D_k50/100000     58.9 us         58.2 us        12019 items_per_second=17.2k/s recall=0.958
HNSW_Search_384D_k50/1000000    71.4 us         70.5 us         9926 items_per_second=14.2k/s recall=0.954
HNSW_Search_384D_k50_BigO      0.02 logN       0.02 logN

FAISS_IVF_Insert_1536D/10000    187 ms          185 ms            4 items_per_second=54.1k/s nlist=128
FAISS_IVF_Search_1536D/10000    12.4 us         12.2 us        57394 items_per_second=82.0k/s nprobe=16 recall=0.923
```

**Mit JSON Output**:
```bash
./benchmarks/bench_vector_search --benchmark_format=json --benchmark_out=results.json
```

```json
{
  "context": {
    "date": "2026-02-02T14:23:45+01:00",
    "host_name": "benchmark-server-01",
    "executable": "./benchmarks/bench_vector_search",
    "num_cpus": 32,
    "mhz_per_cpu": 5704,
    "cpu_scaling_enabled": false,
    "caches": [
      {"type": "L1 Data", "level": 1, "size": 32768, "num_sharing": 1},
      {"type": "L1 Instruction", "level": 1, "size": 32768, "num_sharing": 1},
      {"type": "L2 Unified", "level": 2, "size": 524288, "num_sharing": 1},
      {"type": "L3 Unified", "level": 3, "size": 67108864, "num_sharing": 16}
    ],
    "load_avg": [1.23, 0.98, 0.76],
    "library_build_type": "release"
  },
  "benchmarks": [
    {
      "name": "HNSW_Insert_384D/10000",
      "family_index": 0,
      "per_family_instance_index": 1,
      "run_name": "HNSW_Insert_384D/10000",
      "run_type": "iteration",
      "repetitions": 1,
      "repetition_index": 0,
      "threads": 1,
      "iterations": 26,
      "real_time": 26.8234,
      "cpu_time": 26.5182,
      "time_unit": "ms",
      "items_per_second": 377358.4,
      "complexity_n": 10000,
      "recall": 0.972
    }
  ]
}
```

---

### 2. Python Integration Benchmark Ausgabe

**Beispiel**: `python3 complete_benchmark_suite.py --mode ycsb --workloads A`

```
╔════════════════════════════════════════════════════════════════════════════╗
║           CHIMERA SUITE - YCSB Workload A Benchmark Results              ║
║                         ThemisDB v2.0.0                                   ║
╚════════════════════════════════════════════════════════════════════════════╝

[INFO] Starting YCSB Workload A (50% Read, 50% Update)
[INFO] Database: ThemisDB @ http://localhost:8080
[INFO] Operations: 100,000 (Warmup: 10,000)
[INFO] Record Count: 1,000,000
[INFO] Record Size: 1KB (avg)
[INFO] Threads: 16

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Phase 1: Data Loading
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[████████████████████████████████████████] 1,000,000/1,000,000 (100%)
  
  Total Time:        23.4 seconds
  Throughput:        42,735 ops/sec
  Avg Latency:       0.37 ms
  P50 Latency:       0.32 ms
  P95 Latency:       0.89 ms
  P99 Latency:       1.54 ms
  Max Latency:       12.3 ms

Phase 2: Warmup (10,000 ops)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[████████████████████████████████████████] 10,000/10,000 (100%)
  
  Warmup Time:       0.2 seconds
  Status:            ✓ Complete

Phase 3: Workload Execution
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[████████████████████████████████████████] 100,000/100,000 (100%)

  Total Operations:  100,000
  Total Time:        2.14 seconds
  Throughput:        46,729 ops/sec

  ┌─────────────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
  │ Operation       │   Count  │  Avg(ms) │  P50(ms) │  P95(ms) │  P99(ms) │
  ├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
  │ READ            │  50,012  │    0.18  │    0.16  │    0.42  │    0.78  │
  │ UPDATE          │  49,988  │    0.24  │    0.21  │    0.56  │    1.03  │
  ├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
  │ TOTAL           │ 100,000  │    0.21  │    0.19  │    0.49  │    0.91  │
  └─────────────────┴──────────┴──────────┴──────────┴──────────┴──────────┘

  Error Rate:        0.00% (0/100,000)
  Success Rate:      100.00%

Statistical Analysis (α=0.05, n=10 repetitions)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  Mean Throughput:   46,729 ± 1,234 ops/sec (95% CI)
  Coefficient of Variation: 2.64%
  Effect Size (Cohen's d): 2.89 (Large)
  Statistical Power: 0.98

Performance Grade: A+ (Excellent)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[✓] Benchmark completed successfully
[✓] Results saved to: results/ycsb_workload_a_20260202_142345.json
[✓] Report generated: results/ycsb_workload_a_20260202_142345.html
```

**JSON Output**:
```json
{
  "benchmark_suite": "CHIMERA",
  "version": "2.0.0",
  "database": "ThemisDB",
  "workload": "YCSB-A",
  "timestamp": "2026-02-02T14:23:45+01:00",
  "system_info": {
    "cpu": "AMD Ryzen 9 7950X",
    "cores": 32,
    "memory_gb": 128,
    "os": "Ubuntu 24.04 LTS"
  },
  "configuration": {
    "operations": 100000,
    "record_count": 1000000,
    "record_size_bytes": 1024,
    "threads": 16,
    "warmup_operations": 10000
  },
  "results": {
    "load_phase": {
      "duration_seconds": 23.4,
      "throughput_ops_per_sec": 42735,
      "latency_ms": {
        "mean": 0.37,
        "median": 0.32,
        "p95": 0.89,
        "p99": 1.54,
        "max": 12.3
      }
    },
    "run_phase": {
      "duration_seconds": 2.14,
      "throughput_ops_per_sec": 46729,
      "operations": {
        "READ": {
          "count": 50012,
          "latency_ms": {
            "mean": 0.18,
            "median": 0.16,
            "p95": 0.42,
            "p99": 0.78
          }
        },
        "UPDATE": {
          "count": 49988,
          "latency_ms": {
            "mean": 0.24,
            "median": 0.21,
            "p95": 0.56,
            "p99": 1.03
          }
        }
      },
      "error_rate": 0.0,
      "success_rate": 1.0
    },
    "statistical_analysis": {
      "repetitions": 10,
      "mean_throughput": 46729,
      "std_dev": 1234,
      "confidence_interval_95": [45495, 47963],
      "coefficient_of_variation": 0.0264,
      "cohens_d": 2.89,
      "statistical_power": 0.98
    }
  },
  "grade": "A+"
}
```

---

### 3. Docker Benchmark Ausgabe

**Beispiel**: `python3 run_multi_shard_raid_benchmark.py --shards 4 --raid-mode MIRROR`

```
╔════════════════════════════════════════════════════════════════════════════╗
║              Multi-Shard RAID Benchmark Suite v2.0                        ║
║                     Configuration: 4 Shards, RAID MIRROR                  ║
╚════════════════════════════════════════════════════════════════════════════╝

[INFO] Starting Docker containers...
[INFO] Creating network: themis-raid-net
[INFO] Starting Shard 0: themis-shard-0 (MIRROR, RF=2)
[INFO] Starting Shard 1: themis-shard-1 (MIRROR, RF=2)
[INFO] Starting Shard 2: themis-shard-2 (MIRROR, RF=2)
[INFO] Starting Shard 3: themis-shard-3 (MIRROR, RF=2)
[INFO] Starting Coordinator: themis-coordinator
[INFO] Waiting for cluster formation... (15s)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Cluster Status
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  ✓ Shard 0: HEALTHY (http://localhost:8080)
  ✓ Shard 1: HEALTHY (http://localhost:8081)
  ✓ Shard 2: HEALTHY (http://localhost:8082)
  ✓ Shard 3: HEALTHY (http://localhost:8083)
  ✓ Coordinator: HEALTHY (http://localhost:9090)
  
  Replication Status: 2x MIRROR (All replicas synchronized)

Phase 1: Single-Shard Insert Performance
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[INFO] Inserting 100,000 entities per shard...
  
  Shard 0: [████████████████████] 100,000/100,000 → 38,245 ops/s
  Shard 1: [████████████████████] 100,000/100,000 → 37,892 ops/s
  Shard 2: [████████████████████] 100,000/100,000 → 38,512 ops/s
  Shard 3: [████████████████████] 100,000/100,000 → 38,076 ops/s
  
  Average:           38,181 ops/s
  Total Entities:    400,000
  Total Time:        10.48 seconds
  RAID Overhead:     45.2% (vs. single node)

Phase 2: Cross-Shard Query Performance
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[INFO] Executing 10,000 cross-shard queries...

  Query Type: Range Scan (age > 25 AND age < 35)
  Shards Touched: 4
  
  Total Queries:     10,000
  Success Rate:      100.00%
  Total Time:        3.42 seconds
  Throughput:        2,924 queries/s
  
  ┌──────────────┬──────────┬──────────┬──────────┬──────────┐
  │ Metric       │  Avg(ms) │  P50(ms) │  P95(ms) │  P99(ms) │
  ├──────────────┼──────────┼──────────┼──────────┼──────────┤
  │ Query Time   │    3.42  │    3.12  │    7.89  │   14.56  │
  │ Shard Fanout │    4.00  │    4.00  │    4.00  │    4.00  │
  │ Network RTT  │    0.23  │    0.19  │    0.56  │    0.89  │
  └──────────────┴──────────┴──────────┴──────────┴──────────┘

Phase 3: Failover Test
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[INFO] Simulating Shard 2 failure...
  ⚠ Stopping container: themis-shard-2
  
  [INFO] Coordinator detected failure in 127ms
  [INFO] Routing queries to replica shard...
  
  Query Execution During Failover:
    Queries Executed:   1,000
    Success Rate:       99.8% (998/1,000)
    Failed Queries:     2 (during failover window)
    Avg Latency:        4.12ms (+20.5% vs. normal)
    Failover Time:      89ms
  
  ✓ Failover completed successfully
  
[INFO] Restoring Shard 2...
  [INFO] Replication catch-up: 1,234 operations replayed
  [INFO] Shard 2 back online (5.2 seconds)

Summary
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  Configuration:     4 Shards, RAID MIRROR, Replication Factor 2
  Total Entities:    400,000
  Write Throughput:  38,181 ops/s (45% of single node)
  Read Throughput:   68,542 ops/s (182% of single node)
  Cross-Shard Query: 2,924 queries/s
  Failover Time:     89ms
  Data Durability:   100% (no data loss)
  
  Performance Grade:  A (Excellent)
  Reliability Grade:  A+ (Excellent)

[✓] Benchmark completed successfully
[✓] Results saved to: results/raid_mirror_4shards_20260202.json
[INFO] Cleaning up Docker containers...
```

---

### 4. CHIMERA Suite Ausgabe (Scientific)

**Beispiel**: `python3 scientific_benchmark_runner.py --workloads ycsb tpcc --databases themis postgresql`

```
╔════════════════════════════════════════════════════════════════════════════╗
║                  CHIMERA SUITE - Scientific Benchmark                     ║
║       Comprehensive Hybrid Inferencing & Multi-model Evaluation           ║
║                           v2.0.0 (IEEE/ACM)                               ║
╚════════════════════════════════════════════════════════════════════════════╝

[INFO] Scientific Protocol: ACTIVE
[INFO] Statistical Rigor: α=0.05, Power=0.80, n=10 repetitions
[INFO] Databases: ThemisDB v2.0.0, PostgreSQL 16.2
[INFO] Workloads: YCSB (A-F), TPC-C

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Benchmark 1/2: YCSB Workload A (50% Read, 50% Update)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Database: ThemisDB
─────────────────────────────────────────────────────────────────────────────
  Repetition  1/10: [████████████████████] 46,823 ops/s
  Repetition  2/10: [████████████████████] 47,012 ops/s
  Repetition  3/10: [████████████████████] 46,634 ops/s
  Repetition  4/10: [████████████████████] 46,945 ops/s
  Repetition  5/10: [████████████████████] 47,198 ops/s
  Repetition  6/10: [████████████████████] 46,712 ops/s
  Repetition  7/10: [████████████████████] 46,889 ops/s
  Repetition  8/10: [████████████████████] 47,034 ops/s
  Repetition  9/10: [████████████████████] 46,756 ops/s
  Repetition 10/10: [████████████████████] 46,923 ops/s
  
  Mean:    46,893 ± 184 ops/s (95% CI: [46,709, 47,077])
  Median:  46,906 ops/s
  Std Dev: 185 ops/s
  CV:      0.39% (Excellent stability)

Database: PostgreSQL
─────────────────────────────────────────────────────────────────────────────
  Repetition  1/10: [████████████████████] 28,234 ops/s
  Repetition  2/10: [████████████████████] 28,456 ops/s
  Repetition  3/10: [████████████████████] 28,012 ops/s
  Repetition  4/10: [████████████████████] 28,378 ops/s
  Repetition  5/10: [████████████████████] 28,567 ops/s
  Repetition  6/10: [████████████████████] 28,145 ops/s
  Repetition  7/10: [████████████████████] 28,298 ops/s
  Repetition  8/10: [████████████████████] 28,423 ops/s
  Repetition  9/10: [████████████████████] 28,189 ops/s
  Repetition 10/10: [████████████████████] 28,334 ops/s
  
  Mean:    28,304 ± 176 ops/s (95% CI: [28,128, 28,480])
  Median:  28,316 ops/s
  Std Dev: 177 ops/s
  CV:      0.63%

Statistical Comparison
─────────────────────────────────────────────────────────────────────────────
  ThemisDB vs. PostgreSQL:
  
  ✓ Mann-Whitney U Test:      p < 0.001 (Highly Significant)
  ✓ Effect Size (Cohen's d):  105.2 (Very Large)
  ✓ Speedup Factor:           1.66x (66% faster)
  ✓ Statistical Power:        > 0.99
  
  Confidence: ★★★★★ (Very High)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Benchmark 2/2: TPC-C (New-Order Transaction)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Database: ThemisDB
─────────────────────────────────────────────────────────────────────────────
  Warehouses: 10
  Scale Factor: Medium
  
  TpmC (Transactions per Minute): 12,456 ± 234
  Latency (ms): 48.2 ± 1.2
  
  Transaction Mix:
    New-Order:     45.0% → 15,234 tx/min
    Payment:       43.0% → 14,567 tx/min
    Order-Status:   4.0% →  1,234 tx/min
    Delivery:       4.0% →  1,234 tx/min
    Stock-Level:    4.0% →  1,234 tx/min

Database: PostgreSQL
─────────────────────────────────────────────────────────────────────────────
  TpmC: 9,123 ± 198
  Latency (ms): 65.8 ± 1.8

Statistical Comparison
─────────────────────────────────────────────────────────────────────────────
  ✓ ThemisDB 36.5% faster (p < 0.001)
  ✓ Latency 26.7% lower (p < 0.001)
  ✓ Effect Size: 15.4 (Very Large)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Final Results Summary
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ┌──────────────┬─────────────┬──────────────┬───────────┬──────────────┐
  │ Workload     │   ThemisDB  │  PostgreSQL  │  Speedup  │ Significance │
  ├──────────────┼─────────────┼──────────────┼───────────┼──────────────┤
  │ YCSB-A       │ 46,893 ops/s│ 28,304 ops/s │   1.66x   │   p<0.001    │
  │ TPC-C        │ 12,456 tpmC │  9,123 tpmC  │   1.37x   │   p<0.001    │
  └──────────────┴─────────────┴──────────────┴───────────┴──────────────┘

  Overall Performance Grade: A+ (Excellent)
  Scientific Rigor: ✓ IEEE/ACM Compliant
  Reproducibility: ✓ Full (seed=42, warmup=10s, n=10)

[✓] Reports generated:
    - results/scientific_ycsb_20260202.html
    - results/scientific_tpcc_20260202.html
    - results/scientific_comparison_20260202.pdf
    
[✓] Artifacts available for ACM Artifact Badging
```

---

## 🔧 Fehlerquellen & Troubleshooting

### Problem 1: Benchmark stürzt mit Segmentation Fault ab

**Symptome**:
```
./benchmarks/bench_vector_search
Segmentation fault (core dumped)
```

**Ursachen & Lösungen**:

**1. RocksDB nicht initialisiert**:
```cpp
// FALSCH
VectorIndexManager vector_index(*db_);  // db_ ist nullptr

// RICHTIG
db_ = std::make_unique<RocksDBWrapper>(config);
if (!db_->open()) {
    throw std::runtime_error("Failed to open database");
}
vector_index_ = std::make_unique<VectorIndexManager>(*db_);
```

**2. Stack Overflow bei großen Vektoren**:
```bash
# Stack Size erhöhen
ulimit -s unlimited

# Oder in CMakeLists.txt
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,stack-size=16777216")
```

**3. Out of Memory (OOM)**:
```bash
# Memory Limits prüfen
free -h
cat /proc/meminfo | grep -i available

# Swap aktivieren (temporär)
sudo fallocate -l 16G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# Benchmark mit kleineren Datensätzen
./benchmarks/bench_vector_search --benchmark_filter=HNSW.*1000$
```

**4. Debug Build für Diagnose**:
```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make bench_vector_search

# Mit GDB debuggen
gdb ./benchmarks/bench_vector_search
(gdb) run
(gdb) backtrace
(gdb) frame 0
(gdb) print variable_name
```

**5. Valgrind für Memory Leaks**:
```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./benchmarks/bench_vector_search
```

---

### Problem 2: Benchmark hängt / macht keinen Fortschritt

**Symptome**:
```
./benchmarks/bench_transaction_throughput
Run on (32 X 5704 MHz CPU s)
...
[Keine weitere Ausgabe, Prozess hängt]
```

**Ursachen & Lösungen**:

**1. Deadlock in Transaktionen**:
```bash
# Mit Timeout laufen lassen
timeout 60s ./benchmarks/bench_transaction_throughput

# Deadlock Detection aktivieren
export THEMIS_DEADLOCK_DETECTION=true
export THEMIS_DEADLOCK_TIMEOUT_MS=5000

# Mit strace analysieren
strace -f -e trace=futex ./benchmarks/bench_transaction_throughput
```

**2. RocksDB Compaction blockiert**:
```bash
# Compaction deaktivieren (nur für Benchmarks!)
export THEMIS_DISABLE_AUTO_COMPACTION=true

# Oder in Code:
rocksdb::Options options;
options.disable_auto_compactions = true;
```

**3. Infinite Loop in Benchmark-Code**:
```cpp
// FALSCH: Endlosschleife
BENCHMARK_DEFINE_F(Fixture, BadBenchmark)(benchmark::State& state) {
    for (auto _ : state) {
        while (true) {  // FEHLER!
            doSomething();
        }
    }
}

// RICHTIG: Kontrollierte Iteration
BENCHMARK_DEFINE_F(Fixture, GoodBenchmark)(benchmark::State& state) {
    for (auto _ : state) {
        doSomething();  // Pro Iteration einmal
    }
}
```

**4. Netzwerk-Timeout (Python Benchmarks)**:
```python
# FALSCH
response = httpx.get("http://localhost:8080/api/v1/entity")  # Default timeout 5s

# RICHTIG
async with httpx.AsyncClient(timeout=httpx.Timeout(30.0, connect=10.0)) as client:
    response = await client.get("http://localhost:8080/api/v1/entity")
```

**5. Process diagnostizieren**:
```bash
# Top anzeigen
top -H -p <PID_HERE>

# Stack Trace von laufendem Prozess
sudo gdb -p <PID_HERE>
(gdb) thread apply all bt
(gdb) quit

# Oder mit pstack
sudo pstack <PID_HERE>
```

---

### Problem 3: Performance viel schlechter als erwartet

**Symptome**:
```
HNSW_Search_384D_k10/10000    250 us    (Erwartet: 21 us)
```

**Ursachen & Lösungen**:

**1. Debug Build statt Release**:
```bash
# Build-Type prüfen
grep CMAKE_BUILD_TYPE CMakeCache.txt

# Release Build erstellen
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -march=native" \
      ..
make -j$(nproc)
```

**2. CPU Frequency Scaling (Power Saving)**:
```bash
# Current Governor
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Performance Mode
sudo cpupower frequency-set -g performance

# Turbo Boost prüfen (Intel)
cat /sys/devices/system/cpu/intel_pstate/no_turbo  # 0 = enabled

# SMT/Hyper-Threading aktiv?
lscpu | grep "Thread(s) per core"
```

**3. System unter Last**:
```bash
# Load Average prüfen
uptime

# Processes mit hoher CPU
top -o %CPU

# I/O Wait
iostat -x 1

# Benchmark isoliert laufen lassen
nice -n -20 taskset -c 0-15 ./benchmarks/bench_vector_search
```

**4. Thermisches Throttling**:
```bash
# CPU Temperature
sensors | grep Core

# Throttling Events
dmesg | grep -i throttl

# Turbo Frequency
watch -n 1 "grep MHz /proc/cpuinfo | head -n 1"

# Lösung: Bessere Kühlung oder niedrigere Clocks
sudo cpupower frequency-set --max 4.5GHz
```

**5. Disk I/O Bottleneck**:
```bash
# Disk Usage während Benchmark
iostat -x 1 /dev/nvme0n1

# Wenn >50% util: I/O ist Bottleneck
# Lösungen:
# - Größerer MemTable
export THEMIS_MEMTABLE_SIZE_MB=512

# - Block Cache erhöhen
export THEMIS_BLOCK_CACHE_SIZE_MB=8192

# - WAL deaktivieren (nur Benchmarks!)
export THEMIS_DISABLE_WAL=true

# - tmpfs verwenden
sudo mount -t tmpfs -o size=20G tmpfs /tmp/benchmark
export THEMIS_DB_PATH=/tmp/benchmark/themisdb
```

**6. Memory Bandwidth Limit**:
```bash
# Memory Bandwidth messen
sudo apt-get install intel-cmt-cat  # oder likwid
sudo pqos -m all:0-15  # Monitor Memory Bandwidth

# Lösung: Kleinere Batches, besseres Caching
```

---

### Problem 4: Python Benchmarks schlagen mit Connection Refused fehl

**Symptome**:
```python
httpx.ConnectError: [Errno 111] Connection refused
```

**Ursachen & Lösungen**:

**1. ThemisDB Server nicht gestartet**:
```bash
# Server Status prüfen
sudo systemctl status themisdb

# Oder manuell:
ps aux | grep themisdb

# Server starten
cd build
./src/themisdb --config /etc/themisdb/config.toml &

# Oder mit Docker
docker run -d -p 8080:8080 themisdb/themisdb:latest
```

**2. Falscher Port / Host**:
```python
# In benchmark script prüfen
BASE_URL = "http://localhost:8080"  # Korrekter Port?

# Port Binding prüfen
sudo netstat -tlnp | grep 8080
# Oder
sudo ss -tlnp | grep 8080

# Falls nicht 8080:
export THEMIS_HTTP_PORT=8080
```

**3. Firewall blockiert**:
```bash
# Firewall Status
sudo ufw status

# Port öffnen
sudo ufw allow 8080/tcp

# Oder iptables
sudo iptables -A INPUT -p tcp --dport 8080 -j ACCEPT
```

**4. Server Crash / OOM**:
```bash
# Logs prüfen
journalctl -u themisdb -f

# Oder Docker logs
docker logs themis-container

# Memory Usage
docker stats themis-container

# Mehr Memory zuweisen
docker run -m 16g -p 8080:8080 themisdb/themisdb:latest
```

**5. Network Issues (Docker)**:
```bash
# Docker Network prüfen
docker network ls
docker network inspect bridge

# Port Forwarding prüfen
docker port themis-container

# Curl Test
curl http://localhost:8080/health

# Falls nicht erreichbar:
docker run -p 8080:8080 --network host themisdb/themisdb:latest
```

---

### Problem 5: Docker Compose Benchmarks starten nicht

**Symptome**:
```bash
python3 run_docker_benchmarks.py
ERROR: Network themis-raid-net not found
```

**Ursachen & Lösungen**:

**1. Network existiert nicht**:
```bash
# Network erstellen
docker network create themis-raid-net

# Oder mit docker-compose
docker-compose -f docker-compose.multi-shard-raid.yml up -d --force-recreate
```

**2. Port already in use**:
```bash
# ERROR: Bind for 0.0.0.0:8080 failed: port is already allocated

# Prozess finden
sudo lsof -i :8080

# Oder anderen Port verwenden
docker-compose -f docker-compose.multi-shard-raid.yml up -d \
  --scale themis-shard-0=1 \
  -p 8180:8080
```

**3. Zu wenig Disk Space**:
```bash
# Disk Usage prüfen
df -h

# Docker cleanup
docker system prune -af --volumes

# Docker Images cleanup
docker rmi $(docker images -q -f dangling=true)
```

**4. Docker Daemon Limits**:
```bash
# /etc/docker/daemon.json
{
  "max-concurrent-downloads": 10,
  "max-concurrent-uploads": 10,
  "default-ulimits": {
    "nofile": {
      "Name": "nofile",
      "Hard": 1048576,
      "Soft": 1048576
    }
  }
}

# Docker neu starten
sudo systemctl restart docker
```

**5. Container Logs prüfen**:
```bash
# Alle Container Logs
docker-compose -f docker-compose.multi-shard-raid.yml logs -f

# Einzelner Container
docker logs themis-shard-0 --tail 100

# Container Status
docker-compose -f docker-compose.multi-shard-raid.yml ps

# Container neu starten
docker-compose -f docker-compose.multi-shard-raid.yml restart themis-shard-0
```

---

### Problem 6: LLM Benchmarks schlagen mit CUDA Out of Memory fehl

**Symptome**:
```
CUDA error: out of memory
Tried to allocate 2.50 GiB
```

**Ursachen & Lösungen**:

**1. GPU VRAM voll**:
```bash
# VRAM Usage prüfen
nvidia-smi

# NVIDIA processes beenden (falls nötig)
sudo fuser -k /dev/nvidia0
```

**2. Batch Size zu groß**:
```cpp
// Batch Size reduzieren
BENCHMARK_REGISTER_F(LoRAFixture, InferenceWithAdapter)
    ->Arg(1)->Arg(2)->Arg(4)  // Statt ->Arg(16)->Arg(32)
    ->Unit(benchmark::kMillisecond);
```

**3. Model Quantisierung verwenden**:
```bash
# Statt FP16/FP32: INT8 oder INT4
export THEMIS_LLM_MODEL_PATH=/models/tinyllama-1.1b-chat-q4_k_m.gguf  # Q4 Quantization

# Oder in Code:
llm_engine_->loadModel("TinyLlama", {
    .quantization = QuantizationType::INT4_K_M,
    .gpu_layers = 20  // Nur 20 statt 33 Layers auf GPU
});
```

**4. GPU Memory Fragmentation**:
```python
# Python: Explizites Cleanup
import torch
torch.cuda.empty_cache()

# C++: Model explizit freigeben
llm_engine_->unloadModel("TinyLlama");
llm_engine_.reset();  // Destructor aufrufen
```

**5. Multi-GPU verwenden**:
```bash
# Modell auf mehrere GPUs verteilen
export CUDA_VISIBLE_DEVICES=0,1

# In Code:
llm_engine_->loadModel("Llama-2-7B", {
    .tensor_split = {0.6, 0.4},  // 60% GPU0, 40% GPU1
    .gpu_layers = 32
});
```

---

### Problem 7: Benchmark-Ergebnisse nicht reproduzierbar

**Symptome**:
```
Run 1: 46,823 ops/s
Run 2: 38,234 ops/s  (-18%)
Run 3: 52,145 ops/s  (+11%)
```

**Ursachen & Lösungen**:

**1. Unzureichendes Warmup**:
```cpp
// FALSCH: Kein Warmup
BENCHMARK_REGISTER_F(Fixture, MyBench);

// RICHTIG: Mit Warmup
BENCHMARK_REGISTER_F(Fixture, MyBench)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1000)
    ->Repetitions(5)
    ->MinTime(2.0);  // Mindestens 2 Sekunden pro Run
```

**2. System nicht idle**:
```bash
# Vor Benchmarks:
# - Alle Browser schließen
# - Updates deaktivieren
# - Cron Jobs stoppen
sudo systemctl stop cron

# Load Average sollte < 1.0 sein
uptime
```

**3. CPU Frequency Variations**:
```bash
# CPU auf feste Frequenz setzen
sudo cpupower frequency-set --min 4.0GHz --max 4.0GHz

# Turbo Boost deaktivieren (für Stabilität)
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
```

**4. Thermisches Throttling**:
```bash
# Vor jedem Run abkühlen lassen
sleep 60

# Oder Benchmark mit niedrigerer Last
BENCHMARK_REGISTER_F(Fixture, MyBench)
    ->MinTime(5.0)  # Statt 10+ Sekunden
```

**5. Randomness/Non-Determinismus**:
```cpp
// FALSCH: Zufällige Seeds
std::random_device rd;
std::mt19937 gen(rd());  // Nicht-deterministisch!

// RICHTIG: Fester Seed
std::mt19937 gen(42);  // Reproduzierbar
```

**6. CHIMERA Scientific Protocol verwenden**:
```python
# Mit statistischer Validierung
python3 scientific_benchmark_runner.py \
  --repetitions 10 \
  --confidence-level 0.95 \
  --warmup-time 30 \
  --seed 42
```

---

## 📝 Test-Runbook für neue Benchmarks

### Neuen C++ Mikro-Benchmark hinzufügen

**Schritt 1: Benchmark-Datei erstellen**

```bash
cd benchmarks
touch bench_my_new_feature.cpp
```

**Schritt 2: Grundgerüst implementieren**

```cpp
// benchmarks/bench_my_new_feature.cpp
#include <benchmark/benchmark.h>
#include "my_feature/my_component.h"
#include "storage/rocksdb_wrapper.h"
#include <memory>
#include <filesystem>

using namespace themis;

// ============================================================================
// Test Fixture
// ============================================================================

class MyFeatureFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /*state*/) override {
        // Cleanup existing test database
        test_db_path_ = "./data/bench_my_feature_tmp";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        // Create RocksDB wrapper
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 128;
        config.block_cache_size_mb = 256;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open database");
        }
        
        // Initialize your component
        my_component_ = std::make_unique<MyComponent>(*db_);
    }
    
    void TearDown(const ::benchmark::State& /*state*/) override {
        // Cleanup
        my_component_.reset();
        db_.reset();
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

protected:
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<MyComponent> my_component_;
};

// ============================================================================
// Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(MyFeatureFixture, BasicOperation)(benchmark::State& state) {
    const size_t dataset_size = state.range(0);
    
    // Setup: Create test data (outside measurement)
    std::vector<TestData> test_data = generateTestData(dataset_size);
    
    // Benchmark loop
    for (auto _ : state) {
        state.PauseTiming();  // Pause for setup
        auto data = getNextTestItem(test_data);
        state.ResumeTiming();  // Resume for actual measurement
        
        // This is what we're measuring
        auto result = my_component_->performOperation(data);
        
        // Prevent compiler optimizations
        benchmark::DoNotOptimize(result);
    }
    
    // Report metrics
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(TestData));
    state.counters["dataset_size"] = dataset_size;
}

// Register benchmark with different parameters
BENCHMARK_REGISTER_F(MyFeatureFixture, BasicOperation)
    ->RangeMultiplier(10)
    ->Range(100, 100000)
    ->Unit(benchmark::kMicrosecond);

// Example: Benchmark with complexity analysis
BENCHMARK_DEFINE_F(MyFeatureFixture, ComplexityAnalysis)(benchmark::State& state) {
    const size_t n = state.range(0);
    
    for (auto _ : state) {
        auto result = my_component_->operationWithComplexity(n);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetComplexityN(n);
}

BENCHMARK_REGISTER_F(MyFeatureFixture, ComplexityAnalysis)
    ->RangeMultiplier(2)
    ->Range(1<<10, 1<<18)
    ->Complexity();  // Automatically compute complexity (O(n), O(log n), etc.)

// Example: Benchmark with custom counters
BENCHMARK_DEFINE_F(MyFeatureFixture, WithCustomCounters)(benchmark::State& state) {
    size_t total_bytes = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    
    for (auto _ : state) {
        auto [result, hit] = my_component_->operationWithCache();
        benchmark::DoNotOptimize(result);
        
        total_bytes += result.size();
        if (hit) ++cache_hits;
        else ++cache_misses;
    }
    
    state.counters["bytes"] = benchmark::Counter(total_bytes);
    state.counters["cache_hit_rate"] = benchmark::Counter(
        static_cast<double>(cache_hits) / (cache_hits + cache_misses),
        benchmark::Counter::kAvgThreads
    );
}

BENCHMARK_REGISTER_F(MyFeatureFixture, WithCustomCounters)
    ->Threads(1)->Threads(4)->Threads(8)->Threads(16)
    ->Unit(benchmark::kMillisecond);

// Example: Benchmark with multi-threading
BENCHMARK_DEFINE_F(MyFeatureFixture, MultiThreaded)(benchmark::State& state) {
    // state.threads() gibt die Anzahl der Threads zurück
    const int num_threads = state.threads();
    
    for (auto _ : state) {
        // Each thread executes this independently
        auto result = my_component_->threadSafeOperation();
        benchmark::DoNotOptimize(result);
    }
    
    // Metrics are automatically aggregated across threads
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(MyFeatureFixture, MultiThreaded)
    ->ThreadRange(1, 32)
    ->UseRealTime()  // Use wall-clock time instead of CPU time
    ->Unit(benchmark::kMillisecond);

// Example: Benchmark with manual timing
BENCHMARK_DEFINE_F(MyFeatureFixture, ManualTiming)(benchmark::State& state) {
    for (auto _ : state) {
        // Expensive setup that we don't want to measure
        auto large_dataset = prepareHugeDataset();
        
        // Manual timing for the actual operation
        auto start = std::chrono::high_resolution_clock::now();
        my_component_->processDataset(large_dataset);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(
            end - start
        );
        
        state.SetIterationTime(elapsed_seconds.count());
    }
}

BENCHMARK_REGISTER_F(MyFeatureFixture, ManualTiming)
    ->UseManualTime()
    ->Iterations(100);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
```

**Schritt 3: CMakeLists.txt aktualisieren**

```cmake
# benchmarks/CMakeLists.txt (am Ende hinzufügen)

add_executable(bench_my_new_feature bench_my_new_feature.cpp)
target_link_libraries(bench_my_new_feature
    PRIVATE
        themis_core
        benchmark::benchmark
        benchmark::benchmark_main
)
target_include_directories(bench_my_new_feature
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Optional: Install target
install(TARGETS bench_my_new_feature
    RUNTIME DESTINATION bin/benchmarks
)
```

**Schritt 4: Kompilieren und testen**

```bash
# Rebuild
cd build
cmake ..
make bench_my_new_feature -j$(nproc)

# Erste Ausführung (Quick Test)
./benchmarks/bench_my_new_feature --benchmark_filter=BasicOperation/100

# Vollständige Ausführung
./benchmarks/bench_my_new_feature

# Mit JSON Output
./benchmarks/bench_my_new_feature \
  --benchmark_format=json \
  --benchmark_out=results_my_feature.json

# Mit CSV Output
./benchmarks/bench_my_new_feature \
  --benchmark_format=csv \
  --benchmark_out=results_my_feature.csv
```

**Schritt 5: Dokumentation hinzufügen**

```markdown
# In benchmarks/README.md oder dieses Dokument

## bench_my_new_feature

**Beschreibung**: Testet die Performance von MyComponent.

**Szenarien**:
- `BasicOperation`: Grundlegende Operation mit verschiedenen Dataset-Größen
- `ComplexityAnalysis`: Automatische Komplexitätsanalyse (O-Notation)
- `WithCustomCounters`: Mit Cache Hit Rate Metriken
- `MultiThreaded`: Multi-Threading Skalierung
- `ManualTiming`: Manuelle Zeitmessung für komplexe Setups

**Erwartete Performance** (AMD Ryzen 9 7950X):
```
BasicOperation/1000:      < 10 us
BasicOperation/100000:    < 500 us
MultiThreaded/16:         > 100K ops/s
```

**Ausführung**:
```bash
./benchmarks/bench_my_new_feature
```
```

---

### Neuen Python Integration Benchmark hinzufügen

**Schritt 1: Python-Datei erstellen**

```bash
cd benchmarks
touch my_feature_integration_benchmark.py
chmod +x my_feature_integration_benchmark.py
```

**Schritt 2: Benchmark implementieren**

```python
#!/usr/bin/env python3
"""
My Feature Integration Benchmark
=================================

Tests end-to-end performance of MyFeature over HTTP/REST API.

Usage:
    python3 my_feature_integration_benchmark.py --base-url http://localhost:8080
    python3 my_feature_integration_benchmark.py --mode full --output results/
"""

import asyncio
import argparse
import httpx
import time
import json
import statistics
from typing import List, Dict, Any
from dataclasses import dataclass
from datetime import datetime


@dataclass
class BenchmarkResult:
    """Benchmark result container."""
    name: str
    total_operations: int
    total_time_seconds: float
    throughput_ops_per_sec: float
    latencies_ms: List[float]
    errors: int
    
    @property
    def avg_latency_ms(self) -> float:
        return statistics.mean(self.latencies_ms)
    
    @property
    def p50_latency_ms(self) -> float:
        return statistics.median(self.latencies_ms)
    
    @property
    def p95_latency_ms(self) -> float:
        return statistics.quantiles(self.latencies_ms, n=20)[18]  # 95th percentile
    
    @property
    def p99_latency_ms(self) -> float:
        return statistics.quantiles(self.latencies_ms, n=100)[98]  # 99th percentile


class MyFeatureBenchmark:
    """Benchmark suite for MyFeature."""
    
    def __init__(self, base_url: str = "http://localhost:8080"):
        self.base_url = base_url
        self.client = None
    
    async def __aenter__(self):
        """Async context manager entry."""
        self.client = httpx.AsyncClient(
            base_url=self.base_url,
            timeout=httpx.Timeout(30.0, connect=10.0)
        )
        return self
    
    async def __aexit__(self, exc_type, exc_val, exc_tb):
        """Async context manager exit."""
        await self.client.aclose()
    
    async def health_check(self) -> bool:
        """Verify server is reachable."""
        try:
            response = await self.client.get("/health")
            return response.status_code == 200
        except Exception as e:
            print(f"Health check failed: {e}")
            return False
    
    async def benchmark_basic_operation(
        self,
        num_operations: int = 10000
    ) -> BenchmarkResult:
        """Benchmark basic operation."""
        print(f"\n[INFO] Running BasicOperation benchmark ({num_operations} ops)...")
        
        latencies = []
        errors = 0
        
        start_time = time.perf_counter()
        
        for i in range(num_operations):
            op_start = time.perf_counter()
            
            try:
                response = await self.client.post(
                    "/api/v1/my-feature/operation",
                    json={"id": f"item_{i}", "value": i * 2}
                )
                response.raise_for_status()
                
            except Exception as e:
                errors += 1
                if errors <= 10:  # Only print first 10 errors
                    print(f"[ERROR] Operation {i} failed: {e}")
            
            op_end = time.perf_counter()
            latencies.append((op_end - op_start) * 1000)  # Convert to ms
        
        total_time = time.perf_counter() - start_time
        throughput = num_operations / total_time
        
        result = BenchmarkResult(
            name="BasicOperation",
            total_operations=num_operations,
            total_time_seconds=total_time,
            throughput_ops_per_sec=throughput,
            latencies_ms=latencies,
            errors=errors
        )
        
        self._print_result(result)
        return result
    
    async def benchmark_concurrent_operations(
        self,
        num_operations: int = 10000,
        concurrency: int = 16
    ) -> BenchmarkResult:
        """Benchmark concurrent operations."""
        print(f"\n[INFO] Running ConcurrentOperations benchmark "
              f"({num_operations} ops, {concurrency} concurrent)...")
        
        latencies = []
        errors = 0
        
        async def single_operation(op_id: int):
            nonlocal errors
            op_start = time.perf_counter()
            
            try:
                response = await self.client.post(
                    "/api/v1/my-feature/operation",
                    json={"id": f"item_{op_id}", "value": op_id * 2}
                )
                response.raise_for_status()
            except Exception as e:
                errors += 1
            
            op_end = time.perf_counter()
            return (op_end - op_start) * 1000
        
        start_time = time.perf_counter()
        
        # Execute operations in batches with concurrency limit
        for batch_start in range(0, num_operations, concurrency):
            batch_end = min(batch_start + concurrency, num_operations)
            tasks = [
                single_operation(i) 
                for i in range(batch_start, batch_end)
            ]
            batch_latencies = await asyncio.gather(*tasks)
            latencies.extend(batch_latencies)
        
        total_time = time.perf_counter() - start_time
        throughput = num_operations / total_time
        
        result = BenchmarkResult(
            name=f"ConcurrentOperations (concurrency={concurrency})",
            total_operations=num_operations,
            total_time_seconds=total_time,
            throughput_ops_per_sec=throughput,
            latencies_ms=latencies,
            errors=errors
        )
        
        self._print_result(result)
        return result
    
    def _print_result(self, result: BenchmarkResult):
        """Print benchmark result to console."""
        print(f"\n  Results:")
        print(f"    Total Operations: {result.total_operations:,}")
        print(f"    Total Time:       {result.total_time_seconds:.2f}s")
        print(f"    Throughput:       {result.throughput_ops_per_sec:,.0f} ops/s")
        print(f"    Avg Latency:      {result.avg_latency_ms:.2f} ms")
        print(f"    P50 Latency:      {result.p50_latency_ms:.2f} ms")
        print(f"    P95 Latency:      {result.p95_latency_ms:.2f} ms")
        print(f"    P99 Latency:      {result.p99_latency_ms:.2f} ms")
        print(f"    Errors:           {result.errors} ({result.errors/result.total_operations*100:.2f}%)")
    
    def save_results(self, results: List[BenchmarkResult], output_path: str):
        """Save results to JSON file."""
        output_data = {
            "timestamp": datetime.now().isoformat(),
            "base_url": self.base_url,
            "results": [
                {
                    "name": r.name,
                    "total_operations": r.total_operations,
                    "total_time_seconds": r.total_time_seconds,
                    "throughput_ops_per_sec": r.throughput_ops_per_sec,
                    "latency_ms": {
                        "avg": r.avg_latency_ms,
                        "p50": r.p50_latency_ms,
                        "p95": r.p95_latency_ms,
                        "p99": r.p99_latency_ms,
                        "max": max(r.latencies_ms)
                    },
                    "errors": r.errors
                }
                for r in results
            ]
        }
        
        with open(output_path, 'w') as f:
            json.dump(output_data, f, indent=2)
        
        print(f"\n[✓] Results saved to {output_path}")


async def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description="MyFeature Integration Benchmark")
    parser.add_argument("--base-url", default="http://localhost:8080",
                        help="Base URL of ThemisDB server")
    parser.add_argument("--operations", type=int, default=10000,
                        help="Number of operations per benchmark")
    parser.add_argument("--concurrency", type=int, default=16,
                        help="Concurrent clients")
    parser.add_argument("--output", default="results/my_feature_benchmark.json",
                        help="Output JSON file")
    
    args = parser.parse_args()
    
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║          MyFeature Integration Benchmark Suite                ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print(f"\n[INFO] Target: {args.base_url}")
    
    async with MyFeatureBenchmark(args.base_url) as benchmark:
        # Health check
        if not await benchmark.health_check():
            print("[ERROR] Server is not reachable. Exiting.")
            return 1
        
        print("[✓] Server is reachable")
        
        # Run benchmarks
        results = []
        
        results.append(await benchmark.benchmark_basic_operation(args.operations))
        results.append(await benchmark.benchmark_concurrent_operations(
            args.operations, args.concurrency
        ))
        
        # Save results
        benchmark.save_results(results, args.output)
    
    print("\n[✓] Benchmark completed successfully")
    return 0


if __name__ == "__main__":
    exit(asyncio.run(main()))
```

**Schritt 3: Dependencies prüfen**

```bash
# requirements.txt erstellen/aktualisieren
cat >> requirements.txt << EOF
httpx>=0.24.0
asyncio
EOF

# Installieren
pip3 install -r requirements.txt
```

**Schritt 4: Ausführen**

```bash
# Server starten
cd build
./src/themisdb &

# Benchmark ausführen
cd benchmarks
python3 my_feature_integration_benchmark.py \
  --operations 10000 \
  --concurrency 16 \
  --output results/my_feature_$(date +%Y%m%d_%H%M%S).json
```

---

## ✅ Best Practices

### DO: Empfohlene Vorgehensweisen

1. **✅ Release Builds verwenden**
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
   ```
   - Debug Builds sind 5-10x langsamer
   - Aktiviere alle Compiler-Optimierungen
   - Verwende `-march=native` für CPU-spezifische Optimierungen

2. **✅ Warmup-Phase einbauen**
   ```cpp
   BENCHMARK_REGISTER_F(Fixture, MyBench)
       ->MinTime(2.0)  // Min 2 Sekunden Warmup
       ->Repetitions(10);  // 10 Wiederholungen
   ```
   - Erste Iterationen sind oft langsamer (Cold Caches)
   - JIT Compilation braucht Zeit
   - RocksDB Compaction kann Performance beeinflussen

3. **✅ Statistisch signifikante Messungen**
   ```python
   # CHIMERA Scientific Protocol
   python3 scientific_benchmark_runner.py \
     --repetitions 10 \
     --confidence-level 0.95 \
     --warmup-time 30
   ```
   - Mindestens 10 Wiederholungen
   - 95% Confidence Intervals
   - Cohen's d Effect Size berechnen

4. **✅ System Performance Mode**
   ```bash
   sudo cpupower frequency-set -g performance
   echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo  # Turbo enable
   ```
   - CPU Governor auf "performance" setzen
   - Turbo Boost aktivieren
   - Hyper-Threading für Parallelität

5. **✅ Disk I/O isolieren**
   ```bash
   export THEMIS_DB_PATH=/tmp/benchmark  # tmpfs
   export THEMIS_DISABLE_WAL=true  # Nur für Benchmarks!
   export THEMIS_BLOCK_CACHE_SIZE_MB=8192  # Großer Cache
   ```
   - tmpfs verwenden für maximale Speed
   - WAL deaktivieren (nicht für Production!)
   - Block Cache maximieren

6. **✅ Benchmark-Code reviewen**
   ```cpp
   // WICHTIG: DoNotOptimize verwenden
   auto result = expensiveOperation();
   benchmark::DoNotOptimize(result);  // Verhindert Wegoptimierung
   benchmark::ClobberMemory();  // Verhindert Caching
   ```
   - Compiler kann leere Loops wegoptimieren
   - `DoNotOptimize()` erzwingt Ausführung
   - `ClobberMemory()` verhindert Register-Caching

7. **✅ Komplexitätsanalyse durchführen**
   ```cpp
   BENCHMARK_REGISTER_F(Fixture, Search)
       ->RangeMultiplier(10)
       ->Range(100, 1000000)
       ->Complexity();  // O(log n), O(n), O(n^2) etc.
   ```
   - Automatische Big-O Berechnung
   - Identifiziert Performance-Regression
   - Validiert Algorithmus-Komplexität

8. **✅ Realistische Workloads simulieren**
   ```cpp
   // Nicht nur sequentiell, sondern auch random access
   std::mt19937 gen(42);  // Fester Seed für Reproduzierbarkeit
   std::uniform_int_distribution<> dist(0, dataset_size - 1);
   auto random_id = dist(gen);
   ```
   - Zufällige Zugriffsmuster
   - Variable Datengrößen
   - Mixed Read/Write Workloads

9. **✅ JSON/CSV Output für Automation**
   ```bash
   ./benchmarks/bench_vector_search \
     --benchmark_format=json \
     --benchmark_out=results.json
   
   # CI/CD Integration
   python3 scripts/compare_benchmarks.py \
     --baseline baseline.json \
     --current results.json \
     --threshold 5  # 5% Regression = FAIL
   ```
   - Maschinenlesbare Formate
   - CI/CD Integration
   - Automatische Regression Detection

10. **✅ Multi-Threading Skalierung testen**
    ```cpp
    BENCHMARK_REGISTER_F(Fixture, Parallel)
        ->ThreadRange(1, 32)
        ->UseRealTime();  // Wall-clock statt CPU time
    ```
    - Testen mit 1, 2, 4, 8, 16, 32 Threads
    - Identifiziere Lock Contention
    - Verifiziere lineare Skalierung

11. **✅ Memory Profiling aktivieren**
    ```bash
    valgrind --tool=massif ./benchmarks/bench_crud
    ms_print massif.out.* > memory_profile.txt
    ```
    - Memory Leaks identifizieren
    - Heap Usage analysieren
    - Allocation Hotspots finden

12. **✅ Ergebnisse dokumentieren**
    ```markdown
    ## Benchmark Results (2026-02-02)
    
    **Hardware**: AMD Ryzen 9 7950X, 128GB DDR5, NVMe SSD
    **Build**: Release, GCC 13.2, -O3 -march=native
    **Results**:
    - Vector Search: 52,145 queries/s (k=10, 384D)
    - CRUD Insert: 43,256 ops/s (1KB blobs)
    - Transaction Throughput: 28,934 tx/s (Serializable)
    ```
    - Hardware-Spezifikationen
    - Build-Konfiguration
    - Reproduzierbare Kommandos

13. **✅ Baseline-Vergleiche durchführen**
    ```bash
    # Baseline erstellen
    ./benchmarks/bench_vector_search \
      --benchmark_out=baseline.json \
      --benchmark_format=json
    
    # Nach Änderungen vergleichen
    ./benchmarks/bench_vector_search \
      --benchmark_out=current.json \
      --benchmark_format=json
    
    python3 tools/compare.py baseline.json current.json
    ```
    - Vor/Nach Performance-Optimierungen
    - Regression Detection
    - A/B Testing

14. **✅ CI/CD Integration**
    ```yaml
    # .github/workflows/benchmarks.yml
    - name: Run Benchmarks
      run: |
        cd build
        ./benchmarks/bench_vector_search --benchmark_format=json > results.json
        python3 ../scripts/check_regression.py results.json
    ```
    - Automatische Benchmark-Runs
    - Performance Gates
    - Trend-Visualisierung

15. **✅ Docker für Reproduzierbarkeit**
    ```dockerfile
    FROM ubuntu:24.04
    RUN apt-get update && apt-get install -y \
        g++-13 cmake ninja-build
    COPY . /app
    WORKDIR /app/build
    RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .. && ninja
    CMD ["./benchmarks/bench_vector_search"]
    ```
    - Identische Build-Umgebung
    - Reproduzierbare Ergebnisse
    - Cross-Platform Testing

---

### DON'T: Zu vermeidende Fehler

1. **❌ Debug Builds benchmarken**
   ```bash
   # FALSCH
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   ./benchmarks/bench_vector_search  # 10x langsamer!
   
   # RICHTIG
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```
   - Debug Builds haben keine Optimierungen
   - Assertions sind aktiv (Performance-Killer)
   - Debug Symbole vergrößern Binary

2. **❌ Kein Warmup**
   ```cpp
   // FALSCH: Erste Iteration misst Cold Cache
   BENCHMARK_REGISTER_F(Fixture, MyBench);
   
   // RICHTIG: Warmup einbauen
   BENCHMARK_REGISTER_F(Fixture, MyBench)
       ->MinTime(2.0)  // 2s Warmup
       ->Repetitions(5);
   ```
   - Cold Caches verfälschen Ergebnisse
   - JIT braucht Warmup
   - Erste Runs sind immer langsamer

3. **❌ System unter Last**
   ```bash
   # FALSCH: Chrome, IDE, Downloads laufen parallel
   ./benchmarks/bench_vector_search
   
   # RICHTIG: System idle, nur Benchmark läuft
   # Chrome schließen, Load < 1.0
   uptime  # Load Average prüfen
   ```
   - Andere Prozesse stehlen CPU/Memory
   - Disk I/O Contention
   - Network Activity

4. **❌ Nicht-deterministische Daten**
   ```cpp
   // FALSCH: Random Seed bei jedem Run anders
   std::random_device rd;
   std::mt19937 gen(rd());
   
   // RICHTIG: Fester Seed
   std::mt19937 gen(42);  // Reproduzierbar
   ```
   - Ergebnisse nicht vergleichbar
   - Keine Reproduzierbarkeit
   - CI/CD schlägt fehl

5. **❌ Setup-Zeit mitmessen**
   ```cpp
   // FALSCH: Setup wird mitgemessen
   for (auto _ : state) {
       std::vector<int> data = generateLargeDataset();  // Langsam!
       doOperation(data);
   }
   
   // RICHTIG: Setup pausieren
   for (auto _ : state) {
       state.PauseTiming();
       auto data = generateLargeDataset();
       state.ResumeTiming();
       doOperation(data);
   }
   ```
   - Setup verfälscht Messungen
   - Nur die eigentliche Operation messen
   - `PauseTiming()` / `ResumeTiming()` verwenden

6. **❌ Zu kurze Benchmark-Dauer**
   ```cpp
   // FALSCH: < 100ms, zu viel Rauschen
   BENCHMARK_REGISTER_F(Fixture, MyBench);
   
   // RICHTIG: Mindestens 1-2 Sekunden
   BENCHMARK_REGISTER_F(Fixture, MyBench)
       ->MinTime(2.0);
   ```
   - Kurze Runs haben hohes Rauschen
   - Statistisch nicht signifikant
   - Timer-Overhead dominiert

7. **❌ Single-Thread-Tests für parallele Workloads**
   ```cpp
   // FALSCH: Nur 1 Thread testen
   BENCHMARK_REGISTER_F(Fixture, ParallelOp);
   
   // RICHTIG: Multi-Threading testen
   BENCHMARK_REGISTER_F(Fixture, ParallelOp)
       ->ThreadRange(1, 32);
   ```
   - Lock Contention wird nicht erkannt
   - Skalierung nicht validiert
   - Real-World Workload hat Parallelität

8. **❌ Disk I/O ignorieren**
   ```cpp
   // FALSCH: Disk I/O dominiert Messung
   export THEMIS_DB_PATH=/mnt/slow_hdd
   
   // RICHTIG: Fast Storage oder tmpfs
   export THEMIS_DB_PATH=/tmp/benchmark  # tmpfs
   export THEMIS_DISABLE_WAL=true  # Kein Disk I/O
   ```
   - HDD ist 100x langsamer als RAM
   - NVMe SSD ist 10x langsamer als RAM
   - tmpfs für CPU-Benchmarks

9. **❌ Memory Leaks**
   ```cpp
   // FALSCH: Memory wird nicht freigegeben
   for (auto _ : state) {
       auto* data = new LargeObject();  // Leak!
       processData(data);
   }
   
   // RICHTIG: RAII / Smart Pointers
   for (auto _ : state) {
       auto data = std::make_unique<LargeObject>();
       processData(*data);
   }  // Automatisches cleanup
   ```
   - Memory Leaks verfälschen Ergebnisse
   - System wird langsamer über Zeit
   - OOM Crashes

10. **❌ Compiler-Optimierungen wegoptimieren**
    ```cpp
    // FALSCH: Compiler optimiert weg
    for (auto _ : state) {
        expensiveOperation();  // Kein Seiteneffekt -> wegoptimiert!
    }
    
    // RICHTIG: DoNotOptimize
    for (auto _ : state) {
        auto result = expensiveOperation();
        benchmark::DoNotOptimize(result);
    }
    ```
    - Compiler ist zu smart
    - Dead Code Elimination
    - `DoNotOptimize()` erzwingt Ausführung

11. **❌ Keine Fehlerbehandlung**
    ```python
    # FALSCH: Errors werden ignoriert
    for i in range(10000):
        response = httpx.post(url, json=data)
        # Keine Status-Prüfung!
    
    # RICHTIG: Error Handling
    for i in range(10000):
        try:
            response = httpx.post(url, json=data)
            response.raise_for_status()
        except Exception as e:
            errors += 1
    ```
    - Failed Operations verfälschen Throughput
    - Silent Failures
    - Error Rate wichtig für Reliability

12. **❌ Thermisches Throttling ignorieren**
    ```bash
    # FALSCH: CPU überhitzt, throttled
    for i in {1..100}; do
        ./benchmarks/bench_heavy_workload
    done
    
    # RICHTIG: Pausen einbauen
    for i in {1..100}; do
        ./benchmarks/bench_heavy_workload
        sleep 30  # Abkühlen lassen
    done
    ```
    - CPU drosselt bei >90°C
    - Performance sinkt um 20-50%
    - `sensors` / `dmesg` prüfen

13. **❌ Netzwerk-Variabilität ignorieren (Multi-Shard)**
    ```python
    # FALSCH: Keine Retry-Logik
    response = httpx.get(shard_url)
    
    # RICHTIG: Retry + Timeout
    for retry in range(3):
        try:
            response = httpx.get(shard_url, timeout=10.0)
            response.raise_for_status()
            break
        except:
            if retry == 2:
                raise
            time.sleep(1)
    ```
    - Netzwerk hat Packet Loss
    - Timeouts müssen konfiguriert sein
    - Retry-Logik für Reliability

14. **❌ Production-Daten in Benchmarks**
    ```bash
    # FALSCH: Production DB verwenden
    export THEMIS_DB_PATH=/var/lib/themisdb/production
    
    # RICHTIG: Isolierte Test-Datenbank
    export THEMIS_DB_PATH=/tmp/benchmark_test
    ```
    - Risiko: Daten löschen
    - Performance durch bestehende Daten beeinflusst
    - Compliance-Probleme (GDPR)

15. **❌ Veraltete Baselines**
    ```bash
    # FALSCH: Baseline von vor 6 Monaten
    python3 compare.py old_baseline.json current.json
    
    # RICHTIG: Aktuelle Baseline
    # Baseline nach jedem Major Release neu erstellen
    ```
    - Hardware kann sich ändern
    - Software-Upgrades beeinflussen Performance
    - Compiler-Versionen ändern sich

---

## 📚 Referenzen

### Interne Dokumentation

**Benchmark-Dokumentation**:
- [`benchmarks/README.md`](../benchmarks/README.md) - Übersicht aller Benchmarks
- [`benchmarks/QUICKSTART.md`](../benchmarks/QUICKSTART.md) - Schnelleinstieg
- [`docs/BENCHMARK_BEST_PRACTICES.md`](./BENCHMARK_BEST_PRACTICES.md) - Best Practices (diese Datei)
- [`benchmarks/CHIMERA_SUITE_README.md`](../benchmarks/CHIMERA_SUITE_README.md) - CHIMERA Framework

**Performance-Dokumentation**:
- [`docs/PERFORMANCE_BENCHMARKS.md`](./PERFORMANCE_BENCHMARKS.md) - Detaillierte Performance-Analyse
- [`benchmarks/BENCHMARK_ANALYSIS.md`](../benchmarks/BENCHMARK_ANALYSIS.md) - Analyse-Tools
- [`benchmarks/BENCHMARK_VISUALIZATION.md`](../benchmarks/BENCHMARK_VISUALIZATION.md) - Visualisierung

**Hardware-Spezifisch**:
- [`benchmarks/HARDWARE_CONSTRAINTS_README.md`](../benchmarks/HARDWARE_CONSTRAINTS_README.md) - Hardware-Limits
- [`benchmarks/HARDWARE_CONFIGURATION_BENCHMARK_SUITE.md`](../benchmarks/HARDWARE_CONFIGURATION_BENCHMARK_SUITE.md) - Hardware-Konfigurationen

**Docker & RAID**:
- [`benchmarks/DOCKER_QUICKSTART.md`](../benchmarks/DOCKER_QUICKSTART.md) - Docker Benchmarks
- [`benchmarks/RAID_SHARDING_QUICKSTART.md`](../benchmarks/RAID_SHARDING_QUICKSTART.md) - RAID/Sharding Tests
- [`benchmarks/MULTI_SHARD_RAID_QUICKSTART.md`](../benchmarks/MULTI_SHARD_RAID_QUICKSTART.md) - Multi-Shard Setup

**LLM/LoRA**:
- [`benchmarks/LLM_NLP_INTEGRATION_FRAMEWORK.md`](../benchmarks/LLM_NLP_INTEGRATION_FRAMEWORK.md) - LLM Integration
- [`benchmarks/BENCH_LORA_AUTO_BINDING_README.md`](../benchmarks/BENCH_LORA_AUTO_BINDING_README.md) - LoRA Benchmarks

**Wissenschaftliche Standards**:
- [`benchmarks/SCIENTIFIC_STANDARDS_README.md`](../benchmarks/SCIENTIFIC_STANDARDS_README.md) - IEEE/ACM Standards
- [`benchmarks/SCIENTIFIC_PROTOCOL_IMPLEMENTATION.md`](../benchmarks/SCIENTIFIC_PROTOCOL_IMPLEMENTATION.md) - Scientific Protocol

---

### Externe Ressourcen

**Google Benchmark**:
- [Google Benchmark GitHub](https://github.com/google/benchmark)
- [User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
- [Perf Counters](https://github.com/google/benchmark/blob/main/docs/perf_counters.md)
- [Random Interleaving](https://github.com/google/benchmark/blob/main/docs/random_interleaving.md)

**Benchmark Standards**:
- [TPC-C Specification](http://www.tpc.org/tpcc/) - Transaction Processing Performance Council
- [TPC-H Specification](http://www.tpc.org/tpch/) - Decision Support Benchmark
- [YCSB Workloads](https://github.com/brianfrankcooper/YCSB/wiki/Core-Workloads) - Yahoo! Cloud Serving Benchmark
- [LDBC SNB](https://ldbcouncil.org/benchmarks/snb/) - Linked Data Benchmark Council

**Vector Search**:
- [ANN Benchmarks](http://ann-benchmarks.com/) - Approximate Nearest Neighbor Benchmarks
- [HNSW Paper](https://arxiv.org/abs/1603.09320) - Hierarchical Navigable Small World Graphs
- [FAISS Documentation](https://github.com/facebookresearch/faiss/wiki)

**Performance Tools**:
- [perf](https://perf.wiki.kernel.org/index.php/Main_Page) - Linux Performance Analysis
- [Valgrind](https://valgrind.org/) - Memory Profiling
- [Flamegraphs](https://www.brendangregg.com/flamegraphs.html) - Visualization
- [Intel VTune](https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html) - Advanced Profiling

**Statistical Analysis**:
- [Cohen's d](https://en.wikipedia.org/wiki/Effect_size#Cohen's_d) - Effect Size
- [Mann-Whitney U Test](https://en.wikipedia.org/wiki/Mann%E2%80%93Whitney_U_test) - Non-parametric Test
- [Confidence Intervals](https://en.wikipedia.org/wiki/Confidence_interval)

**RocksDB**:
- [RocksDB Tuning Guide](https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide)
- [RocksDB Benchmarking](https://github.com/facebook/rocksdb/wiki/Performance-Benchmarks)

**Docker Performance**:
- [Docker Performance Best Practices](https://docs.docker.com/config/containers/resource_constraints/)
- [Container Networking](https://docs.docker.com/network/)

**CPU Performance**:
- [CPU Frequency Scaling](https://www.kernel.org/doc/html/latest/admin-guide/pm/cpufreq.html)
- [Intel Turbo Boost](https://www.intel.com/content/www/us/en/gaming/resources/turbo-boost.html)
- [AMD Precision Boost](https://www.amd.com/en/technologies/precision-boost)

---

### Wissenschaftliche Publikationen

**Database Benchmarking**:
- Gray, J. (Ed.). (1993). *The Benchmark Handbook for Database and Transaction Systems*. Morgan Kaufmann.
- Harizopoulos, S., et al. (2008). "OLTP Through the Looking Glass, and What We Found There." SIGMOD.
- Cooper, B. F., et al. (2010). "Benchmarking Cloud Serving Systems with YCSB." SoCC.

**Vector Search**:
- Malkov, Y., & Yashunin, D. (2018). "Efficient and Robust Approximate Nearest Neighbor Search Using Hierarchical Navigable Small World Graphs." IEEE TPAMI.
- Johnson, J., et al. (2019). "Billion-scale similarity search with GPUs." IEEE Big Data.

**Multi-Model Databases**:
- Lu, J., & Holubová, I. (2019). "Multi-model Databases: A New Journey to Handle the Variety of Data." ACM Computing Surveys.

**Performance Analysis**:
- Mytkowicz, T., et al. (2009). "Producing Wrong Data Without Doing Anything Obviously Wrong!" ASPLOS.
- Georges, A., et al. (2007). "Statistically Rigorous Java Performance Evaluation." OOPSLA.

---

### Community & Support

**ThemisDB Community**:
- [GitHub Repository](https://github.com/themisdb/themisdb)
- [Discord Community](https://discord.gg/themisdb)
- [Stack Overflow Tag: themisdb](https://stackoverflow.com/questions/tagged/themisdb)

**Benchmark-spezifisch**:
- [Google Benchmark Discussions](https://github.com/google/benchmark/discussions)
- [YCSB User Group](https://groups.google.com/g/ycsb-users)

**Performance Engineering**:
- [Brendan Gregg's Blog](https://www.brendangregg.com/blog/) - Linux Performance
- [Mechanical Sympathy](https://mechanical-sympathy.blogspot.com/) - Hardware-aware Programming

---

### Tools & Scripts

**In diesem Repository**:
```
benchmarks/
├── analyze_benchmarks.py          # Benchmark-Analyse
├── generate_benchmark_report.py   # Report-Generierung
├── compare.py                      # Baseline-Vergleiche
├── scripts/
│   ├── check_regression.py        # Regression Detection
│   ├── plot_results.py            # Visualisierung
│   └── hardware_info.py           # Hardware-Info sammeln
tools/
├── benchmark_runner.sh            # Automatisierte Ausführung
└── perf_analysis.sh               # Performance-Analyse
```

**Externe Tools**:
- [Hyperfine](https://github.com/sharkdp/hyperfine) - Command-line Benchmarking
- [wrk](https://github.com/wg/wrk) - HTTP Benchmarking
- [sysbench](https://github.com/akopytov/sysbench) - System Performance
- [fio](https://fio.readthedocs.io/) - I/O Benchmarking

---

## 🤖 LLM/LoRA Model Setup

> **Relevant für:** `bench_llm_*.cpp`, `bench_lora_framework.cpp`, `bench_llm_raid_pipeline.cpp`  
> **Maßnahme #6** (PERFORMANCE_EXPECTATIONS.md §1.4) – Artifact-Vorbereitung standardisieren

### Übersicht

LLM- und LoRA-Benchmarks benötigen Modell-Artefakte (gguf-Dateien, LoRA-Adapter) im Dateisystem.
Diese Sektion beschreibt, wie die Artefakte für CI-Runner und lokale Benchmark-Läufe bereitgestellt werden.

---

### Konfigurationsdatei

`benchmarks/llm_bench_config.json` enthält alle Modellpfade, Adapter-Definitionen und Benchmark-Profile.

Wichtige Felder:

| Feld | Beschreibung |
|------|-------------|
| `model_dir_env` | Umgebungsvariable, die das Basis-Modellverzeichnis angibt (`THEMIS_MODEL_DIR`) |
| `stub_models_cmake_var` | CMake-Variable `THEMIS_LLM_STUB_MODELS` (ON = CI-Stub-Modus) |
| `models.*.is_stub_model` | `true` = Minimal-Stub für CI; `false` = echtes Produktionsmodell |
| `benchmark_profiles.ci_stub` | Profil für CI-Runner ohne GPU (TinyLlama Q4, kein VRAM nötig) |
| `benchmark_profiles.gpu_full` | Profil für vollständige GPU-Benchmarks (Llama-3 8B, ≥8 GB VRAM) |

---

### Artefakt-Vorbereitung

#### Automatisch (empfohlen)

```bash
# CI-Modus: Stub-Modell verwenden (kein echtes Modell erforderlich)
./scripts/prepare_llm_bench_artifacts.sh --stub-only

# Lokaler Lauf: echte Modelle aus THEMIS_MODEL_DIR
export THEMIS_MODEL_DIR=/data/models/themis
./scripts/prepare_llm_bench_artifacts.sh

# Modellverzeichnis explizit angeben
./scripts/prepare_llm_bench_artifacts.sh --model-dir /opt/models/themis
```

Das Skript:
1. Liest `benchmarks/llm_bench_config.json`
2. Erstellt die benötigte Verzeichnisstruktur unter `THEMIS_MODEL_DIR`
3. Lädt den TinyLlama-Stub-Modell falls nötig (wget/curl)
4. Generiert ein minimales LoRA-Stub-Adapter-Binary für CI
5. Gibt einen Fehler (Exit 1) aus, wenn kein Modell verfügbar und kein Stub konfiguriert ist

#### Manuell

```bash
# Stub-Modell manuell platzieren:
mkdir -p "${THEMIS_MODEL_DIR}/gguf"
cp tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf "${THEMIS_MODEL_DIR}/gguf/"

# Echtes Llama-3 8B Modell:
mkdir -p "${THEMIS_MODEL_DIR}/gguf"
cp Meta-Llama-3-8B-Instruct.Q4_K_M.gguf "${THEMIS_MODEL_DIR}/gguf/"
```

---

### Umgebungsvariablen

| Variable | Default | Beschreibung |
|----------|---------|--------------|
| `THEMIS_MODEL_DIR` | `~/.local/share/themis/models` | Basis-Verzeichnis für Modell-Artefakte |
| `THEMIS_LLM_STUB_MODELS` | `OFF` | `ON` = Stub-Modus (CI); `OFF` = echte Modelle erwarten |

---

### CMake-Integration

```cmake
# CMakeLists.txt – LLM-Stub-Modus für CI aktivieren
option(THEMIS_LLM_STUB_MODELS "Use minimal stub models for LLM benchmarks (CI mode)" ON)

if(THEMIS_LLM_STUB_MODELS)
    target_compile_definitions(bench_llm_inference_performance PRIVATE THEMIS_LLM_STUB_MODELS=1)
endif()
```

---

### Troubleshooting

| Problem | Lösung |
|---------|--------|
| `artifact not found` Fehler im CI-Log | `./scripts/prepare_llm_bench_artifacts.sh --stub-only` ausführen |
| `THEMIS_MODEL_DIR` nicht gesetzt | `export THEMIS_MODEL_DIR=/pfad/zu/modellen` |
| Download schlägt fehl (kein Netzwerk) | Modell manuell in `$THEMIS_MODEL_DIR/gguf/` platzieren |
| Unzureichend VRAM | `--stub-only` Modus nutzen oder `gpu_full`-Profil mit GPU ≥8 GB VRAM |

---

### Changelog & Version History

**Version 2.0.0** (2026-02-02):
- ✅ Vollständige Dokumentation aller 80+ Benchmarks
- ✅ CHIMERA Suite Integration
- ✅ LLM/LoRA Benchmark-Szenarien
- ✅ Multi-Shard RAID Tests
- ✅ Wissenschaftliche Standards (IEEE/ACM)
- ✅ Troubleshooting Guide mit 10+ Problemen
- ✅ Test-Runbook für neue Benchmarks
- ✅ Best Practices (15 DO, 15 DON'T)
- ✅ Umfassende Referenzen

**Version 1.0.0** (2025-12-01):
- Initiale Version
- Grundlegende Benchmark-Übersicht
- C++ Mikro-Benchmarks
- Python Integration Tests

---

## 📞 Kontakt & Support

**Bei Fragen oder Problemen**:
1. Suche in diesem Dokument (Ctrl+F)
2. Prüfe `benchmarks/README.md`
3. Schaue in GitHub Issues
4. Frage in Discord Community
5. Erstelle GitHub Issue mit:
   - Hardware-Specs
   - Build-Konfiguration
   - Vollständiger Fehleroutput
   - Schritte zur Reproduktion

**Maintainer**: ThemisDB Benchmark Team  
**Letzte Aktualisierung**: 2026-02-02  
**Dokument-Version**: 2.0.0

---

**🎉 Ende der Benchmark-Dokumentation**

*Dieses Dokument wird kontinuierlich aktualisiert. Pull Requests willkommen!*

