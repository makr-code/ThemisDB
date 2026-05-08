# GPU-Modul – Primäres Inventar

**Datum:** März 2026  
**Modul:** `gpu`  
**Modulpfad:** `src/gpu/`

---

## 1. Dokumentationsdateien im Modul

| Datei | Beschreibung |
|---|---|
| `src/gpu/README.md` | Modulübersicht, Komponentenliste, Architekturdiagramm, Edition-Limits, Versionshistorie |
| `src/gpu/ARCHITECTURE.md` | Detaillierte Architektur: Design-Prinzipien, Komponententabelle, Datenfluss, Threading, Performance, Sicherheit, Konfiguration, Fehlerbehandlung |
| `src/gpu/ROADMAP.md` | Implementierungsstatus, abgeschlossene Features, geplante Features, Phasenmodell, Production-Readiness-Checkliste, Breaking Changes |
| `src/gpu/FUTURE_ENHANCEMENTS.md` | Geplante Erweiterungen mit Implementierungsdetails, Test-Strategie, Performance-Zielen, Sicherheitsanforderungen und wissenschaftlichen IEEE-Referenzen |

---

## 2. Quellcode-Dateien (`src/gpu/`)

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `admin_api.cpp` | `include/themis/gpu/admin_api.h` | JSON Admin-Stats, Tenant-Aufschlüsselung, Dry-Run-Simulation |
| `alerts.cpp` | `include/themis/gpu/alerts.h` | Schwellenwert-basierter Alert-Manager mit Callbacks |
| `audit_log.cpp` | `include/themis/gpu/audit_log.h` | Ring-Buffer-strukturiertes GPU-Audit-Event-Log |
| `cluster_coordinator.cpp` | `include/themis/gpu/cluster_coordinator.h` | Multi-Node-GPU-Cluster-Koordination: Heartbeat, Health-Tracking, Least-Loaded-Scheduling |
| `cluster_topology.cpp` | `include/themis/gpu/cluster_topology.h` | NVLink/InfiniBand-Topologieerkennung und bevorzugtes Interconnect-Routing |
| `config.cpp` | `include/themis/gpu/config.h` | GPU-Konfigurationsvalidierung, Dry-Run-Simulation |
| `device_discovery.cpp` | `include/themis/gpu/device_discovery.h` | CUDA/ROCm-Geräteaufzählung; CPU-Fallback-Sentinel |
| `feature_flags.cpp` | `include/themis/gpu/feature_flags.h` | Pro-Edition-GPU-Feature-Gates mit Runtime-Overrides |
| `gpu_memory_manager_edition.cpp` | `include/themis/gpu/memory_manager.h` | Editions-bewusster VRAM-Slab-Allocator mit Tenant-Quoten |
| `gpu_module.cpp` | `include/themis/gpu/gpu_module.h` | Integrations-Façade: Policy → Circuit-Breaker → Alloc → Launch |
| `graph_cache.cpp` | `include/themis/gpu/graph_cache.h` | LRU CUDA-Graph-Capture-Cache (QueryShape: OpType × rows × param_hash) |
| `kernel_validator.cpp` | `include/themis/gpu/kernel_validator.h` | FNV-1a-Checksum-Whitelist; Validate-before-Launch |
| `launcher.cpp` | `include/themis/gpu/launcher.h` | Typisierter asynchroner Work-Item-/Batch-Launcher |
| `load_balancer.cpp` | `include/themis/gpu/load_balancer.h` | Multi-GPU-Dispatch: ROUND_ROBIN / LEAST_LOADED / FIRST_HEALTHY |
| `memory_pool.cpp` | `include/themis/gpu/memory_pool.h` | Slab-basierter Pre-Allocator mit Fragmentierungstracking |
| `metrics.cpp` | `include/themis/gpu/metrics.h` | Prometheus-kompatible Counter/Gauge-Registry |
| `mig_manager.cpp` | `include/themis/gpu/mig_manager.h` | MIG-Partitions-Lifecycle für NVIDIA Ampere/Hopper (A/H-Serie) |
| `p2p_transfer.cpp` | `include/themis/gpu/p2p_transfer.h` | Peer-to-Peer-GPU-zu-GPU-Direktübertragungen via NVLink/PCIe |
| `policy.cpp` | `include/themis/gpu/policy.h` | Default-Deny-Capability-Gate für GPU-Nutzung |
| `profiler.cpp` | `include/themis/gpu/profiler.h` | NVTX/rocTX-Profiling-Marker für NVIDIA Nsight und ROCm Profiler |
| `query_accelerator.cpp` | `include/themis/gpu/query_accelerator.h` | Paralleles Scan/Filter/Sort/Aggregate/Join; ANN-Suche via cuVS/RAFT |
| `rocm_backend.cpp` | `include/themis/gpu/rocm_backend.h` | ROCm/HIP-Stream-Lifecycle und Gerätespeicher |
| `safe_fail.cpp` | `include/themis/gpu/safe_fail.h` | Circuit-Breaker-Safe-Fail mit GPU→CPU-Fallback |
| `stream_manager.cpp` | `include/themis/gpu/stream_manager.h` | Benannte asynchrone GPU-Streams mit CPU-Fallback-Budget |
| `tensor_buffer.cpp` | `include/themis/gpu/tensor_buffer.h` | Typisierte Tensor-Container mit Shape/Dtype, Views, Checkpointing |
| `time_slice_scheduler.cpp` | `include/themis/gpu/time_slice_scheduler.h` | Round-Robin-Pro-Tenant-GPU-Time-Slice-Dispatcher |
| `training_loop.cpp` | `include/themis/gpu/training_loop.h` | Training-Loop-Koordinator: Batch-Iteration, Loss-Tracking, Early Stopping |
| `unified_memory.cpp` | `include/themis/gpu/unified_memory.h` | CUDA/HIP Unified-Memory-Allocator mit CPU-Fallback |
| `vulkan_backend.cpp` | `include/themis/gpu/vulkan_backend.h` | Plattformübergreifendes Vulkan-Compute-Backend (AMD/Intel/NVIDIA via SPIR-V) |
| `wasm_kernel_sandbox.cpp` | `include/themis/gpu/wasm_kernel_sandbox.h` | WASM-basierte Sandbox für nicht vertrauenswürdige GPU-Kernel-Blobs |

---

## 3. Sekundäre Dokumentation (docs/de/gpu/)

| Datei | Beschreibung |
|---|---|
| `docs/de/gpu/GAP_ANALYSE_GPU_VRAM_NUTZUNG.md` | Gap-Analyse: GPU/VRAM-Nutzung (Dokumentation vs. Implementierung) |
| `docs/de/gpu/GPU_DEFAULT_ENABLED_CHANGES.md` | Änderungen an GPU-Default-Aktivierung |
| `docs/de/gpu/GPU_VRAM_QUICK_REFERENCE.md` | VRAM-Schnellreferenz nach Edition |
| `docs/de/gpu/inventory.md` | Dieses Inventardokument |

---

## 4. Weiterführende Dokumentation

| Datei | Beschreibung |
|---|---|
| `docs/gpu_roadmap.md` | Produktionsreife-Bewertung und vollständige Roadmap |
| `docs/gpu_runbooks.md` | Operative Runbooks für GPU-Vorfälle |
| `docs/GPU_KERNEL_IMPLEMENTATION_GUIDE.md` | Leitfaden zur Kernel-Implementierung |
| `research/GPU_VECTOR_INDEXING_RESEARCH.md` | Forschungsstand GPU-basiertes Vektor-Indexing |

---

## 5. Reality-Check-Ergebnis (Stand: März 2026)

### ✅ Korrekt dokumentiert
- Alle 30 Quelldateien und ihre zugehörigen Header sind vorhanden
- Edition-basierte VRAM-Limits stimmen mit `gpu_memory_manager_edition.cpp` überein
- Alle komponierten Interfaces im README-`Components`-Abschnitt stimmen
- ROADMAP-Status (Completed/In Progress/Planned) spiegelt den Implementierungsstand korrekt wider

### 🔧 Korrigiert (in diesem PR)
- `README.md` Abschnitt "Relevant Interfaces": falsche Dateinamen korrigiert
  - `gpu_memory_manager.cpp` → `gpu_memory_manager_edition.cpp`
  - `circuit_breaker.cpp` → `safe_fail.cpp`
  - `device_enumerator.cpp` → `device_discovery.cpp`
  - `gpu_query_accelerator.cpp` → `query_accelerator.cpp`
  - `gpu_metrics.cpp` → `metrics.cpp`
- `ARCHITECTURE.md` Abschnitt 3.1: Komponentenliste um 10 neue Komponenten erweitert
- `ARCHITECTURE.md` Abschnitt 3.2: Komponentendiagramm aktualisiert
- `ARCHITECTURE.md` Abschnitt 11: Bekannte Einschränkungen aktualisiert (erledigte Features entfernt)
- `ROADMAP.md` Phase 1: falsche Dateinamen korrigiert
  - `gpu/vram_allocator.cpp` → `gpu/gpu_memory_manager_edition.cpp`
  - `gpu/circuit_breaker.cpp` → `gpu/safe_fail.cpp`
- `FUTURE_ENHANCEMENTS.md`: 23 IEEE-Referenzen hinzugefügt

### ⚠️ Bekannte Einschränkungen (Simulation vs. Hardware)
Alle Features sind als CPU-Simulation implementiert. Echte GPU-Hardware ist für folgende Pfade erforderlich:
- `cudaMalloc` / `hipMalloc` in `GPUMemoryManager`
- `cudaGraph_t` in `GPUGraphCache`
- `nvmlDeviceCreateGpuInstance` in `MIGManager`
- `vkQueueSubmit` in `VulkanComputeBackend`
- `cudaMemcpyPeer` / `hipMemcpyPeer` in `GPUP2PTransferManager`
