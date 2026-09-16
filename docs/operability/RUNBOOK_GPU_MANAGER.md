# Runbook: GPU Manager

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB GPU manager module.  Covers the
five most critical incident classes, their log patterns, triage steps, and
recommended remediation actions.

---

## Scenario 1 — GPU OOM (Kernel Out-of-Memory)

**Log pattern:** `[GPU:KernelOOM]`

**Symptoms**
- A GPU kernel launch fails because VRAM is exhausted.
- Log lines contain `[GPU:KernelOOM]` with the kernel type, device index, and
  requested allocation size.
- The affected operation fails closed to CPU; GPU utilisation remains high.

**Triage**
1. Check VRAM utilisation per device: `nvidia-smi --query-gpu=memory.used,memory.free`.
2. Identify the operation type that caused the OOM (query, training, index build).
3. Review the VRAM allocation policy in `IVRAMPolicy` — check quota settings.
4. Look for any VRAM leak indicators: allocations not freed after operation end.

**Remediation**
1. Free unused VRAM pools: `themis_admin gpu vram flush --device <N>`.
2. Reduce per-operation VRAM budget: `gpu.vram_budget_mb` in device configuration.
3. Enable VRAM recycling: `gpu.vram_recycling: true`.
4. If a VRAM leak is suspected, restart the GPU manager service.

**Escalation**  
If OOM recurs after VRAM flush, attach VRAM allocation timeline metrics and
escalate to the GPU platform team.

---

## Scenario 2 — Kernel Timeout

**Log pattern:** `[GPU:KernelTimeout]`

**Symptoms**
- A GPU kernel exceeds the 5-second SLA timeout enforced by `KernelSLAGuard`.
- Log lines contain `[GPU:KernelTimeout]` with kernel ID, device, and elapsed ms.
- The operation is aborted and falls back to CPU.

**Triage**
1. Identify the kernel type and input size that triggered the timeout.
2. Check for GPU thermal throttling: `nvidia-smi -q -d TEMPERATURE,POWER`.
3. Review concurrent kernel count — too many parallel kernels cause queuing delays.
4. Check whether the kernel is compute-bound or memory-bandwidth-bound.

**Remediation**
1. Reduce maximum concurrent kernels: `gpu.max_concurrent_kernels`.
2. If the kernel is expected to take > 5 s for large inputs, split it into
   smaller sub-operations.
3. Ensure thermal management is functioning; clean GPU cooling if temperature is
   above threshold.
4. For persistent timeouts, investigate if the workload requires a larger device.

**Escalation**  
Kernel timeouts on every GPU simultaneously indicate a systemic issue; escalate
to the infrastructure on-call with kernel profile data.

---

## Scenario 3 — Multi-GPU Routing Failure

**Log pattern:** `[GPU:RoutingFailed]`

**Symptoms**
- The GPU load balancer cannot route an operation to any device.
- Log lines contain `[GPU:RoutingFailed]` with the requested topology and
  available device list.
- All devices may report as unhealthy simultaneously.

**Triage**
1. Check load balancer state: `themis_admin gpu loadbalancer status`.
2. Verify device health for all GPU devices: `themis_admin gpu status --all`.
3. Review topology configuration — a misconfigured peer-transfer graph can
   cause routing deadlock.
4. Check whether all devices were marked unhealthy by a cascade from one failure.

**Remediation**
1. Reset device health state: `themis_admin gpu health reset --all`.
2. Reload the routing topology: `themis_admin gpu topology reload`.
3. If a topology misconfiguration is detected, roll back to the previous known-
   good topology file.
4. Reduce topology complexity to direct routing (no peer transfer) while
   investigating the cascade.

**Escalation**  
Routing failures affecting all devices require immediate escalation; page the
GPU infrastructure on-call.

---

## Scenario 4 — CUDA Driver Crash

**Log pattern:** `[GPU:DriverCrash]`

**Symptoms**
- The CUDA driver has crashed or become unresponsive on one or more devices.
- Log lines contain `[GPU:DriverCrash]` with device index and error code.
- All GPU operations on the affected host fail immediately.

**Triage**
1. Check `dmesg` for GPU hardware error messages or Xid error codes.
2. Confirm whether the crash is isolated to one device or affects all GPUs.
3. Check whether a watchdog timer or kernel reset was triggered.
4. Review recent kernel activity logs for any runaway compute.

**Remediation**
1. For a soft crash (driver reset possible without reboot):
   `themis_admin gpu driver reset --device <N>`.
2. Disable the affected device: `themis_admin gpu device disable --index <N>`.
3. Schedule a host reboot during the next maintenance window for a hard crash.
4. If Xid errors indicate hardware failure, flag the device for replacement.

**Escalation**  
CUDA driver crashes are P1 incidents; escalate to hardware operations immediately
with `dmesg` output, Xid codes, and `nvidia-bug-report` output.

---

## Scenario 5 — Memory Leak Detection

**Log pattern:** `[GPU:MemoryLeak]`

**Symptoms**
- VRAM usage increases monotonically without a corresponding workload increase.
- Log lines contain `[GPU:MemoryLeak]` with device index and unreleased bytes.
- Operations eventually fail with OOM after extended uptime.

**Triage**
1. Capture a VRAM allocation snapshot: `themis_admin gpu vram snapshot`.
2. Compare consecutive snapshots to identify the growing allocation class.
3. Review recent code changes for missing `CudaDeviceMemoryGuard` usage.
4. Check for any operations that bypass the RAII allocation lifecycle.

**Remediation**
1. Flush idle VRAM pools: `themis_admin gpu vram flush --device <N>`.
2. Restart the GPU manager to force release all tracked allocations.
3. Deploy a patch that wraps the leaking allocation site with
   `CudaDeviceMemoryGuard` / `IVRAMPolicy`.
4. Enable allocation tracking in debug builds to confirm the leak source.

**Escalation**  
If the leak rate exceeds 100 MB/hour and cannot be reproduced, escalate with
VRAM allocation timeline and the most recent snapshot diff.

---

## Reference

| Log Pattern            | Scenario                      |
|------------------------|-------------------------------|
| `[GPU:KernelOOM]`      | GPU OOM                       |
| `[GPU:KernelTimeout]`  | Kernel Timeout                |
| `[GPU:RoutingFailed]`  | Multi-GPU Routing Failure     |
| `[GPU:DriverCrash]`    | CUDA Driver Crash             |
| `[GPU:MemoryLeak]`     | Memory Leak Detection         |

**Related resources**
- `src/gpu/ROADMAP.md` — Wave D operability tracking
- `tests/integration/test_gpu_manager_soak.cpp` — Soak test evidence
- `tests/gpu/test_gpu_highcardinality_stress.cpp` — Stress test evidence
- `benchmarks/gpu/bench_gpu_dedicated_gates.cpp` — GPU-BM-01..04 gates
- `include/gpu/cuda_raii.h` — CUDA RAII wrappers
- `include/themis/gpu/gpu_timeout.h` — KernelSLAGuard
