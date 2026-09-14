# ThemisDB Workflow Registry (Lean Core)

> Author: ThemisDB Contributors
> Created: 2026-09-09
> Last Updated: 2026-09-09
> Status: active

## Zielbild
Dieses Repository nutzt bewusst ein schlankes, release-orientiertes CI/CD-Set.
Alle nicht zwingenden Modul-/Spezial-Workflows wurden entfernt, um Wartung,
Signalqualität und Release-Stabilitaet zu verbessern.

## Leitprinzipien
- Keep it lean: nur Workflows mit direktem Beitrag zu Release, klar isolierten Qualitaetspruefungen oder Tooling-Governance.
- Modularisierung: wiederverwendbare Workflows statt duplizierter Build-Logik.
- Klare Verantwortlichkeit je Lane: nur explizit begrenzte Trigger statt repo-weiter Aktivierung.
- Keine Schatten-CI: neue oder reaktivierte Workflows nur mit begruendeter Notwendigkeit und Registry-Update.

## Aktiver Workflow-Kern

### Fokus-Workflows
- `.github/workflows/gate-pr-core.yml`
  — Fast PR-Gate-Layer inkl. `release-critical-tests` (mandatory), Boundary- und Policy-Gates
- `.github/workflows/gate-pr-doxygen-governance.yml`
  — PR-Gate fuer geaenderten C/C++-Sourcecode: GS3-Doxygen-Strukturpruefung, Doxygen-Audit-Warnungen, XML-Generierbarkeit, Coverage-/Waiver-Eskalation
- `.github/workflows/gate-pr-doc-metadata.yml`
  — Leichtgewichtiges Markdown-Metadaten-Gate fuer geaenderte Doku-Dateien; prueft Author/Urheber, Created, Last Updated und Status mit klaren Excludes fuer Backlog-/Archiv-/Template-Dateien
- `.github/workflows/build-mainline.yml`
  — Multi-OS Build/Test-Matrix inkl. optionaler Sanitizer-Lane per `workflow_dispatch`
- `.github/workflows/build-clang-fast.yml`
  — Lightweight Clang-Lane fuer PR-Fruehfeedback (ohne heavy Sanitizer-Overhead)
- `.github/workflows/release-mainline.yml`
  — Tag-/Dispatch-gesteuerte Release-Builds; CPack-Packaging (TGZ/DEB/RPM/ZIP/MSI); Manifest-Validierung, GitHub-Release-Erstellung und Publish-Lanes (community + private); Changelog-Automation
- `.github/workflows/build-benchmarks.yml`
  — Entkoppelte schwere Benchmark-Lanes (voice, GPU matrix, nightly sweep)
- `.github/workflows/release-changelog.yml`
  — Reusable/manual changelog update & backfill (artifact-backed proposal, keine Branch-Mutation)
- `.github/workflows/security-consolidated.yml`
  — Konsolidierter Security-Scan: Trivy Vulnerability Scan + Gitleaks Secret Scan + KubeSec Manifest-Scan (Schedule/Dispatch only; 3 einzeln guardierte Jobs; ersetzt security.yml + security-scanning.yml; DAST/ZAP wurde in security-dast-zap.yml ausgelagert)
- `.github/workflows/security-dast-zap.yml`
  — OWASP ZAP DAST: Baseline (wöchentlich passiv), API-Scan (OpenAPI-getrieben, aktiv), Full-Scan (manuell, aktiv); SARIF-Upload auf GitHub Security Tab; reusable via workflow_call für pre-release Gate
- `.github/workflows/security-fortify.yml`
  — Fortify AST Scan (continue-on-error; requires FOD_TENANT/FOD_USER/FOD_PAT secrets)
- `.github/workflows/build-sanitizer-nightly.yml`
  — Geplante ASan/UBSan-Nachtlaeufe auf ausgewaehlten kritischen Targets
- `.github/workflows/security-pentest-quarterly.yml`
  — Quartals-Pentest-Cadence mit Evidence-Artefakten (non-mutating)
- `.github/workflows/compliance-supply-chain.yml`
  — SBOM-/Signatur-/Release-Compliance-Pruefungen
- `.github/workflows/security-codeql.yml`
  — CodeQL Analyse-Workflow
- `.github/workflows/compliance-governance-gates.yml`
  — Compliance-/Governance- und Release-Policy-Gates
- `.github/workflows/maintenance-docs.yml`
  — Dokumentations-Hygiene/Alignment Workflows; deckt auch `ai_context/**` und `ai_working/**` ab (Stale-Cleanup + Orphan-Check)
- `.github/workflows/maintenance-docs-db-build.yml`
  — Docs-to-ThemisDB: Ingests docs/ into a RocksDB database via themis_docs_builder; triggered on docs/** changes (push develop/community); content-hash guard prevents redundant rebuilds; workflow_dispatch supports arbitrary input_dir
- `.github/workflows/maintenance-architecture-ci.yml`
  — Architecture model generator: scans ai_context/, ai_working/, developer_llm_wiki/, api_contracts/, ARCHITECTURE.md, ROADMAP.md, docs/ and produces architecture.json + architecture.md (Mermaid diagram); schedule weekly + dispatch + path-filtered push on develop; opens PR when outputs change
- `.github/workflows/reusable-docs-db-builder.yml`
  — Reusable: generalized folder→ThemisDB-DB pipeline; builds themis_docs_builder, hashes input tree, skips if up-to-date, uploads artifact; called by maintenance-docs-db-build.yml and any future per-folder callers
- `.github/workflows/maintenance-ci-health.yml`
  — Wöchentliches CI Health Dashboard (pass/fail Aggregation, chronische Fehler-Issue; Sunday 06:00 UTC)
- `.github/workflows/maintenance-issues.yml`
  — Konsolidiertes Issue-Maintenance: GS3-Gap-Triage (03:30 UTC) + Security-Alert-SLA-Triage (05:30 UTC); ersetzt maintenance-gs3-gaps.yml + maintenance-security-alerts.yml
- `.github/workflows/maintenance-issue-recommendations.yml`
  — Recommend-only Issue Triage; kommentiert offene Issues mit merged-PR-Evidenz und bleibt bewusst non-destructive
- `.github/workflows/maintenance-milestones.yml`
  — Milestone-Governance: synchronisiert kanonische Milestones aus `.github/milestones.yml` und weist Issues/PRs automatisch anhand von Labels bzw. `Target Version` zu (inkl. HOTPATCH/LONG-TERM)
- `.github/workflows/release-docker-image.yml`
  — Container build/publish lane; triggered via workflow_run after successful CI — Release (koordiniert mit release-mainline.yml)
- `.github/workflows/edition-hyperscaler-ci.yml`
  — Editionsspezifische Hyperscaler-CI Lane
- `.github/workflows/automation-community.yml`
  — Community Automation (Labeling/Onboarding)
- `.github/workflows/build-ollama-router.yml`
  — Scoped CI fuer `tools/copilot-ollama-router/**`
- `.github/workflows/gate-copilot-regression.yml`
  — Copilot/CMake-Regression Guard
- `.github/workflows/copilot-code-review.yml`
  — Self-scoped Copilot review runner declaration (`copilot-setup-steps`) so agentic reviews have an assigned Ubuntu runner
- `.github/workflows/publish-wiki.yml`
  — Publishes docs/architecture, docs/governance, src/*/ROADMAP.md and developer wiki to GitHub Wiki on push to develop or manual dispatch; community guardrail blocks private plugin paths
- `.github/workflows/build-wave-a-gpu.yml`
  — Wave-A GPU CI: build + test GPU index targets on push to develop (gpu/); schedule/dispatch only – no PR trigger (SOC boundary)
- `.github/workflows/build-wave-b-transaction.yml`
  — Wave-B Transaction CI: build + test distributed transaction targets on push to develop; schedule/dispatch only – no PR trigger (SOC boundary)
- `.github/workflows/build-wave-b-llm-benchmarks.yml`
  — Wave-B LLM Wiki Phase-B benchmark validation; dispatch-only; hardware-detection → data-preparation → benchmark-execution matrix
- `.github/workflows/build-content-regression.yml`
  — Content Regression (Wave D): push-triggered ctest --label-include content on develop; Sunday soak run; dispatch manual
- `.github/workflows/build-llm-inference.yml`
  — LLM Inferencing CI lane (TinyLlama + doku.db RAG + AdaLoRA); push + schedule + dispatch
- `.github/workflows/build-widget.yml`
  — Scoped CI for widget path; dispatch-only
- `.github/workflows/gate-distributed-knowledge.yml`
  — Module validation gate for distributed_knowledge; push + PR + dispatch
- `.github/workflows/gate-pr-community-failclosed.yml`
  — PR gate: community fail-closed policy enforcement; workflow_call + PR + push + dispatch
- `.github/workflows/gate-pr-edition-license.yml`
  — PR gate: edition & license validation; workflow_call + PR + push + dispatch
- `.github/workflows/gate-pr-hash-sbom.yml`
  — PR gate: hash & SBOM integrity checks; workflow_call + PR + push + dispatch
- `.github/workflows/gate-pr-plugin-boundary.yml`
  — PR gate: private plugin boundary enforcement; workflow_call + PR + push + dispatch
- `.github/workflows/gate-pr-version-targeting.yml`
  — PR gate: Target Version field and milestone assignment validation; PR-only (opened/edited/synchronize)
- `.github/workflows/maintenance-ai-working.yml`
  — AI Working cleanup (LLM Wiki stale files); schedule + push + dispatch
- `.github/workflows/maintenance-build-issues.yml`
  — Build error issue tracking; workflow_run + schedule + dispatch
- `.github/workflows/maintenance-labels.yml`
  — Label sync from .github/labels.yml; schedule + push + dispatch
- `.github/workflows/maintenance-pr-failure-diagnosis.yml`
  — PR failure diagnosis (recommend-only, non-destructive); workflow_run + dispatch
- `.github/workflows/maintenance-workflow-guardrails-observe.yml`
  — Workflow boundary guard observation; PR + schedule + dispatch
- `.github/workflows/release-build-matrix.yml`
  — Reusable multi-OS build/test matrix; workflow_call only
- `.github/workflows/release-nightly.yml`
  — Automatic nightly builds and releases; schedule nightly + dispatch + push
- `.github/workflows/release-promote.yml`
  — Semi-automatic stable/rc/alpha release triggering via PR labels or dispatch
- `.github/workflows/release-publish.yml`
  — Unified orchestration for multi-registry releases; triggered on version tag push
- `.github/workflows/release-rollback.yml`
  — Manual release rollback (delete artifacts, revert version); dispatch-only
- `.github/workflows/release-winget.yml`
  — Automated WinGet community package submission after stable release; workflow_call + dispatch
- `.github/workflows/reusable-cmake-build.yml`
  — Reusable CMake build pipeline; workflow_call only
- `.github/workflows/reusable-status-flags-and-issues.yml`
  — Reusable: status flags and issue/PR comment/label interface; workflow_call only
- `.github/workflows/security-fuzzing.yml`
  — Fuzz testing (libFuzzer targets: aql_parser, gguf_loader, grammar, …); schedule Sunday + dispatch

## Governance fuer neue Workflows
Neue Workflow-Dateien sind nur erlaubt, wenn mindestens einer der Punkte zutrifft:
- Erforderlich fuer ein neues Release-Artefakt oder ein verpflichtendes Compliance-Gate.
- Nicht sinnvoll als Job in einen bestehenden aktiven Workflow integrierbar.
- Enthalten klare Owner, harte Trigger-Grenzen (`paths`, `branches`), `concurrency` und Wartungsplan.
- Starten nicht repo-weit auf generischen Sammelmustern und fuehren keine Build-/Benchmark-Last auf normalen Doku- oder Metadaten-Aenderungen aus.

## Harte Aktivierungskriterien
- Neue PR-Workflows muessen datei- oder modulspezifische `paths:` besitzen.
- Benchmark-, Audit-, GPU- und Nightly-Workflows sind standardmaessig keine Required Checks.
- Reaktivierte Workflows muessen zunaechst in einer risikoarmen Form starten: `workflow_dispatch`, `schedule` oder non-blocking.
- Doppelte Abdeckung mit bestehenden Workflows ist ein Ablehnungsgrund.

## Validierung
Lokaler Standard-Check:

```powershell
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode all
```

### Test-System im Detail

Das lokale GitHub-Action-Testsystem besteht aus zwei Kernschritten:

- `actionlint` via Docker (`rhysd/actionlint:latest`): validiert die Workflow-Syntax und die strukturelle Korrektheit der YAML-Dateien.
- `act` Dry-Run: simuiert Workflow-Events wie `push`, `pull_request`, `workflow_dispatch` und `schedule` ohne echten GitHub Runner.

Die genaue Logik sitzt in `scripts/test-github-actions-local.ps1` und erzeugt Logs im Standardordner `tmp/`:

```powershell
actionlint_<timestamp>.log
act_dryrun_<event>_<timestamp>.log
```

Die Verifikation besteht aus drei Modi:

```powershell
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode lint
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode dryrun
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode all
```

Hinweis: `Mode all` ist der Standard-Check vor Merge. `act` kann bei Events ohne passende Stages als Skip melden; das ist kein Workflow-Fehler, sondern ein lokales Laufzeit-Limit der Simulation.

## Naming-Migration
Geplante Dateinamen-Harmonisierung (Soll-Format aus Workflow-Design):

- `.github/docs/WORKFLOW_FILENAME_RENAME_MATRIX.md`

## Stand
- Aktive Workflows im Verzeichnis `.github/workflows/`: 57
- Deaktivierte Workflows in `.github/no_workflows/`: 30
- Strategie: Lean + harte Triggergrenzen + Quarantaene fuer uebertriggernde CI
- Der 21er-Zähler war im vorherigen Dokumentationsstand veraltet; der aktuelle Stand wird durch die kanonische Liste in diesem Registry-Dokument und die zugehörigen Workflow-Dateien definiert.
- Naming-Migration Sprint 6 (2026-09-14): 13-wave-* und sanitizer-nightly.yml auf kanonisches Schema umbenannt; pull_request-Boundary-Verletzungen entfernt; fehlende concurrency-Blöcke ergänzt; name:-Felder normalisiert.

## Durchgeführte Konsolidierungen (Workflow Framework Refactoring)

| Aktion | Quelle(n) | Ziel | Sprint |
|---|---|---|---|
| Trigger-Cleanup | build-benchmarks.yml | push/PR entfernt → schedule/dispatch only | 1 |
| Trigger-Cleanup | security-fortify.yml | pull_request entfernt → schedule only | 1 |
| Trigger-Cleanup | compliance-governance-gates.yml | paths: Filter für push/develop | 1 |
| Trigger-Cleanup | maintenance-docs.yml | pull_request entfernt | 1 |
| Entfernt | security-scan.yml | aus aktivem Bestand entfernt | 1 |
| Konsolidiert | security.yml + security-scanning.yml | security-consolidated.yml | 2 |
| ai_context + ai_working Coverage | maintenance-docs.yml | ai-working-hygiene Job hinzugefügt | 2 (new req) |
| Composite Action | — | .github/actions/setup-python-script/ | 3 |
| Konsolidiert | maintenance-gs3-gaps.yml + maintenance-security-alerts.yml | maintenance-issues.yml | 4 |
| Composite Action | build-benchmarks.yml + release-mainline.yml | .github/actions/bootstrap-build-tracker/ | 5 |
| Umbenannt + SOC-Fix | 13-wave-a-gpu-ci-execution.yml | build-wave-a-gpu.yml (pull_request entfernt) | 6 |
| Umbenannt + SOC-Fix | 13-wave-b-transaction-ci-execution.yml | build-wave-b-transaction.yml (pull_request entfernt) | 6 |
| Umbenannt + concurrency | 13-wave-b-llm-wiki-benchmarks.yml | build-wave-b-llm-benchmarks.yml | 6 |
| Umbenannt | sanitizer-nightly.yml | build-sanitizer-nightly.yml | 6 |
| name:-Normalisierung | release-docker-image, security-codeql, security-fuzzing, maintenance-architecture-ci, gate-copilot-regression, gate-pr-version-targeting, compliance-governance-gates, copilot-code-review | name: schema `Domain: Purpose` | 6 |
| concurrency hinzugefügt | security-fuzzing.yml, build-content-regression.yml | cancel-in-progress guard | 6 |
