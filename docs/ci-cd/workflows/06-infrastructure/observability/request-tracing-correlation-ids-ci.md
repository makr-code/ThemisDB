# Request Tracing and Correlation IDs CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/06-infrastructure/observability/request-tracing-correlation-ids-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Request Tracing and Correlation IDs**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (13 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (13 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `request-tracing-tests`
**Anzeigename:** Request Tracing (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true' || needs.ci-scope-classifier.outputs.ha`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run TracingMiddlewareTests** — `set -o pipefail`
- **Run OtlpExporterTests** — `set -o pipefail`
- **Run OtelApiTracingTests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔍 Request Tracing and Correlation IDs CI" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/06-infrastructure/observability/request-tracing-correlation-ids-ci.yml)
- [Alle Workflows](../README.md)
