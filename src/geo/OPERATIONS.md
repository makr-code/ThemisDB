---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
Source_Level: "L1"
SOT_Domain: "operations"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Geo operating guidance

## Purpose and audience

This page describes steady-state health, capacity, and performance checks for
operators and service owners. It is not a symptom-by-symptom recovery
procedure; use [RUNBOOK.md](./RUNBOOK.md) for incident response.

## Runtime health and backend state

- Keep the distinction between compile-time support and runtime availability:
  CUDA/HIP source selection is a build choice, while device detection and
  backend selection depend on the deployed runtime.
- Capture `getGeoDeviceReportJson()` and
  `getGpuSpatialBackendStatsJson()` when establishing a baseline and when
  comparing a degraded deployment. The spatial server handler includes GPU
  backend statistics in its response; scrape it only through the owning
  server's authorized API.
- Where the application exposes `GeoOperatorDiagnostics`, trend incident
  counts and severities alongside service-level request failures and latency.
  The local recent-incident buffer is capped at 256 entries and does not
  replace durable metrics, logs, or alerts.
- Treat CPU fallback as an availability path, not a GPU health signal. Track
  the selected backend and fallback state separately when comparing
  performance.

## Query budgets and capacity

`SpatialQueryPolicy` provides caller-visible defaults of 100,000 index
candidates, depth 64, a 5-second timeout, and 1,000,000 join result pairs.
Zero has field-specific semantics: candidate/result-pair caps are documented
as unlimited, depth selects the implementation default, and timeout disables
timeout enforcement. Tune these values against actual data and query
distributions.

These are policy fields, not proof of universal enforcement. The policy
validator checks non-negative depth and timeout and non-empty allowlist
entries; the header describes execution-layer enforcement for other values.
Verify that the service/query path actually attaches and consumes the policy
before treating a setting as an effective resource ceiling. An empty
`allowed_collections` list is open, and authorization remains the server's
responsibility.

Spatial joins can produce quadratic result counts in the worst case; prefer
selective filters, a justified pair cap, and representative load testing.
GPU DBSCAN's documented maximum is configurable and exists to bound its
adjacency-matrix memory use; larger inputs use CPU according to the clustering
interface. Check the deployed configuration and implementation before
capacity planning.

## Performance evidence

Use [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) to select
mapped benchmark cases and the current baseline. The six cases in
[`bench_geo_release_gates.cpp`](../../benchmarks/geo/bench_geo_release_gates.cpp)
use seeded, CPU-only mock implementations and publish p99 gate thresholds;
they are not production-path throughput evidence and do not measure a GPU.

For a release comparison, record build revision, compiler/build profile,
hardware, backend actually used, input dimensions, warm-up/repetition setup,
and benchmark manifest. Compare like-for-like cases against the stored
baseline. A threshold or configured target alone is not a passing measurement.
The [roadmap](./ROADMAP.md) separately tracks fresh focused executable
evidence; do not treat historical test results as current deployment health.

## Change and rollback controls

- Roll out GPU, precision, or query-policy changes using the owning service's
  staged deployment and observation procedure. The geo module does not define
  a live configuration reload or service rollback mechanism.
- Preserve a known-good artifact and backend configuration before rollout.
- For a CPU-only recovery, build and test with geo CUDA and HIP dispatch
  disabled, then deploy through the service owner's approved rollback path.
  Validate the same request workload and inspect latency/fallback evidence
  before considering the incident closed.
- Do not clear an index or diagnostics buffer until the relevant evidence is
  captured and the component owner confirms the operation is safe.

## Sourcecode verification

- Runtime diagnostics: [`spatial_backend.h`](../../include/geo/spatial_backend.h),
  [`geo_operator_diagnostics.h`](../../include/geo/geo_operator_diagnostics.h),
  and [`spatial_api_handler.cpp`](../../src/server/spatial_api_handler.cpp).
- Policy defaults and validation:
  [`geo_policy.h`](../../include/geo/geo_policy.h) and
  [`geo_policy.cpp`](./geo_policy.cpp).
- Benchmark mapping and status:
  [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md),
  [`bench_geo_release_gates.cpp`](../../benchmarks/geo/bench_geo_release_gates.cpp),
  and [`ROADMAP.md`](./ROADMAP.md).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
