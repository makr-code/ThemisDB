# PERFORMANCE_EXPECTATIONS — src/document

## Scope

- Modul: src/document
- Diese Datei dokumentiert modulspezifische Performance-Erwartungen fuer Document-Store, Diff/Merge und Schema-Validierung.
- Primarquelle fuer Benchmark-Zuordnung: benchmarks/benchmark_target_mapping.json.

## Benchmark-Bezug

- Relevante Benchmark-Dateien (proxy-basiert):
  - benchmarks/bench_crud.cpp
  - benchmarks/bench_api_endpoints.cpp
  - benchmarks/bench_content_processor_paths.cpp

## Spezifische Erwartungswerte

| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| DOC-1 CRUD Throughput | Throughput-Regression <= 10 % gg. Baseline | Proxy: bench_crud |
| DOC-2 Single-Document Serialisierung P95 | <= 5 ms | Proxy: BM_Json_Serialize_SingleDocument in bench_api_endpoints |
| DOC-3 Dokument-Update P99 | <= 20 ms | Proxy: Update-Pfade in bench_crud |
| DOC-4 Diff/Merge Laufzeit | p95 <= 25 ms fuer typische 10-Feld-Dokumente | Proxy: Content/CRUD-Pfade |
| DOC-5 Schema-Validierung Overhead | <= 15 % CPU-Overhead gg. ungeprueftem CRUD-Pfad | Proxy: bench_crud + API serialisation paths |

## Validierung

- Erwartungswerte gelten als erfuellt, wenn Release-Runs stabil und reproduzierbar sind.
- DOC-2 ist direkt als messbarer API-Proxy vorhanden; DOC-1/3/4/5 sind derzeit Proxy-Ziele.
- Folgeaufgabe: dedizierten Benchmark bench_document_store registrieren.
