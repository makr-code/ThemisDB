> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# GPU Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — GPU memory management, device discovery, safe-fail circuit breaker, audit logging, policy enforcement, kernel validation, metrics, multi-GPU load balancing, query acceleration, ROCm/HIP backend parity, memory defragmentation, multi-node GPU cluster coordination with NVLink/InfiniBand topology awareness, GPU profiling integration (NVIDIA Nsight / ROCm Profiler), GPU-accelerated ANN (vector similarity) via cuVS/RAFT, MIG (Multi-Instance GPU) partitioning for NVIDIA A/H series, Vulkan compute backend, and peer-to-peer GPU-to-GPU direct transfers (NVLink/PCIe) are implemented.

## Completed ✅
- [x] Edition-aware VRAM allocation with tenant quotas and pre-allocation hints
- [x] CUDA/ROCm device enumeration with CPU-fallback sentinel
- [x] Circuit-breaker safe-fail with GPU→CPU fallback
- [x] Ring-buffer structured audit event log
- [x] Default-deny capability gate for GPU usage
- [x] Slab-based memory pre-allocator with fragmentation tracking
- [x] Prometheus-compatible counter/gauge metrics registry
- [x] GPU config validation with dry-run simulation
- [x] FNV-1a checksum kernel whitelist (validate-before-launch)
- [x] Threshold-based alert manager with callbacks
- [x] Typed async work-item and batch launcher
- [x] Multi-GPU load balancer (ROUND_ROBIN, LEAST_LOADED, FIRST_HEALTHY)
- [x] Per-edition GPU feature gates with runtime overrides
- [x] JSON admin stats API with tenant breakdown and dry-run simulation
- [x] Integration facade: policy → circuit-breaker → alloc → launch pipeline
- [x] Named async GPU streams with CPU fallback budget
- [x] CUDA stream creation in `GPUStreamManager` for long-running workloads
- [x] Parallel scan/filter/sort/aggregate/join with GPU threshold dispatch
- [x] Typed tensor containers with shape/dtype, views, and checkpointing
- [x] CUDA stream creation in `GPUStreamManager` (`gpu/stream_manager.cpp`)
- [x] Training loop coordinator with batch iteration, loss tracking, and early stopping
- [x] ROCm/HIP backend parity with CUDA feature set (`gpu/rocm_backend.cpp`)
- [x] GPU memory defragmentation routine (`gpu/memory_pool.cpp`)
- [x] Multi-node GPU cluster coordination with NVLink/InfiniBand topology awareness (`gpu/cluster_topology.cpp`, `gpu/cluster_coordinator.cpp`)
- [x] GPU profiling integration with NVIDIA Nsight (NVTX markers) and ROCm Profiler (rocTX markers) (`gpu/profiler.cpp`)
- [x] GPU-accelerated ANN (vector similarity) via cuVS/RAFT — infrastructure with CPU brute-force fallback and cuVS/RAFT stub (`gpu/query_accelerator.cpp`, Issue: #2381)
- [x] Multi-node GPU cluster coordination (`gpu/cluster_coordinator.cpp`)
- [x] ROCm/HIP full feature parity (memory manager, kernel validator, launcher) (Issue: #1786)
- [x] GPU memory defragmentation for long-running workloads (Issue: #1787)
- [x] CUDA graph capture for recurring query execution patterns (Issue: #2379 — core implementation; Issue: #1801 — `createCudaStream`, tests, and CMake registration)
  - Implementation: `include/themis/gpu/graph_cache.h`, `src/gpu/graph_cache.cpp`, `include/themis/gpu/query_accelerator.h`, `src/gpu/query_accelerator.cpp`
  - `GPUGraphCache` captures recurring `QueryShape` tuples (OpType × row_count × param_hash); LRU eviction at 32 entries
  - `GPUQueryAccelerator` integrates graph cache in scan, sort, aggregate, hashJoin, annSearch operations
  - `GPUStreamManager::createCudaStream()` provides first-class CUDA stream creation with CPU fallback
  - Tests: `tests/test_gpu_graph_cache.cpp`, `tests/test_gpu_query_accelerator.cpp`, `tests/test_gpu_stream_manager.cpp`
- [x] FP16/BF16 Tensor Core support in query accelerator (Issue: #1789)
- [x] Per-GPU thermal and power telemetry in metrics registry (Issue: #1790)
- [x] GPU profiling integration (NVIDIA Nsight, ROCm Profiler) (Issue: #1791)
- [x] GPU-accelerated ANN (vector similarity) via cuVS/RAFT (Issue: #2381)
- [x] Unified memory support (CPU+GPU shared address space) (Issue: #1794)
- [x] Dynamic GPU time-slicing for multi-tenant isolation (Issue: #1795)
- [x] WASM-based GPU kernel sandbox for untrusted third-party kernels (Issue: #1796)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)

### Long-term (6-12 months)

## Implementation Phases

### Phase 1: GPU Resource Management & Acceleration (Status: Completed ✅)
- [x] Edition-aware VRAM allocation with tenant quotas (`gpu/gpu_memory_manager_edition.cpp`)
- [x] CUDA/ROCm device enumeration with CPU-fallback sentinel
- [x] Circuit-breaker safe-fail with GPU→CPU fallback (`gpu/safe_fail.cpp`)
- [x] Ring-buffer structured audit event log
- [x] Default-deny capability gate for GPU usage
- [x] Slab-based memory pre-allocator with fragmentation tracking
- [x] Prometheus-compatible metrics registry and threshold-based alert manager
- [x] FNV-1a checksum kernel whitelist (validate-before-launch)
- [x] Multi-GPU load balancer (ROUND_ROBIN, LEAST_LOADED, FIRST_HEALTHY)
- [x] Per-edition GPU feature gates with runtime overrides
- [x] JSON admin stats API with tenant breakdown and dry-run simulation
- [x] Parallel scan/filter/sort/aggregate/join with GPU threshold dispatch (`gpu/query_accelerator.cpp`)
- [x] Typed tensor containers with shape/dtype, views, and checkpointing
- [x] CUDA stream creation in `GPUStreamManager` (`gpu/stream_manager.cpp`)
- [x] Training loop coordinator with batch iteration, loss tracking, and early stopping

### Phase 2: Backend Parity & Cluster Coordination (Status: Complete ✅)
- [x] ROCm/HIP backend parity with CUDA feature set (`gpu/rocm_backend.cpp`, Target: Q2 2026)
- [x] GPU memory defragmentation routine (Target: Q2 2026)
- [x] Multi-node GPU cluster coordination (`gpu/cluster_coordinator.cpp`, Target: Q3 2026)

### Phase 3: Advanced Hardware & Topology (Status: Planned 📋)
- [x] Vulkan compute backend for cross-vendor GPU support (Issue: #1799)
  - Implementation: `include/themis/gpu/vulkan_backend.h`, `src/gpu/vulkan_backend.cpp`
  - Interfaces: `VulkanComputeBackend::{deviceCount, isAvailable, vendorName, createBackendFn, createStream, destroyStream, synchronizeStream, getStream, hasStream, streamNames, getStats, resetStats}` + `StreamHandle`, `Result`, `Stats`
  - Feature gate: `GPUFeatureFlags::Feature::VULKAN_BACKEND` (all editions)
  - CPU simulation path (in-memory registry) always active; tests pass without Vulkan hardware.
  - Tests: `tests/test_gpu_vulkan_backend.cpp`
- [I] Peer-to-peer GPU-to-GPU direct transfers (NVLink/PCIe) (Issue: #1800)
  - Implementation: `include/themis/gpu/p2p_transfer.h`, `src/gpu/p2p_transfer.cpp`
  - Interfaces: `GPUP2PTransferManager::{canAccessPeer, enablePeerAccess, disablePeerAccess, isPeerAccessEnabled, transfer, getStats, reset}` + `TransferRequest`, `TransferResult`, `Status` enum, `Stats`
  - Feature gate: `GPUFeatureFlags::Feature::PEER_TO_PEER` (Enterprise/Hyperscaler editions)
  - CUDA path: `cudaDeviceCanAccessPeer`, `cudaDeviceEnablePeerAccess`, `cudaMemcpyPeer`
  - HIP path: `hipDeviceCanAccessPeer`, `hipDeviceEnablePeerAccess`, `hipMemcpyPeer`
  - CPU simulation path (memcpy fallback) always active; tests pass without GPU hardware.
  - Tests: `tests/test_gpu_p2p_transfer.cpp`
- [x] CUDA Graph capture for recurring query execution patterns
- [I] NVLink topology-aware scheduling for multi-GPU jobs (Issue: #1802)
- [x] MIG (Multi-Instance GPU) partitioning support for NVIDIA A/H series
- [x] GPU-accelerated ANN (vector similarity) via cuVS/RAFT

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1805)
- [x] Integration tests (device discovery, circuit breaker, memory allocation, load balancer)
- [x] Performance benchmarks (GPU vs CPU throughput, memory allocation latency)
- [x] Security audit (kernel whitelist, capability gate, tenant isolation)
- [x] Documentation complete (GPU runbook, roadmap, admin API, `include/themis/gpu/README.md` with all 28 header descriptions)
- [x] API stability guaranteed for GPUModule facade and query accelerator
- [x] All 30 `src/gpu/*.cpp` source files registered in `cmake/EditionFeatures.cmake` (all GPU-enabled editions) and `cmake/ModularBuild.cmake` (4 unconditionally + 26 gated on `THEMIS_ENABLE_GPU` in modular geo sources)
- [x] All GPU test files re-included in `tests/CMakeLists.txt`; 18 dedicated focused test targets added in `cmake/CMakeLists.txt`

## Known Issues & Limitations
- Multi-node GPU cluster coordination requires external orchestration
- CUDA graph capture is implemented as CPU bookkeeping simulation (`GPUGraphCache` / `GPUQueryAccelerator`); production `cudaGraph_t` wiring requires GPU hardware (Issue: #1801)
- MIG partitioning infrastructure is implemented (`MIGManager`); real `nvmlDeviceCreateGpuInstance` calls require CUDA + NVML hardware
- Vulkan compute backend infrastructure is implemented (`VulkanComputeBackend`); real `VkQueue` submission and `synchronizeStream` (`vkQueueWaitIdle`) require Vulkan SDK + hardware

## Breaking Changes
- Multi-node coordination will introduce cluster configuration block (new optional config)
- MIG support will change device discovery output format for partitioned GPUs

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🟡 UNGENUTZT (kein Test, kein externer Aufrufer)

- `MakeCPUFallback` – Erzeugt CPU-Fallback-Device wenn keine GPU verfügbar
- `EnumerateCUDA` – Zählt verfügbare CUDA-Devices auf
- `EnumerateROCm` – Zählt verfügbare ROCm/HIP-Devices auf
- `resolveDevices` – Löst Device-Liste für P2P-Transfer auf (src+dst Devices bestimmen)
  > **Aktion:** Für jedes Symbol entscheiden: (1) Verdrahten, (2) Testen oder (3) als CANDIDATE_FOR_REMOVAL einplanen.

