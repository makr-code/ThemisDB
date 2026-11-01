---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Developer LLM Wiki — Index

Datum: 2026-11-01
Status: Active
Bezug: CI-verwaltete Entwickler-Wissensbasis fuer Coder-LLMs
Primary (Quelle der Wahrheit): DOCUMENTATION_GOVERNANCE.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md, ai_context/COPILOT_INSTRUCTIONS.md

## Scope

- Ziel: Onboarding- und Coding-relevantes Wissen fuer Entwickler-LLMs
- Laufmodus: FULL_SYNC
- Quellen gesamt: 9507
- Quellen-Hash: `5dd3d8c27e9f28ca04bc7dd1207cd4714446750bf012907217cbeead06bd72e2`

## Status And Source-of-Truth Governance

- Release-readiness, GA status, blocker state, and implementation posture are canonical only in `ROADMAP.md` and active governance/release docs.
- `ai_context/developer_llm_wiki/` is operational context; it must not redefine project status.
- `ai_working/` artifacts are historical evidence unless explicitly promoted by root governance.

## Artefakte

- [MODULES_AND_APIS.md](MODULES_AND_APIS.md)
- [BUILD_TEST_CI_AND_OPERATIONS.md](BUILD_TEST_CI_AND_OPERATIONS.md)
- [GOVERNANCE_AND_ROADMAP.md](GOVERNANCE_AND_ROADMAP.md)
- [AQL module context](AQL.md) — assistance API boundaries, validation behavior, and test/benchmark registration.
- [Ethics AI module context](ETHICS_AI.md) — in-tree API/runtime boundaries, LDM helper limits, test/benchmark registration, and canonical plugin source boundary.
- [IMPORTERS.md](IMPORTERS.md) — curated importer module context
- [UPDATES.md](UPDATES.md) — curated updates module context
- [DISTRIBUTED_TENSOR.md](DISTRIBUTED_TENSOR.md) — curated distributed tensor module context
- [PROCESS.md](PROCESS.md) — curated Process module context
- [SOURCE_MANIFEST.json](SOURCE_MANIFEST.json)
- [WIKI_STATUS.json](WIKI_STATUS.json)
- [WIKI_DELTA_REPORT.md](WIKI_DELTA_REPORT.md)

## Source Distribution

- modules_and_api: 3820
- build_test_ci: 203
- governance_and_docs: 5484
- misc: 0

## Source-Priority / Konfliktregel

1. Root Governance/SOT-Dokumente
2. Modul-ROADMAP/FUTURE_ENHANCEMENTS/README
3. Sonstige docs/ und ai_context/ Wissensseiten
4. CI-/Workflow- und Script-Metadaten

Bei widerspruechlichen Aussagen wird markiert statt still ueberschrieben. Root-SOT und aktive Governance gehen vor Modul-Synthese; Modulquellen gehen vor Wiki-Zusammenfassungen.
