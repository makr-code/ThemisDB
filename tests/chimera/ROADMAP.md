> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# ROADMAP

## Current Status
- [x] Modul `tests/chimera` dokumentiert und CMakeLists.txt korrigiert (2026-07-27)
- [x] Fokussierte Test-Infrastruktur aktiv und validiert
- [x] 2200+ LOC Testabdeckung über 3 Testdateien

## In Progress
- [~] Dokumentations- und Qualitätslücken schließen (Target: 2026-Q4)
- [~] Adapter-Lifecycle und Dispatch-Härtung (Target: Q4 2026)

## Planned Features
- [ ] Schnittstellen und Verantwortlichkeiten präzisieren (Target: 2026-Q3)
- [ ] Testabdeckung für kritische Pfade erweitern (Target: 2026-Q3)
- [ ] Betriebs- und Security-Härtung abschließen (Target: 2026-Q4)

## Implementation Phases
### Phase 1: Design / API-Vertrag
- [ ] Öffentliche und interne Interfaces des Moduls konsolidieren (Target: 2026-Q2)
- [ ] Eingabe-/Ausgabeverträge und Fehlermodelle festlegen (Target: 2026-Q2)

### Phase 2: Core-Implementierung
- [ ] Kernlogik gemäß Schnittstellenvertrag vervollständigen (Target: 2026-Q3)
- [ ] Integrationspunkte zu abhängigen Modulen stabilisieren (Target: 2026-Q3)

### Phase 3: Fehlerbehandlung & Edge Cases
- [ ] Fehlerfälle systematisch abdecken (Target: 2026-Q3)
- [ ] Edge-Case-Handling und Guardrails ergänzen (Target: 2026-Q3)

### Phase 4: Tests
- [ ] Unit-Tests für Kernpfade ausbauen (Target: 2026-Q3)
- [ ] Integrations- und Regressionstests ergänzen (Target: 2026-Q3)

### Phase 5: Performance/Hardening
- [ ] Performance-Bottlenecks messen und reduzieren (Target: 2026-Q4)
- [ ] Security- und Reliability-Hardening abschließen (Target: 2026-Q4)

### Phase 6: Dokumentation & Abnahme
- [ ] Betriebs- und Entwicklerdokumentation aktualisieren (Target: 2026-Q4)
- [ ] Modulabnahme anhand Checklisten und Akzeptanzkriterien durchführen (Target: 2026-Q4)

## Production Readiness Checklist
- [x] CMakeLists.txt korrigiert - Tests werden jetzt korrekt registriert
- [x] Test-Targets registriert: module_chimera_<test>_focused mit ctest label
- [x] Testabdeckung: 2200+ LOC (test_themisdb_adapter.cpp, test_chimera_streaming.cpp, test_chimera_prepared_statements.cpp)
- [~] Definierte Fehlersemantik und Recovery-Pfade in Härtung (Q4 2026)
- [~] Security-Review und Dependency-Checks (Q4 2026)
- [x] Monitoring/Observability-Anforderungen berücksichtigt

## Known Issues & Limitations
- [!] Detailtiefe der Modul-spezifischen Akzeptanzkriterien variiert und wird sukzessive geschärft.

## Breaking Changes
- Keine bekannten Breaking Changes dokumentiert.
