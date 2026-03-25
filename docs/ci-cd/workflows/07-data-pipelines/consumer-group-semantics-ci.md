# CDC Consumer Group Semantics CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/07-data-pipelines/consumer-group-semantics-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **CDC Consumer Group Semantics**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (13 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (13 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `consumer-group-semantics-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `consumer-group-semantics-tests`
**Anzeigename:** Consumer Group Semantics (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run ConsumerGroupFocusedTests** — `set -o pipefail`
- **Run CdcWsHandlerFocusedTests** — `set -o pipefail`
- **Run ConsumerGroup and CdcWsHandler suites via unified binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/07-data-pipelines/consumer-group-semantics-ci.yml)
- [Alle Workflows](../README.md)
