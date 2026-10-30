---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Acceleration operations

## Purpose and operational boundary

This document helps operators verify backend readiness, observe runtime behavior, and assess performance evidence. Backend support depends on the binary's feature configuration and the host's runtime/driver/device. A backend listed in source or compiled into a build is not by itself proof of operational availability. Use [RUNBOOK.md](./RUNBOOK.md) for incident response.

## Readiness checks

Before enabling an optional backend for a workload:

1. Record the exact build profile and enabled backend features.
2. Confirm driver/runtime initialization and a healthy capability probe on the target host.
3. Check the registry's selected backend for each relevant operation family; a vector selection does not imply graph, geo, or matrix support.
4. Verify the consumer's fallback path on a host/build without the accelerator.
5. Compare the representative workload against the approved release benchmark baseline. Follow [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md); its regression and latency gates require the mapped benchmark run and manifest.

The module roadmap identifies hardware-in-the-loop validation, support tiers, and broader multi-device soak/partial-failure coverage as work that must be evidenced before generalized support claims. Do not translate roadmap targets into current alerts or SLAs without an approved profile-specific threshold.

## Signals available in the module

`DeviceManager::logDeviceInfo()` prints discovered device count, per-device health, backend type, free VRAM, compute version, FP16/BF16 support, and the selected device. The backend registry logs registered and selected backends during initialization. `AiHardwareDispatcher` logs capability availability and records failures while traversing its candidate chain.

`metrics::BackendMetrics` registers metrics under `themis_acceleration_<backend>_`:

- initialization successes/failures and duration
- L2/cosine operation counts, vector counts, and duration histograms
- device-memory usage/availability, queue depth, device count, and active-device gauges
- total errors, kernel launch failures, and allocation failures

`MetricsCollector::exportPrometheus()` and `exportJSON()` serialize registered metrics. These APIs do not themselves establish an HTTP endpoint, and the presence of a registered metric does not prove every backend path records it. Confirm the owning service's exporter and instrumentation before relying on an alert.

For vLLM resource sharing, `VLLMResourceManager::getStats()` reports CPU/RAM/GPU usage and vLLM detection state; `canUseGPU()` is the dispatch gate for its GPU path.

## Performance and capacity operations

- Compare equivalent workload shapes, precision, batch size, and host profiles; keep CPU and accelerator results paired.
- Capture latency distribution and throughput, not only peak throughput. Separate dispatch, transfer/allocation, and kernel time where available.
- Treat the ACC benchmark mappings and baseline thresholds as the release evidence. `AG-1` is defined as regression no greater than 10% versus the release baseline; other mapped latency limits are profile/configuration-specific.
- For multi-device behavior, include topology, device count, utilization, and partial-device conditions. Do not assume linear scaling.

## Escalation and rollback

When readiness or correctness fails, preserve startup diagnostics, build configuration, backend selection, structured error, host/driver versions, and the smallest reproducible workload. Return to the last validated deployment profile; confirm CPU fallback and consumer correctness before restoring optional acceleration. The acceleration module exposes no universal online rollback control.

## Source references

- [Metrics definitions](../../include/acceleration/metrics/backend_metrics.h)
- [Metrics export APIs](../../include/acceleration/metrics/metrics_collector.h)
- [Device diagnostics](./device_manager.cpp)
- [Resource statistics](../../include/acceleration/vllm_resource_manager.h)
- [Release benchmark mappings](./PERFORMANCE_EXPECTATIONS.md)

**Exact model backend not verifiable from final document text alone.**
