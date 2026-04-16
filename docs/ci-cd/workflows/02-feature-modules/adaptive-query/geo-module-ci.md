# Geo Module CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/02-feature-modules_adaptive-query_geo-module-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Geo Module**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (9 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (9 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `geo-module-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `geo-module-unit-tests`
**Anzeigename:** Geo Module tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run GeoRtreeFocusedTests** — `set -o pipefail`
- **Run GeoEwkbFocusedTests** — `set -o pipefail`
- **Run GeoPrecisionModeFocusedTests** — `set -o pipefail`
- **Run GeoStBufferFocusedTests** — `set -o pipefail`
- **Run GeoStUnionDifferenceFocusedTests** — `set -o pipefail`
- **Run Geo3dFunctionsFocusedTests** — `set -o pipefail`
- **Run GeoWgs84SphericalFocusedTests** — `set -o pipefail`
- **Run GeoSpatialJoinFocusedTests** — `set -o pipefail`
- **Run RtreeCpuIntegrationFocusedTests** — `set -o pipefail`
- **Run SpatialIndexFocusedTests** — `set -o pipefail`
- **Run GeoClusteringFocusedTests** — `set -o pipefail`
- **Run GeoRasterFocusedTests** — `set -o pipefail`
- **Run GeoTileServerFocusedTests** — `set -o pipefail`
- **Run TemporalSpatialQueryFocusedTests** — `set -o pipefail`
- **Run GeoDeviceDetectorFocusedTests** — `set -o pipefail`
- **Run GpuBackendProductionFocusedTests** — `set -o pipefail`
- **Run GpuKernelDispatcherFocusedTests** — `set -o pipefail`
- *(+ 3 weitere Schritte)*

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_adaptive-query_geo-module-ci.yml)
- [Alle Workflows](../README.md)


