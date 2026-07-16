# GPU Memory Oversubscription CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/06-infrastructure_gpu_gpu-memory-oversubscription-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **GPU Memory Oversubscription**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (9 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (9 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `gpu-memory-oversubscription-unit-tests`
**Anzeigename:** GPU Memory Oversubscription tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install build dependencies** — `sudo apt-get update -qq`
- **Configure CMake (COMMUNITY edition, CPU-only)** — `cmake -S . -B build \`
- **Build oversubscription test target** — `cmake --build build \`
- **Run GPUMemoryOversubscriptionFocusedTests** — `cd build`
- **Verify oversubscription header is self-contained** — `${{ matrix.cxx }} -std=c++17 \`
- **Verify source compiles cleanly (oversubscription only)** — `${{ matrix.cxx }} -std=c++17 \`
- **Write job summary** — `echo "## 🖥️ GPU Memory Oversubscription CI (${{ matrix.compiler }})" >> "$GITHUB`

### `oversubscription-docs-gate`
**Anzeigename:** Documentation gate

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Verify implementation header is present** — `test -f include/index/gpu_memory_oversubscription.h && \`
- **Verify implementation source is present** — `test -f src/index/gpu_memory_oversubscription.cpp && \`
- **Verify test file is present** — `test -f tests/index/test_gpu_memory_oversubscription.cpp && \`
- **Verify PrefetchStrategy enum is defined** — `grep -q "enum class PrefetchStrategy" \`
- **Verify oversubscription config fields in GPUVectorIndex** — `grep -q "enable_oversubscription" include/index/gpu_vector_index.h && \`
- **Write job summary** — `echo "## 📚 GPU Memory Oversubscription — Documentation Gate" >> "$GITHUB_STEP_SU`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/06-infrastructure_gpu_gpu-memory-oversubscription-ci.yml)
- [Alle Workflows](../README.md)


