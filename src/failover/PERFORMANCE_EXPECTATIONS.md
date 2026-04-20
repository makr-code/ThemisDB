# PERFORMANCE_EXPECTATIONS — src/failover

## Scope
- Modul: `src/failover`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `replication` als Referenzpfad.
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_replication_throughput.cpp`
  - `benchmarks/bench_changefeed_throughput.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| R-1 | ≤ 50 ms @ 10k Writes/s (LAN) | `WalBenchFixture_Append` |
| R-2 | ≥ 500 MB/s/Follower (10 GbE) | `WalBenchFixture_ReadFrom` |
| R-3 | ≤ 10 s | `BM_ReplicationManager_Initialize` |
| R-4 | < 5 µs/Write | `BM_WALEntry_Serialize` |
| R-5 | ≤ 1 µs/Merge | `BM_WALEntry_Deserialize` |
| R-6 | ≥ 200 MB/s; ≤ 10 min | `WalBenchFixture_ReadFrom` |
| R-7 | ≤ 1 ms (Commit → CDC Queue) | `ChangefeedBenchmarkFixture_EventRecordingThroughput` |
| R-8 | ≤ 200 ms P99 (50 ms RTT WAN) | `WalBenchFixture_ReadFrom` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
