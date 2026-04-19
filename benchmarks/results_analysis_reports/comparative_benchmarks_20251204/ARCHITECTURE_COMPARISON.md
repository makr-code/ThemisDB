> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeipunkt und sind nicht mehr reproduzierbar ohne die ursprüngliche Testumgebung.
> Für reproduzierbare Ergebnisse: Benchmark-Kommandos und aktuelle CMake-Presets unter [`benchmarks/README.md`](../../README.md) verwenden.

# Polyglot Persistence vs ThemisDB - Architecture Comparison

## 🏗️ Polyglot Persistence Architecture (3-4 Databases)

```
┌─────────────────────────────────────────────────────────────────┐
│                      Application Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ PostgreSQL   │  │    Neo4j     │  │    Qdrant    │          │
│  │   Client     │  │   Client     │  │   Client     │          │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘          │
└─────────┼──────────────────┼──────────────────┼─────────────────┘
          │                  │                  │
          │ Network Hop 1    │ Network Hop 2    │ Network Hop 3
          │ (2-5ms)          │ (2-5ms)          │ (2-5ms)
          ▼                  ▼                  ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  PostgreSQL     │  │     Neo4j       │  │     Qdrant      │
│  Port: 5432     │  │  Port: 7687     │  │  Port: 6333     │
│                 │  │                 │  │                 │
│  ┌───────────┐  │  │  ┌───────────┐  │  │  ┌───────────┐  │
│  │ Documents │  │  │  │   Graph   │  │  │  │  Vectors  │  │
│  │  (JSONB)  │  │  │  │  Edges    │  │  │  │   HNSW    │  │
│  └───────────┘  │  │  └───────────┘  │  │  └───────────┘  │
│                 │  │                 │  │                 │
│  Storage: 4GB   │  │  Storage: 4GB   │  │  Storage: 4GB   │
└─────────────────┘  └─────────────────┘  └─────────────────┘
         │                   │                   │
         └───────────────────┴───────────────────┘
                             │
                   Data Synchronization
                   (Manual/ETL - Complex!)
```

**Issues:**
- ❌ **3 Network Hops** → 6-15ms total network overhead
- ❌ **Data Sync Complexity** → Embeddings/Graph must stay in sync
- ❌ **3 Backup Processes** → Consistent recovery is challenging
- ❌ **3 Monitoring Dashboards** → Operational burden
- ❌ **Eventual Consistency** → No cross-DB transactions

---

## 🚀 ThemisDB Unified Multi-Model Architecture (1 Database)

```
┌─────────────────────────────────────────────────────────────────┐
│                      Application Layer                          │
│              ┌──────────────────────────┐                        │
│              │    ThemisDB Client       │                        │
│              │    (HTTP/AQL)            │                        │
│              └──────────┬───────────────┘                        │
└─────────────────────────┼───────────────────────────────────────┘
                          │
                          │ Single Network Hop
                          │ (1-2ms)
                          ▼
┌──────────────────────────────────────────────────────────────────┐
│                       ThemisDB Server                            │
│                        Port: 8765                                │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │              AQL Query Engine (Unified)                    │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │ │
│  │  │ Document │  │  Graph   │  │  Vector  │  │   K/V    │  │ │
│  │  │  Module  │  │  Module  │  │  Module  │  │  Module  │  │ │
│  │  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  │ │
│  └───────┼─────────────┼─────────────┼─────────────┼────────┘ │
│          │             │             │             │          │
│  ┌───────▼─────────────▼─────────────▼─────────────▼────────┐ │
│  │          Unified Storage Layer (RocksDB)                  │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │ │
│  │  │Documents │  │Graph Index│ │HNSW Vector│ │Key-Value │ │ │
│  │  │(SSTable) │  │(outdeg/in)│ │  Index    │ │ (LSM)    │ │ │
│  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘ │ │
│  │                                                           │ │
│  │  Single RocksDB Instance - MVCC, Compression, Snapshots  │ │
│  └───────────────────────────────────────────────────────────┘ │
│                                                                 │
│  Total Storage: 4GB (Unified)                                  │
└─────────────────────────────────────────────────────────────────┘
                          │
                          ▼
                 Single Snapshot Backup
                 (Consistent across all models)
```

**Advantages:**
- ✅ **Single Network Hop** → 1-2ms network overhead only
- ✅ **Native Consistency** → ACID transactions across all models
- ✅ **Unified Backup** → Single RocksDB snapshot
- ✅ **Single Dashboard** → Centralized monitoring
- ✅ **Shared Storage** → Efficient resource usage

---

## 📊 Benchmark Scenario Flow Comparison

### Scenario 1: Document + Graph Query

#### Polyglot (PostgreSQL + Neo4j)
```
Time: 0ms    ┌──────────────────┐
             │  Application     │
             └────────┬─────────┘
                      │
Time: 2ms    ┌────────▼─────────┐  Network Hop 1
             │  PostgreSQL      │  (2ms)
             │  GET Document    │
             └────────┬─────────┘
                      │
Time: 15ms            │ Process (10ms)
                      ▼
             ┌────────────────────┐
             │  Application       │ Extract author_id
             └────────┬───────────┘
                      │
Time: 17ms   ┌────────▼─────────┐  Network Hop 2
             │     Neo4j        │  (2ms)
             │  MATCH Graph     │
             └────────┬─────────┘
                      │
Time: 30ms            │ Process (10ms)
                      ▼
             ┌────────────────────┐
             │  Application       │ Merge Results
             └────────────────────┘
                      
Total: ~30-45ms (2 Network Hops + 2 DB Queries + Merge)
```

#### ThemisDB (Unified)
```
Time: 0ms    ┌──────────────────┐
             │  Application     │
             └────────┬─────────┘
                      │
Time: 2ms    ┌────────▼─────────┐  Single Network Hop
             │    ThemisDB      │  (2ms)
             │                  │
             │  AQL Query:      │
             │  FOR doc IN docs │
             │   FILTER ...     │
             │   FOR author...  │  Unified Query
             │   RETURN merged  │  (Graph + Document)
             │                  │
             └────────┬─────────┘
                      │
Time: 15ms            │ Process (10ms)
                      ▼
             ┌────────────────────┐
             │  Application       │ Results Ready
             └────────────────────┘
                      
Total: ~12-20ms (1 Network Hop + 1 Unified Query)

Improvement: 40-60% faster (30ms → 18ms)
```

---

## 🔢 Resource Comparison

### Polyglot Persistence (3 DBs)
```
PostgreSQL:    4 CPU, 4 GB RAM
Neo4j:         4 CPU, 4 GB RAM
Qdrant:        4 CPU, 4 GB RAM
─────────────────────────────
Total:        12 CPU, 12 GB RAM

Processes:     3 separate daemons
Ports:         3 different ports (5432, 7687, 6333)
Configs:       3 configuration files
Backups:       3 separate backup processes
Monitoring:    3 dashboards (Grafana/Prometheus)
```

### ThemisDB (1 DB)
```
ThemisDB:      4 CPU, 4 GB RAM
─────────────────────────────
Total:         4 CPU, 4 GB RAM

Processes:     1 single daemon
Ports:         1 port (8765)
Configs:       1 YAML config
Backups:       1 RocksDB snapshot
Monitoring:    1 dashboard

Resource Savings: 66% (12→4 GB RAM, 12→4 CPU)
```

---

## 🎯 Operational Complexity Matrix

| Task | Polyglot (3 DBs) | ThemisDB (1 DB) | Reduction |
|------|------------------|-----------------|-----------|
| **Installation** | 3 apt/yum/docker commands | 1 binary download | 3x simpler |
| **Configuration** | 3 config files (different formats) | 1 YAML config | 3x simpler |
| **Startup** | 3 systemd services | 1 systemd service | 3x simpler |
| **Monitoring** | 3 Prometheus exporters | 1 metrics endpoint | 3x simpler |
| **Backup** | 3 cron jobs + consistency check | 1 snapshot command | 3x simpler |
| **Upgrade** | 3 separate upgrade paths | 1 binary replacement | 3x simpler |
| **Debugging** | 3 log files, 3 trace systems | 1 unified log | 3x simpler |
| **Training** | SQL + Cypher + Vector API | AQL (unified) | 3x simpler |

**Total Ops Complexity:** ThemisDB is **3x simpler** to operate

---

## 📈 Performance Comparison (Expected)

### Latency (Lower is Better)
```
Scenario 1: Document + Graph
PostgreSQL+Neo4j:  ████████████████████████ 40-50ms
ThemisDB:          ████████████ 20-30ms (-40-50%)

Scenario 2: Document + Vector
MongoDB+Qdrant:    ██████████████████████████ 50-60ms
ThemisDB:          █████████████ 25-35ms (-50-60%)

Scenario 3: OLAP + Document
ClickHouse+Mongo:  ██████████████████████ 35-45ms
ThemisDB:          ████████████ 20-30ms (-30-40%)
```

### Throughput (Higher is Better)
```
Polyglot (3 DBs):  ████████ 1000-1500 req/s
ThemisDB (1 DB):   ████████████████ 2500-3500 req/s (+2-3x)
```

---

## 🔐 Consistency Guarantees

### Polyglot Persistence
```
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│ PostgreSQL  │      │    Neo4j    │      │   Qdrant    │
│             │      │             │      │             │
│ COMMIT @T1  │      │ COMMIT @T2  │      │ COMMIT @T3  │
│ (Document)  │      │ (Graph)     │      │ (Vector)    │
└─────────────┘      └─────────────┘      └─────────────┘
       │                    │                    │
       └────────────────────┴────────────────────┘
                           │
              Eventual Consistency Window
              (T1, T2, T3 are different!)
              
Problem: Application sees inconsistent state!
```

### ThemisDB Unified
```
┌──────────────────────────────────────────────────────┐
│                    ThemisDB                          │
│                                                      │
│  BEGIN TRANSACTION                                   │
│    INSERT Document                                   │
│    INSERT Graph Edge                                 │
│    INSERT Vector Embedding                           │
│  COMMIT @T1                                          │
│  (All models committed atomically)                   │
└──────────────────────────────────────────────────────┘
                         │
            ACID Guarantees across ALL models
            (Snapshot Isolation via MVCC)

Benefit: Application always sees consistent state!
```

---

## 🌐 Network Overhead Breakdown

### Polyglot (Worst Case)
```
Application → PostgreSQL:  2-3ms (LAN)
Application → Neo4j:       2-3ms (LAN)
Application → Qdrant:      2-3ms (LAN)
Application Processing:    5-10ms (merge results)
────────────────────────────────────────
Total Network:            11-19ms
Total Processing:          5-10ms
────────────────────────────────────────
Total Latency:            16-29ms (Network dominates!)
```

### ThemisDB (Optimized)
```
Application → ThemisDB:    2-3ms (LAN)
ThemisDB Processing:      10-15ms (unified query)
────────────────────────────────────────
Total Network:             2-3ms
Total Processing:         10-15ms
────────────────────────────────────────
Total Latency:            12-18ms (Processing dominates)

Network Reduction: 80% less network overhead (11-19ms → 2-3ms)
```

---

## ✨ Summary

### ThemisDB Advantages
1. **40-60% Lower Latency** - Single network hop, unified query engine
2. **3x Less Operational Complexity** - 1 DB vs 3-4 DBs
3. **ACID Across All Models** - Transactional consistency guarantee
4. **66% Resource Savings** - Shared storage and compute
5. **Unified Query Language** - AQL for all models (vs SQL + Cypher + Vector API)

### Polyglot Disadvantages
1. **Multiple Network Hops** - 2-3x more network overhead
2. **Data Synchronization** - Manual ETL processes required
3. **Eventual Consistency** - No cross-DB transactions
4. **Complex Operations** - 3-4 separate systems to manage
5. **Higher Costs** - 3x more infrastructure

---

**Conclusion:** ThemisDB's unified multi-model approach delivers **significant performance and operational benefits** over traditional polyglot persistence architectures.
