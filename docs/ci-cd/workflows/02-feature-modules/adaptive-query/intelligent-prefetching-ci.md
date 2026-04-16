# Intelligent Prefetching CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/02-feature-modules_adaptive-query_intelligent-prefetching-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Intelligent Prefetching**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `intelligent-prefetching-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `intelligent-prefetching-unit-tests`
**Anzeigename:** Intelligent Prefetching tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Configure and build (intelligent prefetching test target)** — `./.github/actions/configure-themis`
- **Run Intelligent Prefetching unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🧠 Intelligent Prefetching System – Unit Tests" >> "$GITHUB_STEP_SUMMARY`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_adaptive-query_intelligent-prefetching-ci.yml)
- [Alle Workflows](../README.md)


