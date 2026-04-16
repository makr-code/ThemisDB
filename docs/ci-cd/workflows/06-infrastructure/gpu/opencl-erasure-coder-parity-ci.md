# OpenCL ErasureCoder Parity CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/06-infrastructure_gpu_opencl-erasure-coder-parity-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **OpenCL ErasureCoder Parity**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `opencl-erasure-coder-parity-tests`
**Anzeigename:** OpenCL ErasureCoder parity tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run OpenCL ErasureCoder parity tests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔷 OpenCL Erasure Coder Parity – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

### `opencl-erasure-coder-parity-gate`
**Anzeigename:** OpenCL ErasureCoder Parity Gate

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `opencl-erasure-coder-parity-tests`
**Bedingung:** `always()`

**Schritte:**

- **Check test status** — `result="${{ needs.opencl-erasure-coder-parity-tests.result }}"`
- **Write job summary** — `result="${{ needs.opencl-erasure-coder-parity-tests.result }}"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/06-infrastructure_gpu_opencl-erasure-coder-parity-ci.yml)
- [Alle Workflows](../README.md)


