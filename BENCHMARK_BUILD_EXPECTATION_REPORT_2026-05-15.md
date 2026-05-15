# Benchmark Build + Erwartungsabgleich Report (2026-05-15)

## Scope
- Configure preset: `vscode-windows-bench-release`
- Build preset: `windows-bench-release`
- Verglichene Benchmarks:
  - `bench_query`
  - `bench_transaction_throughput`
  - `bench_simd_distance`
  - `bench_gorilla_codec`
  - `bench_tpcc`
  - `bench_ycsb`

## Build-Fixes umgesetzt

1. Runtime-DLL-Sync Race/Self-Copy behoben
- Datei: `cmake/CopyRuntimeDlls.cmake`
- Problem: `copy_if_different` versuchte DLLs auf sich selbst zu kopieren (z. B. `bin/themis_ingestion.dll` -> `bin/`), waehrend Link lief.
- Fix:
  - Skip bei identischem Source-/Destination-Verzeichnis (`REAL_PATH`-Vergleich).
  - Skip bei identischer Source-/Destination-Datei.
- Effekt: Vorheriger Build-Stop bei DLL-Kopiervorgang wurde beseitigt.

2. Vulkan-Benchmark robust gegen fehlendes Vulkan SDK gemacht
- Datei: `benchmarks/CMakeLists.txt`
- Problem: `bench_vulkan_lora` wurde gebaut, obwohl lokal kein nutzbares Vulkan SDK fuer Header vorhanden war.
- Fix: Target wird nur noch hinzugefuegt, wenn `THEMIS_ENABLE_GPU`, `THEMIS_ENABLE_VULKAN` und Vulkan SDK effektiv verfuegbar (`Vulkan_FOUND` oder `Vulkan::Vulkan`) sind.
- Effekt: Kein harter Compile-Abbruch durch Vulkan-Header-`#error`.

3. Private API-Nutzung im Observability-Benchmark korrigiert
- Datei: `benchmarks/bench_observability_goals.cpp`
- Problem: Zugriff auf private `MetricsCollector::incrementCounter`.
- Fix: Umstellung auf public API `addCounter(..., 1)`.
- Effekt: C2248 Compile-Fehler behoben.

4. Ingestion-Benchmark an aktuelle Ingestion-Model-Structs angepasst
- Datei: `benchmarks/bench_ingestion_quality_judge.cpp`
- Problem: Veraltete Felder (`source_uri`, `doc_id`, `BaseEntity.label`, `BaseEntity.type`).
- Fix: Umstellung auf aktuelle Felder (`manifest.original_path`, `manifest.file_id`, `entity_type`, `text`, `source_file_id`).
- Effekt: C2039 Compile-Fehler behoben.

## Nachtraeglich behobener Build-Blocker

- Target: `bench_ingestion_quality_judge`
- Vorheriges Fehlerbild: LNK2019/LNK1120 auf `IngestionQualityJudge`-Symbole.
- Root Cause:
  - `src/ingestion/ingestion_quality_judge.cpp` war in den Ingestion-Source-Listen nicht eingebunden.
  - Beim Einbinden traten API-Drifts zur aktuellen Ingestion-API auf (`WorkflowEngine::run` veraltet, Feldnamen veraltet).
- Umgesetzter Fix:
  - `ingestion_quality_judge.cpp` in `cmake/CMakeLists.txt` und `cmake/ModularBuild.cmake` zu den Ingestion-Sources hinzugefuegt.
  - `src/ingestion/ingestion_quality_judge.cpp` auf aktuelle Modelle angepasst (`manifest.file_id/original_path`, `BaseEntity.text`, `EntityRelation.relation_type`, `WorkflowEngine::execute*`).
- Ergebnis: gezielter Build `bench_ingestion_quality_judge` im Preset `windows-bench-release` erfolgreich.

## Soll-Ist Vergleich

Quelle Ist:
- `build/windows-bench-release/bench-results/*.json`

Quelle Soll/Baseline:
- `*.smoke.json` im Repo-Root
- plus globale Gates aus `PERFORMANCE_EXPECTATIONS.md` (u. a. Query P99 < 50 ms, global P99 <= 100 ms)

### Vergleichstabelle

| Benchmark | Metrik | Baseline (smoke) | Aktuell | Delta |
|---|---:|---:|---:|---:|
| bench_query | max_p99_us | 7413.8 | 46659.6 | +529.31% |
| bench_query | max_qps_est | 1461276.18 | 1438848.92 | -1.53% |
| bench_transaction_throughput | max_items_per_second | 640000.00 | 3185777.78 | +397.78% |
| bench_ycsb | max_items_per_second | 640000.00 | 800000.00 | +25.00% |
| bench_tpcc | max_items_per_second | 3200000.00 | 4848000.00 | +51.50% |
| bench_simd_distance | speedup_scalar_div_simd_L2_128 | 7.13 | 6.30 | -11.68% |
| bench_gorilla_codec | max_points_per_sec | 106666666.67 | 128512000.00 | +20.48% |

## Erwartungsbewertung (Kurzfazit)

1. Globale/Root-Latenz-Gates
- `bench_query` max `p99_us=46659.6` entspricht `46.66 ms`.
- Damit wird sowohl `Query P99 < 50 ms` als auch global `P99 <= 100 ms` eingehalten.

2. Throughput/Gesamttrend
- Transaction, TPCC, YCSB und Gorilla zeigen gegenueber Smoke-Baseline positive Throughput-Deltas.
- Query-QPS liegt leicht unter Smoke-Baseline (-1.53%), bleibt aber innerhalb eines 10%-Regressionsbandes.

3. SIMD
- SIMD-vs-Scalar-Speedup bei L2/128 ist geringer als Baseline (-11.68%).
- Kein harter Gate-Fehler aus den globalen Root-Gates ableitbar, aber als Performance-Drift relevant.

## Empfehlungen

1. SIMD-Drift beobachten
- Ursachencheck: Compiler-Flags, CPU-Frequenz/Power-State, Datenlayout-/Alignment-Aenderungen.

2. Query-Latenz-Regression genauer aufsplitten
- P99 stieg deutlich ggü. Smoke, obwohl Gate noch eingehalten ist. Regressionstreiber in P99-Pfaden priorisiert messen.

## Geaenderte Dateien
- `cmake/CopyRuntimeDlls.cmake`
- `cmake/CMakeLists.txt`
- `cmake/ModularBuild.cmake`
- `benchmarks/CMakeLists.txt`
- `benchmarks/bench_observability_goals.cpp`
- `benchmarks/bench_ingestion_quality_judge.cpp`
- `src/ingestion/ingestion_quality_judge.cpp`
