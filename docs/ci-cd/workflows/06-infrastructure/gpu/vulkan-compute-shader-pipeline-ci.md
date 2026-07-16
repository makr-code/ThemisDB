# Vulkan Compute Shader Pipeline CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/06-infrastructure_gpu_vulkan-compute-shader-pipeline-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Vulkan Compute Shader Pipeline**.

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

### `vulkan-compute-shader-pipeline-tests`
**Anzeigename:** Vulkan Compute Shader Pipeline (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install build dependencies** — `sudo apt-get update -qq`
- **Configure CMake (CPU-only, Vulkan disabled)** — `cmake -S cmake -B build \`
- **Build themis_tests** — `cmake --build build --target themis_tests --parallel $(nproc)`
- **Run VulkanComputeShaderHardening tests (Vulkan disabled)** — `cd build`
- **Verify shader files include specialization constant headers** — `echo "Checking l2_distance.comp for local_size_x_id..."`
- **Verify MoltenVK probe implementation in graphics_backends.cpp** — `echo "Checking VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME probe..."`
- **Verify double-buffer staging implementation** — `echo "Checking StagingSlot ring buffer..."`
- **Verify specialization constants in pipeline creation** — `echo "Checking VkSpecializationInfo in createComputePipelines..."`
- **Write job summary** — `echo "## 🖥️ Vulkan Compute Shader Pipeline CI (${{ matrix.compiler }})" >> "$GIT`

### `vulkan-compute-shader-pipeline-gate`
**Anzeigename:** Vulkan Compute Shader Pipeline Gate

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `vulkan-compute-shader-pipeline-tests`
**Bedingung:** `always()`

**Schritte:**

- **Check test results** — `result="${{ needs.vulkan-compute-shader-pipeline-tests.result }}"`
- **Write gate summary** — `result="${{ needs.vulkan-compute-shader-pipeline-tests.result }}"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/06-infrastructure_gpu_vulkan-compute-shader-pipeline-ci.yml)
- [Alle Workflows](../README.md)


