# Publish Docker image to Docker Hub (on GitHub Release)

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (aktuell aktiv):** `.github/workflows/04-release_publish-community.yml`
> **Workflow-Datei (historisch):** `.github/workflows/04-release_dockerhub-publish-on-release.yml`
> **Aktueller Stand:** `.github/WORKFLOW_REGISTRY.md`

## Docker Hub Registry

- **Namespace / Repository:** `themisdb/themisdb`
- **Docker Hub URL:** <https://hub.docker.com/r/themisdb/themisdb>
- **Veröffentlichte Tags:** `latest`, `<semver>` (z. B. `1.8.1-rc1`)
- **Plattformen:** `linux/amd64`, `linux/arm64`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Publish Docker image to Docker Hub (on GitHub Release)**.

## Auslöser (Triggers)

- **`release`** — Automatisch beim Veröffentlichen eines GitHub Releases
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

Die folgende Tabelle beschreibt den **aktiven** Workflow
`.github/workflows/04-release_publish-community.yml`.

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `tag_name` | Release-Tag fuer `workflow_dispatch` (z. B. `v1.8.1-rc1`) | ✅ | — |
| `dry_run` | Build ausfuehren, aber nicht pushen (`true` = kein Push) | ✅ | `false` |

> Hinweis: `build_target` war ein Input im historischen Legacy-Workflow,
> ist im aktiven Community-Publish-Workflow jedoch nicht mehr vorhanden.

## Jobs

### `dockerhub`
**Anzeigename:** Build & push multi-arch image

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Resolve tag name** — `set -euo pipefail`
- **Checkout** — `actions/checkout@v4`
- **Compute Docker tags** — `set -euo pipefail`
- **Set up QEMU (for multi-arch)** — `docker/setup-qemu-action@v3`
- **Set up Docker Buildx** — `docker/setup-buildx-action@v3`
- **Log in to Docker Hub** — `docker/login-action@v3`
- **Build and push (multi-arch)** — `docker/build-push-action@v6`
- **Job summary** — `DRY_RUN="${{ github.event.inputs.dry_run }}"`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Aktiver Workflow](../../../../.github/workflows/04-release_publish-community.yml) — `04-release_publish-community.yml`
- [Historische Workflow-Datei](../../../../.github/workflows/04-release_dockerhub-publish-on-release.yml)
- [Alle Workflows](../README.md)

