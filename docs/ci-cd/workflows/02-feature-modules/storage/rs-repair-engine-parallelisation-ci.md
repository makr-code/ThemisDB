# RS Repair Engine Parallelisation CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_storage_rs-repair-engine-parallelisation-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **RS Repair Engine Parallelisation**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (12 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (12 überwachte Pfade)

## Nebenläufigkeit

- **Gruppe:** `rs-repair-parallelisation-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `rs-repair-tests`
**Anzeigename:** RS Repair Parallelisation (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run RSRepairParallelisationFocusedTests** — `set -o pipefail`
- **Run RS Repair Parallelisation tests via unified binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_storage_rs-repair-engine-parallelisation-ci.yml)
- [Alle Workflows](../README.md)
