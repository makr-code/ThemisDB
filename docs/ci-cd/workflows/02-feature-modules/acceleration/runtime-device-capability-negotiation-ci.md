# Runtime Device Capability Negotiation CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules/acceleration/runtime-device-capability-negotiation-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Runtime Device Capability Negotiation**.

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
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `runtime-device-capability-tests`
**Anzeigename:** Runtime Device Capability (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run DeviceManagerFocusedTests via ctest** — `set -o pipefail`
- **Run DeviceManagerTest suite via focused binary** — `set -o pipefail`
- **Run BackendRegistryStartupFocusedTests via ctest** — `set -o pipefail`
- **Run BackendRegistryStartup suite via focused binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules/acceleration/runtime-device-capability-negotiation-ci.yml)
- [Alle Workflows](../README.md)
