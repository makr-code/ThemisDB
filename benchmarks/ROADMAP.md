# ROADMAP

## Current Status
- [x] Wissenschaftliches Evaluations-Framework in `benchmarks/scripts/scientific_evaluation_framework.py` implementiert (Target: 2026-Q2)
- [x] Dokumentierter End-to-End-Playbook- und Katalogpfad in `benchmarks/docs/SCIENTIFIC_EVALUATION_FRAMEWORK.md` bereitgestellt (Target: 2026-Q2)

## In Progress
- [~] CI-Job-Anbindung des Framework-CLI als verpflichtender Gate-Job (Target: 2026-Q2)

## Planned Features
- [ ] Persistente Historisierung von Eval-Reports für Trendanalysen über Releases hinweg (Target: 2026-Q3)
- [ ] Erweiterte Kostenmodelle (Cloud-Instance-Typen, Energie/KWh) in Gate-Entscheidungen integrieren (Target: 2026-Q3)

## Implementation Phases
### Phase 1: Design / API-Vertrag
- [x] Hypothesenvertrag (H0/H1, Effekt-Richtung, Risiken, Stop-Kriterien) als strukturiertes JSON-Schema implementiert (Target: 2026-Q2)
- [x] Workload-Matrix-Validierung für `oltp|olap|graph|vector|rag|hybrid` inkl. Dataset/Query/Concurrency-Profil umgesetzt (Target: 2026-Q2)
- [x] Interferenzfaktoren (Warmup, Caching, NUMA, I/O, GPU) als Pflichtfelder im Szenariovertrag verankert (Target: 2026-Q2)

### Phase 2: Core-Implementierung
- [x] Seeded Runner für deterministische Bootstrap-/Permutation-Analysen implementiert (Target: 2026-Q2)
- [x] Baseline-Freeze-Validierung (Compiler, Flags, Preset, Hardware, OS) als harte Vorbedingung implementiert (Target: 2026-Q2)
- [x] Mindest-Samplegröße `n >= 30` pro Baseline/Treatment technisch erzwungen (Target: 2026-Q2)

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] Guardrails für fehlende/ungültige Felder und NaN/Inf-Samples implementiert (Target: 2026-Q2)
- [x] Fehlende oder fehlerhafte Experimente werden mit klaren Exceptions blockiert statt stillschweigend ignoriert (Target: 2026-Q2)

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

## Production Readiness Checklist
- [x] Definierte Fehlersemantik und Recovery-Pfade (harte Validierungsfehler)
- [x] Ausreichende Testabdeckung inkl. Regression für neue Framework-Logik
- [x] Security-Review: keine externen Netzwerkabhängigkeiten, nur lokale JSON-I/O
- [x] Monitoring/Observability via strukturierte JSON-Reports und Ticket-Outputs
- [x] Dokumentation für Betrieb und Entwicklung vollständig für das Framework-CLI

## Known Issues & Limitations
- [I] Statistische Verfahren basieren auf CPU-seitiger Python-Auswertung; sehr große Stichproben erhöhen Laufzeit deutlich.
- [!] Aktive GitHub-Workflow-Definitionen fehlen im Repository (`.github/no_workflows`), daher ist die CI-Aktivierung aktuell nur über externe/Downstream-Pipelines möglich.

## Breaking Changes
- Keine Breaking Changes: bestehende Benchmark-Skripte bleiben unverändert, neues Framework ist additiv.
