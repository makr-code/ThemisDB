# LLM CUDA GPU CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/02-feature-modules_llm_llm-cuda-gpu-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **LLM CUDA GPU**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (5 überwachte Pfade)
- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (5 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `cuda-compile-check`
**Anzeigename:** CUDA Kernel Compile Check (nvcc, no GPU)

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_llm_changes == 'true' || needs.ci-scope-classifier.outputs.has`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install CUDA toolkit** — `sudo apt-get update -qq`
- **Compile kernel_fusion.cu (sm_${{ matrix.cuda_arch }})** — `# Compile kernel_fusion.cu as a standalone object file.`
- **Report compile success** — `echo "✅ kernel_fusion.cu compiled successfully for sm_${{ matrix.cuda_arch }}"`
- **Write job summary** — `echo "## 🔬 LLM CUDA GPU CI – Compile Check" >> "$GITHUB_STEP_SUMMARY"`

### `cuda-kernel-tests`
**Anzeigename:** CUDA Kernel Tests (GPU Runner)

**Läuft auf:** `['self-hosted', 'gpu-cuda']`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `(needs.ci-scope-classifier.outputs.has_llm_changes == 'true' ||
 needs.ci-scope-classifier.outputs.h`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Verify GPU availability** — `nvidia-smi`
- **Install build dependencies** — `sudo apt-get update -qq`
- **Configure CMake (CUDA enabled)** — `cmake -B build -G Ninja \`
- **Build kernel fusion CUDA test** — `cmake --build build --target themisdb_tests -j$(nproc)`
- **Run CUDA kernel tests** — `cd build`
- **Report GPU test results** — `echo "GPU test run complete. Architecture: $(nvidia-smi --query-gpu=name --forma`
- **Write job summary** — `GPU_NAME=$(nvidia-smi --query-gpu=name --format=csv,noheader | head -1 2>/dev/nu`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_llm_llm-cuda-gpu-ci.yml)
- [Alle Workflows](../README.md)


