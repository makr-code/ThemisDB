---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Wave D Failover Evidence — Source-Checked Status

> Historical evidence file, rechecked against current source on 2026-10-31.
> Prior statements about tracing, metrics, and soak completion are not supported
> by the current failover test/runtime sources and are withdrawn here.

## What the scoped runtime and test establish

`auto_failover_manager.cpp` exposes callback events, in-process statistics, and
`spdlog` calls. It does not define the previously listed trace spans or export
named metrics. `tests/failover/test_failover_wave_d_observability.cpp` contains
five tests for callback registration/diagnostics/statistics, including health
timeout, missing-fencing, and recovery-failure events. It does not test tracing
or metrics export.

The module CMake file registers this focused test with `failover;release_critical`
labels. This is target registration, not a test result. The separately named
`tests/integration/test_failover_soak_60min.cpp` exists and the integration
CMake file conditionally registers it with `wave_d;soak;not_release_critical`
labels. However, the source contains only one smoke assertion (`EXPECT_TRUE(true)`)
and explicitly says it validates compile/link against the current API; it is not
a 60-minute soak test. No execution result is recorded here. Runbook paths
formerly cited by this bundle are outside this documentation scope and have not
been treated as evidence of runtime instrumentation.

## Status

Distributed tracing, metrics export, and long-duration soak coverage remain
unverified for this module. Confirm requirements and provide source plus
execution artifacts before marking them complete. No build, test, or benchmark
was run during this documentation audit.

See [`OPERATIONS.md`](OPERATIONS.md), [`AUDIT.md`](AUDIT.md), and
`tests/failover/test_failover_wave_d_observability.cpp`.
