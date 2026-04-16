# Plugin Manager & Lifecycle CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/plugin-manager-lifecycle-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Plugin Manager & Lifecycle**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (27 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (27 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `plugin-lifecycle-tests`
**Anzeigename:** Plugin Lifecycle Tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run PluginManagerFocusedTests via ctest** — `set -o pipefail`
- **Run PluginManager tests via focused binary** — `set -o pipefail`
- **Run PluginLifecycleFocusedTests via ctest** — `set -o pipefail`
- **Run PluginLifecycle tests via focused binary** — `set -o pipefail`
- **Run GenericPluginRegistryFocusedTests via ctest** — `set -o pipefail`
- **Run GenericPluginRegistry tests via focused binary** — `set -o pipefail`
- **Run PluginHealthMonitorFocusedTests via ctest** — `set -o pipefail`
- **Run PluginHealthMonitor tests via focused binary** — `set -o pipefail`
- **Run PluginHotPlugFocusedTests via ctest** — `set -o pipefail`
- **Run PluginHotPlug tests via focused binary** — `set -o pipefail`
- **Run PluginHotReloadEnhancedFocusedTests via ctest** — `set -o pipefail`
- **Run PluginHotReloadEnhanced tests via focused binary** — `set -o pipefail`
- **Run PluginMetricsFocusedTests via ctest** — `set -o pipefail`
- **Run PluginMetrics tests via focused binary** — `set -o pipefail`
- **Run PluginMetricsIntegrationFocusedTests via ctest** — `set -o pipefail`
- **Run PluginMetricsIntegration tests via focused binary** — `set -o pipefail`
- **Run PluginDependencyResolverFocusedTests via ctest** — `set -o pipefail`
- *(+ 7 weitere Schritte)*

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/plugin-manager-lifecycle-ci.yml)
- [Alle Workflows](README.md)


