# Acceleration Module
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/acceleration/README.md -->

<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/acceleration/ -->

**Stand:** 6. April 2026  
**Version:** 1.1  
**Kategorie:** GPU / Hardware-Beschleunigung  
**Validated:** 2026-05-10 (Reality-Check gegen Sourcecode; siehe [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md))

---

## Übersicht

Das Acceleration-Modul stellt hardware-beschleunigte Compute-Backends für ThemisDB bereit. Es beschleunigt rechenintensive Primitive (Vektorsimilarität / ANN-Suche, Graph-Analytik, Geospatial-Operatoren) bei gleichzeitiger Beibehaltung von **Korrektheit**, **Determinismus** und einem **CPU-Fallback**, wenn kein geeigneter Beschleuniger verfügbar ist.

**Wichtigste Eigenschaften:**

- Laufzeit-Backend-Auswahl (GPU/CPU) ohne Build-Pflichtabhängigkeiten
- Stabile `ComputeBackend`-Schnittstelle — Verbraucher kennen keine CUDA/Vulkan-Details
- Optionaler Build: alle GPU-Pfade sind durch Feature-Flags abgesichert
- Gestufte Fehlerbehandlung: transiente GPU-Fehler → Retry mit Exponential-Backoff → CPU-Fallback

---

## Source-Code Referenz

### Implementierung (`src/acceleration/`)

| Datei / Komponente | Rolle |
|---|---|
| `backend_registry.cpp` | Singleton-Registry: Erkennung, Capability-Scoring, Laufzeitauswahl |
| `compute_backend.cpp` | Abstrakte `ComputeBackend`-Basisklasse und gemeinsame Hilfsfunktionen |
| `device_manager.cpp` | Geräte-Enumeration, Capability-Probing, 60 s TTL-Cache |
| `cuda_backend.cpp` + `cuda/` | CUDA-Kernels und Stream-Management (erfordert `THEMIS_ENABLE_CUDA`) |
| `hip_backend.cpp` + `hip/` | AMD HIP/ROCm-Backend (erfordert `THEMIS_ENABLE_HIP`) |
| `vulkan_backend_full.cpp` + `vulkan/` | Vulkan Compute-Pipeline (erfordert `THEMIS_ENABLE_VULKAN`) |
| `opencl_backend.cpp` | OpenCL-Backend für breite Hardware-Kompatibilität |
| `metal_backend.mm` | Apple Metal-Backend (macOS/iOS) |
| `directx_backend_full.cpp` | DirectX Compute-Backend (Windows) |
| `faiss_gpu_backend.cpp` | FAISS GPU-Wrapper für ANN-Suche im Milliarden-Bereich |
| `multi_gpu_backend.cpp` | Multi-GPU-Lastverteilung und Arbeitsverteilung |
| `nccl_vector_backend.cpp` | NCCL Kollektiv-Operationen (NVIDIA Multi-GPU) |
| `rccl_vector_backend.cpp` | RCCL Kollektiv-Operationen (AMD Multi-GPU) |
| `tensor_core_matmul.cpp` | Tensor Core FP16/BF16 Matrix-Multiplikation |
| `geo_acceleration_bridge.cpp` | Brücke zwischen Geospatial-Operatoren und Acceleration-Schicht |
| `cpu_backend.cpp` / `cpu_backend_mt.cpp` / `cpu_backend_tbb.cpp` | CPU-Referenzimplementierungen (single-thread, pthreads, TBB) |
| `plugin_loader.cpp` | Dynamisches Laden externer Backend-Plugins |
| `plugin_security.cpp` | Signaturprüfung für geladene Plugins |
| `vllm_resource_manager.cpp` | GPU-VRAM-Ressourcenverwaltung für LLM-Inferenz-Pfade |

### Öffentliche Header (`include/acceleration/`)

| Header | Rolle |
|---|---|
| `compute_backend.h` | Abstrakte `ComputeBackend`-Klasse und `DeviceCapabilityInfo` |
| `device_manager.h` | Geräte-Enumeration und Capability-Probing-API |
| `kernel_fallback_dispatcher.h` | `ANNKernelFallbackDispatcher` und `GeoKernelFallbackDispatcher` |
| `kernel_invocation.h` | Eingefrorene ANN- und Geospatial-Kernel-Dispatch-Tabellen (`INTERFACE_VERSION = 100`) |
| `batch_validator.h` | Eingabe-Validierungshelfer für alle Backends |
| `graphics_backends.h` | `DirectXVectorBackend`, `VulkanVectorBackend`, `VulkanGeoBackend`, `OpenGLVectorBackend` |
| `error_codes.h` / `error_context.h` | Strukturierte Fehler-Taxonomie |

**Unterverzeichnisse:**

- `raii/` — Header-only RAII-Wrapper für GPU-Ressourcen (`CudaStream`, `CudaDeviceMemory`, OpenCL- und Vulkan-Ressourcen); siehe [`include/acceleration/raii/README.md`](../../../include/acceleration/raii/README.md)
- `metrics/` — Backend-Metriken (`BackendMetrics`, `MetricsCollector`, `Counter`, `Gauge`, `Histogram`, `Timer`)

**Gesamt:** Aktueller Bestand und Referenzen werden in [`src/MODULE_FUNCTION_USAGE_MAP.md`](../../../src/MODULE_FUNCTION_USAGE_MAP.md) gepflegt (inkl. Header-/Implementierungsübersicht).

---

## Backend-Übersicht

| Backend | Plattform | Feature-Flag | Status |
|---|---|---|---|
| CUDA | NVIDIA GPU (sm_70+) | `THEMIS_ENABLE_CUDA` | 🚧 Release-Candidate |
| HIP/ROCm | AMD GPU | `THEMIS_ENABLE_HIP` | ✅ Implementiert |
| Vulkan | Plattformübergreifend (Mali, Apple M, AMD) | `THEMIS_ENABLE_VULKAN` | 🚧 In Bearbeitung |
| OpenCL | Breite Hardware-Unterstützung | — | ✅ Implementiert |
| Metal | Apple macOS/iOS | — | 🧪 Experimentell |
| DirectX | Windows | — | 🧪 Experimentell |
| FAISS GPU | NVIDIA (über FAISS) | `THEMIS_ENABLE_CUDA` | ✅ Implementiert |
| Multi-GPU | NVIDIA (NCCL) / AMD (RCCL) | `THEMIS_ENABLE_CUDA` | ✅ Implementiert |
| CPU (Single-Thread) | Alle Plattformen | — | ✅ Referenz-Implementierung |
| CPU (pthreads / TBB) | Alle Plattformen | — | ✅ Produktionsreif |

---

## Laufzeitverhalten

### Initialisierung

```cpp
// Einmalig beim Single-Threaded Server-Start aufrufen:
BackendRegistry::instance().initializeRuntime();

// Nach initializeRuntime() können mehrere Threads gleichzeitig auf Backends zugreifen:
auto* backend = BackendRegistry::instance().getSelectedVectorBackend();
```

### Backend-Auswahl

Die `BackendRegistry` wählt automatisch das beste verfügbare Backend anhand von Capability-Scoring:

1. `autoDetect()` — alle verfügbaren Backends (inkl. Plugins) erkennen
2. `scoreCapabilities(requirements)` — für jede Kategorie (Vector, Graph, Geo) bewerten
3. Auswahl des am höchsten bewerteten Backends pro Kategorie
4. Ergebnisse werden gecacht → Zugriff via `getSelectedVectorBackend()`, `getSelectedGraphBackend()`, `getSelectedGeoBackend()`

### Graceful Degradation

```
GPU-Backend verfügbar?
    ├─ Ja  → GPU-Kernel ausführen
    │         ├─ Erfolg → Ergebnis zurückgeben
    │         └─ Transienter Fehler (DeviceLost, Timeout)
    │               → Exponential-Backoff-Retry (max. 3 Versuche)
    │               → Nach Erschöpfung → CPU-Fallback
    └─ Nein → CPU-Fallback sofort
```

---

## Build-Konfiguration

```cmake
# Optionale GPU-Backends aktivieren:
cmake -DTHEMIS_ENABLE_CUDA=ON \
      -DTHEMIS_ENABLE_VULKAN=ON \
      -DTHEMIS_ENABLE_HIP=ON \
      ..

# Ohne GPU-SDKs muss der Build ebenfalls erfolgreich sein:
cmake -DTHEMIS_ENABLE_CUDA=OFF -DTHEMIS_ENABLE_VULKAN=OFF ..
```

| CMake-Flag | Standard | Beschreibung |
|---|---|---|
| `THEMIS_ENABLE_CUDA` | OFF | CUDA-Backend und Kernel-Kompilierung aktivieren |
| `THEMIS_ENABLE_VULKAN` | OFF | Vulkan-Backend und Shader-Kompilierung aktivieren |
| `THEMIS_ENABLE_HIP` | OFF | HIP/ROCm-Backend aktivieren |

---

## Performance-Ziele

| Metrik | Zielwert | Methode |
|---|---|---|
| L2-Suche 1M×128 (CUDA, RTX 3090) | < 8 ms | `benchmarks/vector_bench.cpp` |
| Cosinus-Suche 500K×128 (Vulkan/MoltenVK, M2 Pro) | < 20 ms | Manueller Benchmark |
| Multi-GPU Skalierungs-Effizienz (1→4× A100) | ≥ 75 % | `benchmarks/multi_gpu_bench.cpp` |
| CUDA-Graph-Replay Latenz-Reduktion | ≥ 30 % | Feste Batch-Form, `vector_bench.cpp` |
| Gerät-Probing (4-GPU-System) | < 50 ms | `tests/acceleration/device_probe_test.cpp` |
| CPU-Fallback-Overhead vs. SIMD-Baseline | ≤ 2× | `benchmarks/vector_bench.cpp` |

---

## Sicherheit

- **Plugin-Signierung**: `plugin_security.cpp` verifiziert Ed25519/SHA-256-Signaturen externer Backend-Plugins vor dem Laden
- **Sandbox-Allowlist**: Nur explizit genehmigte Backends können geladen werden
- **Keine Nutzerdaten in Logs**: Capability-Probing protokolliert nur Hardware-Metadaten
- **GPU-Speicher-Nullung**: GPU-Puffer werden vor der Übergabe an Abfrageergebnisse auf null gesetzt

---

## Verwandte Dokumentation

### Primärdokumentation (Quellcode)

- [README (src/acceleration)](../../../src/acceleration/README.md) — Modulübersicht und Entwicklerleitfaden
- [ARCHITECTURE (src/acceleration)](../../../src/acceleration/ARCHITECTURE.md) — Architektur-Leitfaden mit Komponentendiagramm
- [ROADMAP (src/acceleration)](../../../src/acceleration/ROADMAP.md) — Entwicklungs-Roadmap und Produktionsreife-Checkliste
- [FUTURE_ENHANCEMENTS (src/acceleration)](../../../src/acceleration/FUTURE_ENHANCEMENTS.md) — Geplante Features mit Performance-Zielen
- [Header-README (include/acceleration)](../../../include/acceleration/README.md) — Öffentliche API-Header-Übersicht

### Technische Tiefgang-Dokumentation

- [Capability Negotiation](../../acceleration/capability_negotiation.md) — Backend-Auswahlprozess und Fallback-Kette
- [Troubleshooting](../../acceleration/troubleshooting.md) — Betriebliches Fehlerbehebungshandbuch
- [Fehler-Codes](../../acceleration/error_codes.md) — Strukturierte Fehler-Taxonomie

### Reality-Check & Offene Implementierungen

- [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md) — Reality-Check-Bericht: fehlende/unvollständige Implementierungen mit Evidence, Impact und Issue-Vorschlägen (Stand 2026-03-09)
- [missing-implementations.json](missing-implementations.json) — Maschinenlesbares Format des obigen Berichts

### Verwandte Module

- [GPU-Modul (src)](../../../src/gpu/README.md) — Low-Level GPU-Geräteerkennung und Treiber-Wrapper
- [Geo-Modul](../geo/README.md) — Geospatial-Operatoren, deren GPU-Pfad durch `geo_acceleration_bridge.cpp` läuft
- [Graph-Modul (src)](../../../src/graph/README.md) — Graph-Analytik-Engine mit GPU-beschleunigtem Traversal

---

## Weitere Themen

- [Backend-Typen und Konfiguration](backends.md) — Detaillierte Beschreibung der unterstützten Backends
- [Fehlende Implementierungen](MISSING_IMPLEMENTATIONS.md) — Reality-Check-Report: offene Implementierungslücken mit Code-Evidence und Issue-Vorschlägen
