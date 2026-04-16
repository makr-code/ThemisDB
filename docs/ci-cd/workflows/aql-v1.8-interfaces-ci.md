# AQL v1.8.0 Interfaces CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/aql-v1.8-interfaces-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **AQL v1.8.0 Interfaces**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (5 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (5 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `aql-v1.8-interfaces-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `aql-v1-8-interfaces`
**Anzeigename:** AQL v1.8.0 Interfaces (${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure (LLM=OFF, no GPU required)** — `./.github/actions/configure-themis`
- **Run AQLMultimodalTests** — `set -o pipefail`
- **Run AQLAsyncBackendTests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/aql-v1.8-interfaces-ci.yml)
- [Alle Workflows](README.md)


