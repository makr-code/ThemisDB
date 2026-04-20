# PERFORMANCE_EXPECTATIONS — src/distributed_knowledge

## Scope
- Modul: `src/distributed_knowledge`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `sharding` als Referenzpfad.
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_sharding_performance.cpp`
  - `benchmarks/bench_shard_routing.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| SH-1 | < 5 ms | `ScatterGatherFixture_ScatterGatherLatency` |
| SH-2 | > 95 % @ 10k RPS | `ShardRoutingFixture_ConsistentHashPerformance` |
| SH-3 | < 20 ms | `CrossShardJoinFixture_BroadcastHashJoin` |
| SH-4 | 0 ms Read-Unavailability | `RebalancingFixture_BatchSerializationThroughput` |
| SH-5 | < 20 % over Baseline P99 | `RebalancingFixture_BatchSerializationThroughput` |
| SH-6 | < 10 s | `RebalancingFixture_BatchDeserializationThroughput` |
| SH-7 | > 1 GB/s (NVMe, 8 Worker) | `GossipOverheadFixture_MessageSerialization` |
| SH-8 | > 4 GB/s (NVIDIA A10) | `ScatterGatherFixture_ScatterGatherLatency` |
| SH-9 | < 10 s | `CrossShardJoinFixture_BroadcastHashJoin` |
| SH-10 | < 35 % unkomprimiert (ZSTD L3) | `RebalancingFixture_BatchDeserializationThroughput` |
| SH-11 | > 200 MB/s (10 GbE LAN) | `GossipOverheadFixture_MessageSerialization` |
| SH-12 | < 500 ms (100 Nodes, Gossip) | `GossipOverheadFixture_FanoutSelection` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
