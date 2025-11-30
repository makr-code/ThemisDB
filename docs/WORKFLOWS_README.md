# GitHub Workflows – Leitfaden (kostenarm & sicher)

Aktuell sind alle GitHub Actions Workflows entfernt und in `.gitignore` blockiert (`.github/workflows/`). Dieses Dokument beschreibt, wie wir sie später wieder aktivieren – kostenarm (Dry-Run) und sicher.

## Ziele
- Minimale Kosten: kurze Laufzeit, keine Pushes/Publishes
- Sichere Triggers: nur `pull_request` und `workflow_dispatch`
- Reproduzierbar: gleiche Toolchain wie lokale Builds (WSL/Docker)

## Empfehlungen
- Runner-Zeit reduzieren:
  - Keine Multi-Arch Builds (nur `linux/amd64`)
  - Keine `cache-to` Schreibvorgänge (optional nur `cache-from`)
  - Niedrige Timeouts (z. B. 15–20 min)
  - Kleinere Matrix (keine parallelen Builds)
- Dry-Run Grundregeln:
  - Nie login (`docker/login-action`) verwenden
  - `build-push-action` mit `push: false`
  - Artefakte nur bei Bedarf erzeugen

## Minimalvorlage (Dry-Run)

```yaml
name: Dry-Run Build
on:
  pull_request:
    branches: [ main ]
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3
      - name: Build (no push)
        uses: docker/build-push-action@v5
        with:
          context: .
          file: ./Dockerfile
          push: false
          platforms: linux/amd64
          cache-from: type=gha
          # cache-to intentionally omitted to reduce storage
```

## Aktivierungsschritte
1. `.gitignore` Zeile für `.github/workflows/` temporär entfernen
2. Workflow-Datei unter `.github/workflows/` anlegen (siehe Vorlage)
3. Nur PR/Dispatch triggern; beobachten Laufzeit und Logs

## Kosten-Hinweise
- Jeder Lauf verbraucht Minuten des Plans (auch Dry-Run)
- Öffentliches Repo: Minutenkontingente kostenfrei bis Plan-Limit
- Privates Repo: Minuten werden gemäß Plan abgerechnet

## Rückfall / Deaktivierung
- Entferne Workflows oder füge `.github/workflows/` zurück in `.gitignore`

## Kontakt
- Bei Bedarf kann ich die Vorlagen an deine Budget- und Sicherheitsvorgaben anpassen.
