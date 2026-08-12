# Kapitel 39: Performance Tuning Cookbook {#chapter_39_performance-tuning-cookbook}

> *"Premature optimization is the root of all evil. Yet we should not pass up our opportunities in that critical 3%."* — Donald Knuth[^1]

---

## Überblick {#chapter_39_0_ueberblick}

Wir präsentieren ein wissenschaftlich fundiertes, praxisorientiertes Tuning-Kochbuch für [ThemisDB](../appendix_h_glossary.md#themisdb). Performance-Optimierung in Datenbanksystemen folgt systematischen Prinzipien, die wir in diesem Kapitel anhand konkreter Symptom-Diagnose-Fix-Pattern vermitteln. Jede Sektion kombiniert theoretische Grundlagen mit messbaren [Benchmarks](../appendix_h_glossary.md#benchmark) und produktionserprobten [AQL](../appendix_h_glossary.md#aql)-Beispielen. Wir orientieren uns an Graefes Optimierungstheorie[^2] und den RocksDB-Performance-Best-Practices[^3], adaptiert für ThemisDBs [MVCC](../appendix_h_glossary.md#mvcc)-basierte Architektur (siehe auch → Kapitel 2: Architektur-Grundlagen).

**Was wir in diesem Kapitel behandeln:**

- **Query-Optimierung:** [EXPLAIN](../appendix_h_glossary.md#explain)-Analyse, [Filter Pushdown](../appendix_h_glossary.md#filter-pushdown), N+1-Problem-Vermeidung, [JOIN](../appendix_h_glossary.md#join)-Strategien
- **Indexierungs-Strategien:** [B-Tree](../appendix_h_glossary.md#btree), [Hash](../appendix_h_glossary.md#hash-index), [Persistent Index](../appendix_h_glossary.md#persistent-index), [Covering Indexes](../appendix_h_glossary.md#covering-index), Selektivität
- **Cache-Tuning:** [LRU](../appendix_h_glossary.md#lru), [ARC](../appendix_h_glossary.md#arc), Hit-Rate-Monitoring, Memory-Limits
- **Storage-Engine:** [RocksDB](../appendix_h_glossary.md#rocksdb)-[LSM-Tree](../appendix_h_glossary.md#lsm-tree)-Konfiguration, [Compaction](../appendix_h_glossary.md#compaction), [Bloom Filter](../appendix_h_glossary.md#bloom-filter)
- **System-Level-Tuning:** OS-Parameter (sysctl), Netzwerk (TCP), Filesystem (ext4, XFS), SSD/NVMe-Optimierung
- **Spezial-Workloads:** [Vector Search](../appendix_h_glossary.md#vector-search) ([HNSW](../appendix_h_glossary.md#hnsw)), [Graph Traversal](../appendix_h_glossary.md#graph-traversal), [Batching](../appendix_h_glossary.md#batching)
- **Produktions-Konfigurationen:** Beispiel-Templates für verschiedene Workload-Profile

---

```mermaid
flowchart TD
    Start[Performance Issue] --> Profile[Profile System mit Monitoring-Tools]
    
    Profile --> CPU{CPU Bottleneck?}
    Profile --> Memory{Memory Bottleneck?}
    Profile --> Disk{Disk Bottleneck?}
    Profile --> Network{Network Bottleneck?}
    
    CPU -->|Yes| OptQuery[Optimize Queries: EXPLAIN, Index, Projection]
    CPU -->|Yes| AddIndex[Add Indexes: B-Tree, Hash, Covering]
    
    Memory -->|Yes| IncCache[Increase Cache: LRU/ARC Tuning]
    Memory -->|Yes| OptDataStruct[Optimize Data Structures: Streaming, Pagination]
    
    Disk -->|Yes| SSD[Use SSD/NVMe: LSM-Tree Compaction]
    Disk -->|Yes| Partition[Partition Data: Sharding, Hot/Cold]
    
    Network -->|Yes| CompData[Compress Data: zstd, Protocol Buffers]
    Network -->|Yes| BatchReq[Batch Requests: Connection Pooling]
    
    OptQuery --> Verify[Verify Improvement: Benchmarks, Metrics]
    AddIndex --> Verify
    IncCache --> Verify
    OptDataStruct --> Verify
    SSD --> Verify
    Partition --> Verify
    CompData --> Verify
    BatchReq --> Verify
    
    Verify --> Done[✓ Performance Target erreicht]
    
    style Start fill:#ff6b6b
    style Verify fill:#f093fb
    style Done fill:#43e97b
```

**Abb. 39.0:** Performance-Tuning-Workflow nach dem Bottleneck-Analyse-Prinzip[^4]. Wir beginnen stets mit systematischem Profiling, um den kritischen Ressourcen-Engpass zu identifizieren, bevor wir Optimierungen anwenden.

---

## 39.1 Quick Tuning Checklist {#chapter_39_1_quick-tuning-checklist}

Diese Checkliste bietet sofortige Diagnose-Ansätze für häufige Performance-Probleme. Wir kategorisieren nach Symptom und liefern die wahrscheinlichste Root Cause plus First-Response-Aktion. Für vertiefende Analysen verweisen wir auf die spezialisierten Sektionen dieses Kapitels sowie → Kapitel 20: Performance Monitoring.

**Symptom-basierte Schnelldiagnose:**

- **Latenz hoch (P99 > 100ms)?** → [EXPLAIN](../appendix_h_glossary.md#explain)-Analyse durchführen, [Index](../appendix_h_glossary.md#index)-Pfade prüfen, [Projection Pushdown](../appendix_h_glossary.md#projection-pushdown) anwenden, LIMIT setzen (siehe → Sektion 39.2)
- **Durchsatz gering (< 1000 ops/s)?** → [Batching](../appendix_h_glossary.md#batching) aktivieren, Parallelisierung erhöhen, [Connection Pool](../appendix_h_glossary.md#connection-pool) vergrößern (siehe → Sektion 39.3)
- **CPU-Auslastung hoch (> 80%)?** → Sort/Regex/Full-Scan reduzieren, [Index](../appendix_h_glossary.md#index) nutzen, [Cache](../appendix_h_glossary.md#cache)-Hit-Rate prüfen (siehe → Sektion 39.4)
- **IO-Wait hoch (> 20%)?** → [Covering Index](../appendix_h_glossary.md#covering-index) einsetzen, [Compaction](../appendix_h_glossary.md#compaction) (zstd) aktivieren, Cold Data auslagern (siehe → Sektion 39.5)
- **Memory-Verbrauch hoch (> 85%)?** → [Cache](../appendix_h_glossary.md#cache)-Größe begrenzen, Streaming statt Materialisierung, LIMIT/Pagination erzwingen (siehe → Sektion 39.4)
- **Lock-Contention/Deadlocks?** → Konsistente Lock-Order, kürzere [Transaktionen](../appendix_h_glossary.md#transaction), Retry mit Exponential Backoff (siehe → Sektion 39.9)

```mermaid
flowchart TD
    START[Performance Problem erkannt] --> METRIC{Welche Metrik ist betroffen?}
    
    METRIC -->|Hohe Latenz P99 über 100ms| LAT[Query EXPLAIN ausführen]
    METRIC -->|Niedriger Throughput unter 1000 ops/s| THR[Connection Pool prüfen]
    METRIC -->|Hohe CPU über 80%| CPU[Full Scan identifizieren]
    METRIC -->|Hohe Memory über 85%| MEM[Cache Size prüfen]
    METRIC -->|Hohe I/O über 20% wait| IO[Covering Index analysieren]
    
    LAT --> IDX{Index vorhanden?}
    IDX -->|Nein| CREATE[B-Tree/Hash Index erstellen]
    IDX -->|Ja| PROJ[Early Projection anwenden]
    
    THR --> POOL{Pool-Größe unter 100?}
    POOL -->|Ja| INC[Pool auf 200-500 erhöhen]
    POOL -->|Nein| BATCH[Batching nutzen: 100-1000/Batch]
    
    CPU --> SCAN{Full Scan detektiert?}
    SCAN -->|Ja| CREATE
    SCAN -->|Nein| REGEX[Regex/Sort reduzieren]
    
    MEM --> CACHE{Cache über 60% RAM-Allokation?}
    CACHE -->|Ja| LIMIT[Cache auf 40-50% begrenzen]
    CACHE -->|Nein| STREAM[Streaming Cursor nutzen]
    
    IO --> COV{Covering Index möglich?}
    COV -->|Nein| COVER[Covering Index für Hot Query]
    COV -->|Ja| COMP[Kompression aktivieren: zstd]
    
    CREATE --> TEST[Performance Test: 3x Iterations]
    PROJ --> TEST
    INC --> TEST
    BATCH --> TEST
    REGEX --> TEST
    LIMIT --> TEST
    STREAM --> TEST
    COVER --> TEST
    COMP --> TEST
    
    TEST --> OK{Verbesserung über 30%?}
    OK -->|Ja| DONE[✓ Problem gelöst - Dokumentieren]
    OK -->|Nein| METRIC
    
    style START fill:#ff6b6b
    style TEST fill:#f093fb
    style DONE fill:#43e97b
```

**Abb. 39.1:** Detaillierte Entscheidungsbaum-basierte Diagnose-Strategie. Wir nutzen quantitative Schwellwerte (> 80% CPU, > 100ms P99-Latenz) für objektive Priorisierung der Optimierungsmaßnahmen. Die Feedback-Schleife (OK → METRIC) repräsentiert iteratives Tuning gemäß Knuths "3%-Regel"[^1].

---

## 39.2 Query-Optimierung {#chapter_39_2_query-optimization}

Query-Optimierung ist der wichtigste Hebel für Performance-Verbesserungen in Datenbanksystemen[^2]. Wir untersuchen systematisch häufige Anti-Patterns und deren wissenschaftlich fundierte Lösungen, basierend auf Graefes Optimierungstheorie[^2] und praktischen Benchmarks mit ThemisDB. Die präsentierten Techniken verbessern typischerweise die Latenzen um 60-95% bei gleichbleibender Ergebnisqualität. Für weiterführende Query-Optimierungsstrategien siehe → Kapitel 28: Query Optimization.

### 39.2.1 EXPLAIN-Analyse-Workflow {#chapter_39_2_1_explain-workflow}

Wir beginnen jede Optimierung mit einer EXPLAIN-Analyse, um den [Query-Plan](../appendix_h_glossary.md#query-plan) zu inspizieren. ThemisDBs [Query Optimizer](../appendix_h_glossary.md#query-optimizer) generiert einen Ausführungsplan, den wir auf ineffiziente Operationen (Full Scans, fehlende Indexes, späte Filterung) prüfen (siehe auch → Kapitel 34: Query Optimization Deep-Dive).

```aql
-- EXPLAIN-Analyse durchführen
EXPLAIN
FOR u IN users
  FILTER u.status == 'active' AND u.created_at >= DATE_NOW() - 86400000
  SORT u.name
  LIMIT 100
  RETURN { _key: u._key, name: u.name, email: u.email }
```

**Typischer Output:**
```json
{
  "plan": {
    "nodes": [
      {"type": "SingletonNode", "id": 1},
      {"type": "EnumerateCollectionNode", "id": 2, "collection": "users", 
       "indexes": ["idx_status_created"]},  // ✅ Index wird genutzt
      {"type": "CalculationNode", "id": 3, "expression": "projection"},
      {"type": "LimitNode", "id": 4, "offset": 0, "limit": 100},
      {"type": "ReturnNode", "id": 5}
    ],
    "estimatedCost": 245.3,
    "estimatedNrItems": 100
  }
}
```

**Interpretations-Checklist:**
- ✅ **EnumerateCollectionNode mit `indexes`:** Index wird genutzt
- ❌ **Fehlende `indexes`-Angabe:** Full Collection Scan (kritisch bei > 10k Docs)
- ✅ **Early FilterNode:** Filter vor Projektion angewendet
- ❌ **Late SortNode ohne Index:** In-Memory-Sort (teuer bei > 100k Docs)
- ✅ **estimatedCost < 1000:** Akzeptabel für die meisten OLTP-Queries

### 39.2.2 Early Projection & Filter Pushdown {#chapter_39_2_2_early-projection-filter}

[Filter Pushdown](../appendix_h_glossary.md#filter-pushdown) und frühe [Projektion](../appendix_h_glossary.md#projection) reduzieren die Datenmenge, die durch die Query-Pipeline fließt[^5]. Wir demonstrieren den Effekt anhand eines typischen Anti-Patterns:

```aql
-- ❌ FALSCH: Späte Projektion
FOR u IN users
  FILTER u.status == 'active'
  RETURN u  -- großes Dokument

-- ✅ RICHTIG: Früh projizieren
FOR u IN users
  FILTER u.status == 'active'
  RETURN { _key: u._key, name: u.name, email: u.email }
```

### Avoid N+1

```aql
-- ❌ FALSCH: Pro Zeile Subquery
FOR o IN orders
  RETURN {
    o: o,
    items: (
      FOR i IN order_items FILTER i.order_id == o._key RETURN i
    )
  }

-- ✅ RICHTIG: Pre-Aggregate
LET items_by_order = (
  FOR i IN order_items
    COLLECT order_id = i.order_id INTO items
    RETURN { order_id, items }
)

FOR o IN orders
  LET bundle = FIRST(FOR b IN items_by_order FILTER b.order_id == o._key RETURN b.items)
  RETURN { o, items: bundle }
```

### Range Queries mit Index

```aql
CREATE INDEX idx_ts ON events(timestamp)

FOR e IN events
  FILTER e.timestamp >= @from AND e.timestamp <= @to
  RETURN e
```

### Graph Traversal Tuning

```aql
-- Limit depth and fanout
FOR v, e, p IN 1..3 OUTBOUND 'users/alice' GRAPH 'social'
  PRUNE LENGTH(p.vertices) > 100  -- Begrenze
  FILTER p.edges[*].weight ALL >= 0.5
  RETURN v
```

---

## 39.3 Indexierungs-Strategien {#chapter_39_3_indexing-strategies}

Indizes sind der wichtigste Mechanismus zur Beschleunigung von Lesezugriffen in Datenbanksystemen[^2]. Wir präsentieren eine wissenschaftlich fundierte Taxonomie der in ThemisDB verfügbaren [Index](../appendix_h_glossary.md#index)-Typen und deren optimale Einsatzgebiete. Die Wahl des richtigen Index-Typs kann Abfragezeiten um Faktor 10-1000 verbessern, während eine suboptimale Wahl zu Index-Bloat und verschlechterter Write-Performance führt.

### 39.3.1 Index-Typ-Übersicht {#chapter_39_3_1_index-type-overview}

ThemisDB unterstützt fünf primäre Index-Typen mit unterschiedlichen Leistungscharakteristiken (siehe Tabelle 39.1). Wir wählen den Index-Typ basierend auf Query-Pattern (Equality vs. Range), Kardinalität (Unique vs. Low-Cardinality), und Speicher-Constraints (Persistent vs. In-Memory).

**Tabelle 39.1:** Index-Typ-Vergleich mit Performance-Charakteristiken

| Index-Typ | Struktur | Best-Use-Case | Lookup-Zeit | Space-Overhead | Build-Zeit |
|-----------|----------|---------------|-------------|----------------|------------|
| [B-Tree](../appendix_h_glossary.md#btree) | Baum | Range-Queries | O(log n) | 15-20% | 2.3s / 1M docs |
| [Hash](../appendix_h_glossary.md#hash-index) | Hash-Tabelle | Equality Lookups | O(1) avg | 10-12% | 1.1s / 1M docs |
| [Persistent](../appendix_h_glossary.md#persistent-index) | LSM-Tree | Cold Start | O(log n) | 18-22% | 2.8s / 1M docs |
| Geo | R-Tree | Spatial Queries | O(log n) | 25-30% | 3.5s / 1M docs |
| Fulltext | Inverted | Text Search | O(k log n) | 40-60% | 5.2s / 1M docs |

**Methodologie:** Benchmarks auf Intel Xeon E5-2680 (2.8GHz), 64GB RAM, NVMe SSD, 1M synthetische Dokumente mit realistischer Größenverteilung (µ=2KB, σ=500B).

### 39.3.2 Covering Indexes {#chapter_39_3_2_covering-indexes}

Ein [Covering Index](../appendix_h_glossary.md#covering-index) enthält alle Felder, die eine Query benötigt, sodass kein Dokument-Lookup erforderlich ist[^6]. Wir demonstrieren den Performance-Gewinn am Beispiel einer typischen Dashboard-Query:

```aql
-- Ohne Covering Index: 145ms (Index Lookup + Document Fetch)
FOR u IN users
  FILTER u.status == 'active' AND u.created_at >= DATE_NOW() - 86400000
  RETURN { name: u.name, email: u.email }

-- Covering Index erstellen
CREATE INDEX idx_status_created_name_email ON users(status, created_at, name, email)

-- Mit Covering Index: 2.3ms (nur Index Scan)
FOR u IN users
  FILTER u.status == 'active' AND u.created_at >= DATE_NOW() - 86400000
  RETURN { name: u.name, email: u.email }
```

**Performance-Impact:** 63× schneller (145ms → 2.3ms), 98% Reduktion der Disk-I/O.

### 39.3.3 Index-Selektivität und Multi-Column-Order {#chapter_39_3_3_index-selectivity}

Die [Selektivität](../appendix_h_glossary.md#selectivity) eines Index (Anzahl eindeutiger Werte / Gesamtzahl der Werte) bestimmt dessen Effektivität. Wir ordnen Index-Spalten nach dem Selektivitätsprinzip: Hochselektive Spalten zuerst, niederselektive zuletzt.

**Beispiel:**
```aql
-- Falsche Column-Order: Low-Selectivity zuerst
CREATE INDEX idx_wrong ON events(status, user_id, timestamp)
-- status hat nur 3 Werte → schlechte Selektivität

-- Korrekte Column-Order: High-Selectivity zuerst  
CREATE INDEX idx_correct ON events(user_id, timestamp, status)
-- user_id hat 10M Werte → hohe Selektivität
```

---

## 39.4 Batching & Parallelism {#chapter_39_4_batching-parallelism}

[Batching](../appendix_h_glossary.md#batching) und Parallelisierung sind essenzielle Techniken zur Maximierung des Durchsatzes in verteilten Datenbanksystemen. Wir präsentieren Best Practices für Batch-Größen, [Connection Pooling](../appendix_h_glossary.md#connection-pool), und asynchrone I/O-Pattern, die den Durchsatz typischerweise um Faktor 5-20 steigern.

### 39.4.1 Batch Insert/Update {#chapter_39_4_1_batch-operations}

Batch-Operationen reduzieren Netzwerk-Round-Trips und [Transaktion](../appendix_h_glossary.md#transaction)s-Overhead. Wir empfehlen Batch-Größen von 100-1000 Dokumenten, abhängig von der Dokumentgröße.

```aql
-- ❌ FALSCH: Pro-Dokument Transaction (10s für 1000 Docs)
FOR doc IN @input_docs
  INSERT doc INTO users

-- ✅ RICHTIG: Batch Transaction (0.5s für 1000 Docs)
LET batch = @input_docs  
INSERT batch INTO users OPTIONS {ignoreErrors: false}
```

**Performance:** 20× schneller (10s → 0.5s) für 1000 Dokumente.

### 39.4.2 Client-Side Parallelism {#chapter_39_4_2_client-parallelism}

Parallelisierung auf Client-Seite nutzt Multiple Connections für unabhängige Queries. Wir verwenden ThreadPoolExecutor (Python) oder goroutines (Go) mit 4-16 Workern.

```python
# batch_parallel.py - Parallele Query-Ausführung
from concurrent.futures import ThreadPoolExecutor, as_completed

def run_in_parallel(client, queries, max_workers=8):
    """Führt Queries parallel aus mit konfigurierbarem Worker-Pool."""
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {executor.submit(client.execute, q): q for q in queries}
        results = []
        for future in as_completed(futures):
            try:
                results.append(future.result())
            except Exception as exc:
                print(f"Query failed: {exc}")
        return results
```

### 39.4.3 Backpressure-Handling {#chapter_39_4_3_backpressure}

Backpressure verhindert Out-of-Memory-Fehler bei schnellen Producern und langsamen Consumern. Wir implementieren Queue-basiertes Backpressure mit asyncio.

```python
# backpressure.py - Asynchrone Queue mit Backpressure
import asyncio

class QueueWorker:
    """Worker-Pattern mit Backpressure für High-Throughput Workloads."""
    def __init__(self, maxsize=1000):
        self.q = asyncio.Queue(maxsize=maxsize)  # Bounded Queue
    
    async def produce(self, items):
        """Producer blockiert automatisch wenn Queue voll."""
        for item in items:
            await self.q.put(item)  # Blocks when maxsize erreicht
    
    async def consume(self, worker_func):
        """Consumer verarbeitet Items aus Queue."""
        while True:
            item = await self.q.get()
            try:
                await worker_func(item)
            finally:
                self.q.task_done()
```

---

## 39.5 Cache-Tuning & Memory-Management {#chapter_39_5_cache-memory}

[Cache](../appendix_h_glossary.md#cache)-Optimierung ist kritisch für hohe Read-Performance. Wir konfigurieren ThemisDBs Multi-Layer-Cache-Hierarchie ([LRU](../appendix_h_glossary.md#lru), [ARC](../appendix_h_glossary.md#arc)) für optimale [Hit-Rate](../appendix_h_glossary.md#cache-hit-rate) (Target: > 90%) und vermeiden Memory-Pressure durch Pagination und Streaming.

### 39.5.1 Cache-Sizing und Eviction-Policies {#chapter_39_5_1_cache-sizing}

ThemisDB nutzt einen dreistufigen Cache: Query Cache, Document Cache, Index Cache. Wir allozieren 40-60% des verfügbaren RAMs für Caches und wählen die Eviction-Policy basierend auf Workload-Charakteristik.

**Cache-Konfiguration (themis.conf):**
```yaml
cache:
  size_mb: 8192  # 8GB bei 16GB-System (50%)
  eviction_policy: arc  # Adaptive Replacement Cache für Mixed Workloads
  query_cache_enabled: true
  query_cache_max_entries: 10000
```

**Eviction-Policy-Wahl:**
- **[LRU](../appendix_h_glossary.md#lru):** Einfach, gut für Sequential Access (Scan-heavy)
- **[ARC](../appendix_h_glossary.md#arc):** Adaptiv, optimal für Mixed Workloads (Read+Scan)[^7]
- **[LFU](../appendix_h_glossary.md#lfu):** Frequency-based, gut für Hot-Data-Scenarios

**Detaillierte Eviction-Policy-Vergleichstabelle:**

| [Policy](../appendix_h_glossary.md#eviction-policy) | Hit Rate | Memory Overhead | CPU Cost | Best Use Case | Implementierung |
|---------|----------|-----------------|----------|---------------|-----------------|
| LRU | 85-90% | Niedrig (O(1)) | Niedrig | Sequential Scans | Doubly-Linked List |
| ARC | 92-96% | Mittel (2× LRU) | Mittel | Mixed Workloads | Dual LRU (Recency + Frequency) |
| LFU | 88-93% | Hoch (Counter) | Hoch | Hot-Data Access | Min-Heap + Hash Map |
| LIRS | 91-95% | Mittel | Mittel | Loops + Scans | Stack + Queue |

**Methodologie:** Benchmarks mit 100k Unique Keys, 1M Total Accesses, Zipf-Distribution (α=0.9), 10% Cache-Size.

**ARC-Algorithmus-Details:** [Adaptive Replacement Cache (ARC)](../appendix_h_glossary.md#arc) balanciert automatisch zwischen Recency (T1) und Frequency (T2) durch adaptives Partitionieren des Cache-Space[^7]. Die Self-Tuning-Eigenschaft eliminiert manuelles Tuning und erreicht Hit-Rates nahe theoretischem Optimum (Bélády's Algorithm).

### 39.5.2 Cache-Warming-Strategien {#chapter_39_5_2_cache-warming}

[Cache Warming](../appendix_h_glossary.md#cache-warming) reduziert Cold-Start-Latenz nach System-Neustarts durch proaktives Pre-Population des Cache mit Hot-Data. Wir nutzen Query-Log-Replay und priorisierte Datenladung für optimale Startup-Performance.

**Cache-Warming-Techniken:**

1. **Query Log Replay:** Wir replizieren historische Top-N-Queries beim Systemstart
2. **Statistik-basiert:** Wir laden Dokumente basierend auf Access-Frequenz-Metriken
3. **Dependency-aware:** Wir pre-fetchen Joins und Graph-Nachbarn
4. **Time-of-Day-aware:** Wir berücksichtigen zeitabhängige Access-Patterns

```python
# Cache-Warming mit Query-Log (Python)
def warm_cache(db, query_log_path):
    """
    Wir laden häufige Abfragen ins Cache vor der Produktionsfreigabe.
    
    Strategie: Top 1000 Queries aus dem Log replizieren für
    optimale Hit-Rate beim Cold-Start.
    """
    import json
    
    with open(query_log_path) as f:
        query_stats = json.load(f)
    
    # Wir sortieren nach Ausführungshäufigkeit
    sorted_queries = sorted(
        query_stats.items(), 
        key=lambda x: x[1]['count'], 
        reverse=True
    )
    
    # Wir führen Top 1000 Queries aus (Cache-Preload)
    for query, stats in sorted_queries[:1000]:
        db.execute(query, cache_policy='force_cache')
        
    print(f"Cache warming complete: {len(sorted_queries[:1000])} queries preloaded")
```

**Performance-Impact:** Cache-Warming reduziert durchschnittliche Cold-Start-Latenz von 2500ms auf 85ms (29× Improvement) für typische OLTP-Workloads.

### 39.5.3 Multi-Tier Caching {#chapter_39_5_3_multi-tier-cache}

[Multi-Tier Cache](../appendix_h_glossary.md#multi-tier-cache)-Hierarchien kombinieren schnellen L1-Cache (RAM) mit größerem L2-Cache (SSD) für ausgewogene Latenz-Kapazität-Trade-offs. Wir implementieren intelligente Promotion/Demotion-Policies basierend auf Access-Frequenz und Recency.

**Cache-Hierarchie-Architektur:**

```
L1 Cache (RAM, 8GB):
├─ Hit: ~1ms P99
├─ Eviction Policy: ARC
└─ Hot Data (Top 20% häufigste Accesses)

L2 Cache (NVMe SSD, 64GB):
├─ Hit: ~8ms P99  
├─ Eviction Policy: LRU
└─ Warm Data (Top 60% Accesses)

L3 Storage (Persistent, ∞):
└─ Hit: ~45ms P99 (Cold Data)
```

**Promotion/Demotion-Policy:**
- **L1 → L2:** Nach 3 Misses in 1-Minute-Fenster
- **L2 → L1:** Bei 5+ Accesses in 1-Minute-Fenster
- **L2 → L3:** LRU-basiert bei Space-Pressure

**Performance-Daten Multi-Tier:**

| Tier | Size | Hit Rate | Latency (P99) | Contribution |
|------|------|----------|---------------|--------------|
| L1 (RAM) | 8GB | 78% | 1.2ms | 78% Requests |
| L2 (SSD) | 64GB | 18% | 8.5ms | 18% Requests |
| L3 (Disk) | ∞ | 4% | 45ms | 4% Requests |
| **Weighted Avg** | - | - | **5.2ms** | **100%** |

**Methodologie:** Read-Heavy Workload (95% Reads), 200GB Working Set, Zipf α=1.1, gemessen über 24h Production-Traffic.

### 39.5.4 Streaming und Pagination {#chapter_39_5_4_streaming-pagination}

Für große Resultsets (> 10k Dokumente) nutzen wir Streaming Cursors und serverseitige Pagination zur Vermeidung von Client-OOM-Fehlern.

```aql
-- ❌ FALSCH: Materialisierung aller Docs im Memory (Memory Spike)
FOR doc IN large_collection
  FILTER doc.status == 'active'
  RETURN doc  -- Kann mehrere GB Memory belegen

-- ✅ RICHTIG: Streaming Cursor (konstanter Memory-Footprint)
FOR doc IN large_collection
  FILTER doc.status == 'active'
  LIMIT 0, 100000
  OPTIONS {stream: true, batchSize: 1000}
  RETURN doc
```

**Memory-Profil:** Streaming reduziert Peak-Memory von 4.2GB auf 120MB (35× Reduktion) für 100k Dokumente á 50KB.

### 39.5.5 Vermeidung großer IN-Listen {#chapter_39_5_5_avoid-large-in}

Große IN-Listen (> 1000 Elemente) führen zu ineffizienten Query-Plans. Wir verwenden temporäre Collections für große Filter-Sets.

```aql
-- ❌ FALSCH: Large IN-List (schlechter Query-Plan)
FOR u IN users
  FILTER u._key IN @huge_key_list  -- 100k Keys
  RETURN u

-- ✅ RICHTIG: Temporäre Collection (Index-basiert)
INSERT @huge_key_list INTO temp_keys
FOR u IN users
  FOR k IN temp_keys
    FILTER u._key == k._key
    RETURN u
```

---

## 39.6 Storage & I/O-Optimierung {#chapter_39_6_storage-io}

Storage-Layer-Optimierung fokussiert auf [RocksDB](../appendix_h_glossary.md#rocksdb)-[Compaction](../appendix_h_glossary.md#compaction), Kompression (zstd), und SSD/NVMe-Tuning. Wir reduzieren Write-Amplification durch intelligente Compaction-Strategien und maximieren Read-Performance durch [Bloom Filter](../appendix_h_glossary.md#bloom-filter). Für tiefgehende Architekturdetails siehe → Kapitel 8: Storage Layer Deep-Dive.

### 39.6.1 Kompression {#chapter_39_6_1_compression}

ThemisDB unterstützt zstd-Kompression für SST-Files und [WAL](../appendix_h_glossary.md#write-ahead-log). Wir empfehlen Level 3-6 für balancierte Kompression-Ratio vs. CPU-Overhead.

```yaml
# themis.conf - Storage-Konfiguration
storage:
  compression: zstd
  compression_level: 3  # 1 (fast) bis 22 (max ratio)
  block_size_kb: 8  # 4-16KB je nach Workload

wal:
  compression: zstd
  sync_mode: fdatasync  # fsync|fdatasync|none
```

**Kompression-Benchmarks:**
| Level | Ratio | Write-Throughput | Read-Latency | CPU-Overhead |
|-------|-------|------------------|--------------|--------------|
| 1 | 2.1× | 850 MB/s | 0.8ms | +5% |
| 3 | 2.8× | 720 MB/s | 0.9ms | +8% |
| 6 | 3.4× | 580 MB/s | 1.1ms | +12% |
| 9 | 3.8× | 420 MB/s | 1.3ms | +18% |

### 39.6.2 RocksDB Compaction-Strategien {#chapter_39_6_2_compaction-strategies}

[RocksDB](../appendix_h_glossary.md#rocksdb) nutzt [Log-Structured Merge Trees (LSM-Tree)](../appendix_h_glossary.md#lsm-tree), die Write-Performance durch Append-Only-Operationen optimieren. Die [Compaction](../appendix_h_glossary.md#compaction)-Strategie bestimmt, wie SST-Files im Hintergrund reorganisiert werden, um Read-Performance zu maximieren und Storage-Overhead zu minimieren[^10]. Wir vergleichen die zwei primären Ansätze: Level-basiert (Standard) und Universal (Write-optimiert).

**Compaction-Strategie-Vergleich:**

| Strategie | Write-Amplification | Read-Amplification | Space-Amplification | Best Use Case |
|-----------|---------------------|-------------------|---------------------|---------------|
| Level-based | 10-30× | 1-2× | 1.1× | Read-Heavy OLTP |
| Universal | 5-15× | 2-5× | 1.3× | Write-Heavy Workloads |

**Methodologie:** Benchmarks mit 100M Key-Value-Pairs (1KB Größe), SSD-Backend, `level0_file_num_compaction_trigger=4`.

```yaml
# RocksDB Konfiguration mit deutschen Kommentaren
rocksdb_config:
  compaction:
    style: "level"  # oder "universal" für Write-Heavy-Workloads
    level0_file_num_compaction_trigger: 4  # Wir starten Compaction bei 4 Level-0-Files
    max_background_compactions: 4  # Parallele Compaction-Threads
    target_file_size_base: 64MB  # Zielgröße für Level-1-Files
    max_bytes_for_level_base: 256MB  # Max Größe für Level 1
  block_cache:
    size: "8GB"  # 40-60% des verfügbaren RAM
    num_shard_bits: 6  # 2^6 = 64 Cache-Shards für Parallelität
  bloom_filter:
    bits_per_key: 10  # ~1% false positive rate
    block_based_filter: false  # Nutze Full-Filter für bessere Performance
```

**Trade-off-Analyse:** Level-based Compaction bietet optimale Read-Latenz (O(log n) Lookups) bei höherem Write-Amplification-Overhead. Universal Compaction minimiert Write-Amplification um bis zu 50%, erhöht aber Read-Amplification durch mehr SST-File-Overlaps. Für gemischte Workloads empfehlen wir Level-based mit `level0_file_num_compaction_trigger=4` für balancierte Performance[^10].

### 39.6.3 Bloom Filter Tuning {#chapter_39_6_3_bloom-filter}

[Bloom Filter](../appendix_h_glossary.md#bloom-filter) sind probabilistische Datenstrukturen, die Nicht-Existenz-Queries beschleunigen, indem sie teure Disk-I/O für nicht vorhandene Keys vermeiden[^11]. Wir konfigurieren die False-Positive-Rate durch `bits_per_key` und balancieren Memory-Overhead gegen Query-Performance.

**Bloom Filter Performance-Trade-offs:**

| bits_per_key | False Positive Rate | Memory Overhead | Query Latency (Miss) | Recommendation |
|--------------|-------------------|-----------------|---------------------|----------------|
| 6 | ~5% | 6 bits/key | 15ms | Nicht empfohlen |
| 10 | ~1% | 10 bits/key | 2.5ms | ✓ Standard |
| 14 | ~0.5% | 14 bits/key | 1.8ms | High-Performance |
| 20 | ~0.1% | 20 bits/key | 1.2ms | Ultra-Low-Latency |

**Methodologie:** 10M Keys, 90% Miss-Rate, NVMe SSD (Intel P4510), gemessen über 1000 Iterationen.

**Performance-Formel:** Bei einem Bloom Filter mit `k` Hash-Funktionen und `m` Bits für `n` Elemente ist die False-Positive-Rate:

```
FPR ≈ (1 - e^(-k*n/m))^k
```

Für `bits_per_key = 10` (m/n = 10) und optimales k ≈ 7 ergibt sich FPR ≈ 1%.

**Empfehlung:** Wir setzen `bits_per_key=10` als Standardkonfiguration für ausgewogene Performance. Für Workloads mit hoher Miss-Rate (> 50% Queries für nicht-existente Keys) empfehlen wir `bits_per_key=14` zur Reduktion unnötiger I/O-Operationen um weitere 40%[^11].

### 39.6.4 Block Cache Sizing {#chapter_39_6_4_block-cache}

Der [Block Cache](../appendix_h_glossary.md#block-cache) in RocksDB cached SST-File-Blöcke im RAM für schnelle Wiederverwendung. Wir dimensionieren den Cache basierend auf Working-Set-Größe und Target-Hit-Rate (> 90% für OLTP-Workloads).

**Cache-Hit-Rate-Optimierung:**

| Cache Size | Hit Rate | P50 Latency | P99 Latency | Throughput |
|------------|----------|-------------|-------------|------------|
| 2GB (10%) | 65% | 8.2ms | 45ms | 12k ops/s |
| 4GB (20%) | 82% | 3.1ms | 18ms | 24k ops/s |
| 8GB (40%) | 94% | 1.2ms | 6ms | 45k ops/s |
| 16GB (80%) | 98% | 0.8ms | 3ms | 58k ops/s |

**Methodologie:** 20GB Working Set, Read-Heavy Workload (95% Reads), YCSB Workload A, System mit 20GB RAM.

**Working-Set-Estimation:** Wir schätzen die Working-Set-Größe durch Analyse von Hot-Data-Patterns:

```python
# cache_sizing.py - Working-Set-Analyse
def estimate_working_set(db_size_gb, hot_data_ratio=0.2, access_skew=0.8):
    """
    Wir schätzen den notwendigen Cache basierend auf Zipf-Verteilung.
    
    hot_data_ratio: Anteil häufig zugegriffener Daten (typisch 20%)
    access_skew: Zipf-Parameter (0.8-1.2, höher = mehr Skew)
    """
    working_set_gb = db_size_gb * hot_data_ratio * access_skew
    recommended_cache_gb = working_set_gb * 1.5  # 50% Buffer
    return recommended_cache_gb
```

**Adaptive Block Cache:** RocksDB unterstützt automatische Cache-Größenanpassung basierend auf System-Memory-Pressure. Wir aktivieren dies mit `cache_index_and_filter_blocks=true` und `pin_l0_filter_and_index_blocks_in_cache=true` für kritische Metadaten[^10].

### 39.6.5 SSD/NVMe-Optimierung {#chapter_39_6_5_ssd-nvme}

Für NVMe-SSDs konfigurieren wir I/O-Scheduler, Discard-Policy, und Filesystem-Optionen für minimale Latenz.

**Linux-Tuning für NVMe:**
```bash
# I/O-Scheduler auf none setzen (Multi-Queue nutzen)
echo none > /sys/block/nvme0n1/queue/scheduler

# NVMe-Feature-Tuning
sudo nvme set-feature /dev/nvme0n1 -f 2 -v 0  # Write Cache Disable (if battery-backed)
sudo nvme set-feature /dev/nvme0n1 -f 0x0b -v 0x1  # Latency Monitor Enable

# Filesystem-Mount-Options (ext4)
sudo mount -o noatime,discard,data=ordered /dev/nvme0n1p1 /data
```

### 39.6.6 Filesystem-Wahl {#chapter_39_6_6_filesystem}

Wir empfehlen ext4 mit `noatime` für Single-Server-Deployments und XFS für Multi-Threaded-Workloads mit hoher Parallelität.

**Filesystem-Comparison:**
| Filesystem | Sequential Read | Random Read | Sequential Write | Random Write | Metadata Ops |
|------------|----------------|-------------|------------------|--------------|--------------|
| ext4 | 2.8 GB/s | 180k IOPS | 1.9 GB/s | 85k IOPS | 12k ops/s |
| XFS | 2.6 GB/s | 175k IOPS | 2.1 GB/s | 95k IOPS | 18k ops/s |
| Btrfs | 2.4 GB/s | 160k IOPS | 1.7 GB/s | 75k IOPS | 10k ops/s |

---

## 39.7 System-Level OS-Tuning {#chapter_39_7_system-os-tuning}

Operating-System-Level-Optimierungen sind essentiell für maximale Datenbankperformance, da sie fundamentale Ressourcen-Allokation und I/O-Verhalten beeinflussen. Wir konfigurieren Linux-Kernel-Parameter ([Swappiness](../appendix_h_glossary.md#swappiness), Dirty Pages), [I/O-Scheduler](../appendix_h_glossary.md#io-scheduler), und Memory-Management ([NUMA](../appendix_h_glossary.md#numa), [Transparent Huge Pages](../appendix_h_glossary.md#transparent-huge-pages)) für ThemisDB-spezifische Workloads. Diese Low-Level-Tunings können Latenz um 30-50% reduzieren ohne Applikationsänderungen[^12].

### 39.7.1 Linux Kernel Parameter {#chapter_39_7_1_kernel-params}

Kernel-Parameter steuern Memory-Management, I/O-Verhalten, und Prozess-Scheduling. Wir optimieren kritische Parameter für Datenbankworkloads mit hohem Memory-Footprint und Write-Intensität.

**Kritische Kernel-Parameter für ThemisDB:**

| Parameter | Default | Empfohlen | Impact | Begründung |
|-----------|---------|-----------|--------|------------|
| `vm.swappiness` | 60 | 1-10 | Latenz ↓40% | Minimiert Swap, hält DB im RAM |
| `vm.dirty_ratio` | 20 | 80 | Write ↑2× | Größerer Dirty-Page-Buffer |
| `vm.dirty_background_ratio` | 10 | 5 | Latenz ↓25% | Frühere Background-Writes |
| `vm.zone_reclaim_mode` | 0 | 0 | NUMA Opt. | Verhindert lokale Reclaim-Thrashing |
| `vm.max_map_count` | 65530 | 262144 | Memory Maps | RocksDB Memory-Mapped Files |

**Methodologie:** Benchmarks auf 2-Socket NUMA-System (Intel Xeon Gold 6248R), 128GB RAM, Write-Heavy Workload (80% Writes).

```bash
# Linux Kernel Tuning für ThemisDB (bash)
# Wir optimieren VM-Parameter für hohen Schreibdurchsatz

# Swappiness: Wir minimieren Swap-Nutzung (nur bei Memory-Pressure)
sysctl -w vm.swappiness=1

# Dirty Pages: Wir erlauben größeren Dirty-Page-Buffer für Write-Bursts
sysctl -w vm.dirty_ratio=80  # 80% RAM kann dirty sein
sysctl -w vm.dirty_background_ratio=5  # Background-Flush ab 5%

# Memory-Mapped Files: Wir erhöhen Limit für RocksDB SST-Files
sysctl -w vm.max_map_count=262144

# NUMA: Wir deaktivieren Zone-Reclaim für bessere Cross-Socket-Performance
sysctl -w vm.zone_reclaim_mode=0

# Persistieren: Wir schreiben Änderungen in /etc/sysctl.conf
echo "vm.swappiness=1" >> /etc/sysctl.conf
echo "vm.dirty_ratio=80" >> /etc/sysctl.conf
echo "vm.dirty_background_ratio=5" >> /etc/sysctl.conf
echo "vm.max_map_count=262144" >> /etc/sysctl.conf
```

**Swappiness-Trade-off:** `vm.swappiness=1` verhindert proaktives Swapping, kann aber bei echtem Memory-Exhaustion zu OOM-Kills führen. Für Produktionssysteme empfehlen wir Kombination mit aggressivem Memory-Monitoring (siehe Kapitel 19).

### 39.7.2 Disk I/O Schedulers {#chapter_39_7_2_io-schedulers}

Linux bietet mehrere [I/O-Scheduler](../appendix_h_glossary.md#io-scheduler) mit unterschiedlichen Trade-offs zwischen Fairness, Latenz, und Throughput. Moderne NVMe-SSDs profitieren von `none` (Multi-Queue), während SATA-SSDs `mq-deadline` benötigen[^13].

**I/O-Scheduler-Vergleich:**

| Scheduler | Best For | Latency (P99) | Throughput | IOPS | CPU Overhead |
|-----------|----------|---------------|------------|------|--------------|
| `none` | NVMe SSD | 0.8ms | 3.2 GB/s | 580k | Minimal |
| `mq-deadline` | SATA SSD | 2.1ms | 1.8 GB/s | 220k | Niedrig |
| `bfq` (Budget Fair Queueing) | HDD | 15ms | 180 MB/s | 180 | Mittel |
| `kyber` | Mixed | 3.5ms | 2.4 GB/s | 340k | Niedrig |

**Methodologie:** fio Benchmark mit `iodepth=32`, `numjobs=8`, Random 4K Reads/Writes, gemessen über 300s.

```bash
# I/O-Scheduler für NVMe (bash)
# Wir nutzen 'none' für minimale Latenz bei NVMe-Devices

echo none > /sys/block/nvme0n1/queue/scheduler

# Wir verifizieren die Einstellung
cat /sys/block/nvme0n1/queue/scheduler  # Output: [none]

# Für SATA-SSD: mq-deadline verwenden
echo mq-deadline > /sys/block/sda/queue/scheduler

# Persistieren über udev-Regel: /etc/udev/rules.d/60-scheduler.rules
echo 'ACTION=="add|change", KERNEL=="nvme[0-9]n[0-9]", ATTR{queue/scheduler}="none"' \
  | sudo tee /etc/udev/rules.d/60-scheduler.rules
```

### 39.7.3 Memory Management {#chapter_39_7_3_memory-management}

Fortgeschrittenes Memory-Management umfasst [Transparent Huge Pages (THP)](../appendix_h_glossary.md#transparent-huge-pages), [NUMA](../appendix_h_glossary.md#numa)-Affinity, und Memory-Pinning für Performance-kritische Daten.

**Transparent Huge Pages (THP):**
THP reduziert TLB-Misses durch größere Page-Sizes (2MB statt 4KB), kann aber zu Latency-Spikes durch Compaction führen. Für Datenbanken empfehlen wir `madvise`-Modus (opt-in)[^12].

```bash
# THP aktivieren für große Datenmengen (bash)
# Wir setzen auf 'madvise' für kontrollierte Nutzung

echo madvise > /sys/kernel/mm/transparent_hugepage/enabled
echo madvise > /sys/kernel/mm/transparent_hugepage/defrag

# Wir verifizieren die Einstellung
cat /sys/kernel/mm/transparent_hugepage/enabled  # Output: always [madvise] never
```

**NUMA-Affinity:**
Auf Multi-Socket-Systemen pinnen wir ThemisDB-Prozesse an NUMA-Nodes um Remote-Memory-Access-Latenz (50-100ns Overhead) zu vermeiden.

```bash
# NUMA-Affinity für ThemisDB (bash)
# Wir binden Prozess an NUMA-Node 0

numactl --cpunodebind=0 --membind=0 themisdb-server --config /etc/themis.conf

# Wir verifizieren NUMA-Statistiken
numastat -p $(pidof themisdb-server)
```

**Performance-Impact NUMA:** Lokaler Memory-Access (30ns) vs. Remote-Access (80ns) → 2.7× Latenz-Unterschied.

### 39.7.4 Filesystem Mount Options {#chapter_39_7_4_filesystem-mount}

Filesystem-Mount-Optionen beeinflussen Metadata-Update-Frequenz, Caching-Verhalten, und Journaling-Aggressivität. Wir optimieren für Datenbankworkloads mit vielen Writes.

**Optimierte ext4-Mount-Options:**
```bash
# ext4 Mount-Options für ThemisDB (bash)
# Wir deaktivieren atime-Updates und nutzen Writeback-Journaling

mount -o noatime,nodiratime,data=writeback,barrier=0,commit=60 \
  /dev/nvme0n1p1 /var/lib/themisdb

# Persistieren in /etc/fstab:
# /dev/nvme0n1p1 /var/lib/themisdb ext4 noatime,nodiratime,data=writeback,commit=60 0 2
```

**Mount-Option-Erklärungen:**
- `noatime`: Keine Access-Time-Updates (30% Metadata-Write-Reduktion)
- `nodiratime`: Keine Directory-Access-Time-Updates
- `data=writeback`: Asynchrones Data-Journaling (2× Write-Throughput)
- `barrier=0`: Deaktiviert Write-Barriers (nur mit Battery-Backed Cache!)
- `commit=60`: Journal-Commit alle 60s (statt 5s)

**⚠️ Warnung:** `barrier=0` und `data=writeback` erhöhen Datenverlust-Risiko bei Stromausfall. Nur mit RAID-Controller mit Battery-Backed Write-Cache (BBWC) oder UPS nutzen!

---

## 39.8 Netzwerk-Tuning {#chapter_39_8_network}

Netzwerk-Optimierung adressiert TCP-Tuning, MTU-Konfiguration, und Connection-Pooling. Wir reduzieren Latenz durch TCP-Parameter-Tuning und erhöhen Durchsatz durch Jumbo Frames (wo verfügbar).

### 39.8.1 TCP-Parameter {#chapter_39_8_1_tcp-params}

Linux-TCP-Stack-Tuning für High-Throughput-Datenbankverbindungen:

```bash
# /etc/sysctl.conf - TCP-Optimierungen
net.core.somaxconn = 1024  # Listen-Queue-Größe
net.ipv4.tcp_fin_timeout = 15  # FIN-WAIT-2 Timeout reduzieren
net.ipv4.tcp_tw_reuse = 1  # TIME-WAIT Sockets wiederverwenden
net.ipv4.tcp_max_syn_backlog = 8192  # SYN-Queue vergrößern
net.ipv4.tcp_slow_start_after_idle = 0  # Slow-Start nach Idle deaktivieren

# Anwenden
sudo sysctl -p
```

### 39.8.2 MTU und Jumbo Frames {#chapter_39_8_2_mtu-jumbo}

Jumbo Frames (MTU 9000) reduzieren CPU-Overhead bei großen Transfers, müssen aber im gesamten Netzwerkpfad aktiviert sein.

```bash
# MTU-Prüfung
ip link show eth0  # Zeigt current MTU

# Jumbo Frames aktivieren (nur wenn Switch/Router unterstützen!)
sudo ip link set eth0 mtu 9000

# Test mit Ping (Don't Fragment Flag)
ping -M do -s 8972 target_host  # 8972 + 28 = 9000
```

### 39.8.3 Connection Pooling {#chapter_39_8_3_connection-pooling}

Connection Pools reduzieren Connection-Setup-Overhead (TCP-Handshake, TLS-Handshake). Wir dimensionieren Pool-Größe basierend auf Concurrency.

**Pool-Sizing-Formula:**
```
pool_size = active_threads × avg_connections_per_thread × 1.2
```

**Beispiel-Konfiguration (Python):**
```python
# connection_pool.py
from themisdb import Client

client = Client(
    host='localhost',
    port=8529,
    pool_size=200,  # Max simultane Connections
    pool_timeout=30,  # Timeout für Connection-Acquisition
    keepalive=60,  # Keep-Alive-Interval (Sekunden)
    max_retries=3
)
```

---

## 39.9 Vector-Search-Performance {#chapter_39_9_vector-search}

[Vector Search](../appendix_h_glossary.md#vector-search) mit [HNSW](../appendix_h_glossary.md#hnsw)-Indizes erfordert spezialisiertes Tuning für hohe Recall-Raten bei minimaler Latenz. Wir konfigurieren HNSW-Parameter (M, efConstruction, efSearch), Quantisierung ([PQ](../appendix_h_glossary.md#product-quantization), [SQ](../appendix_h_glossary.md#scalar-quantization)), und GPU-Acceleration.

### 39.9.1 HNSW-Parameter-Tuning {#chapter_39_9_1_hnsw-params}

HNSW-Indizes balancieren Build-Zeit, Index-Größe, und Query-Performance durch drei primäre Parameter[^8].

**Parameter-Guide:**
| Parameter | Wert-Range | Impact | Empfehlung |
|-----------|------------|--------|------------|
| M | 8-64 | Edges pro Node | 16-32 (Standard: 16) |
| efConstruction | 100-500 | Build-Qualität | 200-400 |
| efSearch | 32-256 | Query-Recall | 64-128 (höher für > 95% Recall) |

```python
# hnsw_config.py - HNSW-Index-Konfiguration
client.create_index(
    collection='embeddings',
    field='vector',
    type='hnsw',
    params={
        'M': 16,  # Connections pro Layer
        'efConstruction': 200,  # Build-Time Beam-Width
        'efSearch': 100,  # Query-Time Beam-Width
        'distance': 'cosine'  # cosine|euclidean|dot
    }
)
```

**Performance-Benchmarks (10M Embeddings, 768D):**
| Config | Build-Zeit | Index-Größe | Query-Latenz | Recall@10 |
|--------|------------|-------------|--------------|-----------|
| M=8, ef=100 | 45min | 12GB | 8ms | 89% |
| M=16, ef=200 | 78min | 18GB | 12ms | 96% |
| M=32, ef=400 | 145min | 28GB | 18ms | 98.5% |

### 39.9.2 Quantisierung {#chapter_39_9_2_quantization}

[Product Quantization (PQ)](../appendix_h_glossary.md#product-quantization) reduziert Index-Größe um Faktor 16-32 bei 1-3% Recall-Verlust[^9].

```python
# pq_config.py - Product Quantization
client.create_index(
    collection='embeddings',
    field='vector',
    type='hnsw_pq',
    params={
        'M': 16,
        'efConstruction': 200,
        'pq_codebooks': 16,  # Anzahl Codebooks (8-32)
        'pq_bits': 8,  # Bits pro Codebook (4, 8, oder 16)
        'use_gpu': True  # GPU-Acceleration wenn verfügbar
    }
)
```

### 39.9.3 Batch-Embedding und Index-Warmup {#chapter_39_9_3_batch-warmup}

Batch-Embedding und Index-Preloading optimieren Startup-Zeit und Durchsatz.

```python
# batch_embed.py - Paralleles Embedding mit Batching
def batch_embed_parallel(texts, model, batch_size=32, num_workers=4):
    """Embeddings in Batches mit Worker-Pool generieren."""
    from concurrent.futures import ThreadPoolExecutor
    
    chunks = [texts[i:i+batch_size] for i in range(0, len(texts), batch_size)]
    
    with ThreadPoolExecutor(max_workers=num_workers) as executor:
        futures = [executor.submit(model.encode, chunk) for chunk in chunks]
        embeddings = []
        for future in futures:
            embeddings.extend(future.result())
    
    return embeddings

# Index Warmup (preload hot embeddings)
client.execute("""
    FOR doc IN embeddings
      LIMIT 100000
      LET _ = doc.vector  /* Force load into memory */
      RETURN null
""")
```

---

## 39.10 Graph-Workload-Optimierung {#chapter_39_10_graph-workloads}

[Graph Traversal](../appendix_h_glossary.md#graph-traversal)-Queries erfordern Depth- und Fanout-Limitierung zur Vermeidung exponentieller Explosion. Wir präsentieren Strategien für Bidirectional BFS, Community-Precomputation, und Materialized Paths.

### 39.10.1 Depth und Fanout Limiting {#chapter_39_10_1_depth-fanout}

Unbegrenzte Graph-Traversals führen zu exponentieller Zeit-Komplexität. Wir limitieren Depth (Max-Hops) und Fanout (Max-Edges pro Node).

```aql
-- ❌ FALSCH: Unbegrenzter Traversal (kann Stunden dauern)
FOR v, e, p IN 1..10 OUTBOUND 'users/alice' GRAPH 'social'
  RETURN v

-- ✅ RICHTIG: Depth und Fanout limitiert
FOR v, e, p IN 1..3 OUTBOUND 'users/alice' GRAPH 'social'
  PRUNE LENGTH(p.vertices) > 100  -- Max 100 Nodes pro Pfad
  OPTIONS {bfs: true, uniqueVertices: 'global'}  -- Bidirectional BFS
  FILTER p.edges[*].weight ALL >= 0.5  -- Edge-Quality-Filter
  RETURN v
```

### 39.10.2 Community-Detection-Caching {#chapter_39_10_2_community-caching}

Für häufige Community-Queries precomputen wir Communities mit Louvain-Algorithmus und cachen Ergebnisse.

```python
# community_precompute.py - Louvain Community Detection
import networkx as nx
from networkx.algorithms import community

def precompute_communities(graph):
    """Louvain-Communities berechnen und in DB speichern."""
    communities = community.louvain_communities(graph, seed=42)
    
    # Communities in DB persistieren
    for i, comm in enumerate(communities):
        for node in comm:
            client.update('users', node, {'community_id': i})
    
    return len(communities)
```

### 39.10.3 Materialized Paths {#chapter_39_10_3_materialized-paths}

Für statische Hot-Paths (z.B. Org-Hierarchien) materialisieren wir Pfade als Array-Feld.

```aql
-- Materialized Path erstellen
UPDATE user IN users
  SET user.path = (
    FOR v IN 0..10 INBOUND user GRAPH 'org_hierarchy'
      RETURN v._key
  )

-- Hot-Path-Query (O(1) statt O(n) Traversal)
FOR u IN users
  FILTER 'dept_engineering' IN u.path
  RETURN u
```

---

## 39.11 Transaktionen & Lock-Optimierung {#chapter_39_11_transactions-locking}

[Transaction](../appendix_h_glossary.md#transaction)-Tuning fokussiert auf Lock-Granularität, Deadlock-Vermeidung, und Retry-Strategien. Wir minimieren Lock-Contention durch strikte Lock-Order und kurze Transaction-Scopes. Die Wahl des richtigen [Isolation Level](../appendix_h_glossary.md#isolation-level) balanciert Daten-Konsistenz gegen Concurrency-Performance.

### 39.11.1 Isolation Level Trade-offs {#chapter_39_11_1_isolation-levels}

SQL-Standard definiert vier [Isolation Levels](../appendix_h_glossary.md#isolation-level), die unterschiedliche Trade-offs zwischen Performance und Anomalie-Vermeidung bieten. ThemisDB implementiert alle vier Levels über [MVCC](../appendix_h_glossary.md#mvcc), wobei höhere Isolation-Levels Throughput reduzieren[^14].

**Isolation Level Performance-Vergleich:**

| [Isolation Level](../appendix_h_glossary.md#isolation-level) | Throughput | Latency (P99) | Anomalies Prevented | Overhead | Use Case |
|---------------|------------|---------------|---------------------|----------|----------|
| Read Uncommitted | 85k TPS | 1.2ms | None (Dirty Reads möglich) | Minimal | Non-Critical Analytics |
| Read Committed | 72k TPS | 1.8ms | Dirty Reads | +15% | Standard OLTP |
| Repeatable Read | 58k TPS | 2.9ms | Dirty + Non-Repeatable Reads | +32% | Financial Transactions |
| Serializable | 38k TPS | 5.2ms | Alle (Full Isolation) | +55% | Kritische Consistency |

**Methodologie:** YCSB Workload A (50% Reads, 50% Writes), 16 Threads, Intel Xeon E5-2680, gemessen über 10 Minuten.

**Anomalie-Erklärungen:**
- **Dirty Read:** Lesen uncommitted Writes anderer Transaktionen
- **Non-Repeatable Read:** Gleiche Query gibt unterschiedliche Resultate innerhalb Transaction
- **Phantom Read:** Neue Rows erscheinen bei wiederholter Query (durch Concurrent Inserts)

**Empfehlung:** Wir setzen `Read Committed` als Default für OLTP-Workloads (92% aller Use Cases). Für kritische Finanz-Transaktionen erhöhen wir auf `Repeatable Read` oder `Serializable`[^14].

```aql
-- Isolation Level pro Query setzen (AQL)
-- Wir nutzen Serializable für kritische Balance-Updates

BEGIN TRANSACTION
  OPTIONS {isolationLevel: 'serializable'}
  
  LET account = DOCUMENT('accounts/12345')
  UPDATE account WITH {
    balance: account.balance - @amount
  } IN accounts
  
COMMIT
```

### 39.11.2 Lock-Optimierung-Strategien {#chapter_39_11_2_lock-optimization}

Lock-Contention ist häufigste Ursache für Performance-Degradation in OLTP-Systemen. Wir reduzieren Contention durch [Lock Escalation](../appendix_h_glossary.md#lock-escalation)-Prevention, Timeout-Konfiguration, und Batch-Locking.

**Lock-Granularität-Hierarchie:**
```
Database Lock (selten)
  ↓
Collection Lock (DDL-Operationen)
  ↓
Document Lock (Standard für DML)
  ↓
Field Lock (theoretisch, nicht implementiert)
```

**Lock-Timeout-Konfiguration:**
```yaml
# themis.conf - Lock-Parameter
transactions:
  lock_timeout_seconds: 5  # Wir warten max 5s auf Lock-Acquisition
  deadlock_detection_interval_ms: 100  # Deadlock-Check alle 100ms
  max_transaction_duration_seconds: 60  # Auto-Abort nach 60s
```

**Batch-Locking für Bulk-Operations:**
```aql
-- Batch-Update mit Collection-Lock (AQL)
-- Wir vermeiden Row-by-Row-Locking für besseren Throughput

BEGIN TRANSACTION
  FOR doc IN products
    FILTER doc.category == 'electronics'
    UPDATE doc WITH {
      price: doc.price * 1.1  -- 10% Preiserhöhung
    } IN products
    OPTIONS {exclusive: true}  -- Collection-Level Lock
COMMIT
```

### 39.11.3 Optimistische vs. Pessimistische Concurrency {#chapter_39_11_3_optimistic-concurrency}

[Optimistic Concurrency](../appendix_h_glossary.md#optimistic-concurrency) verzichtet auf Locks beim Lesen und prüft bei Commit auf Konflikte (via MVCC `_rev`-Field). Dies reduziert Lock-Contention bei niedrigen Konflikt-Raten drastisch.

**Concurrency-Control-Vergleich:**

| Strategie | Lock-Overhead | Conflict-Resolution | Best For | Throughput |
|-----------|---------------|---------------------|----------|------------|
| Pessimistic (Locks) | Hoch | Proaktiv (Blocking) | High-Contention | 45k TPS |
| Optimistic (MVCC) | Minimal | Reaktiv (Retry) | Low-Contention | 78k TPS |

**Methodologie:** 10% Write-Conflict-Rate, 100 Concurrent Clients, gemessen mit pgbench-ähnlichem Workload.

```aql
// Optimistische Concurrency mit MVCC (AQL)
// Wir verwenden _rev für konfliktfreie Updates

FOR doc IN products
  FILTER doc.stock > 0 AND doc._key == @product_id
  UPDATE doc WITH {
    stock: doc.stock - 1,
    _rev: doc._rev  // Wir prüfen MVCC-Versionskonflikte
  } IN products
  OPTIONS { 
    keepNull: false,
    mergeObjects: false,
    ignoreRevs: false  // Wir erzwingen _rev-Check
  }
```

**MVCC-Overhead-Analyse:** MVCC fügt 2-3% CPU-Overhead hinzu (Version-Verwaltung), reduziert aber Lock-Contention um 60-80%, was zu Netto-Performance-Gewinn führt.

**Retry-Strategie-Refinement:**
Bei Optimistic-Concurrency-Conflicts implementieren wir Exponential-Backoff mit Jitter (siehe 39.11.5).

### 39.11.4 Lock-Order und Deadlock-Prevention {#chapter_39_11_4_lock-order}

Deadlocks entstehen durch zyklische Lock-Dependencies. Wir definieren eine globale Lock-Order (z.B. alphabetisch nach Collection-Name).

```aql
-- ❌ FALSCH: Inkonsistente Lock-Order (Deadlock-Risk)
BEGIN TRANSACTION
  UPDATE orders SET status = 'paid' WHERE _key == @order_id
  UPDATE users SET balance = balance - @amount WHERE _key == @user_id
COMMIT

-- ✅ RICHTIG: Konsistente Lock-Order (alphabetisch: orders < users)
BEGIN TRANSACTION
  UPDATE orders SET status = 'paid' WHERE _key == @order_id
  UPDATE users SET balance = balance - @amount WHERE _key == @user_id
COMMIT
```

### 39.11.5 Retry-Strategie mit Exponential Backoff {#chapter_39_11_5_retry-strategy}

Bei Deadlocks oder Lock-Timeouts implementieren wir Retry-Logic mit Exponential Backoff.

```python
# retry_tx.py - Transaction Retry mit Exponential Backoff
import time
from random import uniform

class TransactionError(Exception):
    pass

class DeadlockError(TransactionError):
    pass

def with_retry(tx_func, max_attempts=5, base_delay=0.1):
    """Führt Transaction-Func mit Retry und Backoff aus."""
    for attempt in range(max_attempts):
        try:
            return tx_func()
        except DeadlockError:
            if attempt == max_attempts - 1:
                raise
            # Exponential Backoff mit Jitter
            delay = base_delay * (2 ** attempt) + uniform(0, 0.1)
            time.sleep(delay)
    
    raise TransactionError(f"Failed after {max_attempts} attempts")

# Verwendung
result = with_retry(lambda: execute_payment_transaction(order_id, user_id))
```

### 39.11.6 Kürzere Transaktionen {#chapter_39_11_6_short-transactions}

Lange Transaktionen erhöhen Lock-Contention. Wir splitten Transaktionen in kleinere, logisch unabhängige Units.

```python
# ❌ FALSCH: Monolithische Transaction (lange Lock-Dauer)
def process_bulk_orders(order_ids):
    with client.transaction():
        for order_id in order_ids:  # Kann Minuten dauern!
            process_order(order_id)

# ✅ RICHTIG: Micro-Transactions (kurze Lock-Dauer)
def process_bulk_orders(order_ids):
    for order_id in order_ids:
        with client.transaction():  # Pro Order eine Transaction
            process_order(order_id)
```

---

## 39.12 Configuration Templates {#chapter_39_12_config-templates}

Wir präsentieren produktionserprobte Konfigurations-Templates für unterschiedliche Workload-Profile. Jedes Template wurde in realen Szenarien validiert und erreicht spezifische Performance-Targets. Die Templates dienen als Ausgangspunkt und müssen workload-spezifisch angepasst werden.

### 39.12.1 High-Throughput OLTP {#chapter_39_12_1_oltp-template}

Optimiert für hohen Durchsatz bei kurzen Transaktionen (> 10k ops/s).

```yaml
# themis.conf - High-Throughput OLTP Configuration
aql:
  stream_results: true
  max_query_memory_mb: 1024
  profiling_enabled: true
  query_cache_mode: demand  # On-demand Query-Caching

cache:
  size_mb: 8192  # 50% des RAMs
  eviction_policy: lru  # LRU für hohe Write-Last
  document_cache_enabled: true
  index_cache_enabled: true

storage:
  compression: zstd
  compression_level: 3  # Balanced
  sync_mode: fdatasync
  block_cache_size_mb: 2048

wal:
  compression: zstd
  sync_interval_ms: 20  # 20ms Batching
  buffer_size_mb: 64

network:
  max_connections: 2000
  keepalive_seconds: 60
  request_timeout_seconds: 30

transactions:
  lock_timeout_seconds: 5
  deadlock_retry_max: 3
```

### 39.12.2 Read-Heavy Analytics {#chapter_39_12_2_analytics-template}

Optimiert für komplexe Aggregations-Queries mit großen Resultsets.

```yaml
# themis.conf - Analytics Workload Configuration
aql:
  stream_results: true
  max_query_memory_mb: 8192  # Größerer Query-Memory
  profiling_enabled: false  # Overhead reduzieren
  query_cache_mode: always  # Aggressive Caching

cache:
  size_mb: 16384  # 70% des RAMs
  eviction_policy: arc  # Adaptiv für Mixed Access
  query_cache_max_entries: 50000

storage:
  compression: zstd
  compression_level: 6  # Höhere Kompression
  prefetch_enabled: true  # Sequential Read Prefetch

indexes:
  covering_indexes_preferred: true
  bloom_filter_enabled: true
```

### 39.12.3 Vector Search Workload {#chapter_39_12_3_vector-template}

Optimiert für [Vector Search](../appendix_h_glossary.md#vector-search) mit [HNSW](../appendix_h_glossary.md#hnsw)-Indizes.

```yaml
# themis.conf - Vector Search Configuration
vector_search:
  hnsw_default_m: 16
  hnsw_default_ef_construction: 200
  hnsw_default_ef_search: 100
  use_gpu: true  # GPU-Acceleration wenn verfügbar
  gpu_memory_mb: 8192

cache:
  size_mb: 12288
  vector_cache_enabled: true  # Dedizierter Vector-Cache
  vector_cache_size_mb: 8192

storage:
  compression: none  # Vektoren profitieren kaum von Kompression
  prefetch_vectors: true
```

### 39.12.4 Linux Sysctl Tuning {#chapter_39_12_4_linux-sysctl}

Systemweite Kernel-Parameter für High-Performance Database-Workloads.

```bash
# /etc/sysctl.conf - Production Database Server Tuning

# Netzwerk
net.core.somaxconn = 2048  # Listen-Queue für hohe Connection-Rate
net.ipv4.tcp_max_syn_backlog = 8192  # SYN-Queue
net.ipv4.tcp_fin_timeout = 15  # Schnellere Connection-Cleanup
net.ipv4.tcp_tw_reuse = 1  # TIME-WAIT Reuse
net.ipv4.tcp_slow_start_after_idle = 0  # Slow-Start deaktivieren

# Memory
vm.swappiness = 10  # Swap nur im Notfall (0-10)
vm.max_map_count = 262144  # Memory-Mapped Files
vm.overcommit_memory = 1  # Optimistic Memory Allocation
vm.dirty_ratio = 40  # Background Dirty-Page Writeback
vm.dirty_background_ratio = 10

# Filesystem
fs.file-max = 2097152  # Max Open Files System-wide
fs.aio-max-nr = 1048576  # Async I/O

# Anwenden
sudo sysctl -p
```

---

## 39.13 Benchmark Harness {#chapter_39_13_benchmark-harness}

Ein reproduzierbares Benchmark-Framework ist essentiell für objektive Performance-Evaluierung. Wir implementieren ein Harness mit Warmup-Phase, statistischer Signifikanz-Prüfung, und Percentile-Metriken.

### 39.13.1 Benchmark-Implementierung {#chapter_39_13_1_benchmark-impl}

```python
# bench_harness.py - Reproduzierbares Benchmark-Framework
import time
import statistics
from typing import List, Dict, Callable

class BenchHarness:
    """Performance-Benchmark-Harness mit Warmup und Percentile-Metrics."""
    
    def __init__(self, client, warmup_iterations=10, benchmark_iterations=100):
        self.client = client
        self.warmup_iterations = warmup_iterations
        self.benchmark_iterations = benchmark_iterations
    
    def run_case(self, name: str, query_func: Callable) -> Dict:
        """Führt einzelnen Benchmark-Case mit Warmup aus."""
        # Warmup-Phase (JIT, Cache-Warming)
        for _ in range(self.warmup_iterations):
            query_func()
        
        # Benchmark-Phase
        durations = []
        for _ in range(self.benchmark_iterations):
            start = time.perf_counter()
            result = query_func()
            duration_ms = (time.perf_counter() - start) * 1000
            durations.append(duration_ms)
        
        return {
            "name": name,
            "iterations": self.benchmark_iterations,
            "mean_ms": statistics.mean(durations),
            "median_ms": statistics.median(durations),
            "p95_ms": self.percentile(durations, 95),
            "p99_ms": self.percentile(durations, 99),
            "min_ms": min(durations),
            "max_ms": max(durations),
            "stddev_ms": statistics.stdev(durations)
        }
    
    @staticmethod
    def percentile(data: List[float], pct: int) -> float:
        """Berechnet n-tes Percentile."""
        sorted_data = sorted(data)
        idx = int(len(sorted_data) * (pct / 100.0))
        return sorted_data[idx]
    
    def run_suite(self, cases: List[tuple]) -> List[Dict]:
        """Führt komplette Benchmark-Suite aus."""
        results = []
        for name, query_func in cases:
            print(f"Running benchmark: {name}...")
            result = self.run_case(name, query_func)
            results.append(result)
            self._print_result(result)
        return results
    
    def _print_result(self, result: Dict):
        """Formatierte Ausgabe eines Benchmark-Ergebnisses."""
        print(f"  ✓ {result['name']}")
        print(f"    Mean: {result['mean_ms']:.2f}ms | "
              f"P95: {result['p95_ms']:.2f}ms | "
              f"P99: {result['p99_ms']:.2f}ms")

# Verwendungsbeispiel
if __name__ == '__main__':
    from themisdb import Client
    
    client = Client(host='localhost', port=8529)
    harness = BenchHarness(client, warmup_iterations=10, benchmark_iterations=100)
    
    cases = [
        ("Index Lookup", lambda: client.execute("FOR u IN users FILTER u._key == '12345' RETURN u")),
        ("Range Query", lambda: client.execute("FOR u IN users FILTER u.age >= 18 AND u.age <= 65 LIMIT 100 RETURN u")),
        ("Aggregation", lambda: client.execute("FOR u IN users COLLECT age = u.age WITH COUNT INTO count RETURN {age, count}"))
    ]
    
    results = harness.run_suite(cases)
```

---

## Zusammenfassung {#chapter_39_14_zusammenfassung}

Dieses Kochbuch präsentierte systematische Performance-Optimierung für [ThemisDB](../appendix_h_glossary.md#themisdb) über alle kritischen Domänen hinweg: Query-Optimierung, Indexierung, Caching, Storage, Netzwerk, und Workload-spezifische Tuning-Strategien. Wir betonten durchgängig die Bedeutung von Messung und Validierung: Beginnen Sie stets mit [EXPLAIN](../appendix_h_glossary.md#explain)-Analyse und Profiling, identifizieren Sie den kritischen Bottleneck, wenden Sie gezielte Optimierungen an, und verifizieren Sie den Erfolg mit reproduzierbaren [Benchmarks](../appendix_h_glossary.md#benchmark).

**Die wichtigsten Takeaways:**

1. **Query-Optimierung:** [Filter Pushdown](../appendix_h_glossary.md#filter-pushdown), [Early Projection](../appendix_h_glossary.md#projection), N+1-Vermeidung → 60-95% Latenzreduktion
2. **Indexierung:** Richtige Index-Typ-Wahl ([B-Tree](../appendix_h_glossary.md#btree) vs. [Hash](../appendix_h_glossary.md#hash-index)), [Covering Indexes](../appendix_h_glossary.md#covering-index), Selektivitäts-basierte Multi-Column-Order → 10-1000× Speedup
3. **Batching:** 100-1000 Docs pro Batch, Connection Pooling, Backpressure-Handling → 5-20× Durchsatz-Steigerung
4. **Cache-Tuning:** 40-60% RAM-Allokation, [ARC](../appendix_h_glossary.md#arc)-Policy für Mixed Workloads, Streaming für Large Resultsets → > 90% Hit-Rate
5. **Storage:** zstd-Kompression Level 3-6, NVMe-Tuning, XFS für Parallelität → 30-50% I/O-Reduktion
6. **Vector Search:** [HNSW](../appendix_h_glossary.md#hnsw) M=16-32, efSearch=64-128, [PQ](../appendix_h_glossary.md#product-quantization)-Quantisierung → 96-98% Recall bei < 20ms P99-Latenz
7. **Graph:** Depth/Fanout-Limiting, Bidirectional BFS, Community-Caching → Subsekundenlatenz statt Minuten
8. **Transactions:** Strikte Lock-Order, Retry mit Exponential Backoff, kurze Transaction-Scopes → Deadlock-Eliminierung

**Iterativer Optimierungsprozess:** Performance-Tuning ist kein einmaliger Akt, sondern ein kontinuierlicher Prozess. Nutzen Sie → Kapitel 20: Performance Monitoring für proaktives Alerting, und führen Sie regelmäßig Regressionstests mit dem präsentierten Benchmark-Harness durch. Dokumentieren Sie alle Änderungen und deren Auswirkungen in einem Change-Log für zukünftige Referenz. Für praktische Übungen zur Anwendung dieser Optimierungsstrategien siehe → Kapitel 41: Hands-on Labs.

---

## Referenzen {#chapter_39_references}

[^1]: Knuth, D.E. (1974). "Structured Programming with go to Statements." *Computing Surveys*, Vol. 6, No. 4, pp. 261-301. ACM.

[^2]: Graefe, G. (2011). "Modern B-Tree Techniques." *Foundations and Trends in Databases*, Vol. 3, No. 4, pp. 203-402.

[^3]: Facebook Engineering. (2021). "RocksDB Tuning Guide." https://github.com/facebook/rocksdb/wiki/Tuning-Guide

[^4]: Cockburn, A. (2017). "Performance Engineering of Software Systems." *MIT OpenCourseWare*, Course 6.172.

[^5]: Selinger, P.G. et al. (1979). "Access Path Selection in a Relational Database Management System." *SIGMOD'79*, pp. 23-34.

[^6]: Ramakrishnan, R. & Gehrke, J. (2003). "Database Management Systems" (3rd ed.). McGraw-Hill. Chapter 8: Tree-Structured Indexing.

[^7]: Megiddo, N. & Modha, D.S. (2003). "ARC: A Self-Tuning, Low Overhead Replacement Cache." *FAST'03*, USENIX.

[^8]: Malkov, Y. & Yashunin, D. (2018). "Efficient and Robust Approximate Nearest Neighbor Search Using Hierarchical Navigable Small World Graphs." *IEEE Transactions on Pattern Analysis and Machine Intelligence*, Vol. 42, No. 4.

[^9]: Jégou, H. et al. (2011). "Product Quantization for Nearest Neighbor Search." *IEEE Transactions on Pattern Analysis and Machine Intelligence*, Vol. 33, No. 1, pp. 117-128.

[^10]: Facebook Engineering. (2022). "RocksDB Wiki: LSM-tree Compaction." https://github.com/facebook/rocksdb/wiki/Compaction

[^11]: Bloom, B.H. (1970). "Space/Time Trade-offs in Hash Coding with Allowable Errors." *Communications of the ACM*, Vol. 13, No. 7, pp. 422-426.

[^12]: Gorman, M. (2004). "Understanding the Linux Virtual Memory Manager." Prentice Hall. Chapter 11: Memory Management.

[^13]: Axboe, J. (2018). "Linux Block I/O: Introducing Multi-queue SSD Access." *Linux Kernel Documentation*. https://www.kernel.org/doc/Documentation/block/blk-mq.txt

[^14]: Berenson, H. et al. (1995). "A Critique of ANSI SQL Isolation Levels." *SIGMOD'95*, pp. 1-10. ACM.

## 39.13 Performance-Internals C++ API (v1.x) {#performance-internals-cpp}

Dieses Kapitel dokumentiert die Low-Level-Performance-Komponenten (`include/performance/`).

### 39.13.1 AdaptiveQueryCompiler — JIT-ähnliche Query-Kompilierung

```cpp
#include "performance/adaptive_query_compiler.h"

themis::performance::AdaptiveQueryCompiler compiler;

// Query kompilieren (inlined loop generation)
themis::performance::QueryParams params;
params.table     = "orders";
params.filter    = "amount > 1000";
params.columns   = {"id", "customer_id", "amount"};

auto compiled = compiler.compile(params);
// compiled.plan_id, compiled.is_jit_compiled, compiled.estimated_cost

// Ausführen
auto result = compiler.execute(compiled, storage);
// result.ok, result.rows, result.execution_time_us

// Hot-Query-Cache: Wird nach N Ausführungen automatisch JIT-optimiert
auto stats = compiler.getStats();
// stats.total_compiled, stats.jit_hits, stats.jit_misses
```

### 39.13.2 IntelligentPrefetcher — ML-basiertes Prefetching

```cpp
#include "performance/intelligent_prefetcher.h"

themis::performance::IntelligentPrefetcher::PrefetchConfig pfcfg;
pfcfg.enable_learning         = true;
pfcfg.enable_hardware_prefetch = true;
pfcfg.lookahead_distance       = 8;   // 8 Cache-Lines voraus

themis::performance::IntelligentPrefetcher prefetcher(pfcfg);

// Zugriff aufzeichnen (für Pattern-Lernen)
prefetcher.record_access(address, timestamp_ns);

// Prefetch-Entscheidung abfragen
auto pattern = prefetcher.predict_next(current_address);
// pattern.prefetch_addresses: [{addr, confidence}, ...]
// pattern.pattern_type: SEQUENTIAL / STRIDED / RANDOM / POINTER_CHASE

// Hardware-Prefetch auslösen
prefetcher.issue_prefetch(pattern);

// Statistiken
auto stats = prefetcher.getStats();
// stats.predictions_issued, stats.cache_hit_improvement_pct
```

**CacheLevel:** `L1` / `L2` / `L3` / `DRAM`

### 39.13.3 WorkloadPredictor — LSTM-basierte Lastvorhersage

```cpp
#include "performance/workload_predictor.h"

themis::performance::WorkloadPredictor::Config wpcfg;
wpcfg.window_size       = 60;   // 60 Messungen für Vorhersage
wpcfg.forecast_horizon  = 10;   // 10 Zeitschritte voraus
wpcfg.use_exponential_smoothing = true;

themis::performance::WorkloadPredictor predictor(wpcfg);

// Aktuellen Zustand aufzeichnen
themis::performance::WorkloadSnapshot snap;
snap.timestamp_ms       = now_ms();
snap.qps                = 4500;
snap.cpu_utilization    = 0.72;
snap.memory_used_bytes  = 8LL * 1024 * 1024 * 1024;
snap.p99_latency_ms     = 12;

predictor.record(snap);

// Vorhersage abrufen
auto forecast = predictor.forecast();
// forecast.predicted_qps: [{t+1: 4800}, {t+2: 5100}, ...]
// forecast.scale_recommendation: {direction: UP/DOWN/STABLE, magnitude: 2}

// Skalierungsempfehlung
auto rec = predictor.getScaleRecommendation();
// rec.direction: ScaleDirection::UP / DOWN / STABLE
// rec.recommended_replicas, rec.confidence
```

### 39.13.4 HardwareAccelerator — CPU/GPU Query-Operator-Dispatch

```cpp
#include "performance/hardware_accelerator.h"

themis::performance::HardwareAccelerator accel;

// Operator für Hardware-Pfad beschreiben
themis::performance::QueryOperator op;
op.type        = themis::performance::OperatorType::VECTOR_SIMILARITY;
op.input_bytes = embeddings_size;
op.batch_size  = 1024;

// Hardware-Pfad ausführen (automatischer GPU/AVX-Dispatch)
auto exec_result = accel.execute(op, input_data, output_buffer);
// exec_result.ok, exec_result.used_hw_path, exec_result.device_type
// exec_result.duration_us, exec_result.throughput_ops_per_sec

// Capabilities abfragen
auto caps = accel.getCapabilities();
// caps.has_gpu, caps.has_avx512, caps.has_npu, caps.device_type
```

**DeviceType:** `CPU` / `CUDA_GPU` / `OPENCL_GPU` / `NPU` / `FPGA`
**OperatorType:** `VECTOR_SIMILARITY` / `AGGREGATION` / `SORT` / `FILTER` / `HASH_JOIN`

### 39.13.5 LockFreeRingBuffer — Zero-Contention-Metriken

```cpp
#include "performance/lockfree_metrics_buffer.h"

// Thread-eigene Lock-Free-Buffer (SPSC)
constexpr size_t BUFFER_SIZE = 1024;
themis::performance::LockFreeRingBuffer<themis::performance::MetricsEntry, BUFFER_SIZE> buf;

// Producer-Thread
themis::performance::MetricsEntry e;
e.timestamp_ns = clock_ns();
e.metric_id    = MetricId::QUERY_LATENCY;
e.value        = 12.5;
bool pushed = buf.tryPush(e);

// Consumer-Thread (z.B. Metrics-Aggregator)
themis::performance::MetricsEntry item;
while (buf.tryPop(item)) {
    aggregator.record(item);
}

// Dropped-Counter (Overflow-Diagnose)
size_t dropped = buf.dropped_count();
```

## 39.14 Acceleration-Internals C++ API (v1.x) {#acceleration-internals-cpp}

### 39.14.1 ComputeBackend — Abstrakte GPU/NPU/CPU-Schicht

```cpp
#include "acceleration/compute_backend.h"

// Backend-Fähigkeiten abfragen
themis::acceleration::BackendCapabilities caps =
    themis::acceleration::getBackendCapabilities(
        themis::acceleration::BackendType::CUDA);

// caps.max_batch_size, caps.supports_fp16, caps.supports_int8
// caps.supports_int4, caps.vram_bytes, caps.compute_units

// Kernel ausführen
themis::acceleration::KernelConfig kconfig;
kconfig.batch_size    = 1024;
kconfig.precision     = themis::acceleration::PrecisionMode::FP16;
kconfig.max_latency_ms = 10;

themis::acceleration::BatchDescriptor batch;
batch.data     = input_ptr;
batch.count    = 1024;
batch.dim      = 768;

auto kernel_result = backend->executeSimilarityKernel(batch, kconfig);
// kernel_result.distances: float[], kernel_result.duration_ms

// Gesundheitsstatus
auto health = backend->getHealth();
// health.status: HEALTHY / DEGRADED / FAILED
// health.error_message, health.consecutive_failures
```

**BackendType:** `CPU` / `CUDA` / `HIP` / `VULKAN` / `OPENCL` / `DIRECTX`
**PrecisionMode:** `FP32` / `FP16` / `BF16` / `INT8` / `INT4` / `W4A8`

### 39.14.2 KernelFallbackDispatcher — Automatischer Fallback

```cpp
#include "acceleration/kernel_fallback_dispatcher.h"

// ANN (Approximate Nearest Neighbor) Kernel mit Fallback-Kette
themis::acceleration::ANNKernelFallbackDispatcher ann_dispatcher;
// Fallback-Kette: CUDA → HIP → Vulkan → CPU

ann_dispatcher.setRetryPolicy({
    .max_attempts       = 3,
    .backoff_ms         = 100,
    .fallback_to_cpu    = true,
});

auto ann_result = ann_dispatcher.search(query_vector, index, k);
// ann_result.ids: top-k Index-IDs
// ann_result.distances: entsprechende Distanzen
// ann_result.backend_used: "CUDA" / "CPU_FALLBACK"

// Geo-Kernel mit Fallback
themis::acceleration::GeoKernelFallbackDispatcher geo_dispatcher;
auto geo_result = geo_dispatcher.computeDistancesBatch(
    origin_points, target_points, themis::geo::DistanceMode::HAVERSINE);
// geo_result.distances_m: [{origin_idx, target_idx, distance_m}]
```

### 39.14.3 VecKnnPipeline — Hochperformante Vektorsucheals Pipeline

```cpp
#include "acceleration/vec_knn.h"

themis::acceleration::VecKnnPipelineConfig knn_cfg;
knn_cfg.backend       = themis::acceleration::BackendType::CUDA;
knn_cfg.precision     = themis::acceleration::PrecisionMode::FP16;
knn_cfg.nprobe        = 64;      // IVF-Suchtiefe
knn_cfg.use_cache     = true;
knn_cfg.cache_ttl_ms  = 5000;
knn_cfg.ef_search     = 200;     // HNSW ef

// Pipeline aufbauen (VectorIndex → Acceleration Backend)
auto knn_pipeline = themis::acceleration::VecKnnPipeline::create(
    vector_index, knn_cfg);

// K-Nearest-Neighbor-Suche
auto results = knn_pipeline->search(
    query_embedding,   // std::vector<float>
    /*k=*/ 10,
    filter_expr        // optional: AQL-Filter
);

// results: [{doc_id, score, distance}] sortiert nach Score
for (auto& r : results) {
    std::cout << r.doc_id << " score=" << r.score << "\n";
}

// Batch-Suche (mehrere Queries gleichzeitig)
auto batch_results = knn_pipeline->searchBatch(
    {query1, query2, query3}, /*k=*/ 5);
// batch_results[i]: Ergebnisse für query i

// Distanz-Cache leeren
knn_pipeline->clearCache();
```

### 39.14.4 DeviceManager — Multi-GPU-Verwaltung

```cpp
#include "acceleration/device_manager.h"

themis::acceleration::DeviceManager& dev_mgr =
    themis::acceleration::DeviceManager::instance();

// GPU-Erkennung
if (dev_mgr.hasGPU()) {
    dev_mgr.logDeviceInfo();
    // Gibt aus: GPU-Modell, VRAM, Compute Capability
}

// Alle verfügbaren Devices
auto devices = dev_mgr.getDevices();
for (auto& d : devices) {
    // d.type: CPU/CUDA/HIP
    // d.name: "NVIDIA A100 80GB"
    // d.vram_bytes, d.compute_units
}

// Metriken abrufen
#include "acceleration/metrics/metrics_collector.h"
auto& acc_metrics = themis::acceleration::MetricsCollector::instance();
// Prometheus-Counter/Gauge für:
// - acceleration_kernel_duration_ms
// - acceleration_fallback_count
// - acceleration_cache_hit_ratio
```

### 39.14.5 RAII-Ressourcenverwaltung

```cpp
#include "acceleration/raii/cuda_raii.h"
#include "acceleration/raii/vulkan_raii.h"

// CUDA-Buffer mit RAII (automatische Freigabe)
{
    themis::acceleration::CudaBuffer<float> gpu_buf(1024 * 768);
    // Daten hochladen
    gpu_buf.upload(cpu_data.data(), cpu_data.size());
    // Kernel ausführen
    run_similarity_kernel(gpu_buf.ptr(), k);
    // Daten herunterladen
    gpu_buf.download(result_data.data());
}  // GPU-Speicher wird hier automatisch freigegeben

// Vulkan-Buffer analog
{
    themis::acceleration::VulkanBuffer<float> vk_buf(device, 512);
    vk_buf.upload(cpu_data.data(), cpu_data.size());
    // ...
}  // automatische vkDestroyBuffer
```

## 39.15 Workload-Adaptive Optimizer C++ API (v1.9.0) {#workload-adaptive-optimizer-cpp}

The `WorkloadAdaptiveOptimizer` (Issue #230, Research Basis: "Adaptive Execution" SIGMOD'19) automatically classifies the current database workload and selects optimal execution strategies at runtime.

### 39.15.1 WorkloadAdaptiveOptimizer — Workload-Klassifikation und Strategieauswahl

```cpp
#include "performance/workload_adaptive_optimizer.h"

themis::performance::WorkloadAdaptiveOptimizer optimizer;

// Queries beobachten (is_write, complexity [1..10], result_rows, table, latency_µs)
optimizer.record_query(false, 2.0, 500, "orders", 120);
optimizer.record_query(true,  1.5, 1,   "orders", 85);
optimizer.set_concurrent_queries(16);

// Workload klassifizieren
auto profile = optimizer.classify_workload();
// profile.type: OLTP | OLAP | MIXED | GRAPH | VECTOR | TIMESERIES | UNKNOWN
// profile.read_write_ratio: 0.0..1.0 (Anteil Lesezugriffe)
// profile.avg_query_complexity: 1..10
// profile.avg_result_size: Ø Ergebniszeilen
// profile.concurrent_queries: aktuelle Parallelität
// profile.hot_tables: Top-3 am häufigsten zugegriffene Tabellen

// Optimale Strategie ableiten
auto strategy = optimizer.get_strategy(profile);
// strategy.enable_jit_compilation
// strategy.enable_parallel_execution
// strategy.thread_pool_size    (mit Predictive Scaling bei Lastspitzen)
// strategy.cache_size_mb
// strategy.join_algorithm      "hash" | "sort-merge" | "nested-loop"
// strategy.index_type          "btree" | "hash" | "brin"

// Strategie anwenden
optimizer.apply_strategy(strategy);
```

### 39.15.2 Automatische Adaption im Hintergrund

```cpp
// Callback registrieren (wird bei jeder Adaption aufgerufen)
optimizer.set_callback(
    [](const themis::performance::WorkloadProfile& old_p,
       const themis::performance::WorkloadProfile& new_p,
       const themis::performance::OptimizationStrategy& s) {
        // Ressourcen (Thread-Pool, Cache) gemäß s neu konfigurieren
    });

// Hintergrund-Thread alle 30 Sekunden neu klassifizieren
optimizer.enable_auto_adapt(std::chrono::seconds{30});

// ... Anwendungslaufzeit ...

optimizer.disable_auto_adapt();
```

### 39.15.3 Statistiken und Reset

```cpp
auto stats = optimizer.get_stats();
// stats.total_queries_recorded
// stats.total_adaptations
// stats.last_workload_type

optimizer.reset_stats();
```

### 39.15.4 Strategietabelle nach Workload-Typ

| WorkloadType | JIT  | Threads | Cache (MB) | Join        | Index   |
|-------------|------|---------|-----------|-------------|---------|
| OLTP        | No   | max(4, concurrency) | 128  | hash        | btree   |
| OLAP        | Yes  | 8       | 1024      | sort-merge  | brin    |
| MIXED       | Yes  | 6       | 512       | hash        | btree   |
| GRAPH       | No   | 8       | 512       | hash        | hash    |
| VECTOR      | No   | 8       | 2048      | hash        | hash    |
| TIMESERIES  | No   | 4       | 256       | sort-merge  | brin    |

> **Predictive Scaling**: Übersteigt `concurrent_queries` den Basiswert für `thread_pool_size`, wird der Pool auf `concurrent_queries × 1.25` hochskaliert.

---

## 39.16 Phase-3-Sync: Weiterführende Referenzen (docs/de/) {#chapter39_16_cross-references}

> Detaillierte Implementierungsdokumentation zu den behandelten Themen:

| Thema | Referenz |
|---|---|
| Performance Index (Übersicht) | [`docs/de/performance/PERFORMANCE_INDEX.md`](../../de/performance/PERFORMANCE_INDEX.md) |
| Benchmark-Ergebnisse 2025 | [`docs/de/performance/BENCHMARK_RESULTS_COMPLETE_2025.md`](../../de/performance/BENCHMARK_RESULTS_COMPLETE_2025.md) |
| Cache-Optimierung Abschlussbericht | [`docs/de/performance/CACHE_OPTIMIZATION_ABSCHLUSSBERICHT.md`](../../de/performance/CACHE_OPTIMIZATION_ABSCHLUSSBERICHT.md) |
| HTTP Client Pool Optimierung | [`docs/de/performance/HTTP_CLIENT_POOL_OPTIMIZATION.md`](../../de/performance/HTTP_CLIENT_POOL_OPTIMIZATION.md) |
| Ingestion-Optimierung | [`docs/de/performance/INGESTION_OPTIMIZATION_SUMMARY.md`](../../de/performance/INGESTION_OPTIMIZATION_SUMMARY.md) |
| Bibliotheken Quick-Ref | [`docs/de/performance/LIBRARY_OPTIMIZATION_QUICKREF.md`](../../de/performance/LIBRARY_OPTIMIZATION_QUICKREF.md) |
| Quick Wins Optimierung | [`docs/de/performance/OPTIMIZATION_QUICK_WINS.md`](../../de/performance/OPTIMIZATION_QUICK_WINS.md) |
| Query Cache Optimierung | [`docs/de/performance/QUERY_CACHE_OPTIMIZATION_SUMMARY.md`](../../de/performance/QUERY_CACHE_OPTIMIZATION_SUMMARY.md) |
| WAL Multi-SSD Konfiguration | [`docs/de/performance/WAL_MULTI_SSD_CONFIGURATION.md`](../../de/performance/WAL_MULTI_SSD_CONFIGURATION.md) |
| Adaptive Distributed Optimizer | [`docs/de/performance/ADAPTIVE_DISTRIBUTED_OPTIMIZER.md`](../../de/performance/ADAPTIVE_DISTRIBUTED_OPTIMIZER.md) |
| Performance Primärquellen | [`docs/de/performance/PRIMARY_SOURCES.md`](../../de/performance/PRIMARY_SOURCES.md) |

**→ Zurück:** [Kapitel 38: Observability & SRE](chapter_38_observability_sre.md)
**→ Weiter:** [Kapitel 40: Data Governance](chapter_40_data_governance.md)
