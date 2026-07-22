# Benchmarks Index (`benchmarks/`)

> **Navigation:** Dieser Index verweist auf die Modulstruktur. Links bei Umstrukturierung aktualisieren.

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
- **Phase-0 CRUD Baseline:** [`phase0/RUNBOOK.md`](phase0/RUNBOOK.md), [`phase0/MEASUREMENT_PROTOCOL.md`](phase0/MEASUREMENT_PROTOCOL.md) — Initial baseline establishment (P0-D04)
- Wave-3 Full-Function Suite: [`docs/WAVE3_BENCHMARK_SUITE.md`](docs/WAVE3_BENCHMARK_SUITE.md)
- Wave-5 Release-Gate Suite: [`wave5/RUNBOOK_W5.md`](wave5/RUNBOOK_W5.md)
- Wave-7 Final-Signoff-Suite: [`wave7/RUNBOOK_W7.md`](wave7/RUNBOOK_W7.md), [`wave7/WAVE7_BENCHMARK_COVERAGE.md`](wave7/WAVE7_BENCHMARK_COVERAGE.md)
- Registrierungs-Audit: `python3 benchmarks/scripts/audit_benchmark_registration.py`

## Fachbereiche

- Modul-Benchmarks: [`ai/`](ai/), [`analytics/`](analytics/), [`aql/`](aql/), [`core/`](core/), [`query/`](query/), [`rag/`](rag/), [`server/`](server/), [`transaction/`](transaction/)
- Standards & Suites: [`tpc/README.md`](tpc/README.md), [`ycsb/README.md`](ycsb/README.md), [`mmdb/README.md`](mmdb/README.md), [`ann/README.md`](ann/README.md), [`ldbc/README.md`](ldbc/README.md)
- CHIMERA: [`chimera/README.md`](chimera/README.md)
- Analyse/Reports: [`results_analysis_reports/README.md`](results_analysis_reports/README.md)
- Hilfsskripte: [`scripts/README.md`](scripts/README.md)

## Ausführung

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
python3 benchmarks/run_benchmark_orchestrator.py --help
```

## Historie

Legacy-Dokumente mit umgebungsspezifischen Pfaden (z. B. lokale Windows-Pfade) gelten als historisch; dieser Index verweist nur auf aktuelle Einstiege.
