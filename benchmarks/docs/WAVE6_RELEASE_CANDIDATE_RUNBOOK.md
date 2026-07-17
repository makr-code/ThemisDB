# WAVE 6 — Release-Candidate Performance Runbook

## Scope

Diese Runbook-Version operationalisiert Welle 6 für Go/No-Go-Entscheidungen:

- **B6-A:** Release-kritische Workload-Härtung (steady/burst/peak)
- **B6-B:** Soak-/Stress-Stabilität unter Langlauf
- **B6-C:** Degradation/Fault-Charakterisierung inkl. Recovery
- **B6-D:** Finale Gates, Varianz-Kontrolle, Operability/Triage

## Canonical Input / Output

- Input: `benchmarks/ci_wave6_release_candidate_experiments.json`
- Report: `benchmarks/results/wave6_report.json`
- Tickets: `benchmarks/results/wave6_regression_tickets.json`

## Reproducible Execution

```bash
python3 benchmarks/scripts/scientific_evaluation_framework.py \
  --input benchmarks/ci_wave6_release_candidate_experiments.json \
  --output benchmarks/results/wave6_report.json \
  --tickets-output benchmarks/results/wave6_regression_tickets.json
```

## Final Release-Blocking Guardrails

Ein Run ist **release-blocking failed**, wenn mindestens eines gilt:

1. `summary.gate_violations > 0`
2. `summary.wave6_gate_violations > 0`
3. Kritischer Workload (`critical=true`) mit Klassifikation `regressiv`

Zusätzliche Wave-6-Guardrails pro Experiment (je nach Track):

- `max_p99_latency_ms`
- `min_treatment_mean`
- `max_regression_drift_percent`
- `max_recovery_time_seconds_p95`

## Baseline & Variance Policy

1. `baseline_freeze` muss vollständig gesetzt sein (Compiler, Flags, Preset, Hardware, OS).
2. Seed muss fixiert sein (`seed=42` in canonical suite).
3. Mindeststichprobe: `n >= 30` je Baseline/Treatment.
4. Warmups werden separat geführt (`warmup_runs`) und nicht in Samples gemischt.
5. Fixtures/Dataset müssen stabil sein (`dataset_size` + frozen fixture identifiers).

## Diagnosepfad / Triage

Bei Gate-Verletzung:

1. Prüfe `results[].governance.wave6_gate_failures` für konkrete Guardrail-Breaches.
2. Prüfe `results[].wave6_analysis`:
   - `drift` für Langlauf-Degradation
   - `saturation` für Lastgrenzen/Kniepunkte
   - `recovery_time_seconds` für Wiederanlaufverhalten
3. Erzeuge/prüfe Ticket-Backlog via `wave6_regression_tickets.json`.
4. Wiederhole Repro-Lauf mit identischem Seed und identischem `baseline_freeze`.

## Ownership

- Initiale Ownership: `ticket_defaults.owner = release-performance`
- Priorisierung: Gate-Verletzung > regressiv > neutral mit großem Effekt

## Known Residual Risks

- Sehr große Stichproben erhöhen Auswertungszeit (CPU-seitige Python-Statistik).
- Sättigungsanalyse basiert auf bereitgestellten Lastpunkten; fehlende Laststufen senken Diagnosegenauigkeit.

## Post-Release Follow-ups

- Erweiterung auf zusätzliche degradierte Netzwerk-/Storage-Fehlerprofile.
- Persistente Trendhistorie (7/30/90 Tage) für Wave-6-Berichte.
- Automatische Issue-Erstellung aus Regression-Tickets (Dispatcher-Anbindung).
