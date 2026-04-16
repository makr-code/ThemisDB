# Content Embedding Pipeline CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/07-data-pipelines_content-embedding-pipeline-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Content Embedding Pipeline**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (14 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (14 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `content-embedding-pipeline-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `content-embedding-pipeline-tests`
**Anzeigename:** Content Embedding Pipeline (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run ContentEmbeddingPipelineFocusedTests** — `set -o pipefail`
- **Run embedding pipeline tests via unified binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔢 Content Embedding Pipeline – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/07-data-pipelines_content-embedding-pipeline-ci.yml)
- [Alle Workflows](../README.md)


