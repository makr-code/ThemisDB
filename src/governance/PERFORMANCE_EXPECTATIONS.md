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

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
