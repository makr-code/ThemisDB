---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Updates Module Research and Evaluation

This page records the engineering questions and evidence used to evaluate update
orchestration. It is for maintainers comparing update strategies; it is not a
claim that every listed benchmark has been run on current hardware.

## Problem and assumptions

An update changes files and, in some deployments, schema or traffic state. A
partial failure can leave a node or cluster inconsistent. The module therefore
uses explicit state transitions, checksummed artifacts, rollback identifiers,
and bounded rollout stages rather than treating an update as one opaque action.
The relevant implementation is in
[`update_state_machine.cpp`](update_state_machine.cpp),
[`hot_reload_engine.cpp`](hot_reload_engine.cpp), and
[`delta_update_engine.cpp`](delta_update_engine.cpp).

Evaluation assumes that callers provide the correct deployment topology,
version, and update configuration. In particular, the canary controller selects
membership deterministically from the version and node ID; it does not itself
coordinate nodes. See [`canary_rollout.h`](../../include/updates/canary_rollout.h).

## Methods and trade-offs

- **Full artifact versus delta:** `DeltaUpdateEngine` can apply per-file binary
  patches or report a file for full-download fallback. Deltas save transfer
  bytes when the patch is smaller, but add base-version, path, hash, decode,
  ordering, and atomic-replacement failure surfaces.
- **Progressive rollout:** canary stages limit initial exposure and track
  observed successes/errors. A stable node identifier makes stage membership
  repeatable, but per-node controllers do not constitute cluster consensus.
- **Blue/green:** deploy to a standby slot, promote, then retain the
  `HotReloadEngine` rollback ID for recovery. This trades an additional slot and
  rollout state for a bounded promotion/rollback path.
- **State journal and checkpoints:** transitions are recorded as JSON-lines;
  checkpoints support rollback. The state-machine implementation detects
  in-flight history at startup, but this detection alone is not proof that an
  interrupted operation has been safely resumed or rolled back.

## Evaluation protocol

Use focused regressions before comparing performance:

1. Use the CMake-registered suite from
   `test_updates_contract_hardening_focused.cpp` for stable error-code and
   deployment-contract expectations.
2. Use `test_updates_phase2_phase3_hardening_focused.cpp` for patch round-trip,
   corruption rejection, and migration behavior.
3. Use `test_updates_rollback_hardening_focused.cpp` and
   `test_updates_determinism_edge_cases_focused.cpp` for rollback and failure
   boundaries.
4. Use `test_updates_operator_diagnostics_focused.cpp` to verify scenario
   classification and that recovery/alert descriptions are available.
5. Compare release-profile measurements using the benchmark-to-metric mapping
   in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

That mapping defines expectations such as bounded state/manifest/delta paths,
coordinated throughput, canary recovery time, migration duration, and memory
growth. Record machine, compiler, build profile, workload, sample count, and
baseline with each measurement; do not compare numbers from different
environments as if they were controlled results.

## Limits and open evidence

The module contains no statistical inference or learned update-selection
algorithm. The behavior under study is deterministic control flow and
artifact-processing behavior. Benchmark targets in
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) are evaluation
criteria, not current measurements; refresh baselines on representative
deployment hardware before using them for capacity or release decisions.

## Source of truth

Implementation and public contracts take precedence over this research summary:
[`ROADMAP.md`](ROADMAP.md), [`ARCHITECTURE.md`](ARCHITECTURE.md),
[`update_state_machine.h`](../../include/updates/update_state_machine.h),
[`delta_update_engine.h`](../../include/updates/delta_update_engine.h), and
the focused tests under [`tests/updates`](../../tests/updates/).

> Exact model backend not verifiable from final document text alone.
