# PERFORMANCE_EXPECTATIONS — src/sharding

## Scope
- Modul: `src/sharding`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_sharding_performance.cpp`
  - `benchmarks/bench_shard_routing.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| SH-1 | < 5 ms | `ScatterGatherFixture_ScatterGatherLatency` |
| SH-2 | > 95 % @ 10k RPS | `BM_ConnectionPoolHitRate` |
| SH-3 | < 20 ms | `BM_PercolatorCommitLatency` |
| SH-4 | 0 ms Read-Unavailability | `ShardSplitDowntimeFixture_ZeroDowntimeReadAvailability` |
| SH-5 | < 20 % over Baseline P99 | `RebalancingFixture_WriteLatencyDuringMigration` |
| SH-6 | < 10 s | `RebalancingFixture_RebalancerDecisionCycle` |
| SH-7 | > 1 GB/s (NVMe, 8 Worker) | `RebalancingFixture_AntiEntropyScanThroughput` |
| SH-8 | > 4 GB/s (NVIDIA A10) | `RebalancingFixture_GpuReedSolomonThroughput` |
| SH-9 | < 10 s | `RebalancingFixture_SnapshotTransfer1GB` |
| SH-10 | < 35 % unkomprimiert (ZSTD L3) | `RebalancingFixture_SnapshotCompressionRatioZstdL3` |
| SH-11 | > 200 MB/s (10 GbE LAN) | `RebalancingFixture_ReplicaCatchupThroughput` |
| SH-12 | < 500 ms (100 Nodes, Gossip) | `GossipOverheadFixture_TopologyPropagation100Nodes` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
