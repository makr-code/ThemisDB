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
- `.github/workflows/00-shared_changelog-update.yml`
  — Reusable changelog writer fuer Security/Documentation/Infrastructure-Eintraege in `CHANGELOG.md`
- `.github/workflows/00-shared_changelog-backfill.yml`
  — Manueller historischer Changelog-Backfill aus Milestones/PRs ohne Build-Last
- `.github/workflows/02-feature-modules_llm_voice-benchmark-ci.yml`
  — Optional voice benchmark CI (THEMIS_ENABLE_VOICE_ASSISTANT=ON); satisfies PERFORMANCE_EXPECTATIONS.md §1.4 Maßnahme #4 (perf audit check 4d)
- `.github/workflows/06-infrastructure_gpu_gpu-benchmark-matrix-ci.yml`
  — GPU benchmark matrix (CUDA/HIP/Vulkan); satisfies §1.4 Maßnahme #5 (perf audit check 5c)
- `.github/workflows/07-quality_nightly-benchmark-sweep.yml`
  — Nightly benchmark sweep (schedule 02:00 UTC, modules 2..35); satisfies §1.4 Maßnahme #10 (perf audit check 10a)
- `.github/workflows/08-maintenance_root-docs-hygiene.yml`
  — Root-Dokumentationshygiene fuer Top-Level-Dateien (Push/Schedule/Manual)
- `.github/workflows/08-maintenance_src-include-docs-align.yml`
  — Modulabgleich `src/include` ↔ `docs/de` fuer Doku-Abdeckung
- `.github/workflows/08-maintenance_docs-orphan-check.yml`
  — Orphan/Broken-Reference-Pruefung fuer `docs/de` und `docs/en`
- `.github/workflows/08-maintenance_code-maturity.yml`
  — Code-Maturity-Analyse via `code_maturity_header_writer.py` (delegiert intern auf `analyze_code_maturity.py`); check-only per Default (kein Header-Rewrite), Rewrite-Modus nur via `workflow_dispatch` mit `update_headers=true`; enger PR-Trigger auf Script und Workflow-Datei
- `.github/workflows/08-maintenance_epic-5518-retarget.yml`
  — EPIC 5518 PR Retargeting; automatische Umleitung von PRs mit Marker "Part of makr-code/ThemisDB#5518" oder Label `epic/5518` zum Ziel-Branch `epic/hybrid-boundaries-5518` (policy #5545); dokumentiert in `docs/ci-cd/workflows/08-maintenance/epic-5518-retarget.md`
- `.github/workflows/09-pr-gates_workflow-boundary-guard.yml`
  — Enger PR-Gate fuer Workflow-Governance; blockiert Reaktivierungen ohne Quarantaene-Regeln, Doku-Update und harte Triggergrenzen
- `.github/workflows/09-pr-gates_scanner-delta-report.yml`
  — Enger PR-Gate fuer Scanner-Delta-Reporting (Baseline vs Current) mit Artefakt-Upload fuer reproduzierbare Triage
- `.github/workflows/09-pr-gates_high-exception-record.yml`
  — Enger PR-Gate fuer Vollstaendigkeit akzeptierter High-Finding-Exceptions im PR-Text anhand des High-Exception-Records
- `.github/workflows/09-pr-gates_private-plugin-boundary.yml`
  — Enger PR-Gate fuer private-plugin Boundary-, Governance- und Packaging-Aenderungen
- `.github/workflows/08-quality_doxygen-coverage-gate.yml`
  — Doxygen XML Coverage Gate fuer PRs (Threshold zentral in `.github/ci-scope-config.yaml` unter `quality_gates.docs_coverage_threshold`, Default 90%)
- `.github/workflows/license-compliance.yml`
  — Compliance-Gate fuer vcpkg-Lizenz-Whitelist mit Release-Artefakten `license-summary.md` und `vcpkg-license-sbom.json`
- `.github/workflows/security-dast-ci.yml`
  — OWASP-ZAP-basierter DAST-Sicherheitscheck fuer API-Pfade
- `.github/workflows/sbom-ci.yml`
  — Supply-Chain-SBOM-Erzeugung, Verifikation und Signierung fuer Releases
- `.github/workflows/soc2-evidence-ci.yml`
  — Geplanter SOC-2-Evidence-Export mit Artefaktablage fuer Audit-Nachweise
- `.github/workflows/copilot-ollama-router-ci.yml`
  — Scoped CI fuer das lokale VS-Code-Extension-Tooling unter `tools/copilot-ollama-router/**`
- `.github/workflows/copilot-regression-guard.yml`
  — Zielgerichteter Guard fuer Copilot/CMake-Regressionen mit klar begrenztem PR-Scope
- `.github/workflows/performance-regression-check.yml`
  — Enger Storage-Performance-Check fuer Benchmarks und Performance-relevante C++-Aenderungen
- `.github/workflows/08-quality_clang-tidy-analysis.yml`
  — Statische Analyse via clang-tidy (SARIF → GitHub Code Scanning) und clang-format-Stil-Pruefung; `workflow_dispatch`-only (Phase 4.1 TODO_KI_WORKFLOW_IMPROVEMENTS.md); erzeugt Artefakt `clang-tidy-results` + SARIF-Upload; optionaler Hard-Gate via `fail_on_warnings`/`fail_on_format_violations`
- `.github/workflows/09-pr-gates_submodule-commit-pins.yml`
  — Enger PR-Gate fuer Wave-1 private Plugin-Submodule: blockiert `.gitmodules`-Aenderungen ohne 40-Hex-SHA Commit-Pin fuer alle sieben Wave-1-Submodul-Pfade; Trigger begrenzt auf `.gitmodules` und eigene Workflow-Datei; Fehlermeldung mit Reparaturanleitung und Policy-Verweis (`docs/plugins/PLUGIN_ABI_POLICY.md`)
- `.github/workflows/09-pr-gates_community-pipeline-policy.yml`
  — Enger PR-Gate fuer Community/Minimal-Branches: scannt geaenderte CI/Build-Dateien auf verbotene private Credentials (Secret-Referenzen, hart kodierte Submodule-Tokens, `WITH_PRIVATE_PLUGINS=ON`) und prueft, dass kein Community-Workflow private Submodule auscheckt; Policy: `docs/plugins/COMMUNITY_BUILD_VALIDATION_GUIDE.md §No Private Content`

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
- Aktive Workflows im Verzeichnis `.github/workflows/`: 24
- Deaktivierte Workflows in `.github/no_workflows/`: 23
- Strategie: Lean + harte Triggergrenzen + Quarantaene fuer uebertriggernde CI
