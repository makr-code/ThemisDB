# PERFORMANCE_EXPECTATIONS — src/server

## Scope
- Modul: `src/server`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `network` als Referenzpfad.
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_api_endpoints.cpp`
  - `benchmarks/bench_security.cpp`
  - `benchmarks/bench_stream_protocol.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| NET-1 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_GraphQL_Execute_MockResolver` |
| NET-2 | Siehe Zielbeschreibung: TLS 1.3 Handshake P99 | `BM_AES256GCM_Encrypt_1KB` |
| NET-3 | Siehe Zielbeschreibung: TLS 1.3 Session Resumption P99 | `BM_AES256GCM_Encrypt_64KB` |
| NET-4 | Siehe Zielbeschreibung: WebSocket Round-Trip P99 | `BM_GraphQL_Parse_Simple_Cached` |
| NET-5 | Siehe Zielbeschreibung: QUIC 0-RTT Resumption P99 | `BM_AES256GCM_Decrypt_1MB` |
| NET-6 | Siehe Zielbeschreibung: UDP Fast-Path GET P99 | `BM_GraphQL_Parse_Complex_Uncached` |
| SP-1 | > 50 M ops/s | `BM_StreamProtocol_FrameHeaderBuild` |
| SP-2 | < 1 ms (16 KiB Payload) | `BM_StreamProtocol_LZ4Roundtrip` |
| SP-3 | < 5 ms (10k Samples) | `BM_StreamProtocol_MetricsSnapshot` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
