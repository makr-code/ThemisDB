# Publish · Enterprise Edition

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/04-release_publish-enterprise.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Publish · Enterprise Edition**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `tag_name` | Tag to publish (e.g. enterprise-v2.1.0) | ✅ | — |
| `dry_run` | Dry-run: build but do NOT publish | — | `true` |

## Jobs

### `build`
**Anzeigename:** Build Enterprise + Military

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Resolve tag name** — `if [[ "${{ github.event_name }}" == "workflow_dispatch" ]]; then`
- **Checkout at tag** — `actions/checkout@v4`
- **Initialize llama.cpp submodule** — `git submodule update --init --depth=1 llama.cpp`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Build Enterprise edition** — `./.github/actions/configure-themis`
- **Run Enterprise smoke tests** — `set -o pipefail`
- **Build Military edition** — `./.github/actions/configure-themis`
- **Run Military smoke tests** — `set -o pipefail`
- **Package Enterprise artefacts** — `set -euo pipefail`
- **Upload build artefacts** — `actions/upload-artifact@v4`
- **Write build summary** — `echo "## 🏢 Enterprise Build — ${{ steps.resolve-tag.outputs.tag }}" >> "$GITHUB_`

### `publish`
**Anzeigename:** Publish Enterprise artefacts

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `build`

**Schritte:**

- **Resolve version** — `if [[ "${{ github.event_name }}" == "workflow_dispatch" ]]; then`
- **Download build artefacts** — `actions/download-artifact@v4`
- **Log in to Enterprise registry** — `docker/login-action@v3`
- **Sign artefacts with GPG** — `set -euo pipefail`
- **Publish artefacts** — `DRY_RUN="${{ steps.version.outputs.dry_run }}"`
- **Write publish summary** — `DRY_RUN="${{ steps.version.outputs.dry_run }}"`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/04-release_publish-enterprise.yml)
- [Alle Workflows](../README.md)


