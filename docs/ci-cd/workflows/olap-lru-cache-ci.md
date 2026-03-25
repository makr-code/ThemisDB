# OLAP LRU Cache CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/olap-lru-cache-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **OLAP LRU Cache**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (6 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (6 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/ci-scope-classifier.yml`

### `olap-lru-cache-tests`
**Anzeigename:** OLAP LRU cache tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run OLAPLRUCacheFocusedTests via ctest** — `set -o pipefail`
- **Run tests via focused binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🚀 OLAP LRU Cache CI" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/olap-lru-cache-ci.yml)
- [Alle Workflows](README.md)
