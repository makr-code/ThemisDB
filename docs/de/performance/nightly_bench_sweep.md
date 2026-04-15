# Nightly Benchmark Sweep — Modulweise Coverage (§1.4 Maßnahme #10)

**Status:** ✅ Aktiv (ERLEDIGT)  
**Workflow:** [`.github/workflows/nightly-benchmark-sweep.yml`](../../../../.github/workflows/nightly-benchmark-sweep.yml)  
**CMake-Preset:** `nightly-bench-sweep`  
**Report-Tool:** [`tools/bench_coverage_report.py`](../../../../tools/bench_coverage_report.py)

---

## Überblick

Der **Nightly Benchmark Sweep** läuft täglich um **02:00 UTC** automatisch per
`schedule`-Trigger und deckt alle definierten Benchmark-Module (Nr. 2–33) ab.

Für jedes Modul wird ein Traffic-Light-Status erzeugt:

| Symbol | Bedeutung |
|--------|-----------|
| 🟢 | Kein Fehler, keine Regression |
| 🟡 | Leichte Regression (5–10 % langsamer als Vortag) |
| 🔴 | Regression ≥ 10 % oder Fehler in Benchmark-Ausgabe |
| ⚪ | Nicht abgedeckt (kein Benchmark-Binary für dieses Modul gefunden) |

Der Delta-Vergleich erfolgt automatisch gegen den Vortags-Lauf (Artefakt
`nightly-bench-report-previous`).

---

## Modulgruppen (2–33)

| Modul # | Name | Erkennungs-Pattern |
|---------|------|--------------------|
| 2  | Storage | `storage`, `rocksdb`, `lsm`, `blob`, `delta` |
| 3  | Index | `index`, `inverted`, `vector_search`, `ann`, `hnsw`, `ivf` |
| 4  | Query | `query`, `aql`, `join`, `scan`, `filter`, `where` |
| 5  | Graph | `graph`, `traversal`, `bfs`, `dfs`, `shortest` |
| 6  | Transactions | `transaction`, `tx`, `mvcc`, `commit`, `rollback` |
| 7  | CDC / Realtime | `cdc`, `changefeed`, `realtime`, `stream`, `wal` |
| 8  | Timeseries | `timeseries`, `ts_`, `_ts_`, `hypertable`, `downsamp`, `gorilla` |
| 9  | Analytics (OLAP) | `olap`, `analytics`, `ivm`, `agg` |
| 10 | Vector / Embedding | `vector`, `embedding`, `knn`, `pq`, `bq`, `binary_quant` |
| 11 | Geospatial | `geo`, `spatial`, `rtree`, `tile`, `haversine`, `radius` |
| 12 | Full-Text Search | `fulltext`, `fts`, `text_search`, `lucene`, `bm25` |
| 13 | Auth / Security | `auth`, `jwt`, `token`, `oauth`, `security`, `rls`, `encrypt` |
| 14 | Sharding | `shard`, `consistent_hash`, `partition` |
| 15 | Replication / HA | `repl`, `raft`, `ha_`, `failover`, `crdt` |
| 16 | Backup / Storage Tiers | `backup`, `restore`, `pitr`, `tiered`, `archiv` |
| 17 | LLM Integration | `llm`, `inference`, `prompt`, `llama`, `rag`, `embeddings_llm` |
| 18 | ML / Training | `ml_`, `train`, `lora`, `finetune`, `automl` |
| 19 | Exporters | `export`, `parquet`, `arrow`, `jsonl`, `hugging` |
| 20 | Importers | `import`, `postgres_import`, `mongo_import`, `kafka_import` |
| 21 | API / Protocols | `api`, `http`, `grpc`, `websocket`, `wire_proto`, `quic` |
| 22 | Plugins / WASM | `plugin`, `wasm`, `extension` |
| 23 | Scheduler | `scheduler`, `task_sched`, `cron_`, `job_queue` |
| 24 | Observability | `observ`, `metric`, `tracing`, `profil`, `alert` |
| 25 | Acceleration | `accel`, `dispatch`, `gpu`, `cuda`, `simd`, `avx` |
| 26 | Adaptive Query Compilation | `adaptive_query`, `jit`, `query_compil`, `query_cache` |
| 27 | Chimera Suite | `chimera` |
| 28 | AQL Reference | `aql_func`, `aql_valid`, `aql_optim` |
| 29 | Schema / Metadata | `schema`, `catalog`, `metadata`, `info_schema` |
| 30 | Cluster Updates | `update_sched`, `canary`, `hot_reload`, `blue_green` |
| 31 | Governance | `governance`, `policy`, `compliance`, `masking`, `lineage` |
| 32 | Ethics AI | `ethics`, `constitutional`, `confidence_detect` |
| 33 | System-Level (TPC/YCSB) | `tpcc`, `ycsb`, `tpc_`, `system_bench` |

---

## Workflow-Ablauf

```
schedule: cron '0 2 * * *'  (täglich 02:00 UTC)
│
├── Checkout + Python-Setup
├── LLM/LoRA-Artefakt-Vorbereitung (--stub-only, kein GPU erforderlich)
├── CMake configure  (Preset: nightly-bench-sweep)
├── Build: cmake --build build/nightly --target all
├── Run: bench_* --benchmark_format=json → artifacts/nightly/bench_<name>.json
├── Download Vortags-Artefakt (nightly-bench-report-previous)  ← Delta-Basis
├── python3 tools/bench_coverage_report.py
│     → artifacts/nightly/audit/coverage_report.md   (Markdown, Traffic Light)
│     → artifacts/nightly/audit/coverage_report.json (Machine-readable)
├── Upload aktuelle Ergebnisse als nightly-bench-report-previous (2 Tage Retention)
├── Upload vollständiger Report als nightly-bench-report-<run_number> (30 Tage)
└── GitHub Step Summary mit Coverage-Report-Tabelle
```

---

## Artefakte

Alle Artefakte sind im GitHub Actions-Run des Nightly-Sweep verfügbar:

| Artefakt | Inhalt | Retention |
|----------|--------|-----------|
| `nightly-bench-report-<N>` | Alle `bench_*.json`, `coverage_report.md`, `coverage_report.json`, Logs | 30 Tage |
| `nightly-bench-report-previous` | Letzter erfolgreicher Run (Delta-Basis für nächsten Tag) | 2 Tage |

---

## Lokaler Betrieb

```bash
# Report aus vorhandenem artifacts/nightly/-Verzeichnis generieren:
python3 tools/bench_coverage_report.py \
  --bench-dir artifacts/nightly \
  --output-dir artifacts/nightly/audit

# Mit Delta-Vergleich gegen Vortag:
python3 tools/bench_coverage_report.py \
  --bench-dir artifacts/nightly \
  --prev-dir artifacts/nightly-prev \
  --output-dir artifacts/nightly/audit

# CMake konfigurieren (nightly-bench-sweep Preset):
cmake --preset nightly-bench-sweep
cmake --build build/nightly --target all -j$(nproc)
```

---

## CI-Audit-Checks

Das Vorhandensein dieser Infrastruktur wird durch zwei Audit-Tools verifiziert:

- **`tools/perf_expectations_audit.py`** – Checks 10a, 10b, 10c (Maßnahme #10):
  - **10a**: Nightly-Workflow mit `schedule`/`cron`-Trigger vorhanden
  - **10b**: `CMakePresets.json` enthält `nightly-bench-sweep` Preset
  - **10c**: `tools/bench_coverage_report.py` Orchestrierungs-Script vorhanden
- **`tools/perf_coverage_top10_audit.py`** – M10: Nightly benchmark sweeps

---

## Referenz

- `PERFORMANCE_EXPECTATIONS.md` §1.4, Maßnahme #10
- `tools/bench_coverage_report.py` – Report-Generator mit Traffic-Light-Logik
- `CMakePresets.json` – Preset `nightly-bench-sweep`
- `.github/workflows/nightly-benchmark-sweep.yml` – CI-Workflow
