# GPU Module Roadmap

## Current Status
**Beta** — GPU memory management, device discovery, safe-fail circuit breaker, audit logging, policy enforcement, kernel validation, metrics, multi-GPU load balancing, and query acceleration are implemented. ROCm parity and multi-node GPU support are still in progress.

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

## In Progress 🚧
- [ ] ROCm/HIP backend parity with CUDA feature set (Target: Q2 2026)
- [ ] GPU memory defragmentation routine (Target: Q2 2026)
- [ ] Multi-node GPU cluster coordination (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] ROCm/HIP full feature parity (memory manager, kernel validator, launcher)
- [ ] GPU memory defragmentation for long-running workloads
- [ ] CUDA graph capture for recurring query execution patterns
- [ ] FP16/BF16 Tensor Core support in query accelerator
- [ ] Per-GPU thermal and power telemetry in metrics registry
- [ ] GPU profiling integration (NVIDIA Nsight, ROCm Profiler)

### Long-term (6-12 months)
- [ ] Multi-node GPU cluster with NVLink/InfiniBand topology awareness
- [ ] GPU-accelerated ANN (vector similarity) via cuVS/RAFT
- [ ] Unified memory support (CPU+GPU shared address space)
- [ ] Dynamic GPU time-slicing for multi-tenant isolation
- [ ] WASM-based GPU kernel sandbox for untrusted third-party kernels
- [ ] MIG (Multi-Instance GPU) partitioning support for NVIDIA A/H series

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (device discovery, circuit breaker, memory allocation, load balancer)
- [x] Performance benchmarks (GPU vs CPU throughput, memory allocation latency)
- [x] Security audit (kernel whitelist, capability gate, tenant isolation)
- [x] Documentation complete (GPU runbook, roadmap, admin API)
- [x] API stability guaranteed for GPUModule facade and query accelerator

## Known Issues & Limitations
- ROCm/HIP backend has partial feature parity with CUDA backend
- GPU memory defragmentation is not yet implemented; long-running instances may fragment VRAM
- Multi-node GPU cluster coordination requires external orchestration
- CUDA graph capture is not yet implemented
- MIG partitioning is not yet supported

## Breaking Changes
- Multi-node coordination will introduce cluster configuration block (new optional config)
- MIG support will change device discovery output format for partitioned GPUs
