# Publish · Hyperscaler Edition

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/04-release_publish-hyperscaler.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Publish · Hyperscaler Edition**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `tag_name` | Tag to publish (e.g. hyperscaler-v3.0.0) | ✅ | — |
| `dry_run` | Dry-run: build but do NOT publish | — | `true` |

## Jobs

### `build`
**Anzeigename:** Build Hyperscaler Edition

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Resolve tag name** — `if [[ "${{ github.event_name }}" == "workflow_dispatch" ]]; then`
- **Checkout at tag** — `actions/checkout@v4`
- **Initialize llama.cpp submodule** — `git submodule update --init --depth=1 llama.cpp`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Build Hyperscaler edition** — `./.github/actions/configure-themis`
- **Run Hyperscaler smoke tests** — `set -o pipefail`
- **Validate Helm charts** — `set -euo pipefail`
- **Build Docker image (Hyperscaler)** — `docker/build-push-action@v6`
- **Package Hyperscaler artefacts** — `set -euo pipefail`
- **Upload build artefacts** — `actions/upload-artifact@v4`
- **Write build summary** — `echo "## 🌐 Hyperscaler Build — ${{ steps.resolve-tag.outputs.tag }}" >> "$GITHUB`

### `publish-docker`
**Anzeigename:** Publish Hyperscaler Docker image

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `build`

**Schritte:**

- **Resolve version** — `if [[ "${{ github.event_name }}" == "workflow_dispatch" ]]; then`
- **Checkout at tag** — `actions/checkout@v4`
- **Set up QEMU** — `docker/setup-qemu-action@v3`
- **Set up Docker Buildx** — `docker/setup-buildx-action@v3`
- **Log in to Hyperscaler registry** — `docker/login-action@v3`
- **Build and push Hyperscaler Docker image** — `docker/build-push-action@v6`
- **Write Docker publish summary** — `DRY_RUN="${{ steps.version.outputs.dry_run }}"`

### `publish-helm`
**Anzeigename:** Publish Helm charts

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `build`

**Schritte:**

- **Resolve version** — `if [[ "${{ github.event_name }}" == "workflow_dispatch" ]]; then`
- **Checkout at tag** — `actions/checkout@v4`
- **Install Helm** — `azure/setup-helm@v4`
- **Package Helm charts** — `set -euo pipefail`
- **Log in to Helm repository** — `helm registry login "${{ secrets.HYPERSCALER_HELM_REPO_URL }}" \`
- **Push Helm charts** — `DRY_RUN="${{ steps.version.outputs.dry_run }}"`
- **Write Helm publish summary** — `DRY_RUN="${{ steps.version.outputs.dry_run }}"`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/04-release_publish-hyperscaler.yml)
- [Alle Workflows](../README.md)


