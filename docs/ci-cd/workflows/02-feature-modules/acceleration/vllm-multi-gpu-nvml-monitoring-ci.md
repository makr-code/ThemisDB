# VLLMResourceManager Multi-GPU NVML Monitoring CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_acceleration_vllm-multi-gpu-nvml-monitoring-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **VLLMResourceManager Multi-GPU NVML Monitoring**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (5 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (5 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `vllm-multi-gpu-nvml-tests`
**Anzeigename:** VLLMResourceManager Multi-GPU NVML (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run VLLMResourceStatsFocusedTests via ctest** — `set -o pipefail`
- **Run VLLMResourceStatsTest suite via focused binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_acceleration_vllm-multi-gpu-nvml-monitoring-ci.yml)
- [Alle Workflows](../README.md)
