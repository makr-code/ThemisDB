# Multi-Tenant Update Scheduling CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_multi-tenant-update-scheduling-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Multi-Tenant Update Scheduling**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)

## Nebenläufigkeit

- **Gruppe:** `multi-tenant-update-scheduling-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `multi-tenant-update-scheduling-tests`
**Anzeigename:** Multi-Tenant Update Scheduling (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run MultiTenantUpdateSchedulingFocusedTests** — `set -o pipefail`
- **Run MultiTenantUpdateScheduling tests via unified binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/02-feature-modules_multi-tenant-update-scheduling-ci.yml)
- [Alle Workflows](../README.md)
