# PERFORMANCE_EXPECTATIONS — src/prompt_engineering

## Scope
- Modul: `src/prompt_engineering`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_prompt_engineering.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| PE-1 | Siehe Zielbeschreibung: Prompt Construction P99 | `BM_PromptManager_InjectContext` |
| PE-2 | Siehe Zielbeschreibung: Template Compilation (4 KB) | `BM_PromptManager_CreateTemplate` |
| PE-3 | Siehe Zielbeschreibung: Compiled Template Render P99 (2 KB) | `BM_PromptManager_GetTemplate_Hit` |
| PE-4 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_PromptManager_ValidateTemplate_Valid` |
| PE-5 | Siehe Zielbeschreibung: Full 3-Iteration Reflection (kein LLM) | `BM_PromptManager_GetTemplate_Miss` |
| PE-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_PromptManager_GetTemplate_Hit` |
| PE-7 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_PromptManager_InjectContext` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
