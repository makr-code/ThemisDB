# PERFORMANCE_EXPECTATIONS — src/server

## Scope
- Diese Datei definiert die Performance-Erwartungen für `src/server` im Produktivbetrieb.
- Ziele basieren auf Benchmarks unter `benchmarks/` und, falls notwendig, auf expliziten Annahmen.

## Benchmark-Bezug
- Abdeckungsmodus: **Direkte Modul-Benchmarks**
- Relevante Quellen (Auszug):
  - `benchmarks/bench_api_endpoints.cpp`
  - `benchmarks/bench_auth_token_validation.cpp`
  - `benchmarks/bench_postgres_e2e.cpp`
  - `benchmarks/bench_postgres_protocol.cpp`
  - `benchmarks/bench_wal_apply_grpc.cpp`

## Annahmen
- Messungen erfolgen in Release-Builds mit stabiler CPU/GPU-Taktung und ohne Debug-Instrumentierung.
- Datenverteilungen und Lastprofile orientieren sich an den jeweils genannten Benchmark-Szenarien.
- Für CI-Gates wird eine Regression gegen die zuletzt akzeptierte Baseline desselben Benchmarks bewertet.

## Service-Level-Ziele (SLO)
| KPI | Erwartung | Gate |
|---|---|---|
| Throughput | Keine Regression > 10 % gegenüber Baseline | Warnung > 8 %, Fehler > 10 % |
| P95-Latenz | Keine Regression > 15 % gegenüber Baseline | Warnung > 10 %, Fehler > 15 % |
| P99/P50-Verhältnis | Stabilität in Lastspitzen, Ziel ≤ 2.5x | Warnung > 2.3x, Fehler > 2.5x |
| Speicher/VRAM | Peak ≤ 120 % der Baseline (gleicher Workload) | Warnung > 110 %, Fehler > 120 % |

## Validierung
- Vor einem Release sollen die oben referenzierten Benchmarks (oder Proxy-Benchmarks) mindestens 3-mal reproduzierbar laufen.
- Ausreißerbehandlung: Median + P95/P99 pro Lauf dokumentieren; Regressionen nur nach Root-Cause-Analyse akzeptieren.
- Änderungen an Algorithmen, Speicherlayout oder Parallelisierung erfordern ein Baseline-Update mit Begründung.

## Nicht-Ziele / Hinweise
- Diese Erwartungen ersetzen keine funktionalen Korrektheitstests und keine Security-Validierung.
- Bei fehlender direkter Benchmark-Abdeckung sind die Ziele konservativ und müssen bei erster Messung präzisiert werden.
