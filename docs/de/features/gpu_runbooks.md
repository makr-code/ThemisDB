# GPU Module — Operational Runbooks

On-call procedures for common GPU module incidents.  Each runbook follows a
**Detect → Diagnose → Mitigate → Verify → Follow-up** structure.

---

## 1. GPU OOM (Out-Of-Memory)

### Symptoms

- `TryAllocateGPU` returns `false`; `GPUSafeFail` routes to CPU fallback.
- `themis_gpu_alloc_total{result="fail_global_limit"}` counter climbing.
- `VRAM_HIGH` alert firing (`themis_gpu_vram_allocated_bytes / limit ≥ 0.80`).

### Detect

```
GET /admin/gpu/stats
# Look for: allocated_bytes / max_bytes > 0.80, alloc_fail_global_limit > 0
```

Or query metrics:

```
themis_gpu_vram_allocated_bytes
themis_gpu_alloc_total{result="fail_global_limit"}
```

### Diagnose

1. Call `GET /admin/gpu/tenants` to find which tenant(s) hold the most VRAM.
2. Use `GPUMemoryManager::GetActiveAllocations()` to list live allocation tags.
3. Check audit log (`GPUAuditLog::snapshot()`) for `ALLOC_FAIL_GLOBAL_LIMIT`
   events and their `tag` / `tenant_id`.

### Mitigate

- **Short-term**: CPU fallback is already active.  Monitor latency impact;
  alert if p99 exceeds budget.
- **Reduce tenant usage**: call `SetTenantQuota(tenant_id, lower_limit)` to
  cap the largest consumer temporarily.
- **Force deallocation**: if a specific allocation is known to be leaked, call
  `DeallocateGPU(size, tenant_id)` against the owning tenant.
- **Upgrade edition**: if the OOM is legitimate load growth, plan an edition
  upgrade to raise the VRAM cap.

### Verify

```
GET /admin/gpu/stats  →  allocated_bytes should decrease
themis_gpu_fallback_total  →  should stop growing
```

### Follow-up

- Add or tighten per-tenant quotas with `SetTenantQuota`.
- Review `FUTURE_ENHANCEMENTS.md` memory-pool preallocation hint item.
- File a capacity-planning ticket if load is genuinely growing.

---

## 2. GPU Device Unavailable / Lost

### Symptoms

- `DeviceDiscovery::Enumerate()` returns only `CPU_FALLBACK`.
- `DEVICE_UNAVAILABLE` alert firing.
- `GPUSafeFail` state transitions to `FAILED` or `CIRCUIT_OPEN`.
- `CIRCUIT_OPEN` alert firing.

### Detect

```
GET /admin/gpu/stats  →  backend == "CPU_FALLBACK"
themis_gpu_circuit_open_total  →  non-zero
```

### Diagnose

1. Check host GPU health: `nvidia-smi` (CUDA) or `rocm-smi` (ROCm).
2. Inspect `GPUSafeFail::getStatus().last_error` for the driver error message.
3. Check audit log for `DEVICE_UNAVAILABLE` and `CIRCUIT_OPENED` events.
4. If `circuit_opened_at` is recent, the circuit breaker timeout has not yet
   elapsed — no GPU retries will happen until `circuit_reset_timeout_secs`.

### Mitigate

- **Wait for automatic reset**: after `circuit_reset_timeout_secs` the circuit
  transitions to `DEGRADED` and probes resume.
- **Manual reset**: call `GPUSafeFail::forceHealthy()` after the hardware is
  confirmed operational.
- **Persistent failure**: call `GPUSafeFail::forceFailed("maintenance")` to
  keep CPU fallback active while hardware is replaced; re-enable with
  `forceHealthy()` once resolved.
- **Re-enumerate**: call `DeviceDiscovery::Enumerate()` and pass the result
  to `GPULoadBalancer::updateDevices()` to refresh the device list.

### Verify

```
GPUSafeFail::getStatus().state == HEALTHY
DeviceDiscovery::HasGPU() == true
DEVICE_UNAVAILABLE alert resolved
```

### Follow-up

- Root-cause the driver crash (kernel panic, power event, ECC error).
- Add this device-loss pattern to CI fault-injection harness.

---

## 3. Tenant Quota Exhaustion

### Symptoms

- `TryAllocateGPU(size, tag, tenant_id)` returns `false` only for one tenant.
- `themis_gpu_alloc_total{result="fail_tenant_quota", tenant=...}` climbing.
- `ALLOC_FAIL_TENANT_QUOTA` events in the audit log.

### Detect

```
GET /admin/gpu/tenants  →  find tenant where allocated_bytes ≈ quota_bytes
themis_gpu_alloc_total{result="fail_tenant_quota"}
```

### Diagnose

1. `GPUMemoryManager::GetTenantStats(tenant_id)` — compare `allocated_bytes`
   vs `quota_bytes`.
2. `GPUMemoryManager::GetActiveAllocations()` — filter by `tenant_id` to see
   which tags are consuming VRAM.
3. `GPUMemoryManager::GetTenantHeadroom(tenant_id)` — how much is left.

### Mitigate

- **Temporary increase**: `SetTenantQuota(tenant_id, new_higher_quota)`.
  Validate first with `GPUConfig::simulateAllocation`.
- **Ask tenant to free**: if the tenant owns stale allocations, trigger their
  cleanup path so `DeallocateGPU(size, tenant_id)` is called.
- **Reduce other tenants**: if global VRAM is also tight, lower quotas for
  lower-priority tenants first.

### Verify

```
GetTenantHeadroom(tenant_id) > 0
TryAllocateGPU(..., tenant_id) == true
```

### Follow-up

- Review tenant growth trend; adjust default quota policy.
- Consider per-tenant audit log alerts at 90% quota utilisation.

---

## 4. Kernel Load / Validation Failure

### Symptoms

- `GPUKernelValidator::validate()` returns `CHECKSUM_MISMATCH` or
  `UNKNOWN_KERNEL`.
- Kernel launches refused; workload fails or falls back to CPU.
- `themis_gpu_alloc_total{result="fail_global_limit"}` may not be involved —
  this is a security gate, not an OOM.

### Detect

```
GPUKernelValidator::getStats()
  unknown_kernel_count  > 0  →  unregistered kernel submitted
  checksum_mismatch_count > 0  →  tampered or stale kernel blob
```

### Diagnose

1. Identify which `kernel_id` failed via the validation result or audit log.
2. If `CHECKSUM_MISMATCH`: the blob on disk differs from the registered
   checksum.  Possible causes: deployment race (new binary, old checksum),
   accidental overwrite, or active tampering.
3. If `UNKNOWN_KERNEL`: a new kernel was deployed without registering it via
   `GPUKernelValidator::registerKernel()`.

### Mitigate

- **UNKNOWN_KERNEL — legitimate deployment**: register the new kernel:
  ```cpp
  // Load canonical blob from trusted build artifact.
  GPUKernelValidator::GetInstance().registerKernel(
      "kernel_id", canonical_blob);
  ```
- **CHECKSUM_MISMATCH — deployment race**: re-register with the new blob
  after verifying its provenance.
- **CHECKSUM_MISMATCH — suspected tampering**: do **not** re-register.
  Quarantine the host, rotate secrets, and escalate to the security team.

### Verify

```
GPUKernelValidator::isValid(kernel_id, blob) == true
No further checksum_mismatch_count increments
```

### Follow-up

- Add pre-deployment step that auto-registers all kernel blobs and their
  checksums during the CI/CD pipeline.
- Integrate HMAC/signature verification (see `FUTURE_ENHANCEMENTS.md`).

---

## 5. High CPU-Fallback Rate / Latency Degradation

### Symptoms

- `FALLBACK_RATE_HIGH` alert firing.
- p99 latency above SLO; CPU capacity spike.
- `themis_gpu_fallback_total` growing rapidly.

### Detect

```
themis_gpu_fallback_total / themis_gpu_alloc_total > 0.20 (threshold)
p99 query latency metric
```

### Diagnose

1. Check whether `GPUSafeFail` circuit is open (`CIRCUIT_OPEN` alert).
2. If circuit is open, see Runbook 2 (Device Unavailable).
3. If circuit is closed but fallback rate is high: VRAM exhaustion is likely.
   See Runbook 1 (OOM).
4. Check `GPUSafeFail::getStatus().error_rate` for sustained failures below
   the circuit-breaker threshold.

### Mitigate

- Address the root cause (OOM or device issue) per the relevant runbook.
- If CPU fallback is intentional (scheduled maintenance), suppress the alert
  by temporarily raising `fallback_rate_threshold` in `GPUAlerts::Config`.
- Scale out CPU capacity to absorb fallback load while GPU is unavailable.

### Verify

```
themis_gpu_fallback_total stops growing
FALLBACK_RATE_HIGH alert resolves
p99 latency returns to SLO
```

### Follow-up

- Review `CPU fallback performance budgets` item in `gpu_roadmap.md`.
- Consider pre-warming CPU vector index to reduce cold-start latency when
  fallback is triggered.

---

## 6. GPU Geospatial Backend Issues

The `GpuBatchBackend` (`src/geo/gpu_backend_stub.cpp`) handles spatial
intersection queries for the geo module.  In CI and on machines without a
GPU it runs entirely on CPU via the circuit-breaker fallback path.

### Symptoms

- Geo intersection queries return all-zero results or empty masks.
- `themis_gpu_fallback_total{reason="batch_cpu_fallback"}` or
  `{reason="device_unavailable"}` counters growing.
- `GpuBatchBackend::getStats().batch_avg_latency_us` unusually high.
- Geo index queries are slower than expected (backend is falling back).

### Detect

```
GET /admin/gpu/stats
# Look for: circuit_open == true, exact_errors > 0, batch_fallbacks climbing

# Or via GPUMetrics snapshot:
themis_gpu_fallback_total{reason="batch_cpu_fallback"}
themis_gpu_fallback_total{reason="device_unavailable"}
```

Check the audit log for `FALLBACK_TO_CPU` and `DEVICE_UNAVAILABLE` events
tagged `"geo_backend_init"` or `"batchIntersects: cpu fallback"`.

### Diagnose

1. Call `getGpuSpatialBackend()->isAvailable()`.  If `false`:
   - Check `DeviceDiscovery::HasGPU()` — no GPU present → CPU fallback is
     expected and correct.
   - Check `GPUSafeFail::getStatus().state` — if `CIRCUIT_OPEN`, the device
     has experienced repeated failures.
2. Inspect `GpuBatchBackend::getStats()` for non-zero `exact_errors`.
3. Check `SpatialBatchInputs` population: if `geoms_a`/`geoms_b` are empty
   and `count > 0` the caller is not populating geometry pairs — all mask
   entries will be 0.  This is a caller bug, not a backend bug.
4. If `batch_avg_latency_us` is high: large polygons or many pairs per call
   increase the CPU fallback cost.  Review the caller's batch size vs the
   configured `gpu_batch_threshold` (default 64).

### Mitigate

- **No GPU / CPU-only environment (expected)**: CPU fallback is intentional.
  No action required; latency SLO must account for CPU cost.
- **Circuit breaker open**: follow Runbook 2 (Device Unavailable) to reset
  the circuit after hardware recovery.
- **Caller not populating geometry vectors**: fix the call site to set
  `SpatialBatchInputs::geoms_a` and `geoms_b` alongside `count`.
- **High latency on large batches**: reduce `count` per call, or split into
  smaller sub-batches to stay within `fallback_budget_ms`.

### Verify

```cpp
auto* backend = getGpuSpatialBackend();
backend->isAvailable();                  // true when GPU present and healthy
backend->exactIntersects(pt, poly);      // returns correct result

SpatialBatchInputs in;
in.count = 1;
in.geoms_a = {pt}; in.geoms_b = {poly};
auto r = backend->batchIntersects(in);
assert(r.mask[0] == 1u);                 // hit confirmed
```

### Follow-up

- Once CUDA/ROCm kernels are available for spatial ops, replace the
  CPU compute loop in `batchIntersects` with a real kernel launch
  (see `FUTURE_ENHANCEMENTS §GPU_SPATIAL_KERNELS`).
- Add per-call latency histogram to `GPUMetrics` when a latency-histogram
  API is introduced.
