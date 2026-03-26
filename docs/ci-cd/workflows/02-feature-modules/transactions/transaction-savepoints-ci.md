# Transaction Savepoints CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_transactions_transaction-savepoints-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Transaction Savepoints**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (8 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (8 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `transaction-savepoints-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `transaction-savepoints-unit-tests`
**Anzeigename:** Transaction Savepoints tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build (savepoint test target)** — `./.github/actions/configure-themis`
- **Run Transaction Savepoint unit tests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 💾 Transaction Savepoints – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_transactions_transaction-savepoints-ci.yml)
- [Alle Workflows](../README.md)
