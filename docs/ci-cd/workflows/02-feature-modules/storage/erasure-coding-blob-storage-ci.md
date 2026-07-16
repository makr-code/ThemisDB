# Erasure Coding for Blob Storage CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/02-feature-modules_storage_erasure-coding-blob-storage-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Erasure Coding for Blob Storage**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (10 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (10 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `erasure-coding-tests`
**Anzeigename:** Erasure Coding tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run ErasureCodingFocusedTests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🗜️ Erasure Coding for Blob Storage – Unit Tests" >> "$GITHUB_STEP_SUMMA`

### `erasure-coding-gate`
**Anzeigename:** Erasure Coding Gate

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `erasure-coding-tests`
**Bedingung:** `always()`

**Schritte:**

- **Check test status** — `result="${{ needs.erasure-coding-tests.result }}"`
- **Write job summary** — `result="${{ needs.erasure-coding-tests.result }}"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_storage_erasure-coding-blob-storage-ci.yml)
- [Alle Workflows](../README.md)


