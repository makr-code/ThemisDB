# GPU Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — GPU memory management, device discovery, safe-fail circuit breaker, audit logging, policy enforcement, kernel validation, metrics, multi-GPU load balancing, query acceleration, and ROCm/HIP backend parity are implemented. Multi-node GPU coordination and memory defragmentation are still in progress.

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
- [x] Parallel scan/filter/sort/aggregate/join with GPU threshold dispatch
- [x] Typed tensor containers with shape/dtype, views, and checkpointing
- [x] Training loop coordinator with batch iteration, loss tracking, and early stopping
- [x] ROCm/HIP backend parity with CUDA feature set (`gpu/rocm_backend.cpp`)

## In Progress 🚧
- [I] GPU memory defragmentation routine (Target: Q2 2026) (Issue: #2191)
- [I] Multi-node GPU cluster coordination (Target: Q3 2026) (Issue: #2378)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] ROCm/HIP full feature parity (memory manager, kernel validator, launcher) (Issue: #1786)
- [I] GPU memory defragmentation for long-running workloads (Issue: #1787)
- [!] CUDA graph capture for recurring query execution patterns (Issue: #2379)
- [I] FP16/BF16 Tensor Core support in query accelerator (Issue: #1789)
- [I] Per-GPU thermal and power telemetry in metrics registry (Issue: #1790)
- [I] GPU profiling integration (NVIDIA Nsight, ROCm Profiler) (Issue: #1791)

### Long-term (6-12 months)
- [I] Multi-node GPU cluster with NVLink/InfiniBand topology awareness (Issue: #1792)
- [!] GPU-accelerated ANN (vector similarity) via cuVS/RAFT (Issue: #2381)
- [I] Unified memory support (CPU+GPU shared address space) (Issue: #1794)
- [I] Dynamic GPU time-slicing for multi-tenant isolation (Issue: #1795)
- [I] WASM-based GPU kernel sandbox for untrusted third-party kernels (Issue: #1796)
- [!] MIG (Multi-Instance GPU) partitioning support for NVIDIA A/H series (Issue: #2380)

## Implementation Phases

### Phase 1: GPU Resource Management & Acceleration (Status: Completed ✅)
- [x] Edition-aware VRAM allocation with tenant quotas (`gpu/vram_allocator.cpp`)
- [x] CUDA/ROCm device enumeration with CPU-fallback sentinel
- [x] Circuit-breaker safe-fail with GPU→CPU fallback (`gpu/circuit_breaker.cpp`)
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
- [x] Training loop coordinator with batch iteration, loss tracking, and early stopping

### Phase 2: Backend Parity & Cluster Coordination (Status: Partially Complete 🔶)
- [x] ROCm/HIP backend parity with CUDA feature set (`gpu/rocm_backend.cpp`, Target: Q2 2026)
- [~] GPU memory defragmentation routine (Target: Q2 2026)
- [ ] Multi-node GPU cluster coordination (Target: Q3 2026)

### Phase 3: Advanced Hardware & Topology (Status: Planned 📋)
- [I] Vulkan compute backend for cross-vendor GPU support (Issue: #1799)
- [I] Peer-to-peer GPU-to-GPU direct transfers (NVLink/PCIe) (Issue: #1800)
- [ ] CUDA Graph capture for recurring query execution patterns
- [I] NVLink topology-aware scheduling for multi-GPU jobs (Issue: #1802)
- [ ] MIG (Multi-Instance GPU) partitioning support for NVIDIA A/H series
- [ ] GPU-accelerated ANN (vector similarity) via cuVS/RAFT

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1805)
- [x] Integration tests (device discovery, circuit breaker, memory allocation, load balancer)
- [x] Performance benchmarks (GPU vs CPU throughput, memory allocation latency)
- [x] Security audit (kernel whitelist, capability gate, tenant isolation)
- [x] Documentation complete (GPU runbook, roadmap, admin API)
- [x] API stability guaranteed for GPUModule facade and query accelerator

## Known Issues & Limitations
- GPU memory defragmentation is not yet implemented; long-running instances may fragment VRAM
- Multi-node GPU cluster coordination requires external orchestration
- CUDA graph capture is not yet implemented
- MIG partitioning is not yet supported

## Breaking Changes
- Multi-node coordination will introduce cluster configuration block (new optional config)
- MIG support will change device discovery output format for partitioned GPUs
