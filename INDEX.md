# ThemisDB Root Index

Datum: 2026-09-23
Status: Active
Bezug: Root-Navigation, Onboarding-Reihenfolge und AI-Wiki-Betrieb
Primary (Quelle der Wahrheit): README.md, ROADMAP.md, DOCUMENTATION_GOVERNANCE.md, AI_WIKI_INTEGRATION_PLAYBOOK.md

> Canonical root navigation and onboarding order.

---

## Canonical Onboarding Order

1. [README.md](README.md) — product overview and entry point
2. [QUICKSTART.md](QUICKSTART.md) — first install/start in minutes
3. [SETUP.md](SETUP.md) — full development environment setup
4. [SUPPORT.md](SUPPORT.md) — help channels and escalation paths
5. [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) — release lanes and lifecycle
6. [CTEST.md](CTEST.md) — current test inventory and known test-state context
7. [AI_WIKI_INTEGRATION_PLAYBOOK.md](AI_WIKI_INTEGRATION_PLAYBOOK.md) — operating model for AI wiki integration
8. [LOG.md](LOG.md) — append-only timeline for AI wiki operations

---

## Core Root Documents

- [ROADMAP.md](ROADMAP.md) — cross-module delivery roadmap
- [CHANGELOG.md](CHANGELOG.md) — released changes
- [AUDIT.md](AUDIT.md) — root audit navigation (points to canonical `audit/` hub)
- [VERSIONING.md](VERSIONING.md) — semantic versioning policy
- [SECURITY.md](SECURITY.md) — vulnerability reporting and security policy
- [CONTRIBUTING.md](CONTRIBUTING.md) — contribution process
- [CMakePresets.json](CMakePresets.json) — canonical configure/build/test presets
- [VERSION](VERSION) — current project version marker

---

## Developer Area Entry Points

- [src/README.md](src/README.md)
- [include/README.md](include/README.md)
- [tests/README.md](tests/README.md)
- [examples/README.md](examples/README.md)
- [tools/README.md](tools/README.md)

---

## AI Wiki Operations (Root)

- [AI_WIKI_INTEGRATION_PLAYBOOK.md](AI_WIKI_INTEGRATION_PLAYBOOK.md) — verbindliche Analyse, SOP, Routing und Rollenmodell
- [LOG.md](LOG.md) — chronologisches Betriebslog (append-only)
- [ai_context/COPILOT_INSTRUCTIONS.md](ai_context/COPILOT_INSTRUCTIONS.md) — agentische Arbeitsregeln im Repository
- [ai_context/KNOWLEDGE_LINT_REPORT.md](ai_context/KNOWLEDGE_LINT_REPORT.md) — Standardvorlage fuer periodische Knowledge-Lintpruefungen
- [ai_context/KNOWLEDGE_CONFLICTS.md](ai_context/KNOWLEDGE_CONFLICTS.md) — zentrales Konfliktregister fuer Wissenswidersprueche
- [DOCUMENTATION_GOVERNANCE.md](DOCUMENTATION_GOVERNANCE.md) — SOT-Domains, Prioritaeten und Doku-Governance

---

## AI Context Reference Library

### Developer LLM Wiki

- [ai_context/developer_llm_wiki/INDEX.md](ai_context/developer_llm_wiki/INDEX.md) — hub for module-level API and operational documentation
- [ai_context/developer_llm_wiki/MODULES_AND_APIS.md](ai_context/developer_llm_wiki/MODULES_AND_APIS.md) — API contracts, module boundaries, and symbol relationships
- [ai_context/developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md](ai_context/developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md) — CI/CD workflows, test infrastructure, and operational runbooks
- [ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md](ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md) — governance rules, release gates, and roadmap tracking
- [ai_context/developer_llm_wiki/AI_METADATA_AND_PROVENANCE.md](ai_context/developer_llm_wiki/AI_METADATA_AND_PROVENANCE.md) — AI decision history and knowledge evolution

### API Contracts

- [ai_context/api_contracts/](ai_context/api_contracts/) — API contract specifications by module:
  - [api_contracts/api.md](ai_context/api_contracts/api.md)
  - [api_contracts/auth.md](ai_context/api_contracts/auth.md)
  - [api_contracts/index.md](ai_context/api_contracts/index.md)
  - [api_contracts/llm.md](ai_context/api_contracts/llm.md)
  - [api_contracts/storage.md](ai_context/api_contracts/storage.md)
  - [api_contracts/transaction.md](ai_context/api_contracts/transaction.md)

### Core Development Policies

- [ai_context/memory_management_policy.md](ai_context/memory_management_policy.md) — RAII, ownership, and resource management guidelines
- [ai_context/OOP_AND_SOC_PRINCIPLES.md](ai_context/OOP_AND_SOC_PRINCIPLES.md) — object-oriented design and separation of concerns
- [ai_context/FUNCTION_CLASSIFICATION.md](ai_context/FUNCTION_CLASSIFICATION.md) — function categorization (hot-path, concurrency, plugin boundaries)
- [ai_context/COPILOT_INSTRUCTIONS.md](ai_context/COPILOT_INSTRUCTIONS.md) — (also cross-listed above)

### AI Context Archive

The following files are kept for historical reference and are no longer actively maintained:

- **Module Status Reports** (2026-07-18 — 2026-08-09): GOVERNANCE_MODULE_STATUS_2026_07_18.md, API_MODULE_STATUS_2026_07_18.md, UTILS_MODULE_STATUS_2026_07_18.md, INDEX_MODULE_STATUS_2026_08_02.md, INDEX_MODULE_STATUS_2026_08_09.md
- **Implementation Reports** (2026-08-09): DOCUMENTATION_UPDATE_EXECUTION_SUMMARY_2026_08_09.md, RESEARCH_INTEGRATION_AUDIT_2026_08_09.md
- **Governance Phase Reports** (2026-07-18 — 2026-08-09): GOVERNANCE_PHASE2_3_IMPLEMENTATION_REPORT.md, GOVERNANCE_PHASE2B_3A_3B_IMPLEMENTATION_REPORT.md
- **Analysis & Classification** (2026-07-28): AI_CONTEXT_NAMESPACE_INDEX.md, API_CONTRACT_TEMPLATES.md, ARCHITECTURE_CLASSIFICATION.md, MODULES_AND_NAMESPACES.md, OOP_AND_SOC_PRINCIPLES.md (core version also above)
- **Phase Checklists** (2026-08-23 — 2026-08-26): PHASE_1_6_CHECKLIST.md, PHASE_1_6_IMPLEMENTATION_FRAMEWORK.md
- **Doxygen & Module Analysis**: MODULE_DIRECT_DOXYGEN_CHECK.md, MODULE_DOXYGEN_BASELINE_SUMMARY.md, MODULE_DOXYGEN_BATCH_.md, MODULE_DOXYGEN_COVERAGE_SUMMARY.md, MODULE_DOXYGEN_SMOKE_SUMMARY.md, RETRIEVAL_DIRECT_DOXYGEN_CHECK.md, SOLL_IST_GAP_SUMMARY.md, WIKI_DELTA_REPORT.md
- **Closure & Issue Reports**: ISSUE_5647_CLOSURE_SUMMARY.md, MODULE_DOCUMENTATION_REMEDIATION_CHECKLIST.md
- **Developer LLM Wiki Framework** (2026-08-14): DEVELOPER_LLM_WIKI_FRAMEWORK.md
- **Research Exports** (2026-02 — 2026-11): research/gemini_exports/ (external research and analysis documents including:
  - Architektur der verteilten Intelligenz: Vorzüge des Themis-Sharding für militärische Operationsführung
  - Die Architektur der digitalen Souveränität (Datenökonomie 2025/2026)
  - Forschungsbericht ThemisDB 2025 (PUBLIC_REPORT)
  - Gemini-Exports (2025-11-02)
  - Hybride Datenbankarchitektur C++/Rust
  - Hyperscaler-Einordnung für ThemisDB-Bericht
  - KI-Strategie: ThemisDB, Hyperscaler, Ethik
  - Konvergente Datenarchitekturen für souveräne KI
  - Strategische Analysen: Multi-Modell-Architektur, ACID-Fundament für RAG-LLM
  - Technische Tiefenanalyse: ThemisDB v1.0.0 vs. Hyperscaler
  - ThemisDB: Skalpell vs. Schweizer Taschenmesser für deutsche Verwaltung
  - Themis vs. Hyperscaler Datenbanken Vergleich
  - Themis, Llama.cpp und KMU-Cloud-Vergleich
  - ThemisDB Dokumentation und Berichtsanalyse
  - ThemisDB Marktanalyse & Validierung
  - ThemisDB Recherche: Fehlende Dokumentation
  - ThemisDB Retry-Strategien und MVCC
  - ThemisDB Skalierbarkeit mit RAID-Sharding
  - ThemisDB v1.0.0 Strategische und Technische Gesamtanalyse
  - VCCDB Design
)

---

## Documentation & Review/Audit References

- [docs/Home.md](docs/Home.md)
- [docs/governance/documentation-history/DOCUMENTATION_REVIEW_SCHEDULE.md](docs/governance/documentation-history/DOCUMENTATION_REVIEW_SCHEDULE.md)
- [docs/PR_DOCUMENTATION_CHECKLIST.md](docs/PR_DOCUMENTATION_CHECKLIST.md)
- [docs/de/development/SOURCE_CODE_AUDIT.md](docs/de/development/SOURCE_CODE_AUDIT.md)
- [audit/AUDIT.md](audit/AUDIT.md)
- [audit/docs/audit-framework/AUDIT_RUNBOOK.md](audit/docs/audit-framework/AUDIT_RUNBOOK.md)

---
Zuletzt geprueft (Root-Sync): 2026-09-23
