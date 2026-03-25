# Hardware-Accelerated Query Execution CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/hardware-accelerated-query-execution-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Hardware-Accelerated Query Execution**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `hardware-accelerated-query-execution-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/ci-scope-classifier.yml`

### `hardware-accelerator-unit-tests`
**Anzeigename:** Hardware Accelerator tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Configure and build (hardware accelerator test target)** — `./.github/actions/configure-themis`
- **Run Hardware Accelerator unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## ⚡ Hardware-Accelerated Query Execution – Unit Tests" >> "$GITHUB_STEP_S`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/hardware-accelerated-query-execution-ci.yml)
- [Alle Workflows](README.md)
