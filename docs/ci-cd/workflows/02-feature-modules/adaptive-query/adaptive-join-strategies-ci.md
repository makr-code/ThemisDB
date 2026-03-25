# Adaptive Join Strategies CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules/adaptive-query/adaptive-join-strategies-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Adaptive Join Strategies**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (5 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (5 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `adaptive-join-unit-tests`
**Anzeigename:** Adaptive Join tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure (adaptive join test targets)** — `./.github/actions/configure-themis`
- **Build AdaptiveJoin focused test binary** — `cmake --build build --target test_adaptive_join_strategies_focused -- -j$(nproc)`
- **Run AdaptiveJoin unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔗 Adaptive Join Strategies – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules/adaptive-query/adaptive-join-strategies-ci.yml)
- [Alle Workflows](../README.md)
