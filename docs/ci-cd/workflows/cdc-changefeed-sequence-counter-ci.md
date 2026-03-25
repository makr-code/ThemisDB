# CDC Changefeed Sequence Counter CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/cdc-changefeed-sequence-counter-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **CDC Changefeed Sequence Counter**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (5 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (5 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `cdc-changefeed-sequence-counter-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/ci-scope-classifier.yml`

### `cdc-changefeed-sequence-counter-tests`
**Anzeigename:** CDC Sequence Counter (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run CDCChangefeedSequenceCounterTests** — `set -o pipefail`
- **Run sequence counter suites via unified binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/cdc-changefeed-sequence-counter-ci.yml)
- [Alle Workflows](README.md)
