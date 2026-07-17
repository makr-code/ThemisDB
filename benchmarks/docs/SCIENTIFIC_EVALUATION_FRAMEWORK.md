# Scientific Evaluation Framework (Performance, Qualität, Regressionen)

## 1. Experiment-Playbook

### 1.1 Hypothesen-Template (H0/H1)
Jedes Experiment muss enthalten:
- `h0`: Nullhypothese
- `h1`: Alternativhypothese
- `expected_effect_direction`: `improve|regress|neutral`
- `risks`: dokumentierte Interferenz-/Validitätsrisiken
- `stop_criteria`: harte Abbruchkriterien

### 1.2 Workload-Matrix (standardisiert)
Pflicht-Familien:
- `oltp`
- `olap`
- `graph`
- `vector`
- `rag`
- `hybrid`

Pflichtfelder pro Szenario:
- `dataset_size`
- `query_mix`
- `concurrency_profile`
- `warmup_runs`
- `cache_mode`
- `numa_mode`
- `io_profile`
- `gpu_allocation`

### 1.3 Reproduzierbare Ausführung
`benchmarks/scripts/scientific_evaluation_framework.py` erzwingt:
- Seeded Analyse-Pipeline (`seed`)
- Baseline-Freeze (`compiler`, `compiler_flags`, `preset`, `hardware_profile`, `os_image`)
- Mehrfachläufe mit `n >= 30` Samples pro Baseline/Treatment

## 2. Statistische Auswertung

Für jedes Experiment:
- Bootstrap-Konfidenzintervall (`bootstrap_ci`) für Mittelwertdifferenz
- Effektstärken: `cohens_d`, `cliffs_delta`
- p-Wert (Permutationstest)
- Practical Significance via `practical_significance_percent`
- Klassifikation: `regressiv | neutral | signifikant_positiv`

## 3. Governance und Gates

- Kritische Metriken (`critical=true`) werden gegen `performance_budget_percent` geprüft.
- Bei Gate-Verletzung wird ein Regression-Ticket-Objekt erzeugt (`regression_tickets`).
- CI kann `summary.gate_violations > 0` als Fail-Gate nutzen.
- Für Release-Candidate-Härtung (Wave 6) werden zusätzliche Guardrails über `wave6_guardrails` ausgewertet:
  - `max_p99_latency_ms`
  - `min_treatment_mean`
  - `max_regression_drift_percent`
  - `max_recovery_time_seconds_p95`

## 4. Benchmark-Katalog (Initial)

| Family | Beispiel-Metrik | Ziel |
|---|---|---|
| OLTP | `p95_latency_ms` | Latenzbudget und Fehlerfreiheit |
| OLAP | `throughput_rows_per_sec` | Durchsatzsteigerung bei stabiler p95 |
| Graph | `traversal_latency_ms` | Deterministische Traversal-Performance |
| Vector | `topk_latency_ms` | Latenz/Kosten unter Last |
| RAG | `end_to_end_latency_ms` | Retrieval+Generation Stabilität |
| Hybrid | `hybrid_query_latency_ms` | Kein Regressionseffekt bei gemischter Last |

## 5. CI-Report und Evidence-Backlog

Das Framework schreibt einen revisionssicheren JSON-Report mit:
- Baseline-Freeze
- Statistik und Klassifikation je Experiment
- Gate-Entscheidung
- Auto-generierten Regressionstickets

Backlog-Priorisierung erfolgt evidenzbasiert über:
1. `gate_violation=true` (höchste Priorität)
2. `classification=regressiv` mit großem Effekt (`cohens_d`/`cliffs_delta`)
3. große neutrale Effekte ohne Signifikanz (Sampling/Design nachschärfen)

## 6. CLI (End-to-End in CI)

```bash
python3 benchmarks/scripts/scientific_evaluation_framework.py \
  --input /path/to/evaluation_input.json \
  --output /path/to/evaluation_report.json \
  --tickets-output /path/to/regression_tickets.json
```

Damit ist ein vollständiger End-to-End-Evaluationszyklus CI-fähig.

## 7. Wave 6 (Release Candidate Hardening)

Die Wave-6-Suite liegt in:

- `benchmarks/ci_wave6_release_candidate_experiments.json`

Zusätzliche Wave-6-Ausgaben im Report:

- `results[].wave6_track` (B6-A/B6-B/B6-C/B6-D Zuordnung)
- `results[].wave6_analysis.drift` (Throughput-/Latency-Drift über Zeitfenster)
- `results[].wave6_analysis.saturation` (geschätzter Sättigungspunkt)
- `results[].wave6_analysis.recovery_time_seconds` (p50/p95/p99 Recovery-Verteilung)
- `results[].governance.wave6_gate_failures` (konkrete Guardrail-Verletzungen)
- `summary.wave6_gate_violations` und `summary.wave6_tracks`
