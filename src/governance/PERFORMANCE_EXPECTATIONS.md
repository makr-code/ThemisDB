# PERFORMANCE_EXPECTATIONS — src/governance

## Scope
- Modul: `src/governance`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_governance_policy_latency.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| GOV-1 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_Evaluate_NoYAML_Offen` |
| GOV-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_Evaluate_CCPA_OptedOut` |
| GOV-3 | Siehe Zielbeschreibung: CCPA Report (90 Tage, 1M Subjects) | `BM_Evaluate_CCPA_NotOptedOut` |
| GOV-4 | Siehe Zielbeschreibung: Policy Evaluation P99 (500 Rules) | `BM_Evaluate_Throughput` |
| GOV-5 | Siehe Zielbeschreibung: DataMasker (50-Feld-Dokument) | `BM_CheckQueryPermission_WithMaskingRules` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| GVG-1 | <= 30 ms (Policy Evaluation P95) | p95 aus `BM_Evaluate_Throughput` |
| GVG-2 | <= 55 ms (Masking Path P99) | p99 aus `BM_CheckQueryPermission_WithMaskingRules` |
| GVG-3 | >= 10000 eval/s (Evaluate Throughput) | mean aus `BM_Evaluate_Throughput` |
| GVG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.

## Numerische Mindestziele (Release Gate)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| NG-1 Latenz P95 | <= 50 ms | p95 aus Benchmark-Run (`--benchmark_repetitions=5`) |
| NG-2 Latenz P99 | <= 100 ms | p99 aus Benchmark-Run (`--benchmark_repetitions=5`) |
| NG-3 Throughput-Stabilitaet | Regression <= 10 % gegen letzte Baseline | `(current - baseline) / baseline` |

Hinweis:
- Diese Mindestziele gelten als moduluebergreifende Release-Grenzen solange kein strengeres, modulspezifisches Ziel hinterlegt ist.
- Bei `proxy` oder `not_measurable` bleibt das Ziel numerisch gueltig, wird aber ueber den dokumentierten Proxy-Pfad verifiziert.