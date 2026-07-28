# Chimera Error Recovery & Retry CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_chimera_chimera-error-recovery-retry-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Chimera Error Recovery & Retry**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (6 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (6 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `chimera-error-recovery-retry-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `chimera-retry-tests`
**Anzeigename:** Chimera Retry Policy (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run ChimeraRetryPolicyTests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_chimera_chimera-error-recovery-retry-ci.yml)
- [Alle Workflows](../README.md)
