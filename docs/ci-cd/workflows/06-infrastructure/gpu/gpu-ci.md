# GPU Module CI Gate

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/06-infrastructure_gpu_gpu-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **GPU Module Gate**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (12 überwachte Pfade)
- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (12 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `gpu-module-cpu-fallback`
**Anzeigename:** GPU Module (CPU Fallback, ${{ matrix.edition }})

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_gpu_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install mimalloc** — `sudo apt-get update && sudo apt-get install -y libmimalloc-dev`
- **Install build dependencies** — `sudo apt-get install -y --no-install-recommends \`
- **Configure CMake (${{ matrix.edition }})** — `cmake -S . -B build-${{ matrix.edition }} \`
- **Build GPU module sources** — `# GPU sources are compiled into themis_core / themis_tests.`
- **Run GPU unit tests** — `cd build-${{ matrix.edition }}`
- **Verify CPU-fallback compilation** — `# Ensure the module compiles even when GPU hardware is absent.`
- **Check GPU headers are self-contained** — `for header in include/themis/gpu/*.h; do`
- **Write job summary** — `echo "## 🖥️ GPU CI – Module CPU Fallback (${{ matrix.edition }})" >> "$GITHUB_ST`

### `gpu-static-analysis`
**Anzeigename:** GPU Static Analysis (clang-tidy)

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_gpu_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install clang-tidy** — `sudo apt-get update -qq`
- **Run clang-tidy on GPU sources** — `TIDY_ERRORS=0`
- **Write job summary** — `echo "## 🔍 GPU CI – Static Analysis (clang-tidy)" >> "$GITHUB_STEP_SUMMARY"`

### `gpu-device-loss-simulation`
**Anzeigename:** GPU Device Loss Simulation

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_gpu_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install dependencies** — `sudo apt-get update -qq`
- **Verify device-loss / safe-fail code paths exist** — `# Confirm circuit breaker and fallback logic is present.`
- **Confirm chaos/fault test files exist** — `CHAOS_TESTS=$(find tests/ \( -name "test_gpu_*.cpp" -o -name "test_geo_gpu_backe`
- **Write job summary** — `echo "## ⚠️ GPU CI – Device Loss Simulation" >> "$GITHUB_STEP_SUMMARY"`

### `gpu-docs-gate`
**Anzeigename:** GPU Documentation Gate

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_gpu_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Verify GPU documentation exists** — `for doc in docs/gpu_roadmap.md docs/gpu_runbooks.md; do`
- **Verify GPU headers are documented** — `UNDOCUMENTED=0`
- **Write job summary** — `echo "## 📚 GPU CI – Documentation Gate" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/06-infrastructure_gpu_gpu-ci.yml)
- [Alle Workflows](../README.md)


