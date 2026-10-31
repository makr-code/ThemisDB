# Developer LLM Wiki — Index

Datum: 2026-10-31
Status: Active
Bezug: CI-verwaltete Entwickler-Wissensbasis fuer Coder-LLMs
Primary (Quelle der Wahrheit): DOCUMENTATION_GOVERNANCE.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md, ai_context/COPILOT_INSTRUCTIONS.md

## Scope

- Ziel: Onboarding- und Coding-relevantes Wissen fuer Entwickler-LLMs
- Laufmodus: FULL_SYNC
- Quellen gesamt: 9424
- Quellen-Hash: `99796e682ea3e5859b116343ccd6b3dd95d8b003e3382af1c5396321c1aa75e7`

## Status And Source-of-Truth Governance

- Release-readiness, GA status, blocker state, and implementation posture are canonical only in `ROADMAP.md` and active governance/release docs.
- `ai_context/developer_llm_wiki/` is operational context; it must not redefine project status.
- `ai_working/` artifacts are historical evidence unless explicitly promoted by root governance.

## Artefakte

- [MODULES_AND_APIS.md](MODULES_AND_APIS.md)
- [BUILD_TEST_CI_AND_OPERATIONS.md](BUILD_TEST_CI_AND_OPERATIONS.md)
- [GOVERNANCE_AND_ROADMAP.md](GOVERNANCE_AND_ROADMAP.md)
- [IMPORTERS.md](IMPORTERS.md) — curated importer module context
- [UPDATES.md](UPDATES.md) — curated updates module context
- [SOURCE_MANIFEST.json](SOURCE_MANIFEST.json)
- [WIKI_STATUS.json](WIKI_STATUS.json)
- [WIKI_DELTA_REPORT.md](WIKI_DELTA_REPORT.md)

## Source Distribution

- modules_and_api: 3744
- build_test_ci: 203
- governance_and_docs: 5477
- misc: 0

## Source-Priority / Konfliktregel

1. Root Governance/SOT-Dokumente
2. Modul-ROADMAP/FUTURE_ENHANCEMENTS/README
3. Sonstige docs/ und ai_context/ Wissensseiten
4. CI-/Workflow- und Script-Metadaten

Bei widerspruechlichen Aussagen wird markiert statt still ueberschrieben. Root-SOT und aktive Governance gehen vor Modul-Synthese; Modulquellen gehen vor Wiki-Zusammenfassungen.
