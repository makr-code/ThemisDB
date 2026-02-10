# ThemisDB GPU/VRAM Support - Quick Reference

**Letzte Aktualisierung:** 15. Januar 2026

---

## GPU-Unterstützung nach Edition

| Edition | VRAM-Limit | GPU-Backends Verfügbar | Standard-Build | GPU-Nutzung Standardmäßig | Aktivierung |
|---------|-----------|------------------------|----------------|---------------------------|-------------|
| **MINIMAL** | 0 GB | ❌ Nein | Keine GPU-Flags | ❌ Nein | Nicht verfügbar |
| **COMMUNITY** | 24 GB | ✅ Ja | `OFF` | ❌ Nein | `-DTHEMIS_ENABLE_CUDA=ON` |
| **ENTERPRISE** | 256 GB | ✅ Ja | `OFF` | ❌ Nein | `-DTHEMIS_ENABLE_CUDA=ON` |
| **HYPERSCALER** | Unlimited | ✅ Ja | `OFF` | ❌ Nein | `-DTHEMIS_ENABLE_CUDA=ON` |

---

## GPU-Backend-Unterstützung

| Backend | Plattform | Implementiert | Standard | CMake-Flag | Abhängigkeiten |
|---------|-----------|--------------|----------|------------|----------------|
| **CUDA** | NVIDIA | ✅ Ja | ❌ OFF | `-DTHEMIS_ENABLE_CUDA=ON` | CUDA Toolkit 11.0+ |
| **Vulkan** | Cross-platform | ✅ Ja (partial) | ❌ OFF | `-DTHEMIS_ENABLE_VULKAN=ON` | Vulkan SDK |
| **FAISS GPU** | NVIDIA | ✅ Ja | ❌ OFF | `-DTHEMIS_ENABLE_CUDA=ON` | FAISS + CUDA |
| **HIP** | AMD | ✅ Ja | ❌ OFF | `-DTHEMIS_ENABLE_HIP=ON` | ROCm/HIP |
| **Metal** | Apple | ✅ Ja | ❌ OFF | `-DTHEMIS_ENABLE_METAL=ON` | macOS 10.13+ |
| **DirectX** | Windows | ✅ Ja | ❌ OFF | `-DTHEMIS_ENABLE_DIRECTX=ON` | Windows 10+ |
| **OpenCL** | Cross-platform | ✅ Ja | ❌ OFF | `-DTHEMIS_ENABLE_OPENCL=ON` | OpenCL SDK |
| **OneAPI** | Intel | ✅ Ja | ❌ OFF | `-DTHEMIS_ENABLE_ONEAPI=ON` | Intel OneAPI |
| **ZLUDA** | AMD (CUDA-compat) | ✅ Ja | ❌ OFF | `-DTHEMIS_ENABLE_ZLUDA=ON` | ZLUDA |

---

## Schnellstart: GPU-Beschleunigung Aktivieren

### CUDA (NVIDIA)

```bash
# 1. CUDA Toolkit installieren
# Download von https://developer.nvidia.com/cuda-downloads

# 2. Mit CUDA bauen
cmake -S . -B build \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DCUDAToolkit_ROOT=/usr/local/cuda

cmake --build build

# 3. Verifizieren
./build/themis_server --check-gpu
# Sollte ausgeben: "CUDA backend: Available"
```

### Vulkan (Cross-Platform)

```bash
# 1. Vulkan SDK installieren
# Download von https://vulkan.lunarg.com/

# 2. Mit Vulkan bauen
cmake -S . -B build \
  -DTHEMIS_ENABLE_VULKAN=ON \
  -DVULKAN_SDK=/path/to/vulkan/sdk

cmake --build build

# 3. Verifizieren
vulkaninfo  # Zeigt verfügbare GPUs
./build/themis_server --check-gpu
```

### Metal (Apple Silicon)

```bash
# Nur auf macOS verfügbar

# Mit Metal bauen
cmake -S . -B build \
  -DTHEMIS_ENABLE_METAL=ON

cmake --build build

# Verifizieren
./build/themis_server --check-gpu
# Sollte ausgeben: "Metal backend: Available"
```

---

## Runtime-Verhalten

### Ohne GPU-Backend

```cpp
auto& registry = BackendRegistry::instance();
registry.autoDetect();

auto* backend = registry.getBestVectorBackend();
std::cout << backend->type() << std::endl;
// Output: "CPU"  ← Fallback auf CPU
```

**Logs:**
```
[WARN] No GPU backend available. Using CPU (slower).
[WARN] Rebuild with -DTHEMIS_ENABLE_CUDA=ON for GPU acceleration.
```

### Mit GPU-Backend (z.B. CUDA)

```cpp
auto& registry = BackendRegistry::instance();
registry.autoDetect();

auto* backend = registry.getBestVectorBackend();
std::cout << backend->type() << std::endl;
// Output: "CUDA"  ← GPU wird genutzt

auto caps = backend->getCapabilities();
std::cout << "Device: " << caps.deviceName << std::endl;
std::cout << "VRAM: " << caps.maxMemoryBytes / (1024*1024*1024) << " GB" << std::endl;
```

**Logs:**
```
[INFO] CUDA backend: Available
[INFO] Device: NVIDIA RTX 4090
[INFO] VRAM: 24 GB
[INFO] Using GPU acceleration
```

---

## Performance-Vergleich

### Vector Search (1M Vektoren, 768-dim)

| Backend | Single Query | Batch (1000 queries) | Speedup vs CPU |
|---------|-------------|---------------------|----------------|
| CPU (32 Cores) | 120 ms | 95 s | 1x (Baseline) |
| CUDA (RTX 4090) | 5 ms | 3.2 s | **24x** |
| Vulkan (RTX 4090) | 6 ms | 3.8 s | **20x** |
| Metal (M2 Ultra) | 8 ms | 5.1 s | **15x** |
| HIP (MI250X) | 6 ms | 3.5 s | **22x** |

**Hinweis:** Alle GPU-Performance-Zahlen erfordern Build mit entsprechendem Backend-Flag.

---

## Häufige Fehler

### Fehler 1: GPU wird nicht erkannt

**Symptom:**
```cpp
backend->isAvailable()  // Returns false
```

**Ursache:** Backend nicht aktiviert beim Build.

**Lösung:**
```bash
# Neu bauen mit GPU-Flag
cmake -S . -B build -DTHEMIS_ENABLE_CUDA=ON
cmake --build build
```

---

### Fehler 2: VRAM-Limit vorhanden, aber keine GPU-Nutzung

**Symptom:**
```cpp
edition::GPU_MAX_VRAM_GB == 24  // True
backend->type() == BackendType::CPU  // True ← WTF?
```

**Ursache:** VRAM-Limit bedeutet nicht, dass GPU-Backends aktiviert sind.

**Lösung:** GPU-Backend beim Build aktivieren (siehe oben).

---

### Fehler 3: Performance wie CPU, nicht wie GPU

**Symptom:** Performance-Benchmarks zeigen keine Verbesserung.

**Ursache:** Fallback auf CPU, obwohl GPU erwartet.

**Diagnose:**
```cpp
auto* backend = registry.getBestVectorBackend();
if (backend->type() == BackendType::CPU) {
    LOG_ERROR << "GPU backend not available! Performance will be slow.";
    LOG_ERROR << "Check that ThemisDB was built with -DTHEMIS_ENABLE_CUDA=ON";
}
```

---

## Weitere Informationen

- **Vollständige Analyse:** [`GAP_ANALYSE_GPU_VRAM_NUTZUNG.md`](./GAP_ANALYSE_GPU_VRAM_NUTZUNG.md)
- **English Summary:** [`GAP_ANALYSIS_SUMMARY_EN.md`](./GAP_ANALYSIS_SUMMARY_EN.md)
- **CUDA Backend Docs:** [`performance/performance_cuda.md`](./performance/performance_cuda.md)
- **Vulkan Backend Docs:** [`performance/performance_vulkan.md`](./performance/performance_vulkan.md)

---

**Wichtig:** Diese Referenz spiegelt die Gap-Analyse vom 15. Januar 2026 wider. Die tatsächliche GPU-Nutzung erfordert **explizite Aktivierung beim Build**.
