# Docker Image CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/docker-image.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Docker Image**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `develop`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Branches: `develop`)

## Jobs

### `build`
**Läuft auf:** `ubuntu-latest`

**Schritte:**

- ***(ohne Name)*** — `actions/checkout@v4`
- **Create placeholder for optional proprietary directory** — `mkdir -p internal`
- **Build the Docker image** — `docker build . --file Dockerfile --tag "my-image-name:$(date +%s)"`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/docker-image.yml)
- [Alle Workflows](README.md)


