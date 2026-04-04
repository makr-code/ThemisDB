# CUDA HNSW Large-k CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_acceleration_cuda-hnsw-large-k-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **CUDA HNSW Large-k**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (8 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (8 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `cuda-hnsw-large-k-tests`
**Anzeigename:** CUDA HNSW Large-k (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run CudaHnswLargeKFocusedTests via ctest** — `set -o pipefail`
- **Run CUDA HNSW large-k tests via focused binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_acceleration_cuda-hnsw-large-k-ci.yml)
- [Alle Workflows](../README.md)
