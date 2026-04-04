# Async Retry RemoteRegistryClient CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_resilience_async-retry-remote-registry-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Async Retry RemoteRegistryClient**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)

## Nebenläufigkeit

- **Gruppe:** `async-retry-remote-registry-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `async-retry-tests`
**Anzeigename:** Async Retry RemoteRegistryClient (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run RemoteRegistryClientFocusedTests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_resilience_async-retry-remote-registry-ci.yml)
- [Alle Workflows](../README.md)
