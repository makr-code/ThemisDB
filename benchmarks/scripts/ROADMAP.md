# ROADMAP — benchmarks/scripts

## Current Status
- [x] `scientific_evaluation_framework.py` — vollständig implementiert: Hypothesenvalidierung, seeded Runner, Baseline-Freeze, Bootstrap-CI, Cohen's d, Cliff's Delta, Permutationstest, Governance-Gates, Ticket-Autogenerierung (Target: 2026-Q2)
- [x] `audit_benchmark_registration.py` — CMake-Registrierungsintegrität als CI-geeignetes CLI-Tool implementiert (Target: 2026-Q2)
- [x] Syntax-Fehler in `scientific_evaluation_framework.py` (C-Block-Header) behoben (Target: 2026-Q2)
- [x] Audit-, Security- und Roadmap-Dokumentation für `benchmarks/scripts` vervollständigt (Target: 2026-Q2)

## In Progress
- [~] CLI-Integration-Test für `scientific_evaluation_framework.py` als eigenständiges Test-Target (Target: 2026-Q3)

## Planned Features
- [ ] `load_test_data.py` URL-Allowlist ergänzen (BENCH-S01, Target: 2026-Q3)
- [ ] Persistente Report-Historisierung (rolling 7/30/90 Tage) als separater Aggregator (Target: 2026-Q3)
- [ ] Ticket-Dispatcher an GitHub-Issues-API anbinden — ohne Core-Evaluator zu koppeln (Target: 2026-Q3)
- [ ] Performance-Budget-Policies pro Subsystem in versionierter Konfigurationsdatei zentralisieren (Target: 2026-Q3)

## Implementation Phases

### Phase 1: Design / API-Vertrag
- [x] CLI-Interface (`--input`, `--output`, `--tickets-output`) festgelegt und dokumentiert (Target: 2026-Q2)
- [x] JSON-Eingabe-/Ausgabeverträge (`baseline_freeze`, `experiments[]`, `hypothesis`, `scenario`) implementiert (Target: 2026-Q2)
- [x] Fehlermodell: explizite Exceptions statt stiller Fehlschläge (Target: 2026-Q2)

### Phase 2: Core-Implementierung
- [x] Seeded Runner für deterministische Bootstrap-/Permutationsanalysen (Target: 2026-Q2)
- [x] Mindest-Stichprobengröße n ≥ 30 technisch erzwungen (Target: 2026-Q2)
- [x] Workload-Familien-Whitelist (`oltp|olap|graph|vector|rag|hybrid`) implementiert (Target: 2026-Q2)
- [x] CMake-Registrierungsaudit-Tool mit Exitcodes für CI (Target: 2026-Q2)

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] NaN/Inf-Schutz in Statistikpfad (Target: 2026-Q2)
- [x] Fehlende Pflichtfelder blockieren Pipeline mit klarer Fehlermeldung (Target: 2026-Q2)
- [x] Ungültige Baseline-Freeze-Felder detektiert und gemeldet (Target: 2026-Q2)
- [ ] URL-Allowlist für `load_test_data.py` (Target: 2026-Q3)

### Phase 4: Tests
- [x] Unit-Tests für Klassifikation, Determinismus, n≥30-Validierung und Gate/Ticket-Logik (Target: 2026-Q2)
- [x] Regressionspfad via Ticket-Autogenerierung testabgedeckt (Target: 2026-Q2)
- [ ] CLI-Integration-Test (vollständiger `--input`/`--output`-Aufruf) als Test-Target (Target: 2026-Q3)
- [ ] Contract-Tests für Input-/Output-Schema und Rückwärtskompatibilität (Target: 2026-Q3)

### Phase 5: Performance/Hardening
- [x] Statistische Auswertung deterministisch und reproduzierbar (Target: 2026-Q2)
- [x] Core-Evaluator ohne externe Netzwerkabhängigkeiten (Target: 2026-Q2)
- [x] Syntax-Korrektheit aller Skripte verifiziert (Target: 2026-Q2)
- [ ] Laufzeit-Profiling bei sehr großen Stichproben (> 10K Samples) (Target: 2026-Q4)

### Phase 6: Dokumentation & Abnahme
- [x] AUDIT.md mit konkreten Findings abgeschlossen (Target: 2026-Q2)
- [x] SECURITY.md mit Bedrohungsmodell und Findings abgeschlossen (Target: 2026-Q2)
- [x] scripts/ROADMAP.md konkretisiert (Target: 2026-Q2)
- [x] CI-Gate-Spezifikation in `benchmarks/docs/CI_GATE.md` dokumentiert (Target: 2026-Q2)

## Production Readiness Checklist
- [x] Definierte Fehlersemantik und Recovery-Pfade (harte Validierungsfehler)
- [x] Ausreichende Testabdeckung für Core-Logik (Klassifikation, Determinismus, Gates)
- [x] Security-Review abgeschlossen — kein kritisches/hohes Finding
- [x] Keine externen PyPI-Abhängigkeiten im Core-Evaluator
- [x] Dokumentation für Betrieb und Entwicklung vollständig
- [ ] CLI-Integration-Test als eigenständiges Test-Target (offen)

## Known Issues & Limitations
- [I] `load_test_data.py` akzeptiert externe URLs ohne Allowlist (BENCH-S01 — Low-Risk, Testdaten-Script)
- [I] Statistische Auswertung für sehr große Stichproben (> 10K) erhöht Laufzeit deutlich — CPU-seitig in Python

## Breaking Changes
- Keine Breaking Changes: bestehende Scripts und ihr CLI-Interface bleiben unverändert.
