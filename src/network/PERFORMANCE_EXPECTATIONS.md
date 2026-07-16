# PERFORMANCE_EXPECTATIONS — src/network

## Scope
- Modul: `src/network`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_api_endpoints.cpp`
  - `benchmarks/bench_stream_protocol.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| NET-1 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_GraphQL_Execute_MockResolver` (proxy) |
| NET-2 | Protokoll-Header-Build darf keine signifikante Regression ggü. Baseline zeigen | `BM_StreamProtocol_FrameHeaderBuild` |
| NET-3 | LZ4-Stream-Roundtrip bleibt im Baseline-Korridor (p95/p99 Trend) | `BM_StreamProtocol_LZ4Roundtrip` |
| NET-4 | Metrics-Snapshot-Pfad bleibt im Baseline-Korridor | `BM_StreamProtocol_MetricsSnapshot` |
| NET-5 | BufferPool-Roundtrip bleibt im Baseline-Korridor | `BM_StreamProtocol_BufferPoolRoundtrip` |
| NET-6 | HTTP-Endpoint-Overhead-Regression <= 15 % ggü. Baseline | `BM_HttpServer_Health_Endpoint` (proxy) |
| SP-1 | > 50 M ops/s | `BM_StreamProtocol_FrameHeaderBuild` |
| SP-2 | < 1 ms (16 KiB Payload) | `BM_StreamProtocol_LZ4Roundtrip` |
| SP-3 | < 5 ms (10k Samples) | `BM_StreamProtocol_MetricsSnapshot` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.

## Sourcecode Verification (Module: network/performance)

- Gepruefte Benchmark-Quellen:
  - `benchmarks/bench_stream_protocol.cpp`
  - `benchmarks/bench_api_endpoints.cpp`
- Gepruefte Ziel-Fall-Zuordnung:
  - stream protocol frame/serialization/metrics paths (`BM_StreamProtocol_*`)
  - endpoint-side proxy signal (`BM_HttpServer_Health_Endpoint`, `BM_GraphQL_Execute_MockResolver`)
- Ergebnis:
  - Die referenzierten Benchmark-Faelle existieren im aktuellen Benchmark-Source.
  - Direkte TLS/QUIC/UDP transport-spezifische Mikrobenchmarks sind in dieser Datei weiterhin als Folgeaufgabe zu behandeln, solange keine dedizierten Benchmark-Faelle im Modulbestand vorliegen.
