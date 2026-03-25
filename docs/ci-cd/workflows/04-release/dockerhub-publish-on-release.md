# Publish Docker image to Docker Hub (on GitHub Release)

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/04-release/dockerhub-publish-on-release.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Publish Docker image to Docker Hub (on GitHub Release)**.

## Auslöser (Triggers)

- **`release`** — Automatisch beim Veröffentlichen eines GitHub Releases
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `tag_name` | Release tag to build and push (e.g. v1.4.0 or v1.4.0-rc1) | ✅ | — |
| `build_target` | Docker build target.
runtime = lean production image (default).
debug   = includes gdb/valgrind/strace/source; tag gets -debug suffix; no :latest.
 | — | `runtime` |
| `dry_run` | Dry-run: build the image but do NOT push to Docker Hub | — | `false` |

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

- [Workflow-Datei](../../.github/workflows/04-release/dockerhub-publish-on-release.yml)
- [Alle Workflows](../README.md)
