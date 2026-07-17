> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# ROADMAP

## Current Status
- [x] Cross-Module-Pipeline-Test-Suite Wave 1 abgeschlossen (Target: 2026-Q3)
- [x] Wave 2 Cross-Modul-Härtung: CMX-01..CMX-06 + REC-01..REC-07 umgesetzt (Target: 2026-Q3)
- [x] Wave 3 Critical-Flow- und Recovery-Suiten: FFW-01..FFW-08 + DIR-01..DIR-08 umgesetzt (Target: 2026-Q3)
- [x] Wave 5 Release-Critical E2E- und Failure/Recovery-Suiten: E2E-01..E2E-08 + FIR-01..FIR-08 umgesetzt (Target: 2026-Q3)
- [x] Wave 6 Release-Candidate-Hardening: RCJ-01..RCJ-08 + SSS-01..SSS-08 + FIR-01..FIR-08 umgesetzt (Target: 2026-Q3)
- [x] Wave 7 Final-Signoff- und Recovery/Endurance-Suiten: FJS-01..FJS-08 + HCR-01..HCR-08 + ESC-01..ESC-08 umgesetzt (Target: 2026-Q3)
- [x] Gemeinsame Pipeline-Testhelpers in `test_fixture.h` und `test_data_generator.h` erweitert (Target: 2026-Q3)
- [x] CTest-Label `pipeline_integration` und `cross_module` eingeführt (Target: 2026-Q3)
- [x] `SeededTestDataGenerator` + `MockRetryScheduler` + `DeterministicIntegrationFixture` hinzugefügt (Target: 2026-Q3)

## In Progress
- [~] Pipeline-Suite in CI-Läufen stabilisieren und Flaky-Risiken reduzieren (Target: 2026-Q3)

## Planned Features
- [ ] Weitere produktionsnahe Backends (ohne externe Runtime-Abhängigkeiten) in Pipeline-Tests integrieren (Target: 2026-Q4)
- [ ] Pipeline-spezifische Regression-Gates in Nightly-Läufen ausbauen (Target: 2026-Q4)
- [~] Security-Hardening-Szenarien für Schlüsselrotation und Audit-Pfade erweitern (Target: 2026-Q4)

## Implementation Phases
### Phase 1: Design / API-Vertrag
- [x] Pipeline-Grenzen und Test-IDs für Query/Ingestion/RAG/TXR/Security/Analytics festgelegt (Target: 2026-Q3)
- [x] Naming-Schema und Datei-Layout für `tests/integration/pipeline/` dokumentiert (Target: 2026-Q3)

### Phase 2: Core-Implementierung
- [x] `query_execution_pipeline_test.cpp` mit QP-01..QP-05 umgesetzt (Target: 2026-Q3)
- [x] `ingestion_pipeline_test.cpp` mit IP-01..IP-04 umgesetzt (Target: 2026-Q3)
- [x] `rag_ai_pipeline_test.cpp` mit RAG-01..RAG-04 umgesetzt (Target: 2026-Q3)
- [x] `transaction_replication_pipeline_test.cpp` mit TXR-01..TXR-04 umgesetzt (Target: 2026-Q3)
- [x] `security_pipeline_test.cpp` mit SEC-01..SEC-06 umgesetzt (Target: 2026-Q3)
- [x] `analytics_export_pipeline_test.cpp` mit AEP-01..AEP-03 umgesetzt (Target: 2026-Q3)
- [x] `application_profile_pipeline_test.cpp` mit APP-01..APP-13 für produktionsnahes Nutzungsprofil umgesetzt (Target: 2026-Q3)
- [x] Wave 2 W2-A/W2-B: `cross_module_ingest_index_query_test.cpp` mit CMX-01..CMX-06 umgesetzt (Target: 2026-Q3)
- [x] Wave 2 W2-B: `cross_module_recovery_pipeline_test.cpp` mit REC-01..REC-07 umgesetzt (Target: 2026-Q3)
- [x] Wave 2 W2-C: `SeededTestDataGenerator` + `GenerateDocumentBatch()` + `GenerateVectorBatch()` in `test_data_generator.h` ergänzt (Target: 2026-Q3)
- [x] Wave 2 W2-C: `MockRetryScheduler` + `DeterministicIntegrationFixture` in `test_fixture.h` ergänzt (Target: 2026-Q3)

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] Auth/Syntax/Content/Inference/Failover-Fehlerpfade als eigene Pipeline-Tests ergänzt (Target: 2026-Q3)
- [x] Weitere Edge-Cases für Timeouts, Retries und Circuit-Breaker umgesetzt (APP-11..APP-13) (Target: 2026-Q4)
- [x] Wave 2 Recovery-Szenarien: Retry (REC-01..REC-02), Partial Failure (REC-03), Rollback (REC-04), Re-Ingest (REC-06), Concurrency (REC-07) (Target: 2026-Q3)

### Phase 4: Tests
- [x] Neue Pipeline-Tests unter CTest-Label `pipeline_integration` registriert (Target: 2026-Q3)
- [x] Wave 2 Tests unter CTest-Label `cross_module` + `wave2` registriert (Target: 2026-Q3)
- [x] Wave 3 Tests unter CTest-Label `wave3` + `w3a|w3b` registriert (Target: 2026-Q3)
- [x] Wave 5 Tests unter CTest-Label `wave5` + `release_critical` registriert (Target: 2026-Q3)
- [x] Wave 6 Tests unter CTest-Label `wave6` + `release_candidate|stress_soak|failure_injection` registriert (Target: 2026-Q3)
- [x] Wave 7 Tests unter CTest-Label `wave7` + `release_critical|endurance|stability_cert` registriert (Target: 2026-Q3)
- [x] Offline-Ausführbarkeit über Mocks ohne externe Services sichergestellt (Target: 2026-Q3)

### Phase 5: Performance/Hardening
- [ ] Laufzeit- und Stabilitätsprofile der Pipeline-Tests im Nightly-Run messen (Target: 2026-Q4)
- [ ] Security-/Reliability-Hardening für produktionsnahe Integrationspfade erweitern (Target: 2026-Q4)

### Phase 6: Dokumentation & Abnahme
- [x] `INTEGRATION_TEST_GUIDELINES.md` um Pipeline-Konventionen und Label-Flow ergänzt (Target: 2026-Q3)
- [x] Wave-spezifische Coverage-/Governance-Dokumente für Wave 3/5/6/7 hinterlegt (`WAVE3_TEST_COVERAGE.md`, `WAVE5_TEST_COVERAGE.md`, `WAVE5_TEST_GOVERNANCE.md`, `WAVE6_TEST_COVERAGE.md`, `WAVE7_TEST_COVERAGE.md`, `WAVE7_TRIAGE_RUNBOOK.md`) (Target: 2026-Q3)
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
