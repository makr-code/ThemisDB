# Geo Point Clustering (DBSCAN / k-means) CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/geo-point-clustering-dbscan-kmeans-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Geo Point Clustering (DBSCAN / k-means)**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `geo-point-clustering-dbscan-kmeans-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/ci-scope-classifier.yml`

### `geo-clustering-unit-tests`
**Anzeigename:** Geo Clustering tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run GeoClusteringFocusedTests** — `set -o pipefail`
- **Run clustering tests via unified binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/geo-point-clustering-dbscan-kmeans-ci.yml)
- [Alle Workflows](README.md)
