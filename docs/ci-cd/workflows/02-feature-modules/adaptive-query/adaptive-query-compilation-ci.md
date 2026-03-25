# Adaptive Query Compilation CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules/adaptive-query/adaptive-query-compilation-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Adaptive Query Compilation**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `adaptive-query-compilation-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `adaptive-query-compilation-unit-tests`
**Anzeigename:** Adaptive Query Compilation tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Configure and build (adaptive query compilation test target)** — `./.github/actions/configure-themis`
- **Run Adaptive Query Compilation unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## ⚡ Adaptive Query Compilation – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules/adaptive-query/adaptive-query-compilation-ci.yml)
- [Alle Workflows](../README.md)
