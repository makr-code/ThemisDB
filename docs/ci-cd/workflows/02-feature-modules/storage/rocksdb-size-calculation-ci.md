# RocksDBWrapper Size Calculation CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules/storage/rocksdb-size-calculation-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **RocksDBWrapper Size Calculation**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `rocksdb-size-calculation-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `rocksdb-size-calculation-unit-tests`
**Anzeigename:** RocksDB Size Calculation tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run RocksDB Size Calculation unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📦 RocksDBWrapper Size Calculation – Unit Tests" >> "$GITHUB_STEP_SUMMAR`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules/storage/rocksdb-size-calculation-ci.yml)
- [Alle Workflows](../README.md)
