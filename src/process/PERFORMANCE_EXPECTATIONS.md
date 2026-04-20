# PERFORMANCE_EXPECTATIONS — src/process

## Scope
- Modul: `src/process`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Direkte Mapping-Einträge fehlen; Fallback auf modulnahe Benchmarks `benchmarks/bench_process*.cpp` und globale Systemziele.

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| MOD-BASELINE | Throughput-Regression <= 10 %, P95-Regression <= 15 %, P99/P50 <= 2.5x, Peak-Memory <= 120 % ggü. Baseline | modulnahe Benchmarks |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
