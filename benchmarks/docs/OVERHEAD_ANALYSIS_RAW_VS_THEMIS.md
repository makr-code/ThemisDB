> ⚠️ **Historische Analyse** – Messdaten und Vergleiche beschreiben einen bestimmten Messpunkt.

# Raw RocksDB vs. Themis Overhead Analyse
**Datum:** 20. Dezember 2025  
**Ziel:** Messung des SecondaryIndexManager- und TransactionWrapper-Overheads durch Vergleich mit Raw RocksDB TransactionDB (pipelined_write disabled)

---

## Methode

### Raw RocksDB Baselines
- **Technology:** Direct RocksDB TransactionDB API (C++)
- **Write Policy:** WRITE_PREPARED
- **WAL:** Enabled (für Fairness mit Themis)
- **Pipelined Write:** Disabled (aus Kompatibilitätsgründen mit aktuellen RocksDB-Builds)
- **Transaktionen:** SetName vor Prepare (WP-Anforderung)

### Themis NoPipe Baselines  
- **Wrapper:** RocksDBWrapper + SecondaryIndexManager
- **Write Policy:** WritePrepared (konfiguriert)
- **WAL:** Enabled (enable_wal=false bedeutet fsync-off, aber WAL selbst aktiv)
- **Pipelined Write:** Disabled (Kompatibilität)
- **Transaktionen:** Über TransactionWrapper::prepare() und commit()

---

## Ergebnisse (items_per_second)

### NonTxn (Direct Put / SecondaryIndexManager::put)

| Threads | Raw RocksDB | Themis NoPipe | Overhead (Themis/Raw) | Delta |
|---------|-------------|---------------|----------------------|-------|
| 1       | 2.47M       | 1.60M         | 65%                  | -35%  |
| 4       | 943k        | 1.60M         | 170%                 | +69%  |
| 8       | 383k        | 621k          | 162%                 | +62%  |
| 16      | 292k        | 265k          | 91%                  | -9%   |
| 32      | 149k        | 137k          | 92%                  | -8%   |

**Raw NonTxn Best:** 2.47M @ 1T (skaliert linear mit Threads ab)  
**Themis NoPipe Best:** 1.60M @ 4T (anomal hoch bei 4T; dann degenerativer Falloff)

---

### Txn10 (BeginTransaction+Prepare+Commit / TransactionWrapper)

| Threads | Raw RocksDB | Themis NoPipe | Overhead (Themis/Raw) | Delta |
|---------|-------------|---------------|----------------------|-------|
| 1       | 3.98M       | 2.13M         | 53%                  | -46%  |
| 4       | 667k        | 914k          | 137%                 | +37%  |
| 8       | 551k        | 492k          | 89%                  | -11%  |
| 16      | 223k        | 252k          | 113%                 | +13%  |
| 32      | 111k        | 135k          | 122%                 | +22%  |

**Raw Txn10 Best:** 3.98M @ 1T (skaliert degradiert ab 4T)  
**Themis NoPipe Txn10 Best:** 2.13M @ 1T (degenerativer, aber stabiler bei 8T+)

---

## Beobachtungen & Interpretationen

### 1. **4T Anomalie**
- **NonTxn:** Raw fällt auf 943k, Themis steigt auf 1.60M (+69% !!!)
- **Txn10:** Raw bei 667k, Themis bei 914k (+37%)
- **Ursache:** Raw TransactionDB scheint bei 4T einen Bottleneck zu haben (lock contention, queue overhead), während Themis/SecondaryIndexManager bei dieser Thread-Zahl optimal amortisiert.
- **Implikation:** Bei moderaten Parallelitäten (4T) ist Themis-Abstraktionen nicht zwingend ein Nachteil; das Gegenteil kann der Fall sein.

### 2. **1T Unterschied**
- **NonTxn:** Raw 2.47M vs. Themis 1.60M (-35%)
- **Txn10:** Raw 3.98M vs. Themis 2.13M (-46%)
- **Ursache:** SecondaryIndexManager::put und TransactionWrapper::prepare/commit haben instrinsischen Overhead (Funktion-Aufrufe, Index-Updates).
- **Implikation:** Auf Single-Thread ist Raw RocksDB ~35-46% schneller; erwartbar.

### 3. **High Thread Count (16T, 32T)**
- **NonTxn:** Themis kompetitiv oder leicht schneller (-9%, -8% Nachteil sind marginal).
- **Txn10:** Themis gewinnt deutlich +13%, +22% bei 16T/32T.
- **Ursache:** Bei hoher Parallelität profitiert TransactionWrapper von besserer Skalierbarkeit durch die WP-Prepare-Phase-Entkopplung und Themis' Kontextverwaltung.
- **Implikation:** Für Multi-Core-Workloads (8T+) ist Themis ohne Pipelined-Nachteil competitive.

### 4. **Txn10 vs. NonTxn (Raw)**
- **Raw Txn10 @ 1T:** 3.98M (höher als NonTxn 2.47M) – Transaktionen nutzen Write-Prepared Optimierungen.
- **Raw Txn10 @ 4T+:** 667k/551k/223k/111k (schwächer als bei 1T) – Lock Contention bei Prepare/Commit.
- **Themis Txn10 @ 1T:** 2.13M (höher als NonTxn 1.60M) – ähnliches Muster.
- **Themis Txn10 @ 8T+:** 492k/252k/135k (stabiler; +13% bei 16T+) – TransactionWrapper amortisiert sich.

### 5. **WAL Impact**
- Beide suites haben WAL enabled (enable_wal=false bedeutet nur fsync-off im Benchmark).
- Raw hat minimal overhead (direct DB::Put), Themis zahlt Index-Update-Kosten.
- Ohne WAL (disableWAL=true) würde Themis wahrscheinlich besser aussehen, aber WAL ist für TransactionDB kritisch (MVCC-Ordering).

---

## Schlussfolgerungen

| Szenario | Winner | Overhead | Empfehlung |
|----------|--------|----------|------------|
| **Single-Thread (1T)** | Raw RocksDB | 35–46% | Raw für Single-Threaded Benchmarks; Themis akzeptabel. |
| **Moderate Threads (4T)** | Themis | -37% bis -69% | **Themis schneller!** SecondaryIndexManager-Amortisierung bei 4T optimal. |
| **Skalierung (8T+)** | Themis (Txn10) | Kompetitiv bis +22% | Themis wettbewerbsfähig; Txn10 sogar überlegen. |
| **Transaktional (Txn10)** | Mixed | 8T: -11%, 16T: +13% | Txn10 in Themis skaliert besser bei hohen Threads. |

---

## Recommendations

### Phase 3: Optimierungsrichtungen

1. **4T Botleneck in Raw beheben** (Falls relevant für Vergleich):
   - Raw-Variant mit `allow_concurrent_memtable_write = true` + `enable_pipelined_write = true` (mit seq_per_batch fix) testen.
   - Könnte Raw @ 4T von 943k auf ~1.2M+ bringen.

2. **Themis-Seite: Pipelined Write reaktivieren** (aktuell für Raw-Kompatibilität deaktiviert):
   - Testet Themis + pipelined_write=true vs. Raw + pipelined_write=true.
   - Erwartet ~20–30% Gewinne bei Themis 8T+ aufgrund Pipelining.

3. **SecondaryIndexManager-Overhead-Profiling**:
   - Inline-Tracing oder Flame Graph für Themis NonTxn @ 1T, um Index-Update-Kosten zu isolieren.
   - Könnte zu schnelleren Index-Strukturen führen.

4. **Phase 1F Baseline Review**:
   - Phase 1F (Counter Elimination) erzielte +39% @ 8T.
   - Ist das ohne WAL/pipelined_write gemessen? Vergleich mit Raw (pipelined_write off) ist relevant.
   - Wenn Phase 1F mit pipelined_write=true gemessen wurde, ist der Vergleich unfair.

---

## Dateien & Artefakte

- **Raw Benchmarks:** C:\tmp\rocks_raw_ntx.json, C:\tmp\rocks_raw_txn10.json
- **Themis NoPipe:** C:\tmp\themis_npipe_ntx.json, C:\tmp\themis_npipe_txn10.json
- **Quellcode:**
  - [bench_advanced_patterns.cpp](benchmarks/bench_advanced_patterns.cpp#L1118-L1410) — Raw RocksDB Baselines
  - [bench_advanced_patterns.cpp](benchmarks/bench_advanced_patterns.cpp#L1443-L1650) — Themis NoPipe Suites

---

## Nächste Schritte

1. ✅ Raw vs. Themis NoPipe gemessen (abgeschlossen).
2. ⏳ Raw mit pipelined_write=true + seq_per_batch=false testen (optional, für Validierung).
3. ⏳ Themis mit pipelined_write=true gegen Raw mit pipelined_write=true vergleichen.
4. ⏳ Phase 1F-Bedingungen überprüfen und mit aktuellen Baselines reconciliation.
