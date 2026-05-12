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

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| PEG-1 | <= 40 ms (Prompt Construction P99) | p99 aus `BM_PromptManager_InjectContext` |
| PEG-2 | <= 25 ms (Template Compilation P95) | p95 aus `BM_PromptManager_CreateTemplate` |
| PEG-3 | >= 20000 ops/s (Template Cache Hit Throughput) | mean aus `BM_PromptManager_GetTemplate_Hit` |
| PEG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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