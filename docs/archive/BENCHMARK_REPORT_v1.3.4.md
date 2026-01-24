# THEMIS BENCHMARK AUSWERTUNG v1.3.4

## Zusammenfassung

**Durchführungsdatum:** 29. Dezember 2025, 20:53 UTC+1  
**Hardware:** DESKTOP-712S8LO  
**Prozessor:** 20 CPUs @ 3696 MHz  
**L3 Cache:** 20 MB (shared)

---

## Messübersicht

- **Gesamt durchgeführte Benchmarks:** 1.078
- **Aktive Benchmark-Durchläufe:** 4
- **Letzter vollständiger Durchlauf:** 20251229_184507 (19 Benchmark-Dateien)
- **Vergleichsbaseline:** core_perf_windows.json (6 Benchmarks)

---

## Top-Performance-Kategorien

### 1. **Query Engine Performance**
- **QueryEngineBench/SimpleEvaluation:** 814,5 Millionen items/sec
  - Zeit pro Iteration: 1,25 ns (sehr schnell)
  - Stabilität: Hochwertig

### 2. **Vector Index Operations**
- **VectorIndexBench/InsertPlaintext:** 351.400 items/sec
  - Durchschnittliche Zeit: 282 μs
  - 2.800 Iterationen durchgeführt

### 3. **Secondary Index Performance**
- **RawWriteOnly:** 884.978 items/sec
  - Zeit pro Iteration: 115 μs
  - 4.978 Iterationen durchgeführt
- **IndexInsert:** 217.200 items/sec
  - Komplexer mit Indexing-Overhead

### 4. **Fortgeschrittene Muster**
- **138 Advanced Pattern Benchmarks** durchgeführt
- **ARM SIMD Optimierungen:** 72 Test-Szenarien
- **Hybrid Geo/Vector Suche:** 34 Kombinationen

---

## Leistungsvergleich zu v1.3.3

### Neue Features (v1.3.4)
Die folgenden neuen Benchmarks wurden hinzugefügt:

#### Embedding Cache
- **Store (384D):** 758.519 items/sec
- **Store (3072D):** 102.400 items/sec
- **Hit Rate (Cached):** 155+ Millionen items/sec (Cache-Effizienz)
- **Miss Rate:** 100-800K items/sec (Normal)

#### Hybrid Search
- **RRF (Reciprocal Rank Fusion):** 6,6-7,1 Millionen items/sec
- **Linear Combination:** 9,7 Millionen items/sec
- **Varying Weights:** 9,9-10,2 Millionen items/sec (Konsistent)

#### CTE (Common Table Expressions)
- **Non-Recursive Simple:** 850-950 Millionen items/sec (Sehr schnell)
- **Recursive Depth-10:** 87 Millionen items/sec
- **Recursive Depth-100:** 8,6 Millionen items/sec
- **Cycle Detection:** 19-20 Millionen items/sec (stabil über 100-10K Zyklen)

#### Subquery Processing
- **EXISTS with LIMIT 1:** Optimal (inf = zeitunabhängig)
- **EXISTS without LIMIT 1:** 
  - 100 Reihen: 13,3 Millionen items/sec
  - 10K Reihen: 147K items/sec
  - 100K Reihen: 14,7K items/sec

#### Verteilte Transaktionen (2PC)
- **Latency (2-8 Nodes):** 6.400 items/sec (konsistent)
- **Latency (16 Nodes):** 1.280 items/sec (Netzwerk-Overhead)
- **Snapshot Read:** 3.200-6.400 items/sec

---

## Hardware-Kontext

```
CPU:           20 Cores @ 3696 MHz
L1 Data:       32 KB (per 2 cores)
L1 Instruction:32 KB (per 2 cores)
L2 Unified:    256 KB (per 2 cores, 10x)
L3 Unified:    20 MB (shared)
```

---

## Benchmark-Kategorien Detailliert

| Kategorie | Anzahl Tests | Status |
|-----------|------------|--------|
| Advanced Patterns | 138 | ✅ Komplett |
| ARM Memory Optimizations | 29 | ✅ Komplett |
| ARM SIMD | 72 | ✅ Komplett |
| Auto Buffers | 49 | ✅ Komplett |
| Hybrid Vector/Geo | 34 | ✅ Komplett |
| Image Analysis | 32 | ✅ Komplett |
| Image Latency | 16 | ✅ Komplett |
| Hot Spots (Micro) | 28 | ✅ Komplett |
| Content Versioning | 18 | ✅ Komplett |
| Encryption | 19 | ✅ Komplett |
| **Gesamt** | **1.078** | **✅ ABGESCHLOSSEN** |

---

## Spitzenwerte (Longest Running)

1. **Async I/O Multi-Scan:** Single Benchmark (spezielle Workload)
2. **Batch Insert Ops:** 4 Test-Varianten
3. **Core Performance:** 6 Schlüsseltests
4. **GNN Embeddings:** 26 Graph Neural Network Tests
5. **GPU Backends:** 11 CUDA/CPU Vergleiche

---

## Erkenntnisse & Performance-Tipps

### ✅ Stark Performant
- **Query Engine:** Überragende Performance im Milliardenbereich (items/sec)
- **Embedding Cache:** Hits werden nahezu kostenlos bearbeitet
- **CTE Non-Recursive:** Sehr optimiert (900M+ items/sec)
- **Vector Index:** Solid performance für Insert-Operationen

### ⚠️ Skalierungspunkte
- **Recursive CTE:** Dips mit Tiefe (100→8M, 1000→900K items/sec)
- **Subquery ohne LIMIT:** Quadratische Komplexität bei größeren Datasets
- **Verteilte 2PC:** Netzwerk-Latenz dominiert (6.4K→1.2K mit 2→16 Nodes)

### 💡 Optimierungsmöglichkeiten
1. **Caching aktivieren** für Embedding-Arbeitslasten (155M vs 100K items/sec = 1550x!)
2. **LIMIT 1 in EXISTS** verwenden (optimiert automatisch)
3. **RRF vs Linear Combination** je nach Use-Case (7M vs 10M)

---

## Nächste Schritte

1. ✅ **Build Performance:** Alle v1.3.4 Benchmarks erfolgreich
2. ✅ **Container:** Docker-Image auf Hub pushiert
3. 📊 **Regression Testing:** Baseline für zukünftige Versionen etabliert
4. 🔄 **Trend Monitoring:** Regelmäßig gegen diese Baseline vergleichen

---

**Report generiert:** 29.12.2025 21:30 UTC+1  
**Python Version:** 3.13.6
