# Parquet Exporter Tests

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_parquet-exporter-tests.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Parquet Exporter Tests**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (11 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (11 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `parquet-exporter-unit-tests`
**Anzeigename:** Parquet exporter unit tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 4 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install Apache Arrow and Parquet libraries (Arrow path only)** — `sudo apt-get update -y`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure (parquet exporter tests only)** — `cmake -B build -G Ninja \`
- **Build test binary** — `cmake --build build --target themis_tests -- -j$(nproc)`
- **Run parquet exporter unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📦 Parquet Exporter Tests – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/02-feature-modules_parquet-exporter-tests.yml)
- [Alle Workflows](../README.md)
