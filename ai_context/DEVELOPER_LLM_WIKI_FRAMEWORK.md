# Developer LLM Wiki Framework (ThemisDB)

Datum: 2026-08-12
Status: Active
Bezug: CI/CD-verwaltete Entwickler-Wissensbasis in `ai_context/`
Primary (Quelle der Wahrheit): DOCUMENTATION_GOVERNANCE.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md, .github/workflows/maintenance-docs.yml

## 1) Scope und Zielbild

- Das Entwickler-LLM-Wiki ist eine kuratierte Wissensbasis fuer Coder-LLMs.
- Ablageort ist `ai_context/developer_llm_wiki/`.
- Fokus: onboarding- und coding-relevantes Wissen (Module, APIs, Build/Test, Governance, Workflows, Risiken).
- Keine Endnutzerdokumentation als Primaerziel.

## 2) Zielstruktur

Der Sync-Lauf erzeugt und pflegt diese Artefakte:

- `ai_context/developer_llm_wiki/INDEX.md` (Einstieg, Scope, Prioritaetsregeln)
- `ai_context/developer_llm_wiki/MODULES_AND_APIS.md` (modul-/API-bezogene Quellen)
- `ai_context/developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md` (Build/Test/CI/Operations)
- `ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md` (Governance- und Roadmap-Wissen)
- `ai_context/developer_llm_wiki/SOURCE_MANIFEST.json` (maschinenlesbares Quellinventar)
- `ai_context/developer_llm_wiki/WIKI_STATUS.json` (Laufstatus + Delta-Metadaten)
- `ai_context/developer_llm_wiki/WIKI_DELTA_REPORT.md` (added/removed/changed)

## 3) Quellen und Update-Regeln

### Kanonische Inputs

- Root-SOT/Governance: `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `DOCUMENTATION_GOVERNANCE.md`, `RELEASE_STRATEGY.md`, `BRANCHING_STRATEGY.md`, `VERSIONING.md`, `CHANGELOG.md`
- Modul-/API-Quellen: `src/**/README.md`, `src/**/ROADMAP.md`, `src/**/FUTURE_ENHANCEMENTS.md`, `include/**/*.h`, `include/**/*.hpp`
- Betriebsquellen: `.github/workflows/*.yml`, `scripts/*.py`
- Dokumentationsquellen: `docs/**/*.md`, `ai_context/**/*.md`

### SOT-Prio und Konflikte

Prioritaet:
1. Root-SOT/Governance
2. Modul-ROADMAP/FUTURE/README
3. docs/ und ai_context/
4. Workflow-/Script-Metadaten

Regel:
- Bei Konflikten wird markiert (Delta/Status), nicht still ueberschrieben.

## 4) Pipeline-Integration im bestehenden Workflow

- Der Ablauf ist als eigener Job in `.github/workflows/maintenance-docs.yml` integriert.
- Kein neuer Workflow wird angelegt.
- Steuerung per `workflow_dispatch` Inputs:
  - `llm_wiki_apply_updates`
  - `llm_wiki_full_sync`
  - `llm_wiki_fail_on_findings`

## 5) Qualitaets- und Sicherheitsgates

- Struktur-/Link-Checks auf generierte Wiki-Artefakte.
- Duplikatpruefung (case-insensitive Pfadkollisionen).
- Secret-Pattern-Checks auf generierten Inhalten.
- Private-Boundary: `plugins/private/**` wird nicht als Wiki-Quelle uebernommen.

## 6) Betriebsmodell

- Owner: Maintainer der Docs-/Governance-Lane.
- Review-Cadence:
  - PR/push: read-only/check-orientiert (standardmaessig ohne fail-hard gate)
  - schedule/dispatch: periodische oder manuelle Synchronisation
- Failure Handling:
  - Findings stehen im Job-Summary und als Artefakt (`/tmp/dev-llm-wiki-*.{txt,json}`)
  - Korrekturen im Folge-PR, danach erneuter Lauf

## 7) Rollout-Stufen

- Phase A: Framework + read-only/check-Lauf + Artefakte + Summary
- Phase B: gezielte Auto-Updates via `workflow_dispatch` (`llm_wiki_apply_updates=true`)
- Phase C: optionale strengere Gates (`llm_wiki_fail_on_findings=true`) nach stabiler Signalqualitaet

## 8) Erster globaler Logic-Run (Pflicht nach Framework-Aufbau)

Der initiale Migrationslauf zur Ueberfuehrung bestehender Doku in das Entwickler-LLM-Wiki erfolgt via:

- Workflow: `Maintenance — Docs`
- Trigger: `workflow_dispatch`
- Inputs:
  - `llm_wiki_apply_updates=true`
  - `llm_wiki_full_sync=true`
  - `llm_wiki_fail_on_findings=false` (empfohlen fuer Initiallauf)

Erwartetes Ergebnis:
- Vollstaendige Erstbefuellung von `ai_context/developer_llm_wiki/`
- Delta-/Status-Artefakte als Baseline fuer nachfolgende inkrementelle Laeufe
