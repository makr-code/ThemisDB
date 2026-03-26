# Write-Optimized Merge (WOM) Tree CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_storage_wom-tree-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Write-Optimized Merge (WOM) Tree**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (6 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (6 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `wom-tree-unit-tests`
**Anzeigename:** WOM Tree tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure (WOM Tree focused test target)** — `cmake -B build -G Ninja \`
- **Build focused test binary** — `cmake --build build --target test_wom_tree_focused -- -j$(nproc)`
- **Run WOM Tree unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🌲 Write-Optimized Merge (WOM) Tree – Unit Tests" >> "$GITHUB_STEP_SUMMA`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_storage_wom-tree-ci.yml)
- [Alle Workflows](../README.md)
