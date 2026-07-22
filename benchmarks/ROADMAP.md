# ROADMAP

## Current Status
- [x] Wissenschaftliches Evaluations-Framework in `benchmarks/scripts/scientific_evaluation_framework.py` implementiert (Target: 2026-Q2)
- [x] Dokumentierter End-to-End-Playbook- und Katalogpfad in `benchmarks/docs/SCIENTIFIC_EVALUATION_FRAMEWORK.md` bereitgestellt (Target: 2026-Q2)
- [x] CI-Gate-Spezifikation für Framework-CLI als verpflichtender Gate-Job in `benchmarks/docs/CI_GATE.md` dokumentiert (Target: 2026-Q2)
- [x] Root-Benchmark-Dokumentation auf aktuelle Presets und CMake-Gating normalisiert (README, INDEX, AUDIT, SECURITY, ROADMAP) (Target: 2026-Q2)
- [x] CMake-Registrierungsaudit-Tool `audit_benchmark_registration.py` liefert reproduzierbare Drift-Erkennung — Re-Audit 2026-07-17: 217 Quellen entdeckt, 216 registriert, 1 intentional excluded, 0 unregistrierte (Target: 2026-Q3)
- [x] Syntax-Fehler (C-Block-Header) in `scientific_evaluation_framework.py` behoben (Target: 2026-Q2)
- [x] Wave-6-Release-Candidate-Suite inkl. finaler Guardrails/Drift-/Sättigungs-/Recovery-Auswertung über `benchmarks/ci_wave6_release_candidate_experiments.json` und Framework-Reportfelder umgesetzt (Target: 2026-Q3)

## In Progress
- [~] Wave-6-Operability-Härtung: reproduzierbarer Repro-/Triage-Runbook-Pfad für finale Go/No-Go-Entscheidungen (Target: 2026-Q3)
- [x] **Wave 4 (B4-A):** Release-kritische Governance-Benchmarks (W4A-01..W4A-06) + `release_gate_manifest.json` implementiert (Target: 2026-Q3)
- [x] **Wave 4 (B4-B):** Resilience/Degradations-Szenarien (W4B-01..W4B-06): Latenz-Injektion, Backpressure, CPU-Contention, Partial-Failure, Recovery (Target: 2026-Q3)
- [x] **Wave 4 (B4-C):** Determinismus- und Varianz-Benchmarks (W4C-01..W4C-06): CV-Akzeptanzgates, Seed-Stabilität, Warmup-Effektivität, Teardown-Isolation (Target: 2026-Q3)
- [x] **Wave 4 (B4-D):** Diagnose- und Reporting-Benchmarks (W4D-01..W4D-05) + `report_variance.py` + `RUNBOOK.md` implementiert (Target: 2026-Q3)

- [~] Welle 3 (B3-A/B3-B/B3-C): Full-Function Critical Workloads + Scale/Stress + Regression-Guardrails in `wave3_benchmark_suite.py` und `wave3_workload_profiles.json` (Target: 2026-Q3)
- [x] CMake-Registrierungsdrift für 11 Benchmark-Quellen (`wave5/`, `wave7/`, Root-Dispatch-Binaries) abgebaut — Re-Audit bestätigt 0 Unregistrierte (Target: 2026-Q3)
- [~] Schichtbezogene Benchmark-Matrix (ANN/Tensor/Graph) mit evidenzbasierten CPU-SIMD-vs-GPU-Szenarien, Break-Even-Schwellenwerten und Dynamic-Tensor-Update-Track (Issue: #5466, Target: 2026-Q3)
  - [x] Benchmark-Matrix-Dokument erstellt: `docs/benchmarks/CPU_SIMD_GPU_DISPATCH_BENCHMARK_MATRIX.md`
  - [x] Szenario-Katalog erstellt: `docs/benchmarks/CPU_SIMD_GPU_BENCHMARK_SCENARIO_CATALOG.md`
  - [~] Benchmark-Binaries implementieren: `bench_ann_cpu_gpu_dispatch` ✅, `bench_tensor_cpu_gpu_dispatch` ✅, `bench_graph_cpu_gpu_dispatch`, `bench_tensor_update_dispatch`, `bench_tensor_commit_overhead`, `bench_cross_cutting` (Target: 2026-Q3)
  - [~] CTest-Smoke-Targets für neue Binaries registrieren: `smoke_bench_ann_cpu_gpu_dispatch` ✅, `smoke_bench_tensor_cpu_gpu_dispatch` ✅, weitere Targets offen (Target: 2026-Q3)
  - [ ] Hardware-Runs auf GPU-Runner für Break-Even-Validierung (Target: 2026-Q3)
  - [ ] Performance-Baselines in `benchmarks/baselines/` eintragen (Target: 2026-Q3)

## Planned Features
- [x] Phase-0 CRUD Baseline Benchmark Suite anlegen (P0-D04) — `benchmarks/phase0/` inkl. `bench_p0_crud_baseline.cpp` mit 4 Workloads (insert, read, update, delete) (Target: 2026-Q3)
- [x] Phase-0 Messpfad + Baseline-JSON implementiert: `phase0_fixtures.h` mit kanonischen Konstanten, `baseline_p0_v0.json` mit Erwartungen (Target: 2026-Q3)
- [x] Phase-0 Runbook & Measurement Protocol dokumentiert für manuellen + CI-Messpfad (Target: 2026-Q3)
- [ ] Persistente Historisierung von Eval-Reports für Trendanalysen über Releases hinweg (Target: 2026-Q3)
- [ ] Erweiterte Kostenmodelle (Cloud-Instance-Typen, Energie/KWh) in Gate-Entscheidungen integrieren (Target: 2026-Q3)
- [ ] Schichtbezogene Benchmark-Matrix (ANN/Tensor/Graph/LLM) mit evidenzbasierten Referenzsuites dokumentieren (Target: 2026-Q3)
- [ ] Baseline-Verzeichnis mit initialen Wave-4-Baseline-JSONs befüllen und in CI integrieren (Target: 2026-Q4)
- [ ] GitHub Actions Workflow `.github/workflows/benchmark-gate.yml` aktivieren (Wave 4 gate contract) (Target: 2026-Q4)
- [ ] `report_variance.py` als eigenständiges CI-Test-Target registrieren (Target: 2026-Q4)

## Implementation Phases
### Phase 1: Design / API-Vertrag
- [x] Hypothesenvertrag (H0/H1, Effekt-Richtung, Risiken, Stop-Kriterien) als strukturiertes JSON-Schema implementiert (Target: 2026-Q2)
- [x] Workload-Matrix-Validierung für `oltp|olap|graph|vector|rag|hybrid` inkl. Dataset/Query/Concurrency-Profil umgesetzt (Target: 2026-Q2)
- [x] Interferenzfaktoren (Warmup, Caching, NUMA, I/O, GPU) als Pflichtfelder im Szenariovertrag verankert (Target: 2026-Q2)

### Phase 2: Core-Implementierung
- [x] Seeded Runner für deterministische Bootstrap-/Permutation-Analysen implementiert (Target: 2026-Q2)
- [x] Baseline-Freeze-Validierung (Compiler, Flags, Preset, Hardware, OS) als harte Vorbedingung implementiert (Target: 2026-Q2)
- [x] Mindest-Samplegröße `n >= 30` pro Baseline/Treatment technisch erzwungen (Target: 2026-Q2)
- [~] W3-Suite: produktionsnahe Workload-Profile (`read-heavy|write-heavy|mixed`) mit priorisierten Critical-Flows implementiert (Target: 2026-Q3)

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] Guardrails für fehlende/ungültige Felder und NaN/Inf-Samples implementiert (Target: 2026-Q2)
- [x] Fehlende oder fehlerhafte Experimente werden mit klaren Exceptions blockiert statt stillschweigend ignoriert (Target: 2026-Q2)
- [~] W3-Guardrails für offensichtliche Regressionen (`throughput_drop`, `p95`, `p99`) pro Workload-Profil ergänzt (Target: 2026-Q3)

### Phase 4: Tests
- [x] Fokussierte Unittests für Klassifikation, Determinismus, n>=30-Validierung und Gate/Ticket-Logik hinzugefügt (Target: 2026-Q2)
- [x] Regressionspfad (kritische Metrik verletzt Budget) via Ticket-Autogenerierung testabgedeckt (Target: 2026-Q2)

### Phase 5: Performance/Hardening
- [x] Statistikpfad mit Bootstrap-CI, Cohen's d, Cliff's Delta, Permutationstest und Practical Significance umgesetzt (Target: 2026-Q2)
- [x] Ergebnisklassifikation `regressiv|neutral|signifikant_positiv` für Merge-/Release-Gates eingeführt (Target: 2026-Q2)
- [x] Performance-Budget pro Experiment/Subsystem (`performance_budget_percent`) in Governance integriert (Target: 2026-Q2)

### Phase 6: Dokumentation & Abnahme
- [x] Experiment-Playbook inkl. Ausführung/Statistik/Gates dokumentiert (Target: 2026-Q2)
- [x] Benchmark-Katalog mit initialer Family/Metrik-Zuordnung dokumentiert (Target: 2026-Q2)
- [x] CI-Report- und evidenzbasierter Backlog-Mechanismus dokumentiert (Target: 2026-Q2)
- [x] Wave 4 Runbook (`benchmarks/wave4/RUNBOOK.md`) — Gates, Baselines, Varianz- und Diagnosekonzept, bekannte Restlücken (Target: 2026-Q3)

## Production Readiness Checklist
- [x] Definierte Fehlersemantik und Recovery-Pfade (harte Validierungsfehler)
- [x] Ausreichende Testabdeckung inkl. Regression für neue Framework-Logik
- [x] Security-Review: keine externen Netzwerkabhängigkeiten, nur lokale JSON-I/O
- [x] Monitoring/Observability via strukturierte JSON-Reports und Ticket-Outputs
- [x] Dokumentation für Betrieb und Entwicklung vollständig für das Framework-CLI
- [x] Wave 4 release-kritische Gates (W4A-01..W4A-06) mit versioniertem Manifest implementiert
- [x] Wave 4 Resilience/Degradation-Szenarien (W4B) für verwertbare Performance-Aussagen implementiert
- [x] Wave 4 Varianz-Akzeptanzkriterien (CV ≤ 15%) und Determinismus-Protokoll definiert und benchmarkabgedeckt

## Known Issues & Limitations
- [I] Statistische Verfahren basieren auf CPU-seitiger Python-Auswertung; sehr große Stichproben erhöhen Laufzeit deutlich.
- [I] Ein benchmark-spezifischer Workflow `.github/workflows/benchmark-gate.yml` fehlt weiterhin; aktive PR-Gates existieren bereits unter `.github/workflows/`, decken den Benchmark-Registrierungsaudit aber noch nicht ab. Gate-Spezifikation liegt in `benchmarks/docs/CI_GATE.md` bereit.
- [I] Wave 4 Baseline-Verzeichnis (`benchmarks/wave4/baselines/`) ist initial leer — muss nach erstem validen Release-Lauf befüllt werden.
- [I] `BackpressureSimulator` in `wave4_fixtures.h` ist nicht thread-safe für parallele Benchmark-Bodies; für Multi-Thread-Szenarien pro Thread instanziieren.

## Breaking Changes
- Keine Breaking Changes: bestehende Benchmark-Skripte bleiben unverändert, Wave 4 ist additiv.
