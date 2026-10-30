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

# Failover Module Guide

## Audience and boundaries

Use this guide to navigate the failover module before changing code or
integrating its public API. Public declarations live in `include/failover/`;
runtime code lives in `src/failover/`; focused module tests live in
`tests/failover/`. The `server` module owns the authenticated administrative
surface; failover classes are not command-line handlers and do not implement
caller authentication.

## Start here

1. Read [ARCHITECTURE.md](ARCHITECTURE.md) for ownership and failure boundaries.
2. Read [API.md](API.md) before calling or changing public interfaces.
3. Read [IMPLEMENTATION.md](IMPLEMENTATION.md) before modifying state transitions
   or DR step behavior.
4. Check [RUNBOOK.md](RUNBOOK.md) and [OPERATIONS.md](OPERATIONS.md) before
   changing operator-visible errors or telemetry.
5. Use [TRUST.md](TRUST.md) for the trust assumptions around manager inputs,
   fencing, and the local quorum log.

## Code and test map

| Concern | Implementation | Focused tests |
|---|---|---|
| Health timeouts and adaptive checks | `auto_failover_manager.cpp` | `test_failover_wave_a_health_timeout.cpp`, `test_failover_wave_b_adaptive_health.cpp` |
| Fencing and promotion | `auto_failover_manager.cpp` | `test_failover_wave_a_fencing.cpp`, `test_failover_wave_c_fencing_security.cpp` |
| Durable quorum decisions | `quorum_log.cpp` | `test_failover_wave_a_quorum_persistence.cpp` |
| Topology snapshots | `topology_snapshot.cpp` | `test_failover_wave_a_topology_versioning.cpp` |
| DR validation, sequencing, and hooks | `disaster_recovery_manager.cpp` | `test_failover_dr_edge_scenarios.cpp`, `test_failover_phase2_phase3_focused.cpp` |
| Queue pressure and concurrency | `auto_failover_manager.cpp` | `test_failover_chaos_scenarios.cpp`, `test_failover_contract_hardening_focused.cpp` |

The module CMake file `tests/failover/CMakeLists.txt` creates focused targets
from `test_failover_*.cpp` and registers them with the `failover` and
`release_critical` labels. Use the repository's configured CMake build directory
and run the relevant registered tests with CTest, for example:

```sh
ctest --test-dir <build-directory> -L failover --output-on-failure
```

This command assumes the project has already been configured and the relevant
test targets have been built.

## Change checklist

- Preserve fail-closed quorum and epoch-fencing behavior.
- Keep queue capacity and retry behavior bounded and observable.
- For DR changes, test dry-run behavior, hooks, failed-step stop behavior,
  plan-ID caching, and partial side effects.
- Do not describe a successful built-in restore, traffic shift, spare activation,
  or metadata update as a completed external action; see
  [IMPLEMENTATION.md](IMPLEMENTATION.md).
- Update API, operations, runbook, and trust docs when their contracts change.
