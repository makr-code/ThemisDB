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

# Analytics Module Guide

## Start here

This guide is for engineers onboarding to analytics or changing a component.
Use this order to avoid treating a subsystem summary as the API contract:

1. Read [README.md](./README.md) for scope and known readiness limits.
2. Read [ARCHITECTURE.md](./ARCHITECTURE.md) for execution planes and module
   boundaries.
3. Read [ROADMAP.md](./ROADMAP.md) and [MODULE_GAPS.md](./MODULE_GAPS.md) for
   status; treat dated scanner totals as snapshots rather than live source
   evidence.
4. Identify the owning public header under
   [`include/analytics`](../../include/analytics/) and implementation here.
5. Find the narrowest existing suite under
   [`tests/analytics`](../../tests/analytics/) and inspect its fixtures before
   editing.
6. Check this document's linked
   [API map](./API.md), [implementation contracts](./IMPLEMENTATION.md), and
   relevant [operations guidance](./OPERATIONS.md).

## Choosing the right subsystem

- Use OLAP/aggregation contracts for grouped analytical query execution.
- Use streaming window or CEP APIs for arrival-ordered event processing; pick a
  concrete window type and set resource limits for production workloads.
- Use `ForecastModel` for time-series fit/predict/evaluate and
  `AnomalyDetector` for scored observations; these are distinct workflows and
  their model lifecycle differs.
- Use `ProcessMining` for event-log discovery/conformance and
  `ProcessPatternMatcher` for similarity queries. Do not call the latter's
  character-trigram representation a pretrained semantic embedding.
- Use `ModelServingEngine` for in-process registered AutoML models and
  `MLServingClient` for optional ONNX/TF Serving integration.
- Use exporter APIs for explicit format/status handling; use distributed
  analytics only when the caller can account for partial shard outcomes.

## Build and test discovery

`tests/analytics/CMakeLists.txt` creates a focused executable for each
`test_*.cpp` source and registers it with the `AnalyticsFocusedTests` CTest
label. In a configured build:

1. List module-focused tests with
   `ctest --test-dir <build> -N -L AnalyticsFocusedTests`.
2. Build the matching `module_analytics_test_<name>_focused` target and run
   its registered CTest case; the executable also accepts GoogleTest filters
   for narrower iteration.
3. Use the repository's documented CMake preset and broader CTest suite for
   final verification.
4. For performance-sensitive changes, use the direct cases listed in
   [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md), then compare
   against the manifest-driven hardware baseline artifacts.

The source and test files are authoritative for exact targets and fixtures;
build options can omit optional serving/export backends. Run the existing
doc-gate validation when changing documentation, and validate internal links
from the changed files.

## Change checklist

- Preserve the shared limits/error semantics in
  [`analytics_api_contract.h`](../../include/analytics/analytics_api_contract.h).
- Add or update tests for invalid input, missing optional capabilities, and
  boundary/limit conditions relevant to the code path.
- Keep performance claims tied to a named benchmark and baseline artifact.
- Keep security requirements in [SECURITY.md](./SECURITY.md),
  [TRUST.md](./TRUST.md), and `PRODUCTION_REQUIREMENTS.md` aligned.
- If changing process mining, account for the explicitly enabled Windows stub
  build documented in [IMPLEMENTATION.md](./IMPLEMENTATION.md).
