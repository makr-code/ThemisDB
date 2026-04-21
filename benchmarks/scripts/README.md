> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# scripts

Pfad: `benchmarks/scripts`

## Zweck
Skriptbasierte Benchmark-Helfer und CI-Orchestrierung für reproduzierbare Evaluationsläufe.

## Dateien nach Kategorien
- **Sourcecode**: `load_test_data.py`, `scientific_evaluation_framework.py`
- **Shell**: `run_benchmark.sh`

## Scientific Evaluation Framework

CLI für hypothesengetriebene Experimente mit Statistik- und Gate-Auswertung:

```bash
python3 benchmarks/scripts/scientific_evaluation_framework.py \
  --input /path/to/evaluation_input.json \
  --output /path/to/evaluation_report.json \
  --tickets-output /path/to/regression_tickets.json
```

## Hinweise
- Änderungen in diesem Ordner sollten mit den übergeordneten Architektur- und Sicherheitsrichtlinien des Projekts abgestimmt werden.
- Für tieferliegende Teilbereiche existieren ggf. zusätzliche README- und Moduldokumente.
