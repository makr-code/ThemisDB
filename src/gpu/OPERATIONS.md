---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# GPU Operations

## Audience and deployment boundary

This page is for service operators configuring and monitoring the GPU module.
Backend binaries, drivers, devices, and cluster connectivity are deployment
dependencies; `src/gpu` provides policy, orchestration, metrics, audit, and
admin surfaces but does not install or repair host GPU drivers.

## Startup configuration

Validate `GPUConfig` before enabling workloads. Review at least:

- `max_vram_bytes` and `min_free_vram_bytes` against the edition and device;
- `oom_warning_threshold`, pool slab size/count, and whether pooling is
  enabled;
- circuit failure/success thresholds and reset timeout;
- `enable_cpu_fallback` and `fallback_cpu_budget_ms` (zero disables budget
  enforcement);
- audit capacity and metrics enablement;
- cluster configuration, which is disabled by default for single-node use.

Configuration defaults are not a production sizing recommendation. Follow
[PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md), and treat feature
flags as separate from compile-time backend support. The build can disable a
requested backend when its compiler/toolkit/SDK is unavailable.

## Routine monitoring

Use the module's metrics registry, `GPUModule` snapshots, and enabled admin
surfaces to trend:

- total and failed allocations, per-tenant usage, peak/global VRAM usage;
- GPU dispatch failures, fallback count/reason, circuit-open state, and
  recovery;
- device availability/health and backend selection;
- latency and tail latency for the benchmark-mapped workload paths.

Compare against the deployment's release baseline and thresholds in
[PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md). The module does
not define a universal numeric latency SLO for every device and workload.
Alarm on sustained fallback or unavailable diagnostics in addition to
threshold breaches; a CPU-safe unit test does not provide production hardware
health evidence.

## Change rollout

For backend, memory-limit, or feature-flag changes:

1. Record the current config, feature state, driver/runtime, and benchmark
   baseline.
2. Validate config and run the relevant focused tests on the target build.
3. Roll out to a canary/workload slice with metrics and audit enabled.
4. Compare correctness, allocation denials, fallback/circuit state, and
   latency/VRAM with baseline before increasing traffic.
5. Roll back the changed backend/config if correctness is uncertain, failures
   persist, or quotas/latency exceed approved limits. Preserve diagnostics
   before rollback.

For active incidents use [RUNBOOK.md](RUNBOOK.md). For recovery and mandatory
production controls, see [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md)
and [SECURITY.md](SECURITY.md).
