# ExporterFactory Stub Replacement CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_exporterfactory-stub-replacement-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **ExporterFactory Stub Replacement**.

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
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `arrow-export-factory-tests`
**Anzeigename:** ExporterFactory tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 4 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install Apache Arrow and Parquet libraries (Arrow path only)** — `sudo apt-get update -y`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run ArrowExportFocusedTests via ctest** — `set -o pipefail`
- **Run tests via focused binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📦 ExporterFactory Stub Replacement – Unit Tests" >> "$GITHUB_STEP_SUMMA`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/02-feature-modules_exporterfactory-stub-replacement-ci.yml)
- [Alle Workflows](../README.md)
