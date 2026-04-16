# ThemisDB Workflow Registry (Lean Core)

## Zielbild
Dieses Repository nutzt bewusst ein schlankes, release-orientiertes CI/CD-Set.
Alle nicht zwingenden Modul-/Spezial-Workflows wurden entfernt, um Wartung,
Signalqualität und Release-Stabilitaet zu verbessern.

## Leitprinzipien
- Keep it lean: nur Workflows mit direktem Beitrag zu Branch-Gates, Editions-Build und Release.
- Modularisierung: wiederverwendbare Workflows statt duplizierter Build-Logik.
- Klare Verantwortlichkeit je Lane: `develop`, `main`, `enterprise`, `hyperscaler`.
- Keine Schatten-CI: neue Workflows nur mit begruendeter Notwendigkeit und Registry-Update.

## Aktiver Workflow-Kern

### Core
- `.github/workflows/01-core_ci.yml`

### Editions (modular)
- `.github/workflows/03-editions_ci.yml`

### Release
- `.github/workflows/04-release_bootstrap-release-branches.yml`
- `.github/workflows/04-release_build-binaries.yml`
- `.github/workflows/04-release_publish-community.yml`
- `.github/workflows/04-release_publish-private.yml`

### PR Gates
- `.github/workflows/09-pr-gates_quick-checks.yml`
- `.github/workflows/09-pr-gates_path-policy.yml`

## Governance fuer neue Workflows
Neue Workflow-Dateien sind nur erlaubt, wenn mindestens einer der Punkte zutrifft:
- Erforderlich fuer ein neues Release-Artefakt oder verpflichtendes Compliance-Gate.
- Nicht sinnvoll als Job in bestehendem Core-/Edition-/Gate-Workflow integrierbar.
- Enthalten klare Owner, Trigger-Grenzen (`paths`, `branches`) und Wartungsplan.

## Validierung
Lokaler Standard-Check:

```powershell
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode all
```

## Stand
- Gesamtzahl Workflows: 8
- Strategie: Lean + modular + release-zentriert
