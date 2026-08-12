# ThemisDB Workflow Registry (Lean Core)

## Zielbild
Dieses Repository nutzt bewusst ein schlankes, release-orientiertes CI/CD-Set.
Alle nicht zwingenden Modul-/Spezial-Workflows wurden entfernt, um Wartung,
Signalqualität und Release-Stabilitaet zu verbessern.
Deaktivierte Kandidaten liegen unter `.github/no_workflows/` und bilden eine
Quarantaene, nicht einen inoffiziellen Reservepool fuer schnelle Reaktivierung.

## Leitprinzipien
- Keep it lean: nur Workflows mit direktem Beitrag zu Release, klar isolierten Qualitaetspruefungen oder Tooling-Governance.
- Modularisierung: wiederverwendbare Workflows statt duplizierter Build-Logik.
- Klare Verantwortlichkeit je Lane: nur explizit begrenzte Trigger statt repo-weiter Aktivierung.
- Keine Schatten-CI: neue oder reaktivierte Workflows nur mit begruendeter Notwendigkeit und Registry-Update.
- No-Workflows-Quarantaene: Uebertriggernde oder redundante Workflows bleiben deaktiviert, bis Ursache, neue Triggergrenzen und Rollout-Plan dokumentiert sind.

## Aktiver Workflow-Kern

### Fokus-Workflows
- `.github/workflows/ci-pr-gates.yml`
  — Fast PR-Gate-Layer inkl. `release-critical-tests` (mandatory), Boundary- und Policy-Gates
- `.github/workflows/ci-build.yml`
  — Multi-OS Build/Test-Matrix inkl. optionaler Sanitizer-Lane per `workflow_dispatch`
- `.github/workflows/ci-release.yml`
  — Tag-/Dispatch-gesteuerte Release-Builds; CPack-Packaging (TGZ/DEB/RPM/ZIP/MSI); Manifest-Validierung, GitHub-Release-Erstellung und Publish-Lanes (community + private); Changelog-Automation
- `.github/workflows/ci-benchmarks.yml`
  — Entkoppelte schwere Benchmark-Lanes (voice, GPU matrix, nightly sweep)
- `.github/workflows/release-changelog.yml`
  — Reusable/manual changelog update & backfill (artifact-backed proposal, keine Branch-Mutation)
- `.github/workflows/security-scanning.yml`
  — Security-Scan-Orchestrierung (SAST/DAST-Signale + Artefakte)
- `.github/workflows/security-pentest-quarterly.yml`
  — Quartals-Pentest-Cadence mit Evidence-Artefakten (non-mutating)
- `.github/workflows/compliance-supply-chain.yml`
  — SBOM-/Signatur-/Release-Compliance-Pruefungen
- `.github/workflows/codeql.yml`
  — CodeQL Analyse-Workflow
- `.github/workflows/quality-static-analysis.yml`
  — Statische Qualitaetspruefungen und Artefaktberichte
- `.github/workflows/governance-gates.yml`
  — Governance- und Release-Policy-Gates
- `.github/workflows/maintenance-docs.yml`
  — Dokumentations-Hygiene/Alignment Workflows
- `.github/workflows/maintenance-cache-warming.yml`
  — Wöchentliches vcpkg/sccache Cache-Vorwärmen (Linux + Windows; Monday 00:00 UTC)
- `.github/workflows/maintenance-ci-health.yml`
  — Wöchentliches CI Health Dashboard (pass/fail Aggregation, chronische Fehler-Issue; Sunday 06:00 UTC)
- `.github/workflows/maintenance-security-alerts.yml`
  — Tägliche Security-SLA-Triage (Code-Scanning High/Critical, dedupliziertes Governance-Issue, Auto-Close bei Recovery)
- `.github/workflows/docker-image.yml`
  — Container build/publish lane; triggered via workflow_run after successful CI — Release (koordiniert mit ci-release.yml)
- `.github/workflows/edition-hyperscaler-ci.yml`
  — Editionsspezifische Hyperscaler-CI Lane
- `.github/workflows/automation-community.yml`
  — Community Automation (Labeling/Onboarding)
- `.github/workflows/copilot-ollama-router-ci.yml`
  — Scoped CI fuer `tools/copilot-ollama-router/**`
- `.github/workflows/copilot-regression-guard.yml`
  — Copilot/CMake-Regression Guard

## Quarantaene: `.github/no_workflows/`
Workflows in diesem Verzeichnis sind absichtlich deaktiviert. Eine Rueckverschiebung nach `.github/workflows/` ist nur zulaessig, wenn alle folgenden Punkte vorab dokumentiert sind:
- Warum der alte Trigger zu breit war.
- Welche Dateien, Module oder Release-Pfade den neuen Trigger begrenzen.
- Wer den Workflow besitzt und wie Run-Kosten/False Positives beobachtet werden.
- Ob der erste Rollout nur manuell, nur scheduled oder non-blocking erfolgt.

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
- Reaktivierungen aus `.github/no_workflows/` muessen im selben PR Guidelines und Registry aktualisieren.

## Validierung
Lokaler Standard-Check:

```powershell
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode all
```

## Stand
- Aktive Workflows im Verzeichnis `.github/workflows/`: 20
- Deaktivierte Workflows in `.github/no_workflows/`: 23
- Strategie: Lean + harte Triggergrenzen + Quarantaene fuer uebertriggernde CI
