# GPU Benchmark Matrix Runner

## Zweck

Dieses Dokument beschreibt die **GPU Benchmark Matrix Pipeline** für ThemisDB.
Sie implementiert Maßnahme **M05** aus `PERFORMANCE_EXPECTATIONS.md §1.4` und löst
das Issue **[Agentic AI][Benchmarks] GPU-Benchmark-Matrix (CUDA/HIP/Vulkan) als separaten Runner etablieren**.

Ziel ist es, GPU-gebundene Benchmark-Binaries auf echten GPU-Runnern auszuführen,
statt auf `*_GPUDisabled`-Stubs zurückzufallen, und die Messergebnisse als
CI-Artefakte zu persistieren.

---

## Workflow-Datei

```
.github/workflows/06-infrastructure_gpu_gpu-benchmark-matrix-ci.yml
```

### Trigger

| Trigger | Bedingung |
|---------|-----------|
| `push` | Änderungen an `benchmarks/bench_{fused,gpu,cuda,vulkan,backend}*.cpp`, Acceleration-/LoRA-Sources, Workflow-Datei; nur auf `main`/`develop` |
| `schedule` | Wöchentlich Samstag 03:00 UTC |
| `workflow_dispatch` | Manuell, mit optionalen `backend_filter`- und `benchmark_filter`-Inputs |

---

## Runner-Profile

### CUDA Runner (`[self-hosted, gpu-cuda]`)

| Eigenschaft | Wert |
|-------------|------|
| **Label** | `gpu-cuda` |
| **OS** | Ubuntu 22.04 LTS |
| **CUDA Toolkit** | 12.x |
| **Ziel-Architekturen** | sm_80 (Ampere/A100), sm_89 (Ada/RTX 4090/L4), sm_90 (Hopper/H100) |
| **CMake Flags** | `-DTHEMIS_ENABLE_CUDA=ON -DTHEMIS_ENABLE_GPU=ON` |
| **Artefakt-Prefix** | `gpu-cuda-<arch>-benchmark-results` |

**Voraussetzungen für Runner-Setup:**
```bash
# CUDA Toolkit installieren
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt-get update
sudo apt-get install -y cuda-toolkit-12-4

# GitHub Actions Runner registrieren
./config.sh --url https://github.com/makr-code/ThemisDB \
            --token <RUNNER_TOKEN> \
            --labels "self-hosted,gpu-cuda,ubuntu-22.04"
./svc.sh install && ./svc.sh start
```

---

### HIP/ROCm Runner (`[self-hosted, gpu-hip]`)

| Eigenschaft | Wert |
|-------------|------|
| **Label** | `gpu-hip` |
| **OS** | Ubuntu 22.04 LTS |
| **ROCm Version** | 6.x |
| **Ziel-Architekturen** | gfx1100 (RDNA3/RX 7900 XTX), gfx90a (CDNA2/MI250X) |
| **CMake Flags** | `-DTHEMIS_ENABLE_HIP=ON -DTHEMIS_ENABLE_GPU=ON` |
| **Artefakt-Prefix** | `gpu-hip-<arch>-benchmark-results` |

**Voraussetzungen für Runner-Setup:**
```bash
# ROCm installieren
sudo apt-get install -y rocm-dev hipcc

# GitHub Actions Runner registrieren
./config.sh --url https://github.com/makr-code/ThemisDB \
            --token <RUNNER_TOKEN> \
            --labels "self-hosted,gpu-hip,ubuntu-22.04"
./svc.sh install && ./svc.sh start
```

---

### Vulkan Runner (`[self-hosted, gpu-vulkan]`)

| Eigenschaft | Wert |
|-------------|------|
| **Label** | `gpu-vulkan` |
| **OS** | Ubuntu 22.04 LTS |
| **Vulkan** | 1.3+ (LunarG SDK oder Mesa) |
| **CMake Flags** | `-DTHEMIS_ENABLE_VULKAN=ON -DTHEMIS_ENABLE_GPU=ON` |
| **Artefakt-Prefix** | `gpu-vulkan-benchmark-results` |

**Voraussetzungen für Runner-Setup:**
```bash
# Vulkan SDK installieren
sudo apt-get install -y libvulkan-dev vulkan-tools spirv-tools

# GitHub Actions Runner registrieren
./config.sh --url https://github.com/makr-code/ThemisDB \
            --token <RUNNER_TOKEN> \
            --labels "self-hosted,gpu-vulkan,ubuntu-22.04"
./svc.sh install && ./svc.sh start
```

---

### CPU Fallback Runner (`ubuntu-latest`)

Läuft **immer** auf dem GitHub-hosted Runner.  
Prüft, dass alle GPU-gated Benchmark-Targets in CPU-only-Mode kompilieren
(Disabled-Stub-Pfad) und dass jeder `*_Disabled`-Stub die Policy-Tags
`Deadline:` und `Issue: #` trägt.

| Eigenschaft | Wert |
|-------------|------|
| **Label** | `ubuntu-latest` (GitHub-hosted) |
| **CMake Flags** | alle GPU-Flags OFF |
| **Blocking Gate** | Ja — Fehler hier blockiert den PR |

---

## Prioritätsliste GPU-Benchmarks

| Priorität | Datei | Backends | Gate-Bedingung |
|-----------|-------|----------|----------------|
| 1 | `bench_fused_kernels.cpp` | CUDA, HIP | `THEMIS_ENABLE_CUDA` oder `THEMIS_ENABLE_HIP` |
| 2 | `bench_gpu_backends.cpp` | CUDA, HIP, Vulkan | beliebiges GPU-Flag |
| 3 | `bench_gpu_training_cycle.cpp` | CUDA, HIP | `THEMIS_ENABLE_CUDA` oder `THEMIS_ENABLE_HIP` |
| 4 | `bench_fused_lora_kernels.cpp` | CUDA, HIP | `THEMIS_ENABLE_LLM` + GPU |
| 5 | `bench_vulkan_lora.cpp` | Vulkan | `THEMIS_ENABLE_VULKAN` |
| 6 | `bench_backend_comparison.cpp` | CUDA, HIP | `THEMIS_ENABLE_LLM` + GPU |
| 7 | `bench_cuda_vs_cpu.cpp` | CUDA | `THEMIS_ENABLE_CUDA` |
| 8 | `bench_multi_gpu_scaling.cpp` | CUDA, HIP | GPU-Flag |

---

## Build+Run+Artefakte Pipeline

```
┌──────────────────────────────────────────────────────────────────┐
│                  GPU Benchmark Matrix Pipeline                    │
└──────────────────────────────────────────────────────────────────┘

  Trigger (push/schedule/workflow_dispatch)
       │
       ├─ gpu-bench-cuda  ──────────────────────────────────────────┐
       │    runs-on: [self-hosted, gpu-cuda]                         │
       │    1. apt install deps + CUDA toolkit                       │
       │    2. cmake -DTHEMIS_ENABLE_CUDA=ON                        │
       │    3. cmake --build ... bench_fused_kernels ...             │
       │    4. Run each binary → JSON → BENCH_OUT_DIR/               │
       │    5. upload-artifact: gpu-cuda-<arch>-benchmark-results    │
       │                                                             │
       ├─ gpu-bench-hip   ──────────────────────────────────────────┤
       │    runs-on: [self-hosted, gpu-hip]                          │
       │    1. apt install deps + ROCm                               │
       │    2. cmake -DTHEMIS_ENABLE_HIP=ON                         │
       │    3. cmake --build ... bench_fused_kernels ...             │
       │    4. Run each binary → JSON → BENCH_OUT_DIR/               │
       │    5. upload-artifact: gpu-hip-<arch>-benchmark-results     │
       │                                                             │
       ├─ gpu-bench-vulkan  ────────────────────────────────────────┤
       │    runs-on: [self-hosted, gpu-vulkan]                       │
       │    1. apt install deps + libvulkan-dev                      │
       │    2. cmake -DTHEMIS_ENABLE_VULKAN=ON                      │
       │    3. cmake --build ... bench_vulkan_lora ...               │
       │    4. Run each binary → JSON → BENCH_OUT_DIR/               │
       │    5. upload-artifact: gpu-vulkan-benchmark-results         │
       │                                                             │
       ├─ gpu-bench-cpu-fallback  ──────────────────────────────────┤
       │    runs-on: ubuntu-latest  (BLOCKING)                       │
       │    1. apt install deps                                       │
       │    2. cmake -DTHEMIS_ENABLE_GPU=OFF ...                    │
       │    3. Build all GPU-gated targets (stub path)               │
       │    4. Verify disabled-stub policy (Deadline + Issue)        │
       │    5. Smoke-run bench_gpu_backends (exit 0 check)           │
       │    6. upload-artifact: gpu-cpu-fallback-benchmark-results   │
       │                                                             │
       └─ gpu-benchmark-gate  ──────────────────────────────────────┘
            needs: all 4 jobs
            - CUDA/HIP/Vulkan: continue-on-error (not blocking)
            - CPU fallback: BLOCKING gate
```

### Artefakt-Inhalt

Jedes Artefakt-Verzeichnis enthält:

```
<artifact-dir>/
├── runner_info.txt       # GPU device info (nvidia-smi / rocm-smi / vulkaninfo)
├── build.log             # cmake --build Ausgabe
├── bench_<name>.json     # Google Benchmark JSON Output
├── bench_<name>.run.log  # stdout/stderr des Benchmark-Prozesses
└── summary.json          # Konsolidiertes JSON (CUDA-Only: Anzahl real/skipped)
```

### JSON-Format (Google Benchmark)

```json
{
  "context": { "date": "...", "host_name": "...", "num_cpus": ... },
  "benchmarks": [
    {
      "name": "BM_FusedKernels_CUDA/rank_8/dim_768/real_time",
      "real_time": 1234.5,
      "cpu_time": 1200.3,
      "time_unit": "ns",
      "iterations": 100
    }
  ]
}
```

---

## Fallback-Verhalten

Wenn kein selbst-gehosteter GPU-Runner mit dem jeweiligen Label registriert ist,
queuet GitHub Actions den Job und er bleibt bis zum Runner-Timeout ausstehend
(Standard: 6 Stunden). Da alle drei GPU-Jobs `continue-on-error: true` tragen,
blockieren sie nicht den PR.

Das `gpu-bench-cpu-fallback`-Job läuft immer auf `ubuntu-latest` und stellt sicher,
dass der Disabled-Stub-Pfad kompiliert und die Policy eingehalten wird.

### Dokumentierter Fallback (ohne GPU-Runner)

Für Entwickler, die die Benchmarks lokal mit GPU ausführen möchten:

```bash
# CUDA Build
cmake -B build_gpu -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DTHEMIS_ENABLE_CUDA=ON -DTHEMIS_ENABLE_GPU=ON -DTHEMIS_ENABLE_LLM=ON \
  -S cmake
cmake --build build_gpu --target bench_fused_kernels bench_gpu_backends

# Run mit JSON-Output
./build_gpu/bench_fused_kernels \
  --benchmark_format=json \
  --benchmark_out=results/bench_fused_kernels.json \
  --benchmark_min_time=1s

# HIP Build
cmake -B build_hip -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DTHEMIS_ENABLE_HIP=ON -DTHEMIS_ENABLE_GPU=ON \
  -S cmake

# Vulkan Build
cmake -B build_vulkan -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DTHEMIS_ENABLE_VULKAN=ON -DTHEMIS_ENABLE_GPU=ON \
  -S cmake
```

---

## M05 Audit-Check

Der Workflow erfüllt die Anforderungen des `perf_coverage_top10_audit.py`-Checks für **M05**:

| Anforderung | Erfüllt durch |
|-------------|---------------|
| Workflow mit GPU-Runner-Label | `runs-on: [self-hosted, gpu-cuda]` (Job `gpu-bench-cuda`) |
| `THEMIS_ENABLE_CUDA=ON` | `-DTHEMIS_ENABLE_CUDA=ON` im CMake-Configure-Step |
| `THEMIS_ENABLE_HIP=ON` | `-DTHEMIS_ENABLE_HIP=ON` im CMake-Configure-Step (Job `gpu-bench-hip`) |
| Matrix-Strategy | `strategy.matrix` mit `cuda_arch` bzw. `rocm_arch` |
| Benchmark-Referenzen | `bench_fused_kernels`, `bench_gpu_backends`, `bench_cuda_vs_cpu` |

Audit-Ergebnis: **OK** (gpu_ci_matrix_found)

---

## Verwandte Dokumente

- `PERFORMANCE_EXPECTATIONS.md` §1.4 Maßnahme 5 (M05)
- `docs/ci-cd/perf_coverage_top10_audit.md` M05
- `docs/governance/DISABLED_STUB_POLICY.md`
- `docs/gpu_roadmap.md`
- `docs/gpu_runbooks.md`
- `.github/workflows/06-infrastructure_gpu_gpu-benchmark-matrix-ci.yml`
- `.github/workflows/02-feature-modules_llm_llm-cuda-gpu-ci.yml` (verwandt: CUDA Kernel CI)
