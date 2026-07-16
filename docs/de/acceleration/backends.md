# Acceleration Backends

<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/acceleration/ | Überblick: README.md -->

**Stand:** 6. April 2026  
**Version:** 1.0  
**Kategorie:** GPU / Hardware-Beschleunigung

---

## Übersicht

Das Acceleration-Modul unterstützt mehrere GPU- und CPU-Backends, die zur Laufzeit anhand von Geräteeigenschaften ausgewählt werden. Dieses Dokument beschreibt die verfügbaren Backend-Typen, ihre Konfiguration und den Fallback-Mechanismus.

---

## Unterstützte Backends

### CUDA-Backend (`cuda_backend.cpp`)

Das CUDA-Backend ist das primäre GPU-Backend für NVIDIA-Grafikkarten (ab sm_70 / Volta-Architektur).

**Anforderungen:**
- NVIDIA GPU mit Compute Capability ≥ 7.0 (FP16), ≥ 8.0 (BF16)
- CUDA SDK installiert
- Build mit `THEMIS_ENABLE_CUDA=ON`

**Kernelfunktionen:**
- L2-Distanz (cuBLAS Batched GEMM)
- Kosinus-Distanz (fused L2-Norm + Dot-Product-Kernel)
- Top-K Selektion (CUB `DeviceSegmentedSort` für k≤1024, Thrust-Fallback für k>1024)
- Geospatial: Haversine-Distanz, Punkt-in-Polygon (Ray-Casting)

**CUDA Graph Capture** (`CUDAGraphCache`):
- Wiederkehrende Abfragen gleicher Form (`{dim, numQueries, topK, metric}`) werden als CUDA-Graph aufgezeichnet und mit minimalem Overhead wiedergegeben
- LRU-Verdrängung bei mehr als 32 gecachten Graphen
- Zielwert: ≥ 30 % Latenzsenkung für feste Batch-Formen

```cpp
// Backend wird automatisch über BackendRegistry ausgewählt:
auto* backend = BackendRegistry::instance().getSelectedVectorBackend();
// backend → CUDAVectorBackend (wenn CUDA verfügbar)

// Direkte API (wenn bekannt, dass CUDA verwendet wird):
std::vector<SearchResult> results = backend->batchSimilaritySearch(
    queries,       // float* [numQueries × dim], Host-Pointer
    numQueries,
    dim,
    DistanceMetric::L2,
    topK,
    opts
);
```

---

### HIP/ROCm-Backend (`hip_backend.cpp`)

AMD-Äquivalent zum CUDA-Backend für Radeon-Grafikkarten.

**Anforderungen:**
- AMD GPU mit ROCm-Unterstützung
- ROCm SDK installiert
- Build mit `THEMIS_ENABLE_HIP=ON`

**Implementierte Kernel:**
- ANN-Kernels: `hip/ann_kernels.hip`
- Geospatial-Kernels: `hip/geo_kernels.hip`

**Multi-GPU:**
- RCCL-Backend (`rccl_vector_backend.cpp`) spiegelt die NCCL-Schnittstelle für AMD-Systeme

---

### Vulkan-Backend (`vulkan_backend_full.cpp`)

Plattformübergreifendes GPU-Backend für Geräte ohne CUDA/HIP-Unterstützung.

**Anforderungen:**
- Vulkan 1.3-kompatibler Treiber
- Build mit `THEMIS_ENABLE_VULKAN=ON`

**Zielplattformen:**
- ARM Mali-Grafikprozessoren
- Apple M-Series (via MoltenVK)
- AMD RDNA-Architektur (als Fallback zu ROCm)

**Shader-Infrastruktur:**
- GLSL/HLSL-Shader werden zur Build-Zeit in SPIR-V kompiliert (`glslangValidator`)
- Push Constants für `numVectors`, `dim`, `topK` (vermeidet UBO-Neuallokation pro Abfrage)
- Doppelte Staging-Puffer für DMA-Überlappung (Host→Device und Shader-Dispatch)

**Status:** 🚧 Infrastruktur produktionsreif; SPIR-V-Compute-Shader in Entwicklung

---

### FAISS GPU-Backend (`faiss_gpu_backend.cpp`)

Wrapper für die FAISS-Bibliothek von Meta für ANN-Suche im Milliarden-Bereich.

**Anforderungen:**
- NVIDIA GPU mit CUDA-Unterstützung
- FAISS mit GPU-Unterstützung installiert

**Einsatzbereich:** Sehr große Vektorindizes (> 100 Millionen Vektoren), bei denen Standard-HNSW nicht ausreicht.

---

### Multi-GPU-Backend (`multi_gpu_backend.cpp`)

Verteilt große Embedding-Indizes auf mehrere GPUs.

**Anforderungen:**
- Mindestens 2 NVIDIA-GPUs
- NCCL (bei NVIDIA) oder RCCL (bei AMD)

**Funktionsweise:**
- Bereichsbasiertes Sharding: Vektor-IDs werden in gleichmäßige Bereiche aufgeteilt
- Fan-out KNN-Suche: Abfrage wird an alle GPUs verteilt
- Host-seitiger Top-K-Merge: Ergebnisse aller GPUs werden auf der CPU zusammengeführt
- Graceful Degradation: Bei NCCL-Initialisierungsfehler → Fallback auf Single-GPU oder CPU

**Performance-Ziel:**  
100M × 128-dim Index auf 4× A100 80 GB; Abfragelatenz < 15 ms bei p99 für k=100; Skalierungs-Effizienz ≥ 75 % (1→4 GPUs).

---

### CPU-Backends

Alle CPU-Backends sind immer verfügbar und dienen als Fallback, wenn keine GPU verfügbar ist oder ein GPU-Fehler auftritt.

| Backend | Datei | Parallelismus | Einsatz |
|---|---|---|---|
| Single-Thread CPU | `cpu_backend.cpp` | Keiner | Referenz, Korrektheitsbenchmark |
| Pthreads CPU | `cpu_backend_mt.cpp` | POSIX-Threads | Standardmäßiger Multi-Core-Fallback |
| Intel TBB CPU | `cpu_backend_tbb.cpp` | Intel TBB | Optimiert für x86-Systeme mit TBB |

Der CPU-Fallback ist innerhalb von **≤ 2× der handoptimierten SIMD-Baseline** konfiguriert.

---

## Capability Negotiation (Capability-Aushandlung)

`BackendRegistry::initializeRuntime()` durchläuft beim Start den folgenden Capability-Auswahlprozess:

```
1. CUDA  → GPU-Eigenschaften prüfen (Compute Capability, VRAM, Treiber-Version)
2. HIP   → AMD GPU-Eigenschaften prüfen
3. Vulkan → Vulkan-Gerät prüfen
4. Metal  → Apple GPU prüfen (macOS/iOS)
5. OpenCL → OpenCL-Gerät prüfen
6. CPU   → Immer verfügbar (letzter Fallback)
```

Das Backend mit dem höchsten Capability-Score für jede Operationskategorie (Vector, Graph, Geo) wird ausgewählt und gecacht.

**Detaillierte Dokumentation:** → [Capability Negotiation (EN)](../../acceleration/capability_negotiation.md)

---

## Fallback-Kette und Retry-Mechanismus

Das Modul implementiert eine gestufte Fehlerbehandlung über `ANNKernelFallbackDispatcher` und `GeoKernelFallbackDispatcher`:

| Fehlertyp | Behandlung |
|---|---|
| Transienter Gerätefehler (`DeviceLost`, `OperationTimeout`, `SynchronizationFailed`) | Retry mit Exponential-Backoff; nach `maxAttempts` → CPU-Fallback |
| Permanenter Gerätefehler / OOM | Sofortiger CPU-Fallback; strukturierter Fehler-Log |
| Plugin-Signatur-Fehler | Ablehnung des Ladens; Sicherheits-Alert im Log |
| Backend-Initialisierungsfehler | Backend wird in der Auswahl übersprungen; Warnung ausgegeben |
| Kein Backend verfügbar | `getSelected*Backend()` gibt `nullptr` zurück; Aufrufer muss reagieren |

**Retry-Konfiguration:**

| Parameter | Standard | Beschreibung |
|---|---|---|
| `acceleration.retry.max_attempts` | 3 | Maximale Wiederholungsversuche bei transientem GPU-Fehler |
| `acceleration.retry.initial_delay_ms` | 10 | Initiale Wartezeit in Millisekunden |
| `acceleration.retry.backoff_multiplier` | 2.0 | Exponentieller Multiplikator |

---

## Plugin-System

Das Acceleration-Modul unterstützt externe Backend-Plugins (z. B. `zluda_backend.cpp` für AMD auf CUDA-API, `oneapi_backend.cpp` für Intel oneAPI).

**Sicherheitsanforderungen:**
- Plugins müssen mit Ed25519/SHA-256 signiert sein
- `plugin_security.cpp` prüft die Signatur **vor** dem Laden (`dlopen`)
- Nur Backends auf der Allowlist können geladen werden
- Plugin-ABI ist bis v2.0 nicht stabil; Breaking Changes möglich

**SPIR-V-Shader-Integrität:**
- `shader_integrity.cpp` verifiziert SPIR-V-Shader vor der Pipeline-Erstellung (Vulkan-Backend)

---

## Betriebliche Hinweise

### Initialisierungs-Reihenfolge

```cpp
// 1. Einmalig beim Single-Threaded Server-Start:
BackendRegistry::instance().initializeRuntime();

// 2. Danach ist paralleler Zugriff aus mehreren Threads sicher:
auto* vectorBackend = BackendRegistry::instance().getSelectedVectorBackend();
auto* geoBackend    = BackendRegistry::instance().getSelectedGeoBackend();
auto* graphBackend  = BackendRegistry::instance().getSelectedGraphBackend();

// 3. Beim Herunterfahren:
BackendRegistry::instance().shutdownAll();
```

### Bekannte Einschränkungen

- CUDA ANN-Backends noch in Entwicklung; ANN-Vektoroperationen werden an CPU weitergeleitet (HNSW-Integration noch ausstehend)
- Tensor Core (`CUDAMatrixBackend`) erfordert CUDA-Gerät mit SM 7.0+ (FP16) bzw. SM 8.0+ (BF16)
- Multi-GPU-Sharding nutzt aktuell CPU-Sub-Backends; echte CUDA-Kernels noch ausstehend
- DirectX- und Metal-Backends sind experimentell
- Plugin-ABI-Stabilität ab v2.0 garantiert; Breaking Changes vorher möglich

---

## Verwandte Dokumentation

- [Modulübersicht](README.md) — Einführung in das Acceleration-Modul
- [Capability Negotiation (EN)](../../acceleration/capability_negotiation.md) — Technische Tiefgang-Dokumentation
- [Troubleshooting (EN)](../../acceleration/troubleshooting.md) — Betriebliches Fehlerbehebungshandbuch
- [ARCHITECTURE](../../../src/acceleration/ARCHITECTURE.md) — Architektur-Leitfaden mit Komponentendiagramm
- [FUTURE_ENHANCEMENTS](../../../src/acceleration/FUTURE_ENHANCEMENTS.md) — Geplante Features mit Performance-Zielen
