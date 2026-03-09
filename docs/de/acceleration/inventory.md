# Acceleration-Modul – Primäres Inventar

**Datum:** März 2026  
**Modul:** `acceleration`  
**Primärpfade:** `src/acceleration/`, `include/acceleration/`  
**Validated:** 2026-03-09

---

## 1. Dokumentationsdateien im Modul

### Primärdokumentationen (`src/acceleration/`)

| Datei | Beschreibung | Status |
|---|---|---|
| `src/acceleration/README.md` | Modulübersicht, Verzeichnisstruktur, Runtime-Verhalten, Build-Flags, Entwicklerleitfaden, wissenschaftliche Grundlagen | ✅ Aktuell |
| `src/acceleration/ARCHITECTURE.md` | Detaillierte Architektur: Design-Prinzipien, Komponententabelle, Komponentendiagramm, Datenfluss, Threading, Performance, Sicherheit, Fehlerbehandlung | ✅ Aktuell |
| `src/acceleration/ROADMAP.md` | Entwicklungsstatus, abgeschlossene Features, In-Progress, Planned Features, Phasenmodell (1–6), Production-Readiness-Checkliste, Known Issues, Breaking Changes | ✅ Aktuell (ROADMAP-Inkonsistenz #1387 behoben) |
| `src/acceleration/FUTURE_ENHANCEMENTS.md` | Geplante Erweiterungen mit Implementierungsdetails, Performance-Zielen, API-Skizzen und wissenschaftlichen IEEE-Referenzen | ✅ Aktuell |
| `src/acceleration/future_enhancements.md` | Legacy-Alias (Kleinschreibung) → verweist auf `FUTURE_ENHANCEMENTS.md` | ℹ️ Nur Alias |

### Öffentliche Header-Dokumentation (`include/acceleration/`)

| Datei | Beschreibung | Status |
|---|---|---|
| `include/acceleration/README.md` | Öffentliche API-Header-Übersicht: alle Header, RAII-Unterverzeichnis, Metriken | ✅ Aktuell |
| `include/acceleration/FUTURE_ENHANCEMENTS.md` | Geplante Interface-Erweiterungen für `IComputeBackend`, `IDeviceCapabilityQuery`, async dispatch | ✅ Aktuell |
| `include/acceleration/raii/README.md` | RAII-Wrapper-Übersicht für GPU-Ressourcen | ✅ Aktuell |

---

## 2. Quellcode-Dateien (`src/acceleration/`)

### Backend-Implementierungen

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `backend_registry.cpp` | — | Singleton-Registry: Erkennung, Capability-Scoring, Laufzeitauswahl |
| `compute_backend.cpp` | `compute_backend.h` | Abstrakte `ComputeBackend`-Basisklasse |
| `device_manager.cpp` | `device_manager.h` | Geräte-Enumeration, Capability-Probing, 60 s TTL-Cache |
| `cpu_backend.cpp` | `cpu_backend.h` | CPU-Referenzimplementierung (single-thread) |
| `cpu_backend_mt.cpp` | — | Multi-threaded CPU-Backend (pthreads) |
| `cpu_backend_tbb.cpp` | — | Intel TBB-basiertes paralleles CPU-Backend |
| `cuda_backend.cpp` + `cuda/` | `cuda_backend.h` | CUDA-Kernels und Stream-Management (⚠️ 6 Stubs in `cuda_backend.cpp`) |
| `hip_backend.cpp` + `hip/` | `hip_backend.h` | AMD HIP/ROCm-Backend |
| `vulkan_backend_full.cpp` + `vulkan/` | `vulkan_backend.h` | Vulkan Compute-Pipeline (Stubs: 0, Quality: 94) |
| `opencl_backend.cpp` | `opencl_backend.h` | OpenCL-Backend (⚠️ 1 Stub) |
| `metal_backend.mm` | — | Apple Metal-Backend (macOS/iOS) |
| `directx_backend_full.cpp` + `directx/` | — | DirectX Compute-Backend (Windows) |
| `graphics_backends.cpp` | `graphics_backends.h` | Gemeinsame GPU-Hilfsfunktionen |
| `zluda_backend.cpp` | — | ZLUDA-Backend (AMD über CUDA-API-Kompatibilitätsschicht) |
| `oneapi_backend.cpp` | — | Intel oneAPI/SYCL-Backend |
| `faiss_gpu_backend.cpp` | `faiss_gpu_backend.h` | FAISS GPU-Wrapper für Milliarden-ANN-Suche |
| `multi_gpu_backend.cpp` | `multi_gpu_backend.h` | Multi-GPU-Lastverteilung (⚠️ NCCL-Group-Call-Wiring ausstehend) |
| `nccl_vector_backend.cpp` | `nccl_vector_backend.h` | NCCL Kollektiv-Operationen (NVIDIA Multi-GPU) |
| `rccl_vector_backend.cpp` | `rccl_vector_backend.h` | RCCL Kollektiv-Operationen (AMD Multi-GPU) |
| `tensor_core_matmul.cpp` + `cuda/tensor_core_matmul.cu` | `tensor_core_matmul.h` | Tensor Core FP16/BF16 Matrix-Multiplikation |
| `geo_acceleration_bridge.cpp` | `geo_acceleration_bridge.h` | Brücke Geospatial-Operatoren → Acceleration-Schicht |
| `plugin_loader.cpp` | `plugin_loader.h` | Dynamisches Laden externer Backend-Plugins |
| `plugin_security.cpp` | `plugin_security.h` | Signaturprüfung für geladene Plugins (RTLD_NOW, Dateirechte, Größenkap) |
| `shader_integrity.cpp` | `shader_integrity.h` | SPIR-V Shader-Integritätsprüfung vor Pipeline-Erstellung |
| `vllm_resource_manager.cpp` | `vllm_resource_manager.h` | GPU-VRAM-Ressourcenverwaltung für LLM-Inferenz-Pfade |

### CUDA-Kernel-Dateien (`src/acceleration/cuda/`)

| Datei | Inhalt |
|---|---|
| `cuda/ann_kernels.cu` | 4 `__global__` Kernel für ANN-Suche |
| `cuda/vector_kernels.cu` | 9 `__global__` Kernel für Vektor-Ähnlichkeitssuche |
| `cuda/geo_kernels.cu` | Haversine-Distanz + Ray-Casting Point-in-Polygon |
| `cuda/tensor_core_matmul.cu` | FP16/BF16 Tensor Core GEMM |

### HIP-Kernel-Dateien (`src/acceleration/hip/`)

| Datei | Inhalt |
|---|---|
| `hip/ann_kernels.hip` | AMD HIP ANN-Kernels |
| `hip/geo_kernels.hip` | AMD HIP Geospatial-Kernels |

### Vulkan SPIR-V Compute-Shader (`src/acceleration/vulkan/shaders/`)

| Shader | Operation |
|---|---|
| `l2_distance.comp` | L2 (Euklidische) Distanz |
| `cosine_distance.comp` | Kosinus-Distanz |
| `inner_product_distance.comp` | Innenprodukt-Distanz |
| `batch_search.comp` | Batch-Ähnlichkeitssuche |
| `topk_selection.comp` | Top-K Auswahl |
| `haversine_distance.comp` | Haversine Geospatial-Distanz |
| `point_in_polygon.comp` | Point-in-Polygon Ray-Casting |
| `lora/` | LoRA-Trainings-Shader (dequantization, elementwise, embedding, gradient, matmul, sequence_mean, quantization_nf4) |

### DirectX-Shader (`src/acceleration/directx/shaders/lora/`)

| Shader | Operation |
|---|---|
| `elementwise.hlsl` | Element-wise LoRA-Operationen |
| `embedding_lookup.hlsl` | Embedding-Lookup |
| `gradient.hlsl` | Gradient-Berechnung |
| `matmul.hlsl` | Matrix-Multiplikation |
| `sequence_mean.hlsl` | Sequenz-Mittelwert |

---

## 3. Öffentliche Header-Dateien (`include/acceleration/`)

| Header | Rolle |
|---|---|
| `compute_backend.h` | Abstrakte `ComputeBackend`-Klasse, `DeviceCapabilityInfo`; `BACKEND_CONTRACT_VERSION = 100` |
| `device_manager.h` | `DeviceManager`, `DeviceCapabilityInfo`, Probing-API |
| `kernel_fallback_dispatcher.h` | `ANNKernelFallbackDispatcher`, `GeoKernelFallbackDispatcher`, `RetryPolicy` |
| `kernel_invocation.h` | Eingefrorene Dispatch-Tabellen `ANNKernelDispatch`, `GeoKernelDispatch`; `INTERFACE_VERSION = 100` |
| `batch_validator.h` | Eingabe-Validierungshelfer für alle Backends |
| `cpu_backend.h` | CPU-Backend-API |
| `cuda_backend.h` | CUDA-Backend-API inkl. `CUDAGraphCache` (⚠️ 6 Stubs) |
| `hip_backend.h` | HIP/ROCm-Backend-API |
| `vulkan_backend.h` | Vulkan-Backend-API |
| `opencl_backend.h` | OpenCL-Backend-API |
| `graphics_backends.h` | `DirectXVectorBackend`, `VulkanVectorBackend`, `VulkanGeoBackend`, `OpenGLVectorBackend` |
| `faiss_gpu_backend.h` | FAISS GPU-Wrapper-API |
| `multi_gpu_backend.h` | Multi-GPU-Backend-API |
| `nccl_vector_backend.h` / `rccl_vector_backend.h` | NCCL/RCCL-Kollektiv-API |
| `tensor_core_matmul.h` | Tensor Core FP16/BF16 Matrix-Multiplikation |
| `geo_acceleration_bridge.h` | `GeoKernelDispatch`, `GeoAccelerationBridge` |
| `plugin_loader.h` | Plugin-Loader-API |
| `plugin_security.h` | Plugin-Sicherheits-API |
| `shader_integrity.h` | SPIR-V-Integritätsprüfungs-API |
| `vllm_resource_manager.h` | vLLM-Ressourcenverwaltungs-API |
| `error_codes.h` | `AccelerationErrorCode` Enum + `errorCodeToString()` |
| `error_context.h` | `ErrorContext` Struct für strukturierte Fehlerpropagation |
| `raii/cuda_raii.h` | `CudaStream`, `CudaDeviceMemory`, `ScopedCudaDevice` |
| `raii/hip_raii.h` | HIP-Stream und Gerätespeicher RAII-Wrapper |
| `raii/opencl_raii.h` | OpenCL-Ressourcen RAII-Wrapper |
| `raii/vulkan_raii.h` | Vulkan-Objekte RAII-Wrapper |
| `metrics/backend_metrics.h` | `BackendMetrics`, `Counter`, `Gauge`, `Histogram`, `Timer` |
| `metrics/metrics_collector.h` | `MetricsCollector` für Backend-Telemetrie |

---

## 4. Sekundäre Dokumentation (`docs/de/acceleration/`)

| Datei | Beschreibung |
|---|---|
| `docs/de/acceleration/README.md` | Modulübersicht auf Deutsch mit Backend-Tabelle, Laufzeitverhalten, Build-Konfiguration, Performance-Zielen, Sicherheit und Links |
| `docs/de/acceleration/backends.md` | Detaillierte Beschreibung der unterstützten Backends |
| `docs/de/acceleration/missing-implementations.md` | Reality-Check-Bericht: fehlende/unvollständige Implementierungen mit Evidence und Issue-Vorschlägen |
| `docs/de/acceleration/missing-implementations.json` | Maschinenlesbares Format des Reality-Check-Berichts |
| `docs/de/acceleration/inventory.md` | Dieses Inventardokument |

---

## 5. Verwandte Dokumentation (`docs/acceleration/`)

| Datei | Beschreibung |
|---|---|
| `docs/acceleration/capability_negotiation.md` | Backend-Auswahlprozess, Capability-Matrix, Fallback-Kette |
| `docs/acceleration/troubleshooting.md` | Operatives Fehlerbehebungshandbuch (Runbooks, Diagnostik, plattformspezifische Probleme) |

---

## 6. Reality-Check-Ergebnis (Stand: März 2026)

### ✅ Korrekt dokumentiert
- Alle 25 Quelldateien und ihre zugehörigen Header sind vorhanden und korrekt referenziert
- `BACKEND_CONTRACT_VERSION = 100` in `compute_backend.h` stimmt mit ROADMAP Phase 1 überein
- `KERNEL_INVOCATION_INTERFACE_VERSION = 100` in `kernel_invocation.h` stimmt überein
- CUDA Geospatial-Kernel (`cuda/geo_kernels.cu`): Haversine + Ray-Casting implementiert ✅
- Vulkan SPIR-V Shader (7 Shader + LoRA-Suite): alle implementiert ✅
- `BatchValidator` (`include/acceleration/batch_validator.h`): existiert ✅
- Plugin-Security-Hardening: `RTLD_NOW`, Dateirechte-Check, Größenkap implementiert ✅
- `ANNKernelFallbackDispatcher` / `GeoKernelFallbackDispatcher` mit Retry + CPU-Fallback: implementiert ✅

### 🔧 Korrigiert (in diesem PR)
- **ROADMAP.md**: Duplizierter `[ ] Fallback/retry semantics... (Issue: #1387)` in Long-term Planned Features entfernt — Issue #1387 war bereits als `[x]` in Phase 3 markiert
- **docs/de/acceleration/README.md**: Vulkan-Status von `🚧 In Bearbeitung` auf `✅ Implementiert (MoltenVK-Hardening ausstehend)` korrigiert — Kern des Vulkan-Backends ist production-ready (Stubs: 0, Quality: 94)
- **docs/de/acceleration/missing-implementations.md**: Revision-Referenz auf aktuellen Branch aktualisiert; Eintrag #6 überarbeitet (bezieht sich jetzt korrekt auf Issue #1388, nicht mehr auf #1387)
- **docs/de/acceleration/missing-implementations.json**: Revision aktualisiert; 6 Einträge → 5 Einträge (ACC-005 Vulkan Double-Buffering zusammengeführt; ACC-006 zu ACC-005 umbenannt mit korrektem Fokus auf #1388)

### ⚠️ Bekannte offene Punkte (nicht in diesem PR)
- **ACC-001** 🔴: `cuda_backend.cpp` hat 6 verbleibende Stubs — CUDA ANN/Vektor-Suche fällt auf CPU zurück
- **ACC-002** 🟡: `opencl_backend.cpp` hat 1 verbleibenden Stub bei `PRODUCTION-READY`-Claim
- **ACC-003** 🟡: `MultiGPUVectorBackend` NCCL Group-Call-Wiring ausstehend
- **ACC-004** 🟢: MoltenVK `VK_KHR_buffer_device_address` Capability-Probe fehlt
- **ACC-005** 🟢: Deterministische Tie-Breaking-Semantik bei Partial-Failure (#1388) fehlt
