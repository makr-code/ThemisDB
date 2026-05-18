# Benchmark Build + Erwartungsabgleich Report (2026-05-15)

## Scope
- Configure preset: `vscode-windows-bench-release`
- Build preset: `windows-bench-release`
- Globales Sammel-Target: `themis_benchmarks_all_eligible` baut erfolgreich
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

## Final nachgezogene Benchmark-Blocker

- `bench_ethics_ai_plugin`
  - Root Cause: Benchmark-Target uebernahm nicht die fuer das Plugin erforderliche YAML-/Export-Verdrahtung.
  - Fix: Ethics-AI-Implementierungen, `yaml-cpp::yaml-cpp`, `HAVE_YAML_CPP` und passende Includes gezielt am Benchmark-Target ergaenzt.
- `bench_user_storage_mount_latency`
  - Root Cause: Fehlende Backend-Implementierung und Include-Pfade fuer den gocryptfs-Pfad.
  - Fix: `src/user_storage_encrypted/gocryptfs_backend.cpp` sowie die zugehoerigen Include-Pfade spaet am Target verdrahtet.
- `bench_whisper_transcription`
  - Root Cause: Windows-Target bekam die Whisper-Implementierungen zunaechst nicht; danach fehlten Export-Defines und Nebenimplementierungen.
  - Fix: spaeter `if(TARGET ...)`-Override fuer `whisper_plugin.cpp`, `whisper_config.cpp`, `audio_chunk_reader.cpp`, Include-Pfade `include/whisper`/`src/whisper` und Compile-Definition `THEMIS_PLUGIN_EXPORTS`.
- Ergebnis: Nach diesen Fixes baut `themis_benchmarks_all_eligible` im Preset `windows-bench-release` vollstaendig erfolgreich.

## Soll-Ist Vergleich

Quelle Ist:
- `build/windows-bench-release/bench-results/*.json`

Quelle Soll/Baseline:
- `*.smoke.json` im Repo-Root
- plus globale Gates aus `PERFORMANCE_EXPECTATIONS.md` (u. a. Query P99 < 50 ms, global P99 <= 100 ms)

### Vergleichstabelle

| Benchmark | Metrik | Baseline (smoke) | Aktuell | Delta |
|---|---:|---:|---:|---:|
| bench_query | max_p99_us | 7413.8 | 45011.7 | +507.13% |
| bench_query | max_qps_est | 1461276.18 | 1331557.92 | -8.88% |
| bench_transaction_throughput | max_items_per_second | 640000.00 | 2896156.44 | +352.52% |
| bench_ycsb | max_items_per_second | 640000.00 | 890434.78 | +39.13% |
| bench_tpcc | max_items_per_second | 3200000.00 | 3984000.00 | +24.50% |
| bench_simd_distance | speedup_scalar_div_simd_L2_128 | 6.00 | 6.92 | +15.38% |
| bench_gorilla_codec | max_points_per_sec | 106666666.67 | 133358139.53 | +25.02% |

### Query-P99 Drilldown (vergleichbare Teilmenge)

Hinweis zur Vergleichbarkeit:
- Die aktuelle `bench_query`-Ausgabe enthaelt zusaetzliche P99-Varianten (`*_Batched_*`, `*_IndexKeys_*`), die in `bench_query.smoke.json` nicht vorhanden sind.
- Der starke Anstieg bei `max_p99_us` wird dadurch wesentlich beeinflusst und ist nicht 1:1 als reine Regression eines identischen Benchmark-Sets interpretierbar.
- Zusaetzlicher 1:1-Lauf mit identischer 5er-P99-Teilmenge (`BM_SimpleWhere_P99`, `BM_ComplexWhere_P99`, `BM_JoinUsersPosts_P99`, `BM_QueryMix_Historical_P99`, `BM_PointLookup_P99`) ergibt:
  - comparable `max_p99_us`: 7413.8 -> 7925.3 (`+6.90%`)
  - comparable `max_qps_est`: 1461276.18 -> 1396648.04 (`-4.42%`)

Direkt vergleichbare P99-Pfade (in beiden JSONs vorhanden):

| Query-Pfad | Baseline p99_us | Aktuell p99_us | Delta p99 | Baseline qps_est | Aktuell qps_est | Delta qps |
|---|---:|---:|---:|---:|---:|---:|
| BM_SimpleWhere_P99 | 184.6 | 181.3 | -1.79% | 7087.59 | 7463.80 | +5.31% |
| BM_ComplexWhere_P99 | 224.3 | 223.8 | -0.22% | 5596.52 | 5668.91 | +1.29% |
| BM_JoinUsersPosts_P99 | 815.0 | 798.1 | -2.07% | 1369.06 | 1354.47 | -1.07% |
| BM_QueryMix_Historical_P99 | 7413.8 | 7797.6 | +5.18% | 709.27 | 679.16 | -4.25% |
| BM_PointLookup_P99 | 0.9 | 1.0 | +11.11% | 1461276.18 | 1331557.92 | -8.88% |

## Erwartungsbewertung (Kurzfazit)

1. Globale/Root-Latenz-Gates
- `bench_query` max `p99_us=45011.7` entspricht `45.01 ms`.
- Damit wird sowohl `Query P99 < 50 ms` als auch global `P99 <= 100 ms` eingehalten.

2. Throughput/Gesamttrend
- Transaction, TPCC, YCSB und Gorilla zeigen gegenueber Smoke-Baseline positive Throughput-Deltas.
- Query-QPS liegt unter Smoke-Baseline (-8.88%), bleibt aber innerhalb des zulaessigen 10%-Regressionsbandes.
- Im 1:1-P99-Vergleichslauf bleibt Query-QPS ebenfalls innerhalb des 10%-Bands (-4.42%).

3. Query-Detailbild
- Bei direkt vergleichbaren Query-P99-Pfaden liegen `SimpleWhere`, `ComplexWhere` und `JoinUsersPosts` leicht besser als die Smoke-Baseline.
- Auffaellig bleibt vor allem der Mix-Pfad (`BM_QueryMix_Historical_P99`) mit +5.18% p99 und -4.25% qps.

4. SIMD
- SIMD-vs-Scalar-Speedup bei L2/128 liegt ueber der Smoke-Baseline (+15.38%).
- Aus dem aktuellen Vergleich ergibt sich hier kein Drift-Signal.

## Empfehlungen

1. Query-Latenz-Regression genauer aufsplitten
- Mix-Query-Pfad (`BM_QueryMix_Historical_P99`) als primaeren Treiber behandeln; batched/index-keys-Varianten separat trenden statt in `max_p99_us` mit klassischen Pfaden zu vermischen.

2. Query-Durchsatz gegen das strengere Kernziel nachmessen
- Gegen die Smoke-Baseline ist der aktuelle Lauf innerhalb des 10%-Bands, das in `PERFORMANCE_EXPECTATIONS.md` genannte strengere historische Kernziel von `900 M/s` wird aus diesem Benchmarkbericht allein aber nicht neu belegt.

## Erweiterte Benchmark-Runs (Kontrolliert, 2026-05-15)

Ausgefuehrte Zusatz-Benchmarks (Einzelruns mit Timeout und JSON-Artefakten):
- `bench_distributed_coordinator`
- `bench_distributed_knowledge`
- `bench_replication_throughput`
- `bench_sharding_performance`
- `bench_graph_traversal`
- `bench_vector_search`
- `bench_rag_hybrid_retriever`
- `bench_llm_response_cache`
- `bench_llm_inference_performance`
- `bench_timeseries_ingestion`

Kompakte Artefakt-Summary:
- Datei (Erstlauf): `build/batch_benchmark_compact_20260515.json`
- Datei (final): `build/batch_benchmark_compact_20260515.final.json`
- Final-Ergebnis: 10/10 parsebar und ohne Error-Benchmarks
- Retry-Update LLM: `bench_llm_inference_performance.retry2.json` ist voll parsebar, `error_occurred=0/26` (mit `THEMIS_MODEL_DIR` und lokalem LoRA-Stub)
- Retry-Update Timeseries: Vollrun `bench_timeseries_ingestion.retry4_multithread.json` ist parsebar mit `error_occurred=0/26` (inkl. reaktivierter Thread-Varianten)

Wichtige Maximalwerte aus dem Zusatzlauf:
- `bench_distributed_coordinator`: max `items_per_second` = 40.16 M/s
- `bench_sharding_performance`: max `items_per_second` = 28.67 M/s
- `bench_replication_throughput`: max `items_per_second` = 8.69 M/s
- `bench_rag_hybrid_retriever`: max `items_per_second` = 3.31 M/s
- `bench_vector_search`: max `qps` = 13.44 k/s

Interpretation Zusatzlauf:
- Distributed/Graph/RAG/Vector sind zusaetzlich messbar und liefern stabile Runs.
- LLM-Inference ist nach Retry2 im aktuellen Setup voll parsebar und ohne Error-Cases validiert.
- Timeseries-Ingestion ist nach Retry4 (inkl. Multi-Thread-Varianten) im Vollrun parsebar und ohne Error-Cases validiert.

### Targeted Gap-Runs (R-7/R-8/SH-2/SH-3/SH-4/SH-5/SH-6/SH-7/SH-9/SH-10/SH-11/SH-12)

Ergaenzend zu den 10/10-Batchlaeufen wurden fokussierte Ziel-IDs aus der Wave2-Matrix direkt nachgemessen:

- `bench_changefeed_targeted_v190.json`
  - `BM_RecordEventLatency`: `p99_us=50` -> R-7 PASS (<= 1 ms)
  - `BM_ReplicationLag`: `catch_up_time_ms=32` -> R-8 lokaler Fallback-Wert

- `bench_changefeed_wanlag_v190.json`
  - `BM_ReplicationLagWAN/50`: `lag_p99_ms=75`
  - R-8 damit direkt messbar und PASS fuer Zielwert <= 200 ms (50 ms RTT WAN-Simulation)

- `bench_changefeed_lanlag_v190.json`
  - `BM_ReplicationLagWAN/2`: `lag_p99_ms=37.5`
  - R-1 damit direkt messbar und PASS fuer Zielwert <= 50 ms (SEMI_SYNC LAN)

- `bench_replication_mbps_v190.json`
  - `WalBenchFixture/ReadFrom/500`: `bytes_per_second=9.38477 MiB/s`
  - `WalBenchFixture/ReadFrom/1000`: `bytes_per_second=9.5832 MiB/s`
  - R-2 bleibt unter Zielwert 500 MB/s (direkter FAIL).
  - R-6 bleibt unter Zielwert 200 MB/s (direkter FAIL).

- `bench_replication_failover_v190.json`
  - `BM_ReplicationManager_PromoteToLeader`: `0.055 ms`
  - `leader_promotion_success_rate_pct=100`
  - R-3 ist damit direkt messbar und PASS fuer Zielwert <= 10 s.

- `bench_replication_hlc_v190.json`
  - `BM_HLCConflictDetection/0`: `148 ns`
  - `BM_HLCConflictDetection/50`: `148 ns`
  - `BM_HLCConflictDetection/200`: `147 ns`
  - R-4 ist damit direkt messbar und PASS fuer Zielwert < 5 us/Write.

- `bench_replication_crdt_v190.json`
  - `BM_CRDTMerge/2`: `203 ns`
  - `BM_CRDTMerge/8`: `219 ns`
  - `BM_CRDTMerge/32`: `523 ns`
  - R-5 ist damit direkt messbar und PASS fuer Zielwert <= 1 us/Merge.

- `bench_shard_routing_targeted_v190_small.json`
  - `ConsistentHashPerformance/100`: `5.7344 M lookups/s`
  - `BatchRouting/10/10`: `63.75 s`

- `bench_shard_routing_poolhit_v190.json`
  - `BM_ConnectionPoolHitRate/10000`: `connection_pool_hit_rate=99.9999 %`
  - `BM_ConnectionPoolHitRate/1000`: `connection_pool_hit_rate=99.9999 %`
  - SH-2 ist damit direkt messbar und PASS (SLO > 95 %).

- `bench_sharding_percolator_v190.json`
  - `BM_PercolatorCommitLatency/10`: `0.746 ms`
  - `BM_PercolatorCommitLatency/20`: `0.832 ms`
  - `commit_success_rate_pct=100`
  - SH-3 ist damit direkt messbar und PASS fuer Zielwert < 20 ms.

- `bench_sharding_split_downtime_v190.json`
  - `ShardSplitDowntimeFixture/ZeroDowntimeReadAvailability/10000/256`: `11.01 us`
  - `ShardSplitDowntimeFixture/ZeroDowntimeReadAvailability/100000/1024`: `70.01 us`
  - `read_unavailability_ms=0`, `read_unavailable_events=0`, `read_availability_pct=100`
  - SH-4 ist damit direkt messbar und PASS fuer Zielwert 0 ms Read-Unavailability.

- `bench_sharding_write_migration_v190.json`
  - `RebalancingFixture/WriteLatencyDuringMigration/10000/1024`: `write_overhead_pct=62.82`
  - `RebalancingFixture/WriteLatencyDuringMigration/100000/2048`: `write_overhead_pct=69.03`
  - SH-5 ist damit direkt messbar, verfehlt aber den Zielwert < 20 % (direkter FAIL).

- `bench_sharding_rebalancer_cycle_v190.json`
  - `RebalancingFixture/RebalancerDecisionCycle/128/8`: `decision_cycle_s=0.000003335`
  - `RebalancingFixture/RebalancerDecisionCycle/512/16`: `decision_cycle_s=0.000019160`
  - `RebalancingFixture/RebalancerDecisionCycle/1024/24`: `decision_cycle_s=0.000046755`
  - SH-6 ist damit direkt messbar und PASS fuer Zielwert < 10 s.

- `bench_sharding_anti_entropy_v190.json`
  - `RebalancingFixture/AntiEntropyScanThroughput/131072/8`: `anti_entropy_throughput_gb_s=2.888`
  - `RebalancingFixture/AntiEntropyScanThroughput/262144/8`: `anti_entropy_throughput_gb_s=2.933`
  - SH-7 ist damit direkt messbar und PASS fuer Zielwert > 1 GB/s.

- `bench_sharding_snapshot_v190.json`
  - `RebalancingFixture/SnapshotTransfer1GB/8`: `snapshot_duration_s=0.0422`
  - `RebalancingFixture/SnapshotTransfer1GB/16`: `snapshot_duration_s=0.0654`
  - SH-9 ist damit direkt messbar und PASS fuer Zielwert < 10 s.

- `bench_sharding_snapshot_compression_v190.json`
  - `RebalancingFixture/SnapshotCompressionRatioZstdL3/64`: `snapshot_compression_ratio_pct=0.0138`
  - `RebalancingFixture/SnapshotCompressionRatioZstdL3/128`: `snapshot_compression_ratio_pct=0.0115`
  - SH-10 ist damit direkt messbar und PASS fuer Zielwert < 35 %.

- `bench_sharding_replica_catchup_v190.json`
  - `RebalancingFixture/ReplicaCatchupThroughput/128`: `replica_catchup_mb_s=1031.45`
  - `RebalancingFixture/ReplicaCatchupThroughput/256`: `replica_catchup_mb_s=1040.40`
  - SH-11 ist damit direkt messbar und PASS fuer Zielwert > 200 MB/s.

- `bench_sharding_topology_propagation_v190.json`
  - `GossipOverheadFixture/TopologyPropagation100Nodes/100`: `topology_propagation_ms=240`
  - `GossipOverheadFixture/TopologyPropagation100Nodes/150`: `topology_propagation_ms=200`
  - SH-12 ist damit direkt messbar und PASS fuer Zielwert < 500 ms.

- `bench_backend_vulkan_20260517_091736.json`
  - `BM_Backend_Vulkan/4`: `real_time=819448 ns`, `samples/sec=146.924`
  - `BM_Backend_Vulkan/8`: `real_time=1198287 ns`, `samples/sec=685.871`
  - `BM_Backend_Vulkan/16`: `real_time=2045621 ns`, `samples/sec=3673.09`
  - `BM_Backend_Init_Vulkan`: `real_time=34200250 ns` (`34.20 ms`)
  - `BM_Vulkan_Overhead/1`: `real_time=515162 ns`
  - `BM_Vulkan_Overhead/0`: `ERROR OCCURRED: CUDA not available` (erwartbar bei CUDA OFF).

Folgeeffekt in der Erwartungsmatrix:
- R-7 wurde von N/A auf PASS hochgestuft.
- R-8 wurde von Proxy auf direkte PASS-Messung (WAN-RTT-Simulation) hochgestuft.
- R-1 wurde von Proxy/N/A auf direkte PASS-Messung (LAN-RTT-Simulation) hochgestuft.
- SH-2 wurde von Proxy/N/A auf direkte PASS-Messung umgestellt.
- SH-3 wurde von Proxy auf direkte PASS-Messung (Percolator-Commit) hochgestuft.
- SH-4 wurde von Proxy auf direkte PASS-Messung (Zero-Downtime Split) hochgestuft.
- SH-5 wurde von Proxy auf direkte FAIL-Messung (Write-Overhead waehrend Migration) umgestellt.
- SH-6 wurde von Proxy auf direkte PASS-Messung (Rebalancer Decision Cycle) hochgestuft.
- SH-7 wurde von Proxy auf direkte PASS-Messung (Anti-Entropy Scan Throughput) hochgestuft.
- SH-9 wurde von Proxy auf direkte PASS-Messung (1-GB Snapshot Transfer) hochgestuft.
- SH-10 wurde von Proxy auf direkte PASS-Messung (ZSTD-L3 Snapshot-Kompressionsrate) hochgestuft.
- SH-11 wurde von Proxy auf direkte PASS-Messung (Replica Catch-up Throughput) hochgestuft.
- SH-12 wurde von Proxy auf direkte PASS-Messung (Topology-Change Gossip Propagation) hochgestuft.
- R-2 und R-6 wurden von N/A auf direkte FAIL-Messung (MB/s) umgestellt.
- R-3 wurde von Proxy auf direkte PASS-Messung (Promote/Faillover-Pfad) hochgestuft.
- R-4 wurde von Proxy auf direkte PASS-Messung (HLC-Conflict-Detection) hochgestuft.
- R-5 wurde von Proxy auf direkte PASS-Messung (CRDT-Merge) hochgestuft.

## Geaenderte Dateien
- `cmake/CopyRuntimeDlls.cmake`
- `cmake/CMakeLists.txt`
- `cmake/ModularBuild.cmake`
- `benchmarks/CMakeLists.txt`
- `benchmarks/bench_distributed_knowledge_or.cpp`
- `benchmarks/bench_interval_tree_erase.cpp`
- `benchmarks/bench_llm_raid_pipeline.cpp`
- `benchmarks/bench_observability_goals.cpp`
- `benchmarks/bench_ingestion_quality_judge.cpp`
- `benchmarks/bench_timeseries_ingestion.cpp`
- `src/ingestion/ingestion_quality_judge.cpp`
