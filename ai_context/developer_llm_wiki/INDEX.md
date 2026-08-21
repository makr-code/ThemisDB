# Developer LLM Wiki — Index

Datum: 2026-08-19
Status: Active
Bezug: CI-verwaltete Entwickler-Wissensbasis fuer Coder-LLMs
Primary (Quelle der Wahrheit): DOCUMENTATION_GOVERNANCE.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md, ai_context/COPILOT_INSTRUCTIONS.md

## Scope

- Ziel: Onboarding- und Coding-relevantes Wissen fuer Entwickler-LLMs
- Laufmodus: FULL_SYNC
- Quellen gesamt: 6447
- Quellen-Hash: `d78a5dfa06bd31f17f50c139827d61bc7450da27e8635d78cc3efa17c90dda79`

## Artefakte

- [AI_METADATA_AND_PROVENANCE.md](AI_METADATA_AND_PROVENANCE.md)
- [MODULES_AND_APIS.md](MODULES_AND_APIS.md)
- [BUILD_TEST_CI_AND_OPERATIONS.md](BUILD_TEST_CI_AND_OPERATIONS.md)
- [GOVERNANCE_AND_ROADMAP.md](GOVERNANCE_AND_ROADMAP.md)
- [SOURCE_MANIFEST.json](SOURCE_MANIFEST.json)
- [WIKI_STATUS.json](WIKI_STATUS.json)
- [WIKI_DELTA_REPORT.md](WIKI_DELTA_REPORT.md)

## Source Distribution

- modules_and_api: 2931
- build_test_ci: 90
- governance_and_docs: 3426
- misc: 0

## Source-Priority / Konfliktregel

1. Root Governance/SOT-Dokumente
2. Modul-ROADMAP/FUTURE_ENHANCEMENTS/README
3. Sonstige docs/ und ai_context/ Wissensseiten
4. CI-/Workflow- und Script-Metadaten

Bei widerspruechlichen Aussagen wird markiert statt still ueberschrieben.

## Canonical AI Metadata

- Die Seite [AI_METADATA_AND_PROVENANCE.md](AI_METADATA_AND_PROVENANCE.md) ist die kanonische Entwicklerreferenz fuer Provenienz, Transformationsmetadaten und Degradationslogik.
- Sie gilt fuer Wiki-Seiten, Claims, Graph-Kanten, Importpfade und Query-Ergebnisse.
- Alle neuen AI-nahe Module sollten dieses Schema direkt verwenden, statt eigene Nebenschemata einzufuehren.
