> **Navigation:** Dieser Index verweist auf die Modulstruktur. Links bei Umstrukturierung aktualisieren.

# Benchmarks Index (`benchmarks/`)

Kondensierter Navigationsindex auf Basis des aktuellen Repository-Inhalts.

## Primäre Einstiege

- [`README.md`](README.md)
- [`ROADMAP.md`](ROADMAP.md)
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- [`AUDIT.md`](AUDIT.md)
- [`SECURITY.md`](SECURITY.md)

## Qualitätssicherung & CI

- CI-Gate-Spezifikation: [`docs/CI_GATE.md`](docs/CI_GATE.md)
- Wissenschaftliches Evaluations-Framework: [`docs/SCIENTIFIC_EVALUATION_FRAMEWORK.md`](docs/SCIENTIFIC_EVALUATION_FRAMEWORK.md)
- Wave-3 Full-Function Suite: [`docs/WAVE3_BENCHMARK_SUITE.md`](docs/WAVE3_BENCHMARK_SUITE.md)
- Registrierungs-Audit: `python3 benchmarks/scripts/audit_benchmark_registration.py`

## Fachbereiche

- Modul-Benchmarks: [`ai/`](ai/), [`analytics/`](analytics/), [`aql/`](aql/), [`core/`](core/), [`query/`](query/), [`rag/`](rag/), [`server/`](server/), [`transaction/`](transaction/)
- Standards & Suites: [`tpc/README.md`](tpc/README.md), [`ycsb/README.md`](ycsb/README.md), [`mmdb/README.md`](mmdb/README.md), [`ann/README.md`](ann/README.md), [`ldbc/README.md`](ldbc/README.md)
- CHIMERA: [`chimera/README.md`](chimera/README.md)
- Analyse/Reports: [`results_analysis_reports/README.md`](results_analysis_reports/README.md)
- Hilfsskripte: [`scripts/README.md`](scripts/README.md)

## Ausführung

```bash
cmake --preset linux-ninja-perf
cmake --build --preset linux-ninja-perf
python3 benchmarks/run_benchmark_orchestrator.py --help
```

## Historie

Legacy-Dokumente mit umgebungsspezifischen Pfaden (z. B. lokale Windows-Pfade) gelten als historisch; dieser Index verweist nur auf aktuelle Einstiege.
