> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — GPU Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The GPU module manages GPU device lifecycle, VRAM allocation, kernel execution, multi-GPU cluster coordination, and ANN query acceleration. Security concerns focus on: kernel execution safety (whitelist enforcement), tenant VRAM isolation, circuit-breaker safe-fail, WASM sandbox for untrusted kernels, and protection of the GPU admin API.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Execution of unauthorized GPU kernels | FNV-1a checksum kernel whitelist (`kernel_validator.cpp`) validates every kernel before launch; unknown kernels are rejected |
| Cross-tenant VRAM leakage | Edition-aware VRAM allocator enforces per-tenant quotas; allocations from different tenants use separate memory regions |
| VRAM exhaustion DoS | Slab-based pre-allocator with fragmentation tracking; per-tenant VRAM quota enforced at allocation time |
| GPU crash cascading to host process | Circuit-breaker (`safe_fail.cpp`) detects GPU errors and falls back to CPU; GPU failures are isolated from request processing |
| Untrusted third-party kernel execution | WASM-based GPU kernel sandbox isolates untrusted kernels from the host process (Issue #1796) |
| Admin API unauthorized access | GPU admin API (`admin_api.cpp`) requires privileged authentication scope; all operations are audit-logged |
| Multi-GPU cluster topology poisoning | Cluster coordinator validates node topology via NVLink/InfiniBand probing; unexpected topology changes trigger alerts |
| GPU profiler data exfiltration | NVTX/rocTX markers contain only operation labels; no query data or tenant IDs in profiler payloads |
| Kernel replay attack via CUDA graph capture | `GPUGraphCache` LRU entries keyed by `QueryShape` (OpType × row_count × param_hash); stale graphs are evicted after 32 entries |
| Dry-run simulation revealing resource limits | Dry-run output returns only feasibility (pass/fail); does not expose per-tenant quota details |

## Security Controls

### Kernel Whitelist
- Every kernel launch path in `GPUQueryAccelerator` calls `KernelValidator::validate()` with the FNV-1a checksum of the kernel binary.
- Unknown or modified kernels are rejected before any GPU memory allocation occurs.
- Whitelist is loaded from a trusted configuration path at startup; not modifiable at runtime without a restart.

### VRAM Tenant Isolation
- `GPUMemoryManagerEdition` allocates separate memory regions per tenant; tenant IDs are validated before allocation.
- Per-edition and per-tenant VRAM quotas prevent any single tenant from exhausting device memory.
- Quota enforcement is applied before CUDA/ROCm allocation calls.

### Circuit Breaker
- `safe_fail.cpp` monitors GPU error rates and automatically switches to CPU fallback when error threshold is exceeded.
- Circuit breaker state (CLOSED/OPEN/HALF_OPEN) is exposed in admin API for operator visibility.
- All GPU→CPU fallback transitions are recorded in the ring-buffer audit log.

### WASM Kernel Sandbox
- `WasmPluginSandbox` (from base module) provides process-level isolation for untrusted WASM GPU kernels.
- Sandboxed kernels cannot access GPU memory regions belonging to other tenants.
- Sandbox resource limits (memory, execution time) are enforced before kernel submission.

### Admin API Security
- GPU admin stats and dry-run simulation require `admin:gpu:read` scope.
- Tenant-breakdown reports require `admin:gpu:tenant:read` scope.
- All admin operations are written to the ring-buffer audit log.

### Multi-Node Cluster Security
- NVLink/InfiniBand topology is validated at startup; peer-to-peer transfers are only allowed between verified cluster members.
- Cluster coordinator uses authenticated channels for node-to-node communication.

## Data Handling

- GPU memory holds intermediate computation results (tensor values, vector embeddings, query results); these are never persisted by this module.
- VRAM contents are zeroed on deallocation to prevent cross-tenant data residue.
- Audit log entries contain operation types, timestamps, tenant IDs, and GPU device IDs; no computation data is logged.
- Thermal and power telemetry metrics are per-device aggregate values; no tenant-specific data.
- Profiler markers (NVTX/rocTX) contain operation labels only; no query content.

## Known Limitations

- FP16/BF16 Tensor Core operations require SM 7.0+ (FP16) or SM 8.0+ (BF16); older NVIDIA GPUs fall back to FP32.
- WASM kernel sandbox requires concrete WasmRuntime backend injection (same requirement as base module, Issue #1572).
- Multi-node cluster authentication channel encryption is operator-configured; not enforced by this module.
- GPU memory zeroing on deallocation adds overhead; can be disabled in performance mode (security tradeoff).

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| CUDA Toolkit | NVIDIA GPU execution | Version-pinned; keep CUDA driver updated |
| ROCm/HIP | AMD GPU execution | Keep patched |
| cuVS/RAFT (optional) | GPU-accelerated ANN | NVIDIA-managed; version-pinned |
| NCCL/RCCL (optional) | Multi-GPU collective ops | Version-pinned |
| NVTX (optional) | Profiling markers | System-provided with CUDA toolkit |
