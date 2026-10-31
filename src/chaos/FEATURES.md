---
Author: ThemisDB Contributors
Created: 2026-10-18
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Chaos module capabilities

## Purpose and audience

This page summarizes source-visible capabilities for developers integrating
chaos simulation into tests. It is not a claim that the module produces real
infrastructure failures or is a production control plane.

## Implemented surface

- `FaultInjector` stores, queries, expires, and recovers process-local fault
  descriptors identified by target and fault type.
- `ChaosScheduler` queues time-triggered descriptors and checks for due entries
  using a configured tick and wake strategy.
- Registered callbacks receive synchronous injection/recovery notifications.
- Server files contain guarded `/admin/chaos/*` handler/route code. **TODO:**
  validate the optional build; CMake scopes `THEMIS_CHAOS_ADMIN` to
  `themis_server` although `http_server.cpp` is compiled into `themis_core`, and
  route setup uses a `ChaosScheduler` constructor not declared by the API.
  CMake defaults the option to `OFF`.

The runtime does not enact the fault labels against network or disk I/O. It
does not enforce all limits recommended in `include/chaos/chaos_contract.h`;
callbacks are not exception-isolated, and pending entries are not capacity
bounded.

## Value and limitations

The idea that these APIs improve customer outcomes, incident response, or
release predictability is **TODO/assumption**; no such outcome is established by
the module code or the benchmark source. Benchmarks declare scenarios and
threshold counters, but successful measurements are not established here.

Canonical evidence: [`README.md`](README.md),
[`ARCHITECTURE.md`](ARCHITECTURE.md),
[`../../include/chaos/chaos_framework.h`](../../include/chaos/chaos_framework.h),
[`../../src/chaos/chaos_framework.cpp`](chaos_framework.cpp), and
[`../../benchmarks/chaos/README.md`](../../benchmarks/chaos/README.md).
