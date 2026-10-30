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

# Access Model Implementation

## Purpose and source ownership

This guide is for developers changing the coordinator, policy, metrics, logging,
or trace context. Public contracts are in `include/access_model/`; implementation
is in `src/access_model/`.

| Responsibility | Implementation |
|---|---|
| Coordinator lifecycle, event handling, task queue, transition history | `access_coordinator.cpp` |
| Age/count predicates, hotness classification, policy validation | `age_based_policy.cpp` |
| Counters and latency histograms | `access_metrics.cpp` |
| Transition/event log records | `access_model_logging.cpp` |
| Thread-local correlation context | `access_model_trace.cpp` |
| Operation/result data structures | `promotion_demotion.cpp` |

`src/access_model/CMakeLists.txt` lists these sources and defines the standalone
`themis_access_model` static target when one does not already exist. That target
uses C++17, `spdlog::spdlog`, and `Threads::Threads`; the primary core source
inventory is also registered in `cmake/CMakeLists.txt`.

## Runtime sequence

1. Construct a coordinator with `createAccessCoordinator(worker_count)`; the
   default worker count is four.
2. Call `initialize()` with the tier map, set an `AgeBasedPolicy`, then call
   `start()` to create workers.
3. Cache and storage adapters can send events through the listener interfaces.
   The coordinator applies its event-specific thresholds and queues eligible
   work.
4. Read aggregate counters/histograms and recent transition records through the
   coordinator; structured records use the project logger.
5. Unregister external listeners before destroying the coordinator, then call
   `shutdown()` to stop and join its workers.

## Current implementation limits

The source behavior is narrower than the intended tier-transition contract;
these limits must be accounted for before integrating or claiming production
data movement:

- `initialize()` copies the supplied map and returns true; it does not reject
  an empty map, null tier pointer, missing cache/storage tier, or unhealthy
  tier.
- `setAgePolicy()` stores the policy without calling `AgeBasedPolicy::isValid()`.
- `pending_tasks_` is an unbounded `std::queue`; no `max_queue_depth` setting or
  queue-depth accessor is implemented.
- `processPromotionTask()` creates a successful result and transition record
  without calling any registered tier's `get()` or `put()`. The demotion path
  checks that both tier keys exist, then returns a success result without
  transferring data.
- `getKeyMetrics()` and `getTierMetrics()` currently return default-initialized
  metrics. `getAccessModelMetrics()` returns coordinator counters/histograms.
- The coordinator does not currently call the metrics histogram recording
  methods; its histogram fields remain unpopulated by coordinator event/task
  processing.
- `shutdown()` stops workers; a worker exits when `running_` becomes false and
  does not drain queued work.
- `promoteAsync()` stores a weak reference to its promise in the queued task.
  The local owning pointer is not retained by the coordinator after the call,
  so callers must not assume the returned future will receive a completed
  `PromotionResult`.
- Event-handler paths trim transition history after 1,000 records, but worker
  completion does not apply the same trimming rule; the history can grow beyond
  1,000. `getRecentTransitions(limit)` returns insertion order when the history
  fits within `limit`, but newest-first when it truncates. Do not rely on a
  stable ordering or bounded history until this is corrected.

These are implementation facts, not desired behavior. Tests with mock tiers do
not prove a real tier transfer. Track fixes with the module roadmap and add
regression tests before changing these contracts.

## Validation

Focused module tests are defined in `tests/access_model/CMakeLists.txt` with
targets named `module_access_model_<test-file-stem>_focused`. From a configured
build directory, run:

```bash
ctest -R 'module_access_model_.*_focused' --output-on-failure
```

For policy edge cases, run the promotion/demotion test; for concurrency changes,
run `test_coordination_concurrency`; for logging/context changes, run
`test_access_model_observability`. Benchmark thresholds and evidence status are
maintained in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).

See [`API.md`](./API.md) for caller contracts and
[`TRUST.md`](./TRUST.md) for logging and trust-boundary risks.

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
