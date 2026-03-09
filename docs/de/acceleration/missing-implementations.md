# Acceleration Module — Missing Implementations Report

**Validiert:** 2026-03-09  
**Geprüfte Revision:** `HEAD` (`copilot/check-gpu-documentation-status`)  
**Geprüfte Pfade:** `src/acceleration/`, `include/acceleration/`  
**Methode:** Reality-Check (Doku ↔ Sourcecode); Suche nach `STUB`, `TODO`, `NOT_IMPLEMENTED`; Zeilenzählung via `wc -l`; Kernel-Count via `__global__`-Suche

---

## Zusammenfassung

| Schwere | Anzahl |
|---------|--------|
| 🔴 Kritisch (Produktionsblocker) | 1 |
| 🟡 Mittel (Funktional eingeschränkt) | 2 |
| 🟢 Gering (Hardening / Optimierung) | 2 |

---

## Einträge

### 1. CUDA Vector/ANN Kernel Production Wiring (🔴 Kritisch)

**Claim-Quelle:** `src/acceleration/ROADMAP.md` → Completed ✅, Phase 2 → `[~]`  
**Datei:** `src/acceleration/cuda_backend.cpp`  

**Erwartet:** `cuda/vector_kernels.cu` (396 Zeilen, 9× `__global__`) und `cuda/ann_kernels.cu` (397 Zeilen, 4× `__global__`) vollständig in `cuda_backend.cpp` verdrahtet; alle Kernel-Launch-Funktionen echte CUDA-Calls.

**Beobachtet:** `cuda_backend.cpp` Header-Metadaten: `Stubs: 6`. Die Funktionen `launchL2DistanceKernel`, `launchCosineDistanceKernel`, `launchTopKKernel` etc. (Zeilen 41–55 in `cuda_backend.cpp`) rufen die `.cu`-Kernel noch nicht produktiv auf.

**Evidence:**
- `src/acceleration/cuda/vector_kernels.cu` — 396 Zeilen, 9 `__global__` Kernel implementiert ✅
- `src/acceleration/cuda/ann_kernels.cu` — 397 Zeilen, 4 `__global__` Kernel implementiert ✅
- `src/acceleration/cuda_backend.cpp` — `Open Issues: Stubs: 6`

**Impact:** CUDA ANN- und Vektor-Suche fällt auf CPU-Backend zurück; GPU-Beschleunigung nicht aktiv.

**Issue-Titelvorschlag:** `feat(acceleration): wire cuda/vector_kernels.cu and ann_kernels.cu into cuda_backend production dispatch`  
**Label-Vorschläge:** `module:acceleration`, `kind:implementation`, `priority:high`

---

### 2. OpenCL Backend Production Completeness (🟡 Mittel)

**Claim-Quelle:** `src/acceleration/ROADMAP.md` → Planned (war `[I]`), jetzt `[ ]`  
**Datei:** `src/acceleration/opencl_backend.cpp`  

**Erwartet:** Vollständiges OpenCL-Backend ohne Stubs (Maturity: PRODUCTION-READY deklariert).

**Beobachtet:** `opencl_backend.cpp` — `Open Issues: Stubs: 1`. Datei-Metadaten behaupten `🟢 PRODUCTION-READY`, aber 1 Stub bleibt.

**Evidence:**
- `src/acceleration/opencl_backend.cpp`, Zeile 14: `Stubs: 1`
- Maturity: `🟢 PRODUCTION-READY` — widersprüchlich

**Impact:** OpenCL-Backend nicht vollständig produktionsfähig; könnte bei bestimmten Hardware-Konfigurationen unerwartetes Verhalten erzeugen.

**Issue-Titelvorschlag:** `fix(acceleration): remove remaining stub in opencl_backend.cpp`  
**Label-Vorschläge:** `module:acceleration`, `kind:bug`, `priority:medium`

---

### 3. MultiGPUVectorBackend NCCL Group-Call Wiring (🟡 Mittel)

**Claim-Quelle:** `src/acceleration/FUTURE_ENHANCEMENTS.md` → Multi-GPU Sharding Abschnitt  
**Datei:** `src/acceleration/multi_gpu_backend.cpp`, `src/acceleration/nccl_vector_backend.cpp`  

**Erwartet:** `MultiGPUVectorBackend` mit echten NCCL `ncclGroupStart`/`ncclGroupEnd` Aufrufen für cross-GPU Transfers.

**Beobachtet:** FUTURE_ENHANCEMENTS Eintrag: `[~] Use ncclGroupStart / ncclGroupEnd to batch cross-GPU transfers. (NCCL/RCCL backends initialized; actual group-call wiring is v2.5+ pending real CUDA kernels)` — aktuell CPU Sub-Backends.

**Evidence:**
- `src/acceleration/FUTURE_ENHANCEMENTS.md`, Zeile 110: `[~]` mit Kommentar über v2.5+ pending

**Impact:** Multi-GPU Skalierung läuft über CPU-Backends; NCCL-Kollektiv-Performance nicht nutzbar.

**Issue-Titelvorschlag:** `feat(acceleration): implement NCCL/RCCL group-call wiring in MultiGPUVectorBackend`  
**Label-Vorschläge:** `module:acceleration`, `kind:implementation`, `priority:medium`

---

### 4. MoltenVK Compatibility Probe in Vulkan Backend (🟢 Gering)

**Claim-Quelle:** `src/acceleration/FUTURE_ENHANCEMENTS.md` → Vulkan Abschnitt  
**Datei:** `src/acceleration/vulkan_backend_full.cpp`  

**Erwartet:** Capability Probe für `VK_KHR_buffer_device_address` bei MoltenVK (Apple Silicon).

**Beobachtet:** Probe nicht implementiert; `vulkan_backend_full.cpp` enthält keinen MoltenVK-spezifischen Guard.

**Evidence:**
- `src/acceleration/FUTURE_ENHANCEMENTS.md`: `[ ] MoltenVK path: disable VK_KHR_buffer_device_address if not available`
- `src/acceleration/vulkan_backend_full.cpp`: `Stubs: 0` — Vulkan-Backend ist sonst production-ready

**Impact:** Mögliche Fehler auf Apple Silicon (M1/M2/M3) wenn MoltenVK `VK_KHR_buffer_device_address` nicht unterstützt.

**Issue-Titelvorschlag:** `fix(acceleration): add MoltenVK VK_KHR_buffer_device_address capability probe in vulkan_backend_full.cpp`  
**Label-Vorschläge:** `module:acceleration`, `kind:compatibility`, `priority:low`

---

### 5. Deterministische Tie-Breaking-Semantik bei Partial-Failure (🟢 Gering)

**Claim-Quelle:** `src/acceleration/ROADMAP.md` → Phase 3, Issue: #1388  
**Datei:** `include/acceleration/kernel_fallback_dispatcher.h`, `src/acceleration/cuda_backend.cpp`  

**Erwartet:** Deterministische Tie-Breaking-Semantik bei Partial-Failure (mehrere Backends liefern unterschiedliche Top-K Ergebnisse mit gleichem Score).

**Beobachtet:** Fallback/Retry-Semantik (`ANNKernelFallbackDispatcher`, `GeoKernelFallbackDispatcher`) ist vollständig implementiert. Nur die deterministische Tie-Breaking-Behandlung bei Partial-Failure fehlt noch (Issue: #1388, Phase 3 `[I]`).

**Evidence:**
- `include/acceleration/kernel_fallback_dispatcher.h`: Retry + CPU Fallback implementiert ✅
- `src/acceleration/ROADMAP.md` Phase 3: `[I]` für Issue #1388 — offen
- Duplicate-Entry `[ ] Fallback/retry semantics... (Issue: #1387)` in Long-term Planned Features wurde behoben (Issue #1387 ist `[x]`, Eintrag entfernt)

**Impact:** Bei gleichem Score und unterschiedlichen Backends kann die Ergebnis-Reihenfolge nicht-deterministisch sein; relevant für Reproduzierbarkeit von Testergebnissen.

**Issue-Titelvorschlag:** `feat(acceleration): implement deterministic tie-breaking for partial-failure across backends (Issue #1388)`  
**Label-Vorschläge:** `module:acceleration`, `kind:implementation`, `priority:low`

---

## Nicht gefundene Claims (positiv verifiziert)

Die folgenden ROADMAP-Claims wurden überprüft und als korrekt befunden:

| Claim | Evidence |
|-------|----------|
| CUDA geospatial kernels (`cuda/geo_kernels.cu`) | 396 Zeilen, Haversine + Ray-Casting implementiert ✅ |
| Vulkan shaders (`vulkan/shaders/*.comp`) | 7 GLSL-Compute-Shader implementiert ✅ |
| BatchValidator utility (`include/acceleration/batch_validator.h`) | Datei existiert ✅ |
| `BACKEND_CONTRACT_VERSION = 100` (`compute_backend.h`) | Verifiziert ✅ |
| `KERNEL_INVOCATION_INTERFACE_VERSION = 100` (`kernel_invocation.h`) | Verifiziert ✅ |
| Plugin security hardening (`plugin_security.cpp`) | `RTLD_NOW`, file-permission check, size cap implementiert ✅ |
| Device Manager TTL-Cache (`device_manager.cpp`) | `kCacheTTL = 60 s`, `refresh()` implementiert ✅ |
| HIP ANN + Geo kernels | `hip/ann_kernels.hip`, `hip/geo_kernels.hip` existieren ✅ |
| `CUDAGraphCache` + `batchKnnSearchWithGraph()` | In `cuda_backend.h`/`cuda_backend.cpp` implementiert ✅ |
| `MultiGPUVectorBackend` range-based sharding | `multi_gpu_backend.cpp` implementiert ✅ |
| Tensor Core FP16/BF16 (`tensor_core_matmul.cpp` + `cuda/tensor_core_matmul.cu`) | Beide Dateien existieren ✅ |
