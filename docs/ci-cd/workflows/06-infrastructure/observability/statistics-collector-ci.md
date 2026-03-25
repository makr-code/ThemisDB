# Statistics Collector CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/06-infrastructure/observability/statistics-collector-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Statistics Collector**.

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

### `statistics-collector-unit-tests`
**Anzeigename:** Statistics collector tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure (statistics collector test targets)** — `cmake -B build -G Ninja \`
- **Build StatisticsCollector focused test binary** — `cmake --build build --target test_statistics_collector_focused -- -j$(nproc)`
- **Build StatisticsCollector auto-refresh focused test binary** — `cmake --build build --target test_statistics_auto_refresh_focused -- -j$(nproc)`
- **Run StatisticsCollector unit tests** — `cd build`
- **Run StatisticsCollector auto-refresh unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📊 Statistics Collector – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/06-infrastructure/observability/statistics-collector-ci.yml)
- [Alle Workflows](../README.md)
