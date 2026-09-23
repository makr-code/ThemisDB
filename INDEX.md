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

#### Root ai_context/ — Status Reports & Analysis

- **Module Status Reports** (2026-07-18 — 2026-08-09):
  - [ai_context/GOVERNANCE_MODULE_STATUS_2026_07_18.md](ai_context/GOVERNANCE_MODULE_STATUS_2026_07_18.md)
  - [ai_context/API_MODULE_STATUS_2026_07_18.md](ai_context/API_MODULE_STATUS_2026_07_18.md)
  - [ai_context/UTILS_MODULE_STATUS_2026_07_18.md](ai_context/UTILS_MODULE_STATUS_2026_07_18.md)
  - [ai_context/INDEX_MODULE_STATUS_2026_08_02.md](ai_context/INDEX_MODULE_STATUS_2026_08_02.md)
  - [ai_context/INDEX_MODULE_STATUS_2026_08_09.md](ai_context/INDEX_MODULE_STATUS_2026_08_09.md)

- **Implementation & Audit Reports** (2026-08-09):
  - [ai_context/DOCUMENTATION_UPDATE_EXECUTION_SUMMARY_2026_08_09.md](ai_context/DOCUMENTATION_UPDATE_EXECUTION_SUMMARY_2026_08_09.md)
  - [ai_context/RESEARCH_INTEGRATION_AUDIT_2026_08_09.md](ai_context/RESEARCH_INTEGRATION_AUDIT_2026_08_09.md)

- **Governance Phase Reports** (2026-07-18 — 2026-08-09):
  - [ai_context/GOVERNANCE_PHASE2_3_IMPLEMENTATION_REPORT.md](ai_context/GOVERNANCE_PHASE2_3_IMPLEMENTATION_REPORT.md)
  - [ai_context/GOVERNANCE_PHASE2B_3A_3B_IMPLEMENTATION_REPORT.md](ai_context/GOVERNANCE_PHASE2B_3A_3B_IMPLEMENTATION_REPORT.md)

- **Architecture & Namespace Analysis** (2026-07-28):
  - [ai_context/AI_CONTEXT_NAMESPACE_INDEX.md](ai_context/AI_CONTEXT_NAMESPACE_INDEX.md)
  - [ai_context/API_CONTRACT_TEMPLATES.md](ai_context/API_CONTRACT_TEMPLATES.md)
  - [ai_context/ARCHITECTURE_CLASSIFICATION.md](ai_context/ARCHITECTURE_CLASSIFICATION.md)
  - [ai_context/MODULES_AND_NAMESPACES.md](ai_context/MODULES_AND_NAMESPACES.md)

- **Phase Implementation Checklists** (2026-08-23 — 2026-08-26):
  - [ai_context/PHASE_1_6_CHECKLIST.md](ai_context/PHASE_1_6_CHECKLIST.md)
  - [ai_context/PHASE_1_6_IMPLEMENTATION_FRAMEWORK.md](ai_context/PHASE_1_6_IMPLEMENTATION_FRAMEWORK.md)

- **Closure & Issue Reports**:
  - [ai_context/ISSUE_5647_CLOSURE_SUMMARY.md](ai_context/ISSUE_5647_CLOSURE_SUMMARY.md)
  - [ai_context/MODULE_DOCUMENTATION_REMEDIATION_CHECKLIST.md](ai_context/MODULE_DOCUMENTATION_REMEDIATION_CHECKLIST.md)

- **Developer LLM Wiki Framework** (2026-08-14):
  - [ai_context/DEVELOPER_LLM_WIKI_FRAMEWORK.md](ai_context/DEVELOPER_LLM_WIKI_FRAMEWORK.md)

- **Reference Root**:
  - [ai_context/README.md](ai_context/README.md)

#### API Contracts Directory

- [ai_context/api_contracts/README.md](ai_context/api_contracts/README.md)

#### Developer LLM Wiki — Doxygen & Module Analysis

- [ai_context/developer_llm_wiki/MODULE_DIRECT_DOXYGEN_CHECK.md](ai_context/developer_llm_wiki/MODULE_DIRECT_DOXYGEN_CHECK.md)
- [ai_context/developer_llm_wiki/MODULE_DOXYGEN_BASELINE_SUMMARY.md](ai_context/developer_llm_wiki/MODULE_DOXYGEN_BASELINE_SUMMARY.md)
- [ai_context/developer_llm_wiki/MODULE_DOXYGEN_BATCH_.md](ai_context/developer_llm_wiki/MODULE_DOXYGEN_BATCH_.md)
- [ai_context/developer_llm_wiki/MODULE_DOXYGEN_COVERAGE_SUMMARY.md](ai_context/developer_llm_wiki/MODULE_DOXYGEN_COVERAGE_SUMMARY.md)
- [ai_context/developer_llm_wiki/MODULE_DOXYGEN_SMOKE_SUMMARY.md](ai_context/developer_llm_wiki/MODULE_DOXYGEN_SMOKE_SUMMARY.md)
- [ai_context/developer_llm_wiki/RETRIEVAL_DIRECT_DOXYGEN_CHECK.md](ai_context/developer_llm_wiki/RETRIEVAL_DIRECT_DOXYGEN_CHECK.md)
- [ai_context/developer_llm_wiki/SOLL_IST_GAP_SUMMARY.md](ai_context/developer_llm_wiki/SOLL_IST_GAP_SUMMARY.md)
- [ai_context/developer_llm_wiki/WIKI_DELTA_REPORT.md](ai_context/developer_llm_wiki/WIKI_DELTA_REPORT.md)

#### Research Exports (2026-02 — 2026-11)

External research and analysis documents:

- [ai_context/research/gemini_exports/README.md](ai_context/research/gemini_exports/README.md)
- [ai_context/research/gemini_exports/Architektur_der_verteilten_Intelligenz__Vorzüge_des_Themis-Sharding_für_militärische_Operationsführung.md](ai_context/research/gemini_exports/Architektur_der_verteilten_Intelligenz__Vorzüge_des_Themis-Sharding_für_militärische_Operationsführung.md)
- [ai_context/research/gemini_exports/Die_Architektur_der_digitalen_Souveränität__Eine_strategische_und_technische_Analyse_von_ThemisDB_im_Kontext_der_Datenökonomie_2025_2026.md](ai_context/research/gemini_exports/Die_Architektur_der_digitalen_Souveränität__Eine_strategische_und_technische_Analyse_von_ThemisDB_im_Kontext_der_Datenökonomie_2025_2026.md)
- [ai_context/research/gemini_exports/Forschungsbericht_ThemisDB_2025_THEMISDB_PUBLIC_REPORT.md](ai_context/research/gemini_exports/Forschungsbericht_ThemisDB_2025_THEMISDB_PUBLIC_REPORT.md)
- [ai_context/research/gemini_exports/Gemini-Export_2._November_2025_um_11_44_32_MEZ.md](ai_context/research/gemini_exports/Gemini-Export_2._November_2025_um_11_44_32_MEZ.md)
- [ai_context/research/gemini_exports/Gemini-Export_2._November_2025_um_11_45_21_MEZ.md](ai_context/research/gemini_exports/Gemini-Export_2._November_2025_um_11_45_21_MEZ.md)
- [ai_context/research/gemini_exports/Hybride_Datenbankarchitektur_C++_Rust_variant.md](ai_context/research/gemini_exports/Hybride_Datenbankarchitektur_C++_Rust_variant.md)
- [ai_context/research/gemini_exports/Hyperscaler-Einordnung_für_ThemisDB-Bericht.md](ai_context/research/gemini_exports/Hyperscaler-Einordnung_für_ThemisDB-Bericht.md)
- [ai_context/research/gemini_exports/In_der_Architekturanalyse_fehlt_der_Vergleich_zu....md](ai_context/research/gemini_exports/In_der_Architekturanalyse_fehlt_der_Vergleich_zu....md)
- [ai_context/research/gemini_exports/KI-Strategie__ThemisDB,_Hyperscaler,_Ethik.md](ai_context/research/gemini_exports/KI-Strategie__ThemisDB,_Hyperscaler,_Ethik.md)
- [ai_context/research/gemini_exports/Konvergente_Datenarchitekturen_für_souveräne_KI__ThemisDB_v1.0.0.md](ai_context/research/gemini_exports/Konvergente_Datenarchitekturen_für_souveräne_KI__ThemisDB_v1.0.0.md)
- [ai_context/research/gemini_exports/Strategische_Analyse__ThemisDB_–_Bewertung_einer_nativen_Multi-Modell-Architektur_im_Kontext_von_Sovereign-Cloud-Plattformen_und_On-Premise-RAG-Alternativen.md](ai_context/research/gemini_exports/Strategische_Analyse__ThemisDB_–_Bewertung_einer_nativen_Multi-Modell-Architektur_im_Kontext_von_Sovereign-Cloud-Plattformen_und_On-Premise-RAG-Alternativen.md)
- [ai_context/research/gemini_exports/Strategische_Gesamtanalyse__ThemisDB_als_ACID-Fundament_für_die_RAG-LLM-Strategie_der_deutschen_Verwaltung_–_Eine_komparative_Analyse_zur_Ablösung_der_UDS3-Architektur.md](ai_context/research/gemini_exports/Strategische_Gesamtanalyse__ThemisDB_als_ACID-Fundament_für_die_RAG-LLM-Strategie_der_deutschen_Verwaltung_–_Eine_komparative_Analyse_zur_Ablösung_der_UDS3-Architektur.md)
- [ai_context/research/gemini_exports/Technische_Tiefenanalyse__ThemisDB_v1.0.0_vs._Hyperscaler.md](ai_context/research/gemini_exports/Technische_Tiefenanalyse__ThemisDB_v1.0.0_vs._Hyperscaler.md)
- [ai_context/research/gemini_exports/Themis_DB,_eher_ein_Skalpell_als_schweizer_Taschenmesser_für_die_deutsche_Verwaltung.md](ai_context/research/gemini_exports/Themis_DB,_eher_ein_Skalpell_als_schweizer_Taschenmesser_für_die_deutsche_Verwaltung.md)
- [ai_context/research/gemini_exports/Themis_vs._Hyperscaler_Datenbanken_Vergleich.md](ai_context/research/gemini_exports/Themis_vs._Hyperscaler_Datenbanken_Vergleich.md)
- [ai_context/research/gemini_exports/Themis,_Llama.cpp_und_KMU-Cloud-Vergleich.md](ai_context/research/gemini_exports/Themis,_Llama.cpp_und_KMU-Cloud-Vergleich.md)
- [ai_context/research/gemini_exports/ThemisDB_Dokumentation_und_Berichtsanalyse.md](ai_context/research/gemini_exports/ThemisDB_Dokumentation_und_Berichtsanalyse.md)
- [ai_context/research/gemini_exports/ThemisDB_Marktanalyse_&_Validierung.md](ai_context/research/gemini_exports/ThemisDB_Marktanalyse_&_Validierung.md)
- [ai_context/research/gemini_exports/ThemisDB_Recherche__Fehlende_Dokumentation_identifizieren.md](ai_context/research/gemini_exports/ThemisDB_Recherche__Fehlende_Dokumentation_identifizieren.md)
- [ai_context/research/gemini_exports/ThemisDB_Retry-Strategien_und_MVCC.md](ai_context/research/gemini_exports/ThemisDB_Retry-Strategien_und_MVCC.md)
- [ai_context/research/gemini_exports/ThemisDB_Skalierbarkeit_mit_RAID-Sharding.md](ai_context/research/gemini_exports/ThemisDB_Skalierbarkeit_mit_RAID-Sharding.md)
- [ai_context/research/gemini_exports/ThemisDB_v1.0.0__Strategische_und_Technische_Gesamtanalyse.md](ai_context/research/gemini_exports/ThemisDB_v1.0.0__Strategische_und_Technische_Gesamtanalyse.md)
- [ai_context/research/gemini_exports/ThemisDB__Analyse_und_Vergleich.md](ai_context/research/gemini_exports/ThemisDB__Analyse_und_Vergleich.md)
- [ai_context/research/gemini_exports/VCCDB_Design_variant.md](ai_context/research/gemini_exports/VCCDB_Design_variant.md)

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
