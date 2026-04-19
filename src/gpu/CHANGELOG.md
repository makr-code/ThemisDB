> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — GPU Module

All notable changes to the GPU module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [1.8.0] — 2026-03-21
### Added
- Public header documentation: `include/themis/gpu/README.md` — comprehensive
  documentation for all 28 GPU public headers including key types, APIs, usage
  examples, edition feature gate table, and cross-reference links

## [Unreleased]
*(All planned features are implemented — see `FUTURE_ENHANCEMENTS.md` for long-horizon items.)*

## [1.7.0] — 2026-03-09
### Added
- WASM-based GPU kernel sandbox for untrusted third-party kernels (Issue #1796)
- Dynamic GPU time-slicing for multi-tenant isolation (Issue #1795)
- Unified memory support (CPU+GPU shared address space) (Issue #1794)
- MIG (Multi-Instance GPU) partitioning for NVIDIA A/H series
- Vulkan compute backend for cross-vendor GPU support
- Peer-to-peer GPU-to-GPU direct transfers (NVLink/PCIe)
- Feature flags module (`src/gpu/feature_flags.cpp`) for runtime GPU feature enable/disable

## [1.6.0] — 2026-02-15
### Added
- CUDA graph capture for recurring query execution patterns: `GPUGraphCache` (LRU eviction at 32 entries), `GPUQueryAccelerator` integrating graph cache in scan/sort/aggregate/hashJoin/annSearch (`src/gpu/graph_cache.cpp`) (Issues #2379, #1801)
- `GPUStreamManager::createCudaStream()` providing first-class CUDA stream creation with CPU fallback (Issue #1801)
- FP16/BF16 Tensor Core support in query accelerator (Issue #1789)
- Per-GPU thermal and power telemetry in metrics registry (Issue #1790)
- GPU profiling integration: NVIDIA Nsight (NVTX markers) and ROCm Profiler (rocTX markers) (`src/gpu/profiler.cpp`) (Issue #1791)
- Multi-node GPU cluster coordination with NVLink/InfiniBand topology awareness (`src/gpu/cluster_topology.cpp`, `src/gpu/cluster_coordinator.cpp`)

## [1.5.0] — 2026-01-10
### Added
- ROCm/HIP backend with full feature parity to CUDA (memory manager, kernel validator, launcher) (`src/gpu/rocm_backend.cpp`) (Issue #1786)
- GPU memory defragmentation for long-running workloads (`src/gpu/memory_pool.cpp`) (Issue #1787)
- GPU-accelerated ANN (vector similarity) via cuVS/RAFT: infrastructure with CPU brute-force fallback and cuVS/RAFT stub (`src/gpu/query_accelerator.cpp`) (Issue #2381)
- GPU profiler NVTX/rocTX marker integration (`src/gpu/profiler.cpp`)

## [1.4.0] — 2025-12-01
### Added
- Named async GPU streams with CPU fallback budget (`src/gpu/launcher.cpp`)
- Parallel scan/filter/sort/aggregate/join with GPU threshold dispatch
- Typed tensor containers with shape/dtype, views, and checkpointing
- Training loop coordinator with batch iteration, loss tracking, and early stopping
- Multi-GPU load balancer: ROUND_ROBIN, LEAST_LOADED, FIRST_HEALTHY strategies (`src/gpu/load_balancer.cpp`)

## [1.0.0] — 2024-01-01
### Added
- Edition-aware VRAM allocation with tenant quotas and pre-allocation hints (`src/gpu/gpu_memory_manager_edition.cpp`)
- CUDA/ROCm device enumeration with CPU-fallback sentinel (`src/gpu/device_discovery.cpp`)
- Circuit-breaker safe-fail with GPU→CPU fallback (`src/gpu/safe_fail.cpp`)
- Ring-buffer structured audit event log (`src/gpu/audit_log.cpp`)
- Default-deny capability gate for GPU usage
- Slab-based memory pre-allocator with fragmentation tracking (`src/gpu/memory_pool.cpp`)
- Prometheus-compatible counter/gauge metrics registry (`src/gpu/metrics.cpp`)
- FNV-1a checksum kernel whitelist (validate-before-launch) (`src/gpu/kernel_validator.cpp`)
- Threshold-based alert manager with callbacks (`src/gpu/alerts.cpp`)
- JSON admin stats API with tenant breakdown and dry-run simulation (`src/gpu/admin_api.cpp`)
- GPU config validation with dry-run simulation (`src/gpu/config.cpp`)
- Integration facade: policy → circuit-breaker → alloc → launch pipeline (`src/gpu/gpu_module.cpp`)
