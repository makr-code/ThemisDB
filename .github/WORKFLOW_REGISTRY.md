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
- `.github/workflows/02-feature-modules_llm_voice-benchmark-ci.yml`
  — Optional voice benchmark CI (THEMIS_ENABLE_VOICE_ASSISTANT=ON); satisfies PERFORMANCE_EXPECTATIONS.md §1.4 Maßnahme #4 (perf audit check 4d)
- `.github/workflows/06-infrastructure_gpu_gpu-benchmark-matrix-ci.yml`
  — GPU benchmark matrix (CUDA/HIP/Vulkan); satisfies §1.4 Maßnahme #5 (perf audit check 5c)
- `.github/workflows/07-quality_nightly-benchmark-sweep.yml`
  — Nightly benchmark sweep (schedule 02:00 UTC, modules 2..35); satisfies §1.4 Maßnahme #10 (perf audit check 10a)
- `.github/workflows/09-pr-gates_workflow-boundary-guard.yml`
  — Enger PR-Gate fuer Workflow-Governance; blockiert Reaktivierungen ohne Quarantaene-Regeln, Doku-Update und harte Triggergrenzen
- `.github/workflows/copilot-ollama-router-ci.yml`
  — Scoped CI fuer das lokale VS-Code-Extension-Tooling unter `tools/copilot-ollama-router/**`
- `.github/workflows/copilot-regression-guard.yml`
  — Zielgerichteter Guard fuer Copilot/CMake-Regressionen mit klar begrenztem PR-Scope
- `.github/workflows/performance-regression-check.yml`
  — Enger Storage-Performance-Check fuer Benchmarks und Performance-relevante C++-Aenderungen

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
- Aktive Workflows im Verzeichnis `.github/workflows/`: 7
- Deaktivierte Workflows in `.github/no_workflows/`: 31
- Strategie: Lean + harte Triggergrenzen + Quarantaene fuer uebertriggernde CI
