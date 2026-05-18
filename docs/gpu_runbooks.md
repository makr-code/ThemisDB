# GPU Module — On-Call Runbooks

**ThemisDB GPU Operations Guide**

Version: 1.0
Last Updated: 2026-05-12

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture Quick Reference](#architecture-quick-reference)
3. [Monitoring and Metrics](#monitoring-and-metrics)
4. [Common Operational Procedures](#common-operational-procedures)
5. [Troubleshooting](#troubleshooting)
6. [Geo Spatial Backend](#geo-spatial-backend)
7. [Emergency Procedures](#emergency-procedures)
8. [Maintenance](#maintenance)

---

## Overview

### Purpose

This runbook provides on-call operational procedures for managing the ThemisDB GPU module
(`src/gpu/`) in production environments. It covers VRAM OOM events, device unavailability,
tenant quota exhaustion, kernel load failures, circuit-breaker recovery, and geo spatial
backend issues.

### Scope

- Single-node and multi-GPU (cluster) deployments
- All GPU-enabled editions: Professional, Enterprise, Hyperscaler
- Community edition is CPU-only — GPU sections do not apply

### Prerequisites

- Access to the GPU admin API (`GET /admin/gpu/*`)
- Prometheus / Grafana monitoring dashboard access
- Log aggregation access (audit ring-buffer visible via admin API)
- ThemisDB CLI or equivalent for issuing admin commands

### Related Documentation

- [GPU Module Source](../src/gpu/README.md) — component architecture and quick-start
- [GPU Architecture Guide](../src/gpu/ARCHITECTURE.md) — design principles and data flows
- [GPU Roadmap](../src/gpu/ROADMAP.md) — delivery phases and open issues
- [GPU Security](../src/gpu/SECURITY.md) — threat model and security controls
- [GPU Public Headers](../include/themis/gpu/README.md) — full public API reference
- [GPU Production Readiness Assessment](gpu_roadmap.md)

---

## Architecture Quick Reference

```
GPUModule (gpu_module.h)
 ├── GPUPolicy           – default-deny capability gate
 ├── GPUSafeFail         – circuit-breaker: CLOSED → OPEN → HALF_OPEN
 ├── GPUMemoryManager    – edition-aware VRAM, per-tenant quotas
 ├── GPUMemoryPool       – slab pre-allocator, zero-on-free
 ├── DeviceDiscovery     – CUDA/ROCm enumeration; CPU-fallback sentinel
 ├── GPULoadBalancer     – multi-GPU: ROUND_ROBIN / LEAST_LOADED / FIRST_HEALTHY
 ├── GPULauncher         – typed async work-item / batch launcher
 ├── GPUStreamManager    – named streams, CPU fallback budget
 ├── GPUKernelValidator  – FNV-1a checksum whitelist
 ├── GPUMetrics          – Prometheus counters/gauges, thermal/power telemetry
 ├── GPUAlerts           – threshold-based alert callbacks
 ├── GPUAuditLog         – ring-buffer structured event log
 ├── GPUAdminAPI         – JSON stats / tenant breakdown / dry-run simulation
 ├── GPUFeatureFlags     – per-edition feature gates
 ├── GPUConfig           – startup validation, dry-run simulation
 ├── GPUQueryAccelerator – scan/filter/sort/aggregate/join/ANN
 ├── GPUTensorBuffer     – typed tensors, views, checkpointing
 ├── GPUTrainingLoop     – batch training coordinator
 └── ROCmBackend         – HIP stream lifecycle, device memory
```

### Edition GPU Limits

| Edition | VRAM Limit | Multi-GPU | MIG | Cluster |
|---------|-----------|-----------|-----|---------|
| Community | 0 GB (CPU-only) | ❌ | ❌ | ❌ |
| Professional | 8 GB | ❌ | ❌ | ❌ |
| Enterprise | 24 GB | ✅ | ✅ | ✅ |
| Hyperscaler | No limit | ✅ | ✅ | ✅ |

---

## Monitoring and Metrics

### Key Prometheus Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `gpu_vram_allocated_bytes` | Gauge | Current total VRAM allocated across all tenants |
| `gpu_vram_peak_bytes` | Gauge | Peak VRAM allocation since last reset |
| `gpu_allocations_total` | Counter | Total successful VRAM allocation attempts |
| `gpu_allocation_failures_total` | Counter | Failed VRAM allocation attempts |
| `gpu_fallback_to_cpu_total` | Counter | Number of GPU→CPU fallbacks triggered |
| `gpu_circuit_state` | Gauge | Circuit-breaker state (0=CLOSED, 1=HALF_OPEN, 2=OPEN) |
| `gpu_temperature_celsius{device}` | Gauge | Per-device temperature |
| `gpu_power_draw_watts{device}` | Gauge | Per-device power draw |
| `gpu_tenant_vram_bytes{tenant}` | Gauge | Per-tenant VRAM usage |
| `gpu_kernel_launches_total` | Counter | Total kernel launch attempts |
| `gpu_kernel_validation_failures_total` | Counter | Kernels rejected by whitelist validator |

### Alert Rules

| Alert | Threshold | Severity | Action |
|-------|-----------|----------|--------|
| VRAM > 80% of limit | Gauge > 0.8 × limit | Warning | Review tenant quotas |
| VRAM > 95% of limit | Gauge > 0.95 × limit | Critical | Immediate eviction/rebalancing |
| Alloc failure rate > 5/min | Counter rate spike | Warning | Check tenant quotas and edition limit |
| Circuit breaker OPEN | State == 2 | Critical | GPU device investigation |
| Temperature > 85°C | Gauge > 85 | Warning | Check cooling/throttling |
| Temperature > 95°C | Gauge > 95 | Critical | Emergency workload shedding |

### Admin API Endpoints

Global GPU stats:
```bash
curl http://localhost:8080/admin/gpu/stats
```

Per-tenant VRAM breakdown:
```bash
curl http://localhost:8080/admin/gpu/tenants
```

Per-device load:
```bash
curl http://localhost:8080/admin/gpu/devices
```

Circuit-breaker state:
```bash
curl http://localhost:8080/admin/gpu/circuit
```

Active MIG partitions (Enterprise/Hyperscaler):
```bash
curl http://localhost:8080/admin/gpu/mig
```

Geo spatial backend stats:
```bash
curl http://localhost:8080/admin/gpu/geo
```

Dry-run allocation simulation:
```bash
curl -s -X POST http://localhost:8080/admin/gpu/simulate \
  -H 'Content-Type: application/json' \
  -d '{"bytes": 1073741824, "tenant_id": "tenant-a"}'
```

---

## Common Operational Procedures

### §4.1 Checking GPU Module Health

```bash
curl -s http://localhost:8080/admin/gpu/stats | jq .
```

Expected healthy output includes `"circuit_state": "CLOSED"`, `"allocated_bytes"` (current usage), and `"device_count"` (number of GPUs).

### §4.2 Adjusting Tenant VRAM Quotas

If a tenant is hitting quota limits and legitimate workloads are failing:

1. Check current quota usage:
   ```bash
   curl -s http://localhost:8080/admin/gpu/tenants | jq .
   ```

2. Simulate the new allocation before applying:
   ```bash
   curl -s -X POST http://localhost:8080/admin/gpu/simulate \
     -H 'Content-Type: application/json' \
     -d '{"bytes": 2147483648, "tenant_id": "tenant-a"}'
   ```

3. Apply the quota change via ThemisDB config reload (requires restart or live-config
   support in your deployment).

### §4.3 Granting / Revoking GPU Access for a Caller

GPU access is default-deny. To grant a new service:

```cpp
// In application initialization code:
GPUModule::GetInstance().grantCaller("my-service-id");
```

To revoke:
```cpp
GPUModule::GetInstance().revokeCaller("my-service-id");
```

Both actions are recorded in the audit log.

### §4.4 Registering a New Kernel

All kernels must be whitelisted before launch:

```cpp
#include "themis/gpu/kernel_validator.h"
using namespace themis::gpu;

GPUKernelValidator validator;
// Register by blob — checksum is computed automatically
validator.registerKernelBlob("my-sort-kernel-v2", kernel_binary_bytes);

// Or register a pre-computed FNV-1a checksum:
validator.registerKernel("my-sort-kernel-v2", 0xdeadbeef12345678ULL);
```

---

## Troubleshooting

### §5.1 GPU OOM — VRAM Allocation Failures

**Symptoms:** `gpu_allocation_failures_total` rising; application logs show
`TryAllocateGPU` returning `false`; tenants unable to start GPU workloads.

**Diagnosis:**

Check current VRAM usage:
```bash
curl http://localhost:8080/admin/gpu/stats | jq .allocated_bytes
```

Check per-tenant breakdown to find the heaviest consumer:
```bash
curl http://localhost:8080/admin/gpu/tenants | jq '.[] | {tenant: .tenant_id, used: .current_bytes, quota: .quota_bytes}'
```

Review recent audit events for context:
```bash
curl http://localhost:8080/admin/gpu/audit?last_n=50
```

**Recovery Steps:**

1. Identify the tenant consuming the most VRAM.
2. Check if their workload is complete (and dealloc was missed due to a crash).
3. If the tenant is stuck, trigger a graceful workload eviction through application
   logic (no hard-kill GPU path exists by design — kernel isolation prevents force-free).
4. If VRAM remains exhausted, check whether the edition limit is correctly configured
   (`GPUConfig::max_vram_bytes`). The Community edition has a 0 GB limit; all GPU
   paths fall back to CPU — verify the edition key is set correctly.
5. Restart the GPU module (application restart) to reset all counters if stuck
   allocations cannot be freed through normal means.

### §5.2 Device Unavailable — Circuit Breaker OPEN

**Symptoms:** `gpu_circuit_state` == 2 (OPEN); all GPU work is being routed to CPU;
`gpu_fallback_to_cpu_total` is incrementing rapidly.

**Diagnosis:**
```bash
curl http://localhost:8080/admin/gpu/circuit | jq .
```

Look for: `state`, `failure_count`, `last_error`, `can_use_gpu`.

**Recovery Steps:**

1. Inspect the `last_error` field to identify the root cause (driver crash,
   device lost, timeout, OOM).
2. Check system GPU driver status:
   ```bash
   nvidia-smi   # NVIDIA
   rocm-smi     # AMD ROCm
   ```
3. If the driver reports the device as healthy but the circuit is OPEN, it will
   automatically transition to `HALF_OPEN` after `circuit_breaker_reset_s` seconds
   (default: 60 s), then probe with a single GPU operation.
4. To force an immediate probe, restart the service (the circuit resets to CLOSED
   on module initialization).
5. If the device is permanently lost, update the device list and reload config;
   `GPULoadBalancer` will mark the device as failed and route to remaining devices.

### §5.3 Tenant Quota Exhaustion

**Symptoms:** A specific tenant consistently fails GPU allocations while total VRAM
is not exhausted; `gpu_tenant_vram_bytes{tenant=X}` == quota.

**Recovery Steps:**

1. Confirm it is a quota issue (not total VRAM exhaustion):
   ```bash
   curl http://localhost:8080/admin/gpu/tenants | jq '.[] | select(.tenant_id == "tenant-a")'
   # Look for: current_bytes >= quota_bytes
   ```
2. Decide whether to increase the tenant quota (operational change) or investigate
   a memory leak (tenant not deallocating correctly).
3. To diagnose a leak, review audit log events for the tenant — check that
   `DEALLOCATION` events follow each `ALLOCATION`.
4. If workloads are completing but not deallocating, check that the calling code
   invokes `GPUMemoryManager::DeallocateGPU()` in all code paths including error
   branches.

### §5.4 Kernel Validation Failure

**Symptoms:** Application logs show `KERNEL_NOT_VALIDATED` or `CHECKSUM_MISMATCH`
errors; kernel launches are rejected.

**Root Causes:**
- The kernel binary was updated without updating the whitelist checksum.
- An untrusted or tampered kernel binary was deployed.
- The whitelist was not loaded at startup (config issue).

**Recovery Steps:**

1. Verify the deployed kernel binary hash matches the registered checksum:
   ```cpp
   auto checksum = GPUKernelValidator::computeChecksum(kernel_blob);
   // Compare to registered value
   ```
2. If the kernel is genuinely updated and trusted, update the whitelist entry in
   the configuration and restart the service.
3. If the mismatch is unexpected, treat it as a security event — do not bypass
   validation. Escalate to the security team.

### §5.5 Multi-GPU Load Imbalance

**Symptoms:** One GPU device shows consistently higher utilization than others;
`gpu_vram_allocated_bytes` is unbalanced across devices.

**Recovery Steps:**

1. Check device loads:
   ```bash
   curl http://localhost:8080/admin/gpu/devices | jq .
   ```
2. If using `ROUND_ROBIN` strategy, a single failed device may be causing work to
   pile up on its round-robin neighbor. Verify all devices are healthy:
   ```bash
   nvidia-smi --query-gpu=index,name,memory.used,memory.free,utilization.gpu \
     --format=csv,noheader
   ```
3. Mark the unhealthy device as failed in the load balancer (via application code
   calling `GPULoadBalancer::markFailed(device_index)`) and switch to
   `LEAST_LOADED` strategy to rebalance.

### §5.6 High GPU Temperature / Thermal Throttling

**Symptoms:** `gpu_temperature_celsius` > 85°C; workloads completing slower than
expected; Prometheus temperature alert firing.

**Recovery Steps:**

1. Check temperature and power readings:
   ```bash
   curl http://localhost:8080/admin/gpu/stats | jq '.devices[] | {index, temperature_c, power_watts}'
   ```
2. If temperature is above 90°C, reduce the workload submission rate by lowering
   the GPU row threshold in `GPUQueryAccelerator::Config::gpu_row_threshold` to
   route more work to CPU.
3. Verify server cooling (fans, chassis airflow). If a physical cooling problem
   is detected, power off the GPU workload and engage the hardware team.

---

## Geo Spatial Backend

### §6.1 Overview

The Geo Spatial Backend (`GpuBatchBackend` in `src/geo/gpu_backend_stub.cpp`)
provides GPU-accelerated geometry predicate evaluation (point-in-polygon,
segment intersection). It integrates the full GPU module circuit-breaker,
fallback, and observability stack.

**Key Types:**
- `GpuBatchBackend` — batch geometry predicate evaluator with CPU fallback
- `SpatialBatchInputs` — input containing `geoms_a`/`geoms_b` geometry pair vectors
- `GpuBatchBackend::Stats` — latency and throughput counters

**Factory:** `getGpuSpatialBackend()` returns a `GpuBatchBackend` instance.

### §6.2 Monitoring the Geo Backend

Geo backend-specific stats (latency, pairs processed):
```bash
curl http://localhost:8080/admin/gpu/geo | jq .
```

Expected fields: `batch_avg_latency_us`, `batch_max_latency_us`, `batch_pairs_processed`.

### §6.3 Troubleshooting Geo Batch Failures

**Symptom:** Geo spatial queries returning errors or degraded accuracy.

1. **CPU fallback active:** Check the circuit-breaker state (§5.2). When the GPU
   circuit is OPEN, the geo backend falls back to CPU geometry evaluation
   automatically — results are still correct, only throughput is reduced.

2. **Count/vector mismatch:** `SpatialBatchInputs::geoms_a` and `geoms_b` must
   be the same length. A mismatch returns an error result without panicking;
   check the calling code to ensure input construction is symmetric.

3. **Degenerate geometry:** Zero-length segments, NaN coordinates, or empty
   geometry collections are handled gracefully — they produce a defined result
   (`false` predicate) and are counted in stats. If unexpected degenerate inputs
   appear, validate upstream geometry construction.

4. **Large-batch latency regression:** If `batch_avg_latency_us` spikes, check
   whether GPU dispatch is active (`stats.gpu_dispatched == true`). If the
   workload dropped to CPU, verify the circuit-breaker state and device health.

### §6.4 Testing the Geo Backend

The geo backend has dedicated tests in `tests/test_geo_gpu_backend.cpp` covering:
- Geometry correctness (contains, intersects, disjoint)
- Edge-case / degenerate inputs
- Count-vector mismatch handling
- Large-batch stress
- Concurrent access

Run targeted tests:
```bash
ctest --preset linux-ninja-release -R test_geo_gpu_backend --output-on-failure
```

---

## Emergency Procedures

### §7.1 Complete GPU Subsystem Shutdown

If the GPU module is causing service instability and must be disabled immediately:

1. Revoke all GPU capability grants — existing in-flight work completes; new work
   uses CPU:
   ```cpp
   GPUPolicy policy; // the shared instance
   policy.revokeAll("all-callers"); // once per caller ID
   ```
2. Force the circuit breaker open to prevent new GPU dispatch:
   ```cpp
   GPUSafeFail::GetInstance().forceFailed("emergency shutdown");
   ```
3. Restart the service with `GPUConfig::max_vram_bytes = 0` to operate in
   CPU-only mode until the root cause is resolved.

### §7.2 MIG Partition Emergency Cleanup (Enterprise/Hyperscaler)

If a MIG partition is stuck assigned to a crashed tenant:

List active partitions:
```bash
curl http://localhost:8080/admin/gpu/mig | jq .
```

Destroy via ThemisDB admin CLI:
```bash
themisdb admin gpu mig destroy --instance-id <instance_id>
```

This calls `MIGManager::destroyPartition()` which evicts the tenant assignment
and returns the GPU slice to the shared pool.

---

## Maintenance

### §8.1 Resetting GPU Metrics

Metric counters persist in memory and reset on restart. To reset without a restart:

```cpp
GPUMetrics::GetInstance().reset();
GPUAuditLog::GetInstance().clear();
```

### §8.2 Memory Pool Defragmentation

If fragmentation ratio climbs above 30% (visible in `gpu_memory_pool_fragmentation`
metric), trigger manual defragmentation:

```cpp
auto result = pool.defragment();
// result.bytes_moved, result.slabs_compacted, result.new_fragmentation_ratio
```

Defragmentation is non-disruptive — in-use slabs are not moved; only free slabs
are compacted.

### §8.3 Profiler Trace Collection

To collect a Chrome-format GPU profiling trace for a specific operation:

```cpp
#include "themis/gpu/profiler.h"
using namespace themis::gpu;

{
    ScopedGPURange r("index-build-phase-2");
    // ... operation ...
}

std::string trace = GPUProfiler::GetInstance().exportChromeTrace();
// Write to file and open in chrome://tracing or Perfetto
```

NVTX markers are emitted automatically when `THEMIS_ENABLE_NVTX` is defined at
build time. ROCm rocTX markers require `THEMIS_ENABLE_ROCTX`.
