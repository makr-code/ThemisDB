# PR Quick Checks

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/09-pr-gates_pr-quick-checks.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **PR Quick Checks**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened)
- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`, `enterprise`, `hyperscaler`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `lint`
**Anzeigename:** lint (cppcheck static analysis)

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install cppcheck** — `sudo apt-get update -qq && sudo apt-get install -y cppcheck`
- **Run cppcheck on src/ and include/** — `set -o pipefail`
- **Upload cppcheck report** — `actions/upload-artifact@v4`

### `cmake-validate`
**Anzeigename:** cmake-validate (configure check)

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Run CMake configure (MINIMAL edition, no build)** — `set -euo pipefail`

### `audit`
**Anzeigename:** audit (security & dependency scan)

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install audit tools** — `pip install pip-audit pytest --quiet`
- **Get changed files** — `set -euo pipefail`
- **Run ThemisDB entropy secret scanner** — `set -euo pipefail`
- **Audit Python dependencies (pip-audit)** — `set -o pipefail`
- **Run error-handling audit** — `set -o pipefail`
- **Upload audit reports** — `actions/upload-artifact@v4`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/09-pr-gates_pr-quick-checks.yml)
- [Alle Workflows](../README.md)


