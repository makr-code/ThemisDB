# PERFORMANCE_EXPECTATIONS — src/scraper

## Scope

- Modul: src/scraper
- Diese Datei dokumentiert modulspezifische Performance-Erwartungen fuer Scraper-Pipeline, Rendering und Metadatenpfad.
- Primarquelle fuer Benchmark-Zuordnung: benchmarks/benchmark_target_mapping.json.

## Benchmark-Bezug

- Relevante Benchmark-Dateien (proxy-basiert):
  - benchmarks/bench_text_extraction.cpp
  - benchmarks/bench_content_processor_paths.cpp
  - benchmarks/bench_ingestion_extraction.cpp

## Spezifische Erwartungswerte

| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| SCR-1 Text-Extraktion Throughput | >= 50 MB/s bei Standarddokumenten | Proxy: bench_text_extraction |
| SCR-2 End-to-End Extraktion P95 | <= 50 ms pro Dokument (typische Groesse) | Proxy: bench_ingestion_extraction |
| SCR-3 JS-Renderer Overhead | <= 20 % gg. Non-JS Pfad | Proxy: bench_content_processor_paths |
| SCR-4 Metadaten-Schreibpfad P99 | <= 15 ms | Proxy: bench_ingestion_extraction |
| SCR-5 Stabilitaet unter Last | Throughput-Regression <= 10 % gg. Baseline | Proxy: oben genannte Benchmarks |

## Validierung

- Erwartungswerte gelten als erfuellt, wenn die Proxy-Benchmarks im Release-Profil reproduzierbar laufen.
- Folgeaufgabe: dedizierten Benchmark bench_scraper_pipeline registrieren.
