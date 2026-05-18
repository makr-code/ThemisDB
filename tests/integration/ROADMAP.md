> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# ROADMAP

## Current Status
- [~] Cross-Module-Pipeline-Test-Suite initial umgesetzt (Target: 2026-Q3)
- [x] Gemeinsame Pipeline-Testhelpers in `test_fixture.h` und `test_data_generator.h` erweitert (Target: 2026-Q3)
- [x] CTest-Label `pipeline_integration` eingeführt und für neue Pipeline-Tests vergeben (Target: 2026-Q3)

## In Progress
- [~] Pipeline-Suite in CI-Läufen stabilisieren und Flaky-Risiken reduzieren (Target: 2026-Q3)

## Planned Features
- [ ] Weitere produktionsnahe Backends (ohne externe Runtime-Abhängigkeiten) in Pipeline-Tests integrieren (Target: 2026-Q4)
- [ ] Pipeline-spezifische Regression-Gates in Nightly-Läufen ausbauen (Target: 2026-Q4)
- [ ] Security-Hardening-Szenarien für Schlüsselrotation und Audit-Pfade erweitern (Target: 2026-Q4)

## Implementation Phases
### Phase 1: Design / API-Vertrag
- [x] Pipeline-Grenzen und Test-IDs für Query/Ingestion/RAG/TXR/Security/Analytics festgelegt (Target: 2026-Q3)
- [x] Naming-Schema und Datei-Layout für `tests/integration/pipeline/` dokumentiert (Target: 2026-Q3)

### Phase 2: Core-Implementierung
- [x] `query_execution_pipeline_test.cpp` mit QP-01..QP-05 umgesetzt (Target: 2026-Q3)
- [x] `ingestion_pipeline_test.cpp` mit IP-01..IP-04 umgesetzt (Target: 2026-Q3)
- [x] `rag_ai_pipeline_test.cpp` mit RAG-01..RAG-04 umgesetzt (Target: 2026-Q3)
- [x] `transaction_replication_pipeline_test.cpp` mit TXR-01..TXR-04 umgesetzt (Target: 2026-Q3)
- [x] `security_pipeline_test.cpp` mit SEC-01..SEC-03 umgesetzt (Target: 2026-Q3)
- [x] `analytics_export_pipeline_test.cpp` mit AEP-01..AEP-03 umgesetzt (Target: 2026-Q3)

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] Auth/Syntax/Content/Inference/Failover-Fehlerpfade als eigene Pipeline-Tests ergänzt (Target: 2026-Q3)
- [~] Weitere Edge-Cases für Timeouts, Retries und Circuit-Breaker in Planung (Target: 2026-Q4)

### Phase 4: Tests
- [x] Neue Pipeline-Tests unter CTest-Label `pipeline_integration` registriert (Target: 2026-Q3)
- [x] Offline-Ausführbarkeit über Mocks ohne externe Services sichergestellt (Target: 2026-Q3)

### Phase 5: Performance/Hardening
- [ ] Laufzeit- und Stabilitätsprofile der Pipeline-Tests im Nightly-Run messen (Target: 2026-Q4)
- [ ] Security-/Reliability-Hardening für produktionsnahe Integrationspfade erweitern (Target: 2026-Q4)

### Phase 6: Dokumentation & Abnahme
- [x] `INTEGRATION_TEST_GUIDELINES.md` um Pipeline-Konventionen und Label-Flow ergänzt (Target: 2026-Q3)
- [~] Abnahme der Pipeline-Suite über CI-Stabilisierung und Regression-Baselines offen (Target: 2026-Q4)

## Production Readiness Checklist
- [x] Definierte Fehlersemantik und Recovery-Pfade für die 6 Kernpipelines
- [x] Zusätzliche modulübergreifende Testabdeckung inkl. Regression-Szenarien eingeführt
- [ ] Security-Review und Dependency-Checks vollständig abgeschlossen
- [x] Observability-/Audit-Signale in Pipeline-Tests verifiziert
- [~] Dokumentation weiter mit CI-/Betriebsrunbooks synchronisieren

## Known Issues & Limitations
- [!] Pipeline-Tests laufen offline mit Mocks; vollständige End-to-End-Abdeckung mit externen Diensten bleibt zusätzlich erforderlich.
- [!] Sandbox-Build kann durch fehlende Toolchain-Komponenten (vcpkg/Ninja) blockiert sein und muss in CI gegengeprüft werden.

## Breaking Changes
- Keine bekannten Breaking Changes dokumentiert.
