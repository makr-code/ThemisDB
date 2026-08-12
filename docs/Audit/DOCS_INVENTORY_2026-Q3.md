# ThemisDB Documentation Inventory — Q3 2026

**Stand:** 2026-08-12  
**Erstellt von:** AI Documentation Consolidation Run  
**Scope:** `docs/` root (117 Markdown-Dateien) + `docs/compendium/` Kapitel-Status  
**Governance-Referenz:** [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md)

---

## Legende

| Status | Bedeutung |
|---|---|
| **active** | Aktuelles Governance/Betriebs-Dokument; regelmäßig gepflegt |
| **stale** | Inhalt veraltet oder enthält TODO/FIXME/PLACEHOLDER ohne Abschluss |
| **archive-candidate** | Implementation-Summary oder historischer Report; bereits in ARCHIVED/ vorhanden oder archivierbar |
| **keep-epic** | EPIC-Architektur-Dokument; aktive Design-Artefakte |
| **duplicate** | Semantisch identischer Inhalt an anderer Stelle vorhanden |

---

## 1. docs/ Root — Vollständiges Inventar

### 1.1 Active Governance & Navigation

| Pfad | Level | SOT-Domain | Status | Kommentar |
|---|---|---|---|---|
| docs/00_DOCUMENTATION_INDEX.md | L4 | navigation | **active** | Kanonischer Root-Index; L4-Einstieg |
| docs/BRANCHING_DOCS_INDEX.md | L4 | navigation | **active** | Branch-spezifische Doku-Links; verweist auf 00_INDEX |
| docs/BRANCHING_STRATEGY_EN.md | L3 | governance | **active** | EN-Spiegeldatei zu BRANCHING_STRATEGY.md Root |
| docs/CATEGORY_INDEX.md | L4 | navigation | **active** → consolidate | Index-Duplikat; soll auf 00_DOCUMENTATION_INDEX zeigen |
| docs/CI_CD_WORKFLOWS.md | L3 | ci-cd | **active** | CI-Workflow-Übersicht |
| docs/CI_CD_REVIEW_BRANCHING_STRATEGY.md | L3 | ci-cd | **active** | CI-spezifische Branch-Governance |
| docs/CODING_STANDARDS.md | L3 | development | **active** | Coding-Standards; stale-Marker vorhanden → Review |
| docs/CONTRIBUTING_PLATFORM_GUIDELINES.md | L3 | governance | **active** | Beitragsleitfaden |
| docs/DEVELOPMENT_GUIDE.md | L3 | development | **active** | Entwickler-Einstieg |
| docs/DOCUMENTATION_HUB.md | L4 | navigation | **active** → consolidate | Rollenhub; soll auf 00_DOCUMENTATION_INDEX zeigen |
| docs/DOCUMENTATION_MERGE_PROTOCOL.md | L3 | governance | **active** | Merge-Protokoll für Docs |
| docs/DOCUMENTATION_REVIEW_GUIDELINES.md | L3 | governance | **active** | Review-Richtlinien |
| docs/EDITION_COMPARISON.md | L3 | product | **active** | Community/Enterprise/Military Vergleich |
| docs/FAQ.md | L4 | user-facing | **active** | FAQ; L4-Publikumsdokument |
| docs/MIGRATION_GUIDE.md | L3 | operations | **active** | Migrationsleitfaden |
| docs/OPERATIONS.md | L3 | operations | **active** | Betriebsdokumentation |
| docs/PIPELINE_E2E_GUIDE.md | L3 | ai-pipeline | **active** | E2E AI/ML-Pipeline |
| docs/PIPELINE_LESSONS_LEARNED.md | L3 | ai-pipeline | **active** | Lektionen gelernte; historisch aber aktiv |
| docs/PLATFORM_COMPATIBILITY_MATRIX.md | L3 | release | **active** | Plattform-Kompatibilitätsmatrix |
| docs/PR_DOCUMENTATION_CHECKLIST.md | L3 | governance | **active** | PR-Doku-Checkliste |
| docs/QUICK_REFERENCE.md | L4 | user-facing | **active** | Kurzreferenz |
| docs/README.md | L4 | user-facing | **active** | Haupt-README der docs/ |
| docs/RESOURCE_LIMITS_GUIDE.md | L3 | operations | **active** | Ressourcenlimits und Konfiguration |
| docs/TESTING_AND_BENCHMARKING_GUIDE.md | L3 | testing | **active** | Test- und Benchmark-Leitfaden |
| docs/TESTING_STRATEGY.md | L3 | testing | **active** | Teststrategie |
| docs/TOOLS_INDEX.md | L4 | navigation | **active** | Tools-Übersicht |
| docs/glossary.md | L4 | user-facing | **active** | Glossar |
| docs/replication-ha-guide.md | L3 | operations | **active** | Replikation/HA-Leitfaden |
| docs/backup_recovery_system.md | L3 | operations | **active** | Backup/Recovery |
| docs/DISTRIBUTED_TRANSACTIONS.md | L3 | development | **active** | Distributed Transactions Design |
| docs/INTEGRATION_GUIDE.md | L3 | development | **active** | Integration Guide |
| docs/EXAMPLES_INDEX.md | L4 | user-facing | **active** | Beispiel-Index |
| docs/EXAMPLES_QUICKSTART.md | L4 | user-facing | **active** | Quickstart-Beispiele |
| docs/DOXYGEN_COVERAGE_REPORT.md | L3 | quality | **active** | Doxygen-Abdeckungsreport |
| docs/CDC_OPERATIONS_RUNBOOK.md | L3 | operations | **active** | CDC Operations Runbook |
| docs/Home.md | L4 | navigation | **active** | Wiki-Startseite |
| docs/_Footer.md | L4 | navigation | **active** | Wiki-Footer |
| docs/_Sidebar.md | L4 | navigation | **stale** | Stale-Marker vorhanden; veraltet |
| docs/FEEDBACK_API.md | L3 | api | **active** | Feedback-API Referenz |
| docs/CRITICAL_AUDIT_FINDINGS_2026-08-03.md | L3 | audit | **active** | Aktueller Audit-Befund (Aug 2026) |
| docs/ROOT_GOVERNANCE_COMMUNITY_REVIEW_AUDIT_2026-05-13.md | L3 | audit | **active** | Community-Review-Audit (Mai 2026) |
| docs/SOURCE_AUDIT_REPORT.md | L3 | audit | **active** | Source-Audit |
| docs/SYSTEMATISCHER_REVIEWPLAN.md | L3 | governance | **stale** | Stale-Marker vorhanden → Review |
| docs/TASK_SCHEDULER_CRON_CDC.md | L3 | operations | **active** | Task-Scheduler/CDC |
| docs/CMAKE_BEST_PRACTICES_ANALYSIS.md | L3 | build | **active** | CMake Best Practices |
| docs/CMAKE_ONLY_BUILD_SYSTEM.md | L3 | build | **active** | CMake Build-Dokumentation |
| docs/CPACK_ANALYSIS.md | L3 | build | **active** | CPack Analyse (Aug 2026) |
| docs/COMPLETE_CICD_STRATEGY.md | L3 | ci-cd | **active** | CI/CD Vollstrategie |
| docs/CODE_MATURITY_BOOTSTRAP_GUIDE.md | L3 | quality | **stale** | Stale-Marker vorhanden |
| docs/DOCUMENTATION_UPDATE_APPROACH.md | L3 | governance | **stale** | Stale-Marker vorhanden; temporäres Work-Item |
| docs/DOCUMENTATION_UPDATE_PROGRESS.md | L3 | governance | **stale** | Stale-Marker vorhanden; temporäres Work-Item |

### 1.2 EPIC Architecture Documents (keep-epic)

Diese Dateien sind aktive Architektur-Design-Artefakte aus Epic-1/2/3 der Roadmap.

| Pfad | Level | Status | Kommentar |
|---|---|---|---|
| docs/EPIC1_2_3_DEPENDENCIES.md | L3 | **keep-epic** | Epic-Abhängigkeitsmatrix |
| docs/EPIC1_ANN_FRONTDOOR.md | L3 | **keep-epic** | ANN Frontdoor Design |
| docs/EPIC1_ARCHITECTURE.md | L3 | **keep-epic** | Epic 1 Architektur |
| docs/EPIC1_FEDERATED_SUMMARIES.md | L3 | **keep-epic** | Federated Summaries Design |
| docs/EPIC1_GRAPH_VALIDATION.md | L3 | **keep-epic** | Graph Validation Design |
| docs/EPIC1_LORA_ARTIFACTS.md | L3 | **keep-epic** | LoRA Artifacts Design |
| docs/EPIC1_MODEL_SWITCH.md | L3 | **keep-epic** | Model Switch Design |
| docs/EPIC1_TENSOR_MIDLAYER.md | L3 | **keep-epic** | Tensor Midlayer Design |
| docs/EPIC2_APPROXIMATION_GOVERNANCE.md | L3 | **keep-epic** | Approximation Governance |
| docs/EPIC2_ARCHITECTURE.md | L3 | **keep-epic** | Epic 2 Architektur |
| docs/EPIC2_ARTIFACT_LIFECYCLE.md | L3 | **keep-epic** | Artifact Lifecycle Design |
| docs/EPIC2_BENCHMARK_FRAMEWORK.md | L3 | **keep-epic** | Benchmark Framework Design |
| docs/EPIC2_EVALUATION_METRICS.md | L3 | **keep-epic** | Evaluation Metrics Design |
| docs/EPIC2_HARDWARE_PROFILES.md | L3 | **keep-epic** | Hardware Profiles Design |
| docs/EPIC2_QUERY_PLANNER.md | L3 | **keep-epic** | Query Planner Design |
| docs/EPIC3_ARCHITECTURE.md | L3 | **keep-epic** | Epic 3 Architektur |
| docs/EPIC3_ARTIFACT_CLASSES.md | L3 | **keep-epic** | Artifact Classes Design |
| docs/EPIC3_DISTRIBUTED_RETRIEVAL.md | L3 | **keep-epic** | Distributed Retrieval |
| docs/EPIC3_INTEGRITY_MODEL.md | L3 | **keep-epic** | Integrity Model Design |
| docs/EPIC3_INTEGRITY_MODEL_IMPLEMENTATION.md | L3 | **keep-epic** | Integrity Model Impl |
| docs/EPIC3_MANIFEST_SCHEMA.md | L3 | **keep-epic** | Manifest Schema |
| docs/EPIC3_MERKLE_RECEIPT_INTEGRATION.md | L3 | **keep-epic** | Merkle Receipt Design |
| docs/EPIC3_RECOVERY_STRATEGY.md | L3 | **keep-epic** | Recovery Strategy |
| docs/EPIC3_SHARD_PLACEMENT.md | L3 | **keep-epic** | Shard Placement Design |

### 1.3 Archive Candidates (Implementation Summaries / Historical Reports)

Folgende Dateien sind bereits in `docs/ARCHIVED/implementation-summaries/` vorhanden
oder sind historische Dokumente ohne aktive Governance-Funktion.

| Pfad | Status | Archiv-Ziel | Kommentar |
|---|---|---|---|
| docs/AUTH_P1_IMPLEMENTATION_SUMMARY.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/COMPENDIUM_STATUS_V1.4.0.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/CONFIDENTIAL_DOCUMENTS.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/CROSS_COMPILER_DEBUGGING_IMPLEMENTATION.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/ENHANCEMENT_SUMMARY.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/GENERAL_TRAVERSAL_FEATURE.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/IMPLEMENTATION_COMPLETE.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/LEGAL_LORA_FINAL_SUMMARY.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/PHASE2.3.5_COMPLETE.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/PHASE2_IMPLEMENTATION_GUIDE.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/bpmn-verification-report.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/DOCUMENTATION_IMPROVEMENT_SUMMARY.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Historischer Summary |
| docs/DOCUMENTATION_REORGANIZATION_SUMMARY.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Historischer Summary |
| docs/FINAL_REPORT.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Generischer Final-Report |
| docs/FINAL_SUMMARY.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Generischer Summary |
| docs/FUTURE_WORKS_IMPLEMENTATION.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Historisch |
| docs/IMPLEMENTATION_ROADMAP.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Ersetzt durch Root ROADMAP.md |
| docs/IMPLEMENTATION_COMPLETE.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Bereits dort vorhanden |
| docs/PROMETHEUS_INTEGRATION_COMPLETE.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Integration Complete |
| docs/QUICK_START_PHASE_1.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Stale Marker |
| docs/DOCS_ORGANIZATION_PLAN.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Historischer Plan |
| docs/DOCUMENTATION_ARCHIVAL_PROCESS.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Prozess-Beschreibung |
| docs/DOCUMENTATION_FEEDBACK_MECHANISMS.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Feedback-Mechanismen |
| docs/DOCUMENTATION_CONSOLIDATION_PLAN.md | **stale** | — | Stale-Marker vorhanden |
| docs/ERROR_CLASS_UNITY_NAMESPACE_IMBALANCE.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Technisches Detail |
| docs/FEDERATED_TENSOR_SUMMARIES.md | L3 | **keep-epic** | Aktives Design-Dok |
| docs/FEDERATED_TENSOR_SUMMARIES_EXAMPLES.md | L3 | **keep-epic** | Aktives Design-Dok |
| docs/FUTURE_GPU_SUPPORT.md | **stale** | — | Stale-Marker; GPU Roadmap |
| docs/GEOSPATIAL_FUTURE_ENHANCEMENTS.md | **stale** | — | Stale-Marker |
| docs/GENERIC_INSTALLER_FRAMEWORK_CONCEPT.md | **archive-candidate** | docs/ARCHIVED/implementation-summaries/ | Konzept-Dokument |
| docs/GITHUB_SETTINGS_CONFIGURATION.md | L3 | **active** | GitHub-Konfiguration |
| docs/ITRANSACTION_COORDINATOR.md | L3 | **active** | ITransaction-Schnittstelle |
| docs/LLM_ENGINE_AUSARBEITUNG.md | L3 | **active** | LLM Engine Ausarbeitung |
| docs/TENSOR_MIDLAYER_DESIGN.md | L3 | **keep-epic** | Tensor Midlayer Design |
| docs/TENSOR_REBUILD_GUIDELINES.md | L3 | **stale** | Stale-Marker vorhanden |
| docs/TENSOR_RECOVERY_PROCEDURES.md | L3 | **active** | Recovery Procedures |
| docs/TENSOR_RECOVERY_TEST_STRATEGY.md | L3 | **active** | Test-Strategie |
| docs/TENSOR_STORAGE_STRATEGY.md | L3 | **active** | Storage-Strategie |
| docs/gpu_roadmap.md | L3 | **stale** | Stale-Marker vorhanden |
| docs/gpu_runbooks.md | L3 | **active** | GPU Runbooks |
| docs/llm_roadmap.md | L3 | **stale** | Stale-Marker vorhanden |
| docs/cache_roadmap.md | L3 | **stale** | Stale-Marker vorhanden |
| docs/distributed_training_integration.md | L3 | **active** | Integration Guide |

### 1.4 Stale Dateien mit TODO/FIXME/STALE-Markern (22 bekannte)

Diese Dateien enthalten Stale-Marker und benötigen Review oder Archivierung:

| Pfad | Marker-Typ | Aktion |
|---|---|---|
| docs/_Sidebar.md | TBD/PLACEHOLDER | Stale-Hinweis ergänzen |
| docs/SYSTEMATISCHER_REVIEWPLAN.md | TODO | Stale-Hinweis ergänzen |
| docs/CODE_MATURITY_BOOTSTRAP_GUIDE.md | TODO | Review/Update |
| docs/DOCUMENTATION_UPDATE_APPROACH.md | TODO/TBD | Stale-Hinweis ergänzen |
| docs/DOCUMENTATION_UPDATE_PROGRESS.md | TODO/TBD | Stale-Hinweis ergänzen |
| docs/CODING_STANDARDS.md | TODO | Review/Update |
| docs/FUTURE_GPU_SUPPORT.md | TODO/PLACEHOLDER | Stale-Hinweis ergänzen |
| docs/GEOSPATIAL_FUTURE_ENHANCEMENTS.md | TODO | Stale-Hinweis ergänzen |
| docs/TENSOR_REBUILD_GUIDELINES.md | TODO/FIXME | Review/Update |
| docs/gpu_roadmap.md | TODO | Stale-Hinweis ergänzen |
| docs/llm_roadmap.md | TODO | Stale-Hinweis ergänzen |
| docs/cache_roadmap.md | TODO | Stale-Hinweis ergänzen |
| docs/QUICK_START_PHASE_1.md | TODO | Archivieren |
| docs/DOCUMENTATION_CONSOLIDATION_PLAN.md | TODO | Stale-Hinweis ergänzen |

### 1.5 Duplikate und Index-Fragmentierung

| Problem | Dateien | Aktion |
|---|---|---|
| Index-Duplikate | 00_DOCUMENTATION_INDEX.md, CATEGORY_INDEX.md, DOCUMENTATION_HUB.md, BRANCHING_DOCS_INDEX.md | 00_INDEX = kanonisch; andere verweisen darauf |
| README-Overlap | docs/README.md + docs/README-DOCUMENTATION.md | README-DOCUMENTATION.md als spezifischeren Kontext prüfen |
| Implementation-Summaries doppelt | docs/ root + docs/ARCHIVED/implementation-summaries/ | Root-Kopien entfernen (mit Archiv-Marker) |

---

## 2. docs/compendium/ Kapitel-Status (Phase 3 Mapping)

| Kapitel | Datei | Status | Quellen (docs/de/) | Priorität |
|---|---|---|---|---|
| 0: Genesis | chapter_00_genesis.md | ✅ VOLLSTÄNDIG | Home.md, README.md | — |
| 1: Einführung | chapter_01_introduction.md | ✅ VOLLSTÄNDIG | INDEX.md, MARKETING_MATERIALS | — |
| 2: Architektur | chapter_02_architecture.md | 🔄 ERGÄNZUNG | architecture/ | Medium |
| 3: Multi-Model | chapter_03_multimodel.md | ✅ VOLLSTÄNDIG | — | — |
| 4: Installation | chapter_04_installation.md | ✅ VOLLSTÄNDIG | — | — |
| 5: Relational | chapter_05_relational.md | ✅ VOLLSTÄNDIG | — | — |
| 6: Graph | chapter_06_graph.md | ✅ VOLLSTÄNDIG | — | — |
| 7: Document | chapter_07_document.md | ✅ VOLLSTÄNDIG | — | — |
| 8: Storage | chapter_08_storage_layer.md | 🔄 ERGÄNZUNG | storage/ | Medium |
| 8: Vector | chapter_08_vector.md | 🔄 ERGÄNZUNG | search/ | Medium |
| 9: Timeseries | chapter_09_timeseries.md | 🔄 ERGÄNZUNG | timeseries/ | Medium |
| 10: Enterprise | chapter_10_enterprise.md | 🔄 ERGÄNZUNG | — | Medium |
| 11: Realtime | chapter_11_realtime.md | 🔄 ERGÄNZUNG | — | Medium |
| 12: ComputerVision | chapter_12_computervision.md | 🔄 ERGÄNZUNG | onnx_clip/ | Medium |
| 13: Fulltext | chapter_13_fulltext.md | 🔄 ERGÄNZUNG | search/ | Medium |
| 14: Geospatial | chapter_14_geospatial.md | 🔄 ERGÄNZUNG | geo/ | Medium |
| 15: Analytics | chapter_15_analytics.md | 🔄 ERGÄNZUNG | analytics/ | Medium |
| 16: Sharding | chapter_16_sharding.md | ✅ VOLLSTÄNDIG (Phase 3 done) | sharding/ | — |
| 16: ML | chapter_16_ml.md | 🔄 ERGÄNZUNG | — | Medium |
| **17: LLM Integration** | **chapter_17_llm_integration.md** | **🔄 HIGH** | **llm/, lora/, rag/** | **P1** |
| 17: Scaling | chapter_17_scaling.md | 🔄 ERGÄNZUNG | — | Medium |
| 18: HA | chapter_18_ha.md | 🔄 ERGÄNZUNG | replication/ | Medium |
| 18: ML | chapter_18_ml.md | 🔄 ERGÄNZUNG | — | Medium |
| 19: Monitoring | chapter_19_monitoring.md | 🔄 ERGÄNZUNG | observability/ | Medium |
| 19: Monitoring+Observability | chapter_19_monitoring_observability.md | 🔄 ERGÄNZUNG | observability/ | Medium |
| 20: Backup | chapter_20_backup.md | 🔄 ERGÄNZUNG | — | Medium |
| 20: Performance | chapter_20_performance.md | 🔄 ERGÄNZUNG | performance/ | Medium |
| 21: Auth | chapter_21_auth.md | 🔄 ERGÄNZUNG | auth/ | Medium |
| 21: Performance | chapter_21_performance.md | 🔄 ERGÄNZUNG | performance/ | Medium |
| 22: Clients | chapter_22_clients.md | 🔄 ERGÄNZUNG | — | Medium |
| 22: Encryption | chapter_22_encryption.md | 🔄 ERGÄNZUNG | security/ | Medium |
| 23: Testing | chapter_23_testing_qa.md | 🔄 ERGÄNZUNG | — | Medium |
| 24: AI Ethics | chapter_24_ai_ethics.md | 🔄 ERGÄNZUNG | ethics_ai/ | Medium |
| 25: DevOps | chapter_25_devops_infrastructure.md | 🔄 ERGÄNZUNG | — | Medium |
| 26: Migration | chapter_26_migration_legacy.md | 🔄 ERGÄNZUNG | migration/ | Medium |
| 27: Troubleshooting | chapter_27_troubleshooting.md | 🔄 ERGÄNZUNG | — | Medium |
| 28: AQL Reference | chapter_28_aql_reference.md | ✅ VOLLSTÄNDIG | aql/ | — |
| **29: Analytics+Process** | **chapter_29_analytics_process_mining.md** | **🔄 HIGH** | **analytics/, process/** | **P1** |
| 30: Deployment | chapter_30_deployment_operations.md | 🔄 ERGÄNZUNG | deployment/ | Medium |
| **31: API Protocols** | **chapter_31_api_protocols.md** | **🔄 HIGH** | **apis/, rpc_grpc/** | **P1** |
| 32: API Design | chapter_32_api_design_rest_principles.md | 🔄 ERGÄNZUNG | — | Medium |
| 32: AQL OOP | chapter_32_aql_oop_implementation.md | 🔄 ERGÄNZUNG | aql/ | Medium |
| 33: Best Practices | chapter_33_best_practices.md | 🔄 ERGÄNZUNG | — | Medium |
| 34: Query Optimization | chapter_34_query_optimization.md | 🔄 ERGÄNZUNG | query/ | Medium |
| 35: Data Modeling | chapter_35_data_modeling_patterns.md | 🔄 ERGÄNZUNG | — | Medium |
| 36: Security Hardening | chapter_36_security_hardening.md | 🔄 ERGÄNZUNG | security/ | Medium |
| 37: Ecosystem | chapter_37_ecosystem_integration.md | 🔄 ERGÄNZUNG | — | Medium |
| 38: Observability SRE | chapter_38_observability_sre.md | 🔄 ERGÄNZUNG | observability/ | Medium |
| 39: Performance Tuning | chapter_39_performance_tuning_cookbook.md | 🔄 ERGÄNZUNG | performance/ | Medium |
| **40: Data Governance** | **chapter_40_data_governance_compliance.md** | **🔄 HIGH** | **compliance/, governance/** | **P1** |
| 41: Hands-On Labs | chapter_41_hands_on_labs.md | 🔄 ERGÄNZUNG | — | Medium |
| 42: Docs Assistant | chapter_42_docs_assistant_usage.md | 🔄 ERGÄNZUNG | — | Medium |

---

## 3. docs/de/ Strukturprobleme

| Problem | Pfad | Aktion |
|---|---|---|
| Duplicate indexes | docs/de/INDEX.md + docs/de/00_DOCUMENTATION_INDEX.md | 00_INDEX als kanonisch; INDEX.md → Redirect |
| Historische Phase-Reports | docs/de/phase_reports/ | → docs/de/archive/ verschieben |
| Stub-Verzeichnisse | Subdirs mit nur README | Identifizieren und ggf. mit content befüllen |

---

## 4. Priorisierte Aktionsliste

### Sofort (P0) — Archivierung
1. Root-Dateien die bereits in ARCHIVED/ vorhanden sind: Stale-Marker hinzufügen
2. Index-Duplikate konsolidieren (CATEGORY_INDEX, DOCUMENTATION_HUB)

### Phase 3 (P1) — Compendium-Kapitel-Enrichment
1. chapter_17_llm_integration.md — aus docs/de/llm/, lora/, rag/
2. chapter_31_api_protocols.md — aus docs/de/apis/, rpc_grpc/
3. chapter_40_data_governance_compliance.md — aus docs/de/compliance/, governance/
4. chapter_29_analytics_process_mining.md — aus docs/de/analytics/, process/

### Governance (P2)
1. DOCUMENTATION_GOVERNANCE.md: Compendium-Sync-Prozess ergänzen
2. 00_DOCUMENTATION_INDEX.md: nach Cleanup aktualisieren

---

*Dieses Inventar wird bei jeder Documentation-Consolidation-Run aktualisiert.*
*Nächstes Review: 2026-Q4*
