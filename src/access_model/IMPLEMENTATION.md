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

## Runtime contracts and remaining limits

- `initialize()` rejects empty, cache-only/storage-only, null, unhealthy,
  `UNKNOWN`, or tier-identity-mismatched registries. It validates adapters but
  leaves their initialization and lifetime to the embedding application.
- `setAgePolicy()` and `setPromotionThresholds()` reject invalid threshold
  combinations with `std::invalid_argument`; invalid updates do not replace the
  active policy.
- The task queue and pending demotion-plan registry each use the configured
  `max_queue_depth`. The default is 10,000; use the two-argument factory overload
  to set a deployment-specific bound. Aggregate metrics expose current and
  configured queue depth. Event callbacks are `void`, so inspect failure
  counters/history when queue admission is rejected.
- Promotion calls source `get()` then destination `put()` and keeps the source.
  Demotion calls destination `put()` before source `invalidate()`. A failed
  source invalidation reports failure even though the destination copy may
  exist. The coordinator cannot make independent backends transactional and
  does not retry or deduplicate operations.
- Tier calls happen outside the coordinator mutex. The adapter's `get()`,
  `put()`, health check, and invalidation results determine operation status.
  An eviction callback whose key is no longer readable from its source adapter
  fails safely instead of reporting data movement.
- The coordinator retains per-key counters for up to 10,000 recently touched
  keys and per-tier counters for registered tiers. Aggregate histograms record
  event-processing, policy-decision, promotion, and demotion latency.
- Accepted queued work is drained during `shutdown()`; a currently blocked
  adapter call can delay its return. Stop/unregister event producers first.
- Transition history is bounded to 1,000 records and returned newest-first.
- Policy handlers currently use only selected age/access thresholds; they do not
  implement per-key/tag overrides or frequency-based policy.

The focused coordinator tests use stateful test tiers to verify actual
read/write/invalidate calls. They do not establish persistence, atomicity, or
recovery guarantees for production adapters; adapter-level integration and
representative-hardware evidence remain necessary.

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
