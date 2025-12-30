# THEMIS v1.4 SHARDING BENCHMARK PLAN

**Ziel:** Belastbare Benchmarks in einer sharded Themis-Topologie, direkt vergleichbar mit Hyperscaler-DB-Setups (z.B. AWS Aurora/Redshift, GCP Spanner/BigQuery, Azure Cosmos/SQL MI).  
**Warum:** Single-node Zahlen sind solide; Kunden erwarten Skalierungs- und Kostenmodelle unter realen Sharding-Workloads.

---

## 🎯 MESSZIELE (KPIs)
- **Throughput skalierend**: Linearität bei +1, +2, +4, +8 Shards (max ±10% Abweichung zur Ideallinie).
- **p99-Latenz**: < 2.5× Single-Node bei 8 Shards unter 70/30 Read/Write.
- **Rebalance-Kosten**: < 15% Throughput-Dip während Chunk Move / Re-Shard.
- **Cross-Shard Joins**: p95 < 2× Lokale Queries bei 10% Cross-Shard Rate.
- **Costs**: $/Million Ops (vergleichbar mit Aurora/Spanner Redbook Preisen).

---

## 🏗️ TEST-TOPOLOGIE
- **Knoten**: 8× VM (16 vCPU, 32GB RAM, NVMe), 1× Router/Gateway, 1× Orchestrator.
- **Shards**: Start 2 → 4 → 8 (Hash-basiertes Sharding), Replikationsfaktor 2.
- **Netzwerk**: 10 Gbps, Latenz-Injektion 0ms / 2ms / 10ms für Hyperscaler-Nähe.
- **Storage**: NVMe (lokal) + optional Blob/GCS/S3 Abstraction für Remote-IO-Test.

---

## 📦 DATASETS
- **OLTP**: 100M, 500M, 1B Rows (Orders/Users/Events).
- **Vector**: 100M Embeddings (768D, FP16/INT8 Varianten), Hybrid (vector + scalar).
- **Time-Series**: 10M/min Ingest, 30d Retention, Downsampling aktiv.

---

## 🔥 WORKLOAD-MIX (pro Run)
| Mix | Reads | Writes | Joins | Cross-Shard | Vector | Notes |
|-----|-------|--------|-------|-------------|--------|-------|
| A   | 80%   | 20%    | Low   | 5%          | None   | Read-heavy OLTP |
| B   | 50%   | 50%    | Med   | 10%         | None   | Balanced |
| C   | 70%   | 30%    | High  | 20%         | None   | Join-heavy, hash+range |
| D   | 60%   | 40%    | Med   | 15%         | 20%    | Hybrid vector + scalar |
| E   | 30%   | 70%    | Low   | 5%          | None   | Ingest heavy |

---

## 📊 METRIKEN & ZIELWERTE
- **Throughput (ops/s)** per Mix & Shard Count; Ziel: 0→8 Shards ≈ linear ±10%.
- **Latenz p50/p95/p99**; Ziel: p99 < 2.5× single-node bei 8 Shards.
- **Tail-Impact Rebalance**: ≤15% Einbruch, Erholung < 5 min.
- **Hotspot-Resilienz**: ±5% Varianz zwischen Shards nach 30 min.
- **Vector Recall**: ≥99.5% bei Hybrid Mix D unter Cross-Shard Queries.
- **Cost/Perf**: $/M ops vs Aurora/Spanner/BigQuery (modelliert auf On-Demand Preise).

---

## 🧪 TEST-SCHRITTE
1) **Baseline Single-Node**: Mix A–E, 1 shard, messen als Referenz.
2) **Scale-Out**: 2 → 4 → 8 Shards; gleiche Mixes, gleiche Datenmenge pro Shard (rebalance disabled).
3) **Rebalance Test**: Start mit 2 Shards @ 60% Füllung, expand auf 4, dann 8; Messen Durchsatz/Latenz während Chunk Moves.
4) **Fault Injection**: Kill 1 Replica/Shard; Erwartung: p99 < +25%, Recovery < 60s.
5) **Network Impairment**: 2ms und 10ms RTT; beobachten Degradationsfaktoren vs Hyperscaler-typische Zonen.
6) **Cross-Shard Joins**: 10%/20% Cross-Shard Anteil; Ziel: p95 < 2× lokal.
7) **Vector Hybrid**: Mix D mit 20% Vector-Queries; Recall messen & Latenz p99.

---

## 🛠️ AUTOMATION (Empfohlen)
- **Loader**: `tools/shard_loader.py` (Hash/Range, parallel workers).
- **Runner**: `tools/shard_bench.py` (Mix A–E, configurable R/W, cross-shard %).
- **Faults**: `tools/fault_injector.py` (process kill, packet loss, RTT add via tc).
- **Aggregator**: `tools/aggregate_shard_results.py` → `shard_results.json`.
- **Comparator**: `tools/compare_hyperscaler.py` (Preis/Leistung vs Aurora/Spanner SKU-Modelle).

---

## 🔄 REPORTING (Output)
- `sharding_summary.md`: KPIs pro Mix & Shard-Stufe, Skalierungskurven.
- `rebalance_report.md`: Throughput/Latenz während Shard-Expansion.
- `fault_tolerance.md`: Metriken unter Replica-Kill und Netz-Impairment.
- `cost_comparison.md`: $/M ops vs Aurora/Spanner (On-Demand, ähnlicher HW-Footprint).

---

## 🧭 VERGLEICH MIT HYPERSCALERN (Vorschlag)
- **Aurora/Spanner Vergleich**: 4 vCPU/16GB Knoten, 8 Knoten Cluster; Preislisten Stand 12/2025.
- **Metrik**: $/M Ops, p99 Latenz, Skalierungsfaktor 1→8 Shards.
- **Akzeptanz-Korridor**: 
  - Throughput: ≥90% der Aurora/Spanner-Referenz bei gleicher HW-Klasse.
  - Latenz p99: ≤1.4× Aurora/Spanner bei 2ms RTT.
  - Kosten: ≤70% $/M Ops von Spanner; ≤85% von Aurora.

---

## ✅ ABNAHMEKRITERIEN
- Lineare Skalierung (±10%) bis 8 Shards in Mix A/B/D.
- Rebalance-Dip ≤15%, Recovery <5 min.
- Cross-Shard p95 < 2× lokal bei 10–20% Cross-Shard-Rate.
- Fault Kill: p99 < +25%, Recovery <60s.
- Vector Recall ≥99.5% unter Sharding.
- Kosten: ≤0.70× Spanner, ≤0.85× Aurora ($/M Ops, modelliert).

---

## 📅 ZEITLEISTE (Vorschlag)
- Woche 1: Tooling finalisieren (Loader/Runner/Aggregator), Baseline Single-Node.
- Woche 2: 2→4→8 Shard Runs, Report `sharding_summary.md`.
- Woche 3: Rebalance & Fault Injection, Report `rebalance_report.md`, `fault_tolerance.md`.
- Woche 4: Network impairment & Cross-Shard Joins, Cost Comparison, Final Exec-Summary.

---

## 📌 NÄCHSTE SCHRITTE
1) Fixiere Hardware-Profile (VM Size, Disk, Netz-Latenzprofile).
2) Erstelle Baseline-SKU-Mapping zu Aurora/Spanner (On-Demand Preise erfassen).
3) Implementiere `shard_bench.py` + `aggregate_shard_results.py` (falls noch nicht vorhanden).
4) Plane 4-wöchige Kampagne (oben) und publiziere Ergebnisse in `sharding_summary.md`.

---

**Erstellt:** 29. Dezember 2025  
**Owner:** Performance Team  
**Ziel:** Hyperscaler-Vergleichbarkeit der Sharding-Performance sicherstellen.
