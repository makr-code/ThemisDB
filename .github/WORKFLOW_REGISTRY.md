# ThemisDB Workflow Registry (Lean Core)

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
  — Konsolidierter Security-Scan: Trivy Vulnerability Scan + Gitleaks Secret Scan + KubeSec Manifest-Scan + DAST/ZAP (Schedule/Dispatch only; 4 einzeln guardierte Jobs; ersetzt security.yml + security-scanning.yml)
- `.github/workflows/security-fortify.yml`
  — Fortify AST Scan (continue-on-error; requires FOD_TENANT/FOD_USER/FOD_PAT secrets)
- `.github/workflows/sanitizer-nightly.yml`
  — Geplante ASan/UBSan-Nachtlaeufe auf ausgewaehlten kritischen Targets
- `.github/workflows/security-pentest-quarterly.yml`
  — Quartals-Pentest-Cadence mit Evidence-Artefakten (non-mutating)
- `.github/workflows/compliance-supply-chain.yml`
  — SBOM-/Signatur-/Release-Compliance-Pruefungen
- `.github/workflows/security-codeql.yml`
  — CodeQL Analyse-Workflow
- `.github/workflows/compliance-governance-gates.yml`
  — Governance- und Release-Policy-Gates
- `.github/workflows/maintenance-docs.yml`
  — Dokumentations-Hygiene/Alignment Workflows; deckt auch `ai_context/**` und `ai_working/**` ab (Stale-Cleanup + Orphan-Check)
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
- `.github/workflows/publish-wiki.yml`
  — Publishes docs/architecture, docs/governance, src/*/ROADMAP.md and developer wiki to GitHub Wiki on push to develop or manual dispatch; community guardrail blocks private plugin paths

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
- Aktive Workflows im Verzeichnis `.github/workflows/`: 43
- Deaktivierte Workflows in `.github/no_workflows/`: 30
- Strategie: Lean + harte Triggergrenzen + Quarantaene fuer uebertriggernde CI
- Der 21er-Zähler war im vorherigen Dokumentationsstand veraltet; der aktuelle Stand wird durch die kanonische Liste in diesem Registry-Dokument und die zugehörigen Workflow-Dateien definiert.

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
