# Runbook: Acceleration Module

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB acceleration module.  Covers the
five most critical incident classes, their log patterns, triage steps, and
recommended remediation actions.

---

## Scenario 1 — GPU Kernel Unavailable

**Log pattern:** `[ACCEL:GPUUnavailable]`

**Symptoms**
- Acceleration requests fail to dispatch to any GPU device.
- Log lines contain `[ACCEL:GPUUnavailable]` with backend name and device index.
- All operations fall back to CPU; GPU utilisation drops to zero.

**Triage**
1. Check GPU device health: `themis_admin gpu status --all`.
2. Confirm the CUDA/HIP driver is loaded and responsive:
   `nvidia-smi` / `rocm-smi`.
3. Review the backend registry for any device that has been marked `UNHEALTHY`.
4. Check for recent GPU driver updates that may have introduced incompatibilities.

**Remediation**
1. Restart the GPU backend service: `themis_admin gpu backend restart`.
2. If the driver is unresponsive, schedule a host reboot during the next
   maintenance window.
3. Enable CPU-only mode temporarily: set `acceleration.force_cpu: true` in the
   runtime configuration.
4. File a hardware ticket if all GPUs on a node are simultaneously unavailable.

**Escalation**  
If GPU availability does not recover within 5 minutes of backend restart,
escalate to the infrastructure on-call.

---

## Scenario 2 — Fallback Path OOM

**Log pattern:** `[ACCEL:FallbackOOM]`

**Symptoms**
- CPU fallback path runs out of memory during an accelerated operation.
- Log lines contain `[ACCEL:FallbackOOM]` with the operation ID and byte count.
- System memory pressure metrics spike.

**Triage**
1. Check system memory utilisation at time of failure.
2. Identify the operation type that triggered the fallback OOM.
3. Review concurrent workload count — multiple fallback operations running in
   parallel may exhaust available RAM.
4. Check if the operation size exceeds the configured CPU fallback memory budget.

**Remediation**
1. Increase the CPU fallback memory limit in `acceleration.cpu_fallback_mem_mb`.
2. Reduce the maximum concurrent fallback operations:
   `acceleration.max_concurrent_cpu_fallback`.
3. For large tensor operations, split the workload into smaller chunks.
4. Add system memory to the host if budget permits.

**Escalation**  
Persistent fallback OOM events indicate a sizing problem; escalate to the
capacity planning team with workload profiling data.

---

## Scenario 3 — Dispatch Queue Overflow

**Log pattern:** `[ACCEL:DispatchOverflow]`

**Symptoms**
- Kernel dispatch queue is full; new dispatch requests are being rejected.
- Log lines contain `[ACCEL:DispatchOverflow]` with queue depth and capacity.
- Throughput degrades and latency spikes.

**Triage**
1. Check dispatch queue depth metrics in Prometheus/Grafana.
2. Identify the source of the burst: query acceleration vs training pipeline.
3. Check if individual kernels are taking longer than expected (timeout events).
4. Review upstream rate limiters — are clients respecting backpressure signals?

**Remediation**
1. Increase dispatch queue capacity: `acceleration.dispatch_queue_capacity`.
2. Enable adaptive rate limiting: `acceleration.adaptive_rate_limit: true`.
3. Reduce kernel concurrency to ensure each kernel completes faster.
4. For sustained overflows, scale out the acceleration tier horizontally.

**Escalation**  
If queue overflow persists after capacity increase, escalate with queue depth
time-series and client throughput data.

---

## Scenario 4 — CUDA Context Loss

**Log pattern:** `[ACCEL:CUDAContextLost]`

**Symptoms**
- GPU operations fail with a CUDA context invalidation error.
- Log lines contain `[ACCEL:CUDAContextLost]` with device index.
- All operations on the affected device immediately fail-close to CPU.

**Triage**
1. Check `dmesg` / system journal for GPU hardware error events.
2. Confirm the CUDA driver state: `nvidia-smi -q -d ECC,POWER,TEMP`.
3. Review whether the context loss was triggered by a kernel timeout (> 5 s SLA).
4. Check if another process on the same host caused a driver reset.

**Remediation**
1. Remove the affected device from the pool: 
   `themis_admin gpu device disable --index <N>`.
2. Reinitialise the CUDA context after verifying hardware health:
   `themis_admin gpu context reinit --index <N>`.
3. If the device reports ECC errors, schedule it for hardware replacement.
4. Ensure kernel SLA timeout (`KernelSLAGuard`) is enforced at ≤ 5 seconds to
   prevent context loss from runaway kernels.

**Escalation**  
CUDA context loss on multiple devices simultaneously is a critical incident;
page the infrastructure on-call immediately.

---

## Scenario 5 — HIP Backend Error

**Log pattern:** `[ACCEL:HIPError]`

**Symptoms**
- ROCm / HIP backend reports a runtime error.
- Log lines contain `[ACCEL:HIPError]` with error code and device name.
- Operations on AMD GPU devices fail-close to CPU fallback.

**Triage**
1. Check ROCm runtime status: `rocm-smi --showdriverversion`.
2. Review the HIP error code in the log for actionable context.
3. Confirm the device is not in a soft-hang state: `rocm-smi --showpids`.
4. Check for ROCm driver version mismatches between the host and container.

**Remediation**
1. Restart the HIP backend service: `themis_admin gpu backend restart --hip`.
2. For driver version mismatches, update the container image or host driver.
3. Disable the affected device temporarily and re-enable after driver validation.
4. Consult `docs/gpu/HIP_BACKEND_GUIDE.md` for HIP-specific error codes.

**Escalation**  
Persistent HIP backend errors indicate a ROCm driver or hardware issue; escalate
to the platform team with `rocm-smi` diagnostics and full device logs.

---

## Reference

| Log Pattern                  | Scenario                  |
|------------------------------|---------------------------|
| `[ACCEL:GPUUnavailable]`     | GPU Kernel Unavailable    |
| `[ACCEL:FallbackOOM]`        | Fallback Path OOM         |
| `[ACCEL:DispatchOverflow]`   | Dispatch Queue Overflow   |
| `[ACCEL:CUDAContextLost]`    | CUDA Context Loss         |
| `[ACCEL:HIPError]`           | HIP Backend Error         |

**Related resources**
- `src/acceleration/ROADMAP.md` — Wave D operability tracking
- `tests/integration/test_acceleration_soak.cpp` — Soak test evidence
- `tests/acceleration/test_acceleration_highcardinality_stress.cpp` — Stress evidence
- `benchmarks/acceleration/bench_acceleration_dedicated_gates.cpp` — ACC-BM-01..04
