# Edition Build CI (reusable)

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

♻️ **Reusable Workflow**

> **Workflow-Datei (historisch):** .github/workflows/03-editions_edition-build-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

Wiederverwendbarer Hilfs-Workflow: **Edition Build (reusable)**.

## Auslöser (Triggers)

- **`workflow_call`** — Aufrufbar als wiederverwendbarer Workflow (reusable workflow)

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `edition` | ThemisDB edition to build (MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER | MILITARY) | ✅ | — |
| `build-type` | CMake build type (Debug | Release) | — | `Debug` |

## Jobs

### `edition-build`
**Anzeigename:** ${{ inputs.edition }} · Linux · ${{ inputs.build-type }}

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Initialize llama.cpp submodule (required for LLM-enabled editions)** — `git submodule update --init --depth=1 llama.cpp`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build (${{ inputs.edition }} edition)** — `./.github/actions/configure-themis`
- **Show ccache statistics** — `ccache --show-stats || true`
- **Run build-info tests** — `set -o pipefail`
- **Run module loader tests** — `set -o pipefail`
- **Run wire protocol V2 tests** — `set -o pipefail`
- **Run concerns context tests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `# Edition → LLM/GPU status must stay in sync with the case statement`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/03-editions_edition-build-ci.yml)
- [Alle Workflows](../README.md)


