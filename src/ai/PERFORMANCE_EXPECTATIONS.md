# PERFORMANCE_EXPECTATIONS — src/ai

## Scope

- Modul: src/ai
- Diese Datei dokumentiert modulspezifische Performance-Erwartungen fuer AI-Plugin-Generierung.
- Primarquelle fuer Benchmark-Zuordnung: benchmarks/benchmark_target_mapping.json.

## Benchmark-Bezug

- Relevante Benchmark-Dateien (proxy-basiert):
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_plugin_hot_plug.cpp

## Spezifische Erwartungswerte

| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| AI-1 Prompt-Validierung P99 | <= 1 ms | Proxy: Plugin-System Fast-Path in bench_plugin_system |
| AI-2 Plugin-Generierung Fehlerpfad P99 | <= 5 ms | Proxy: Plugin-System Lifecycle in bench_plugin_system |
| AI-3 Hot-Reload Impact | Throughput-Regression <= 10 % gg. Baseline | Proxy: bench_plugin_hot_plug |
| AI-4 Speicheroverhead je Request | <= 256 KB zusaetzlich pro Anfrage im Mittel | Proxy: Plugin-Lifecycle-Memory in bench_plugin_system |

## Validierung

- Erwartungswerte gelten als erfuellt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen.
- Da aktuell kein dedizierter ai-Benchmark registriert ist, gelten AI-1..AI-4 als Proxy-Ziele.
- Folgeaufgabe: dedizierten Benchmark bench_ai_plugin_generator registrieren.
