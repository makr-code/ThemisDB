<!--
Last Updated: 2026-08-12
Source Level: Level 4 (docs/ audit output)
SOT Domain: architecture-governance
Canonical Sources: DOCUMENTATION_GOVERNANCE.md, docs/00_DOCUMENTATION_INDEX.md
Milestone: DOC-WEEKLY-2026-33
-->

# ThemisDB Docs Inventory — Q3 2026

**Stand:** 2026-08-12  
**Erstellt durch:** Documentation Orchestration (DOC-WEEKLY-2026-33)  
**Scope:** Alle Markdown-Dateien in `docs/` Root (non-ARCHIVED) — 117 Dateien  
**Governance-Basis:** [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md)  
**Kanonischer Root-Index:** [docs/00_DOCUMENTATION_INDEX.md](../00_DOCUMENTATION_INDEX.md)

---

## Legende

| Status | Bedeutung |
|--------|-----------|
| **active** | Aktuelles Governance-/Operational-Dokument, gepflegt |
| **stale** | Enthält TODO/FIXME/STALE/TBD/PLACEHOLDER; Archivierungskandidat |
| **archive-candidate** | Implementation-Summary, Complete-Marker oder Plan ohne aktive Governance-Funktion |
| **duplicate** | Semantisch gleichwertiger Inhalt bereits an anderer Stelle vorhanden |
| **redirect** | Aktiv aber verweist/soll auf kanonischen Index zeigen |

---

## docs/ Root — Komplettinventar

| Pfad | Level | SOT-Domain | Status | Letztes Update (geschätzt) | Kommentar |
|------|-------|-----------|--------|--------------------------|-----------|
| `00_DOCUMENTATION_INDEX.md` | 4 | architecture-governance | **active** | 2026-04-18 | Kanonischer Root-Index — Pflege-Priorität hoch |
| `AUTH_P1_IMPLEMENTATION_SUMMARY.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | ✅ BEREITS in ARCHIVED/implementation-summaries/ |
| `BRANCHING_DOCS_INDEX.md` | 4 | architecture-governance | **redirect** | 2026-04-18 | Redirect-Notice zu 00_DOCUMENTATION_INDEX.md hinzugefügt |
| `BRANCHING_STRATEGY_EN.md` | 4 | architecture-governance | **active** | 2026-04 | EN-Fassung der Branching-Strategie |
| `CATEGORY_INDEX.md` | 4 | architecture-governance | **redirect** | 2026-04-18 | Redirect-Notice zu 00_DOCUMENTATION_INDEX.md hinzugefügt |
| `CDC_OPERATIONS_RUNBOOK.md` | 4 | build-test | **active** | 2026-04 | Operativer Runbook CDC |
| `CI_CD_REVIEW_BRANCHING_STRATEGY.md` | 4 | build-test | **active** | 2026-04 | CI/CD Review Policy |
| `CI_CD_WORKFLOWS.md` | 4 | build-test | **active** | 2026-04 | CI/CD Workflow-Beschreibung |
| `CMAKE_BEST_PRACTICES_ANALYSIS.md` | 4 | build-test | **active** | 2026-03 | CMake-Analyse |
| `CMAKE_ONLY_BUILD_SYSTEM.md` | 4 | build-test | **active** | 2026-04 | CMake-Only Build-System |
| `CODE_MATURITY_BOOTSTRAP_GUIDE.md` | 4 | module-behavior | **stale** | 2026-03 | TODO/FIXME-Marker vorhanden |
| `CODING_STANDARDS.md` | 4 | architecture-governance | **stale** | 2026-03 | TODO/TBD-Marker vorhanden |
| `COMPENDIUM_STATUS_V1.4.0.md` | 4 | release-versioning | **archive-candidate** | 2026-01-10 | ✅ BEREITS in ARCHIVED/implementation-summaries/ |
| `COMPLETE_CICD_STRATEGY.md` | 4 | build-test | **active** | 2026-04 | Vollständige CI/CD Strategie |
| `CONFIDENTIAL_DOCUMENTS.md` | 4 | security | **archive-candidate** | 2026-03 | ✅ BEREITS in ARCHIVED/implementation-summaries/ |
| `CONTRIBUTING_PLATFORM_GUIDELINES.md` | 4 | architecture-governance | **active** | 2026-04 | Platform-spezifische Contributing-Hinweise |
| `CPACK_ANALYSIS.md` | 4 | build-test | **stale** | 2026-02 | TBD/TODO-Marker — CPack-Analyse veraltet |
| `CRITICAL_AUDIT_FINDINGS_2026-08-03.md` | 4 | security | **stale** | 2026-08-03 | TODO/FIXME vorhanden; aktiv aber Findings noch offen |
| `CROSS_COMPILER_DEBUGGING_IMPLEMENTATION.md` | 4 | build-test | **archive-candidate** | 2026-03 | ✅ BEREITS in ARCHIVED/implementation-summaries/ |
| `DEVELOPMENT_GUIDE.md` | 4 | module-behavior | **active** | 2026-04 | Entwickler-Onboarding |
| `DISTRIBUTED_TRANSACTIONS.md` | 4 | module-behavior | **active** | 2026-04 | Distributed Transactions Design |
| `DOCS_ORGANIZATION_PLAN.md` | 4 | architecture-governance | **archive-candidate** | 2026-03 | Planungsdokument ohne aktive Governance-Funktion — MOVE |
| `DOCUMENTATION_ARCHIVAL_PROCESS.md` | 4 | architecture-governance | **active** | 2026-04 | Archivierprozess-Beschreibung |
| `DOCUMENTATION_FEEDBACK_MECHANISMS.md` | 4 | architecture-governance | **active** | 2026-04 | Feedback-Mechanismen |
| `DOCUMENTATION_HUB.md` | 4 | architecture-governance | **redirect** | 2026-04-18 | Redirect-Notice zu 00_DOCUMENTATION_INDEX.md hinzugefügt |
| `DOCUMENTATION_IMPROVEMENT_SUMMARY.md` | 4 | architecture-governance | **archive-candidate** | 2026-03 | Vergangenheits-Summary ohne aktuelle Governance-Funktion — MOVE |
| `DOCUMENTATION_MERGE_PROTOCOL.md` | 4 | architecture-governance | **active** | 2026-04 | Merge-Protokoll für Doku |
| `DOCUMENTATION_REORGANIZATION_SUMMARY.md` | 4 | architecture-governance | **archive-candidate** | 2026-03 | Reorganisierungs-Summary — MOVE |
| `DOCUMENTATION_REVIEW_GUIDELINES.md` | 4 | architecture-governance | **stale** | 2026-03 | TODO-Marker vorhanden |
| `DOCUMENTATION_UPDATE_APPROACH.md` | 4 | architecture-governance | **stale** | 2026-03 | STALE/TODO-Marker vorhanden |
| `DOCUMENTATION_UPDATE_PROGRESS.md` | 4 | architecture-governance | **stale** | 2026-03 | TODO/PLACEHOLDER-Marker |
| `DOXYGEN_COVERAGE_REPORT.md` | 4 | build-test | **active** | 2026-05 | Doxygen Coverage Report |
| `EDITION_COMPARISON.md` | 4 | release-versioning | **active** | 2026-04 | Edition-Vergleich (Community/Enterprise) |
| `ENHANCEMENT_SUMMARY.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | ✅ BEREITS in ARCHIVED/implementation-summaries/ |
| `EPIC1_2_3_DEPENDENCIES.md` | 4 | architecture-governance | **active** | 2026-04 | Epic-Abhängigkeiten |
| `EPIC1_ANN_FRONTDOOR.md` | 4 | module-behavior | **active** | 2026-04 | ANN Frontdoor Design |
| `EPIC1_ARCHITECTURE.md` | 4 | architecture-governance | **active** | 2026-04 | Epic 1 Architektur |
| `EPIC1_FEDERATED_SUMMARIES.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | Implementation-Summary — MOVE |
| `EPIC1_GRAPH_VALIDATION.md` | 4 | module-behavior | **active** | 2026-04 | Graph Validation Design |
| `EPIC1_LORA_ARTIFACTS.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | Artifact-Summary — MOVE |
| `EPIC1_MODEL_SWITCH.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | Model-Switch Implementation Summary — MOVE |
| `EPIC1_TENSOR_MIDLAYER.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | Tensor Midlayer Summary — MOVE |
| `EPIC2_APPROXIMATION_GOVERNANCE.md` | 4 | architecture-governance | **active** | 2026-04 | Epic 2 Approximation Governance |
| `EPIC2_ARCHITECTURE.md` | 4 | architecture-governance | **active** | 2026-04 | Epic 2 Architektur |
| `EPIC2_ARTIFACT_LIFECYCLE.md` | 4 | module-behavior | **stale** | 2026-03 | TODO-Marker vorhanden |
| `EPIC2_BENCHMARK_FRAMEWORK.md` | 4 | build-test | **stale** | 2026-03 | TODO/TBD-Marker vorhanden |
| `EPIC2_EVALUATION_METRICS.md` | 4 | module-behavior | **active** | 2026-04 | Evaluation Metrics |
| `EPIC2_HARDWARE_PROFILES.md` | 4 | module-behavior | **active** | 2026-04 | Hardware-Profile |
| `EPIC2_QUERY_PLANNER.md` | 4 | module-behavior | **active** | 2026-04 | Query Planner Design |
| `EPIC3_ARCHITECTURE.md` | 4 | architecture-governance | **active** | 2026-04 | Epic 3 Architektur |
| `EPIC3_ARTIFACT_CLASSES.md` | 4 | module-behavior | **active** | 2026-04 | Artifact Classes |
| `EPIC3_DISTRIBUTED_RETRIEVAL.md` | 4 | module-behavior | **active** | 2026-04 | Distributed Retrieval |
| `EPIC3_INTEGRITY_MODEL.md` | 4 | security | **stale** | 2026-03 | FIXME/TODO vorhanden |
| `EPIC3_INTEGRITY_MODEL_IMPLEMENTATION.md` | 4 | security | **stale** | 2026-03 | FIXME/TODO vorhanden |
| `EPIC3_MANIFEST_SCHEMA.md` | 4 | module-behavior | **active** | 2026-04 | Manifest Schema |
| `EPIC3_MERKLE_RECEIPT_INTEGRATION.md` | 4 | security | **stale** | 2026-03 | TODO-Marker vorhanden |
| `EPIC3_RECOVERY_STRATEGY.md` | 4 | module-behavior | **stale** | 2026-03 | TODO-Marker vorhanden |
| `EPIC3_SHARD_PLACEMENT.md` | 4 | module-behavior | **active** | 2026-04 | Shard Placement Design |
| `ERROR_CLASS_UNITY_NAMESPACE_IMBALANCE.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | Problem-Report abgeschlossen — MOVE |
| `EXAMPLES_INDEX.md` | 4 | module-behavior | **active** | 2026-04 | Beispiel-Index |
| `EXAMPLES_QUICKSTART.md` | 4 | module-behavior | **active** | 2026-04 | Quickstart-Beispiele |
| `FAQ.md` | 4 | module-behavior | **active** | 2026-04 | FAQ |
| `FEDERATED_TENSOR_SUMMARIES.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | Summary-Dok ohne aktive Funktion — MOVE |
| `FEDERATED_TENSOR_SUMMARIES_EXAMPLES.md` | 4 | module-behavior | **active** | 2026-04 | Beispiele — Inhalt noch referenziert |
| `FEEDBACK_API.md` | 4 | api-contract | **active** | 2026-04 | Feedback-API |
| `FINAL_REPORT.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | Abgeschlossener Phasenbericht — MOVE |
| `FINAL_SUMMARY.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | Abgeschlossene Zusammenfassung — MOVE |
| `FUTURE_WORKS_IMPLEMENTATION.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | Vergangenheits-Implementation-Summary — MOVE |
| `GENERAL_TRAVERSAL_FEATURE.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | ✅ BEREITS in ARCHIVED/implementation-summaries/ |
| `GENERIC_INSTALLER_FRAMEWORK_CONCEPT.md` | 4 | build-test | **archive-candidate** | 2026-03 | Konzeptdok ohne aktiven Governance-Nutzen — MOVE |
| `GEOSPATIAL_FUTURE_ENHANCEMENTS.md` | 4 | module-behavior | **active** | 2026-04 | Geo-Feature Roadmap |
| `GITHUB_SETTINGS_CONFIGURATION.md` | 4 | build-test | **active** | 2026-04 | GitHub-Settings |
| `Home.md` | 4 | architecture-governance | **active** | 2026-04 | Startseite (Wiki-Kompatibilität) |
| `IMPLEMENTATION_COMPLETE.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | ✅ BEREITS in ARCHIVED/implementation-summaries/ |
| `IMPLEMENTATION_ROADMAP.md` | 4 | module-behavior | **active** | 2026-04 | Implementierungs-Roadmap (aktive Planung) |
| `INTEGRATION_GUIDE.md` | 4 | api-contract | **active** | 2026-04 | Integrations-Guide |
| `ITRANSACTION_COORDINATOR.md` | 4 | module-behavior | **active** | 2026-04 | ITransactionCoordinator Interface |
| `LEGAL_LORA_FINAL_SUMMARY.md` | 4 | security | **archive-candidate** | 2026-03 | ✅ BEREITS in ARCHIVED/implementation-summaries/ (stale-Marker vorhanden) |
| `LLM_ENGINE_AUSARBEITUNG.md` | 4 | module-behavior | **active** | 2026-04 | LLM Engine Ausarbeitung |
| `MIGRATION_GUIDE.md` | 4 | module-behavior | **active** | 2026-04 | Migrations-Guide |
| `OPERATIONS.md` | 4 | build-test | **active** | 2026-04 | Operations-Beschreibung |
| `PHASE2.3.5_COMPLETE.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | ✅ BEREITS in ARCHIVED/implementation-summaries/ |
| `PHASE2_IMPLEMENTATION_GUIDE.md` | 4 | module-behavior | **archive-candidate** | 2026-03 | ✅ BEREITS in ARCHIVED/implementation-summaries/ |
| `PIPELINE_E2E_GUIDE.md` | 4 | build-test | **active** | 2026-04 | Pipeline E2E Guide |
| `PIPELINE_LESSONS_LEARNED.md` | 4 | build-test | **active** | 2026-04 | Pipeline Lessons Learned |
| `PLATFORM_COMPATIBILITY_MATRIX.md` | 4 | build-test | **active** | 2026-04 | Platform Compatibility Matrix |
| `PROMETHEUS_INTEGRATION_COMPLETE.md` | 4 | module-behavior | **archive-candidate** | 2026-04 | Complete-Marker — MOVE |
| `PR_DOCUMENTATION_CHECKLIST.md` | 4 | architecture-governance | **active** | 2026-04 | PR-Doku-Checkliste |
| `QUALITY_CONTROL_SYSTEM.md` | 4 | architecture-governance | **active** | 2026-04 | QC-System |
| `QUICK_REFERENCE.md` | 4 | module-behavior | **active** | 2026-04 | Quick Reference |
| `QUICK_START_PHASE_1.md` | 4 | module-behavior | **stale** | 2026-02 | TBD/PLACEHOLDER-Marker vorhanden |
| `README-DOCUMENTATION.md` | 4 | architecture-governance | **active** | 2026-04 | Docs-README |
| `README.md` | 4 | architecture-governance | **active** | 2026-04 | docs/ Haupt-README |
| `RESOURCE_LIMITS_GUIDE.md` | 4 | module-behavior | **active** | 2026-04 | Resource Limits Guide |
| `ROOT_GOVERNANCE_COMMUNITY_REVIEW_AUDIT_2026-05-13.md` | 4 | architecture-governance | **active** | 2026-05-13 | Community Review Audit |
| `SOURCE_AUDIT_REPORT.md` | 4 | architecture-governance | **active** | 2026-05 | Source Audit Report |
| `SYSTEMATISCHER_REVIEWPLAN.md` | 4 | architecture-governance | **stale** | 2026-03 | TODO/TBD-Marker vorhanden |
| `TASK_SCHEDULER_CRON_CDC.md` | 4 | build-test | **active** | 2026-04 | Task Scheduler & CDC |
| `TENSOR_MIDLAYER_DESIGN.md` | 4 | module-behavior | **active** | 2026-04 | Tensor Midlayer Design |
| `TENSOR_REBUILD_GUIDELINES.md` | 4 | module-behavior | **stale** | 2026-03 | TODO/FIXME vorhanden |
| `TENSOR_RECOVERY_PROCEDURES.md` | 4 | module-behavior | **active** | 2026-04 | Tensor Recovery Procedures |
| `TENSOR_RECOVERY_TEST_STRATEGY.md` | 4 | module-behavior | **active** | 2026-04 | Tensor Recovery Test Strategy |
| `TENSOR_STORAGE_STRATEGY.md` | 4 | module-behavior | **active** | 2026-04 | Tensor Storage Strategy |
| `TESTING_AND_BENCHMARKING_GUIDE.md` | 4 | build-test | **active** | 2026-04 | Testing & Benchmarking Guide |
| `TESTING_STRATEGY.md` | 4 | build-test | **active** | 2026-04 | Testing-Strategie |
| `TOOLS_INDEX.md` | 4 | build-test | **active** | 2026-04 | Tools-Index |
| `_Footer.md` | 4 | architecture-governance | **active** | 2026-04 | Wiki-Footer |
| `_Sidebar.md` | 4 | architecture-governance | **stale** | 2026-03 | TODO-Marker vorhanden |
| `backup_recovery_system.md` | 4 | build-test | **active** | 2026-04 | Backup/Recovery |
| `bpmn-verification-report.md` | 4 | build-test | **archive-candidate** | 2026-03 | ✅ BEREITS in ARCHIVED/implementation-summaries/ (stale-Marker vorhanden) |
| `cache_roadmap.md` | 4 | module-behavior | **active** | 2026-04 | Cache Roadmap |
| `distributed_training_integration.md` | 4 | module-behavior | **active** | 2026-04 | Distributed Training Integration |
| `glossary.md` | 4 | architecture-governance | **active** | 2026-04 | Glossar |
| `gpu_roadmap.md` | 4 | module-behavior | **stale** | 2026-03 | TODO/TBD-Marker vorhanden |
| `gpu_runbooks.md` | 4 | build-test | **active** | 2026-04 | GPU Runbooks |
| `llm_roadmap.md` | 4 | module-behavior | **stale** | 2026-03 | TODO/TBD-Marker vorhanden |
| `replication-ha-guide.md` | 4 | build-test | **stale** | 2026-03 | STALE-Marker vorhanden |

---

## Zusammenfassung

| Kategorie | Anzahl |
|-----------|--------|
| **active** | 72 |
| **stale** | 22 |
| **archive-candidate (bereits in ARCHIVED)** | 11 |
| **archive-candidate (MOVE erforderlich)** | 14 |
| **redirect** | 3 |
| **Gesamt** | 122 (inkl. Untergruppen) |

> Hinweis: Einige Dateien tragen sowohl `stale` als auch `archive-candidate`, da sie bereits archiviert wurden aber noch in docs/ präsent sind und Stale-Marker enthalten.

---

## docs/compendium/PHASE3_MAPPING_TABLE.md — Compendium Chapter→docs/de Source Mapping

Quelle: `docs/compendium/PHASE3_MAPPING_TABLE.md`

Für Phase 3 (Compendium-Sync) identifizierte Prioritätskapitel und ihre docs/de-Quellen:

| Kapitel | Kapiteldatei | docs/de-Quellverzeichnisse |
|---------|-------------|--------------------------|
| 17 — LLM-Integration | `chapter_17_llm_integration.md` | `docs/de/llm/`, `docs/de/lora/`, `docs/de/rag/` |
| 29 — Analytics & Process Mining | `chapter_29_analytics_process_mining.md` | `docs/de/analytics/`, `docs/de/process/` |
| 31 — API-Protokolle | `chapter_31_api_protocols.md` | `docs/de/apis/`, `docs/de/rpc_grpc/` |
| 40 — Data Governance & Compliance | `chapter_40_data_governance_compliance.md` | `docs/de/compliance/`, `docs/de/governance/` |

---

## docs/de/ — Stub/Empty-Verzeichnisse

Folgende Verzeichnisse enthalten nur eine README oder sind leer (Kandidaten für Konsolidierung):

| Verzeichnis | Dateianzahl | Kommentar |
|-------------|------------|-----------|
| `docs/de/integration/` | 1 (README) | Stub |
| `docs/de/operations/` | 1 (README) | Stub |
| `docs/de/src/` | 0 | Leer — Kandidat für Löschung/Konsolidierung |
| `docs/de/src/tensor/` | 1 (README) | Stub |
| `docs/de/scraper/` | 1 (README) | Stub |
| `docs/de/migration/` | 1 (README) | Stub |
| `docs/de/compiled/` | 1 (README) | Stub |
| `docs/de/integrations/` | 1 (README) | Stub; semantisches Duplikat zu `integration/` |

---

## docs/de/ — Semantische Duplikate (Dateinamen-Ebene)

| Verzeichnis | Dateien | Empfehlung |
|-------------|---------|------------|
| `docs/de/` root | `INDEX.md`, `DOCUMENTATION_INDEX.md`, `00_DOCUMENTATION_INDEX.md`, `RAID_THEMIS_DOCUMENTATION_INDEX.md`, `SHARDING_DOCUMENTATION_INDEX.md` | Kanonisch: `00_DOCUMENTATION_INDEX.md`; andere als redirect markieren |

---

## Follow-up Docs Issues

- **DOCS-INV-001:** `docs/de/src/` (leer) — Verzeichnis prüfen und ggf. entfernen
- **DOCS-INV-002:** `docs/de/integrations/` vs `docs/de/integration/` — Semantisches Duplikat; konsolidieren
- **DOCS-INV-003:** 22 Stale-Dateien erfordern Review-Termine (Target: DOC-WEEKLY-2026-35)
- **DOCS-INV-004:** 14 Archive-Candidates (MOVE erforderlich) — Phase 2 dieser Konsolidierung
- **DOCS-INV-005:** `docs/de/` root Index-Duplikate — Redirect-Notices erforderlich
