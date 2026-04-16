# S3-Compatible Object Storage Source Connector CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/07-data-pipelines_s3-compatible-object-storage-connector-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **S3-Compatible Object Storage Source Connector**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (9 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (9 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `s3-compatible-object-storage-connector-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `s3-connector-tests`
**Anzeigename:** S3-Compatible Connector (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run S3ConnectorFocusedTests** — `set -o pipefail`
- **Run S3ConnectorTest suite via unified binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## ☁️ S3-Compatible Object Storage Source Connector CI – v1.7.0" >> "$GITH`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/07-data-pipelines_s3-compatible-object-storage-connector-ci.yml)
- [Alle Workflows](../README.md)


