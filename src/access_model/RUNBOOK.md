---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Access Model Runbook

## Purpose and safety boundary

Use this runbook to diagnose coordinator events that are missing, delayed, or
reported as successful without a corresponding tier-side effect. It applies to
the in-process coordinator in `src/access_model`; it is not a runbook for
database authentication or authorization.

The module does not expose an HTTP status endpoint or queue-drain command.
Operators must use the application integration that owns the coordinator,
`AccessCoordinator` query methods, and the project logger. Do not use
unverified endpoints or treat a coordinator success counter as proof of data
movement.

## First response

1. Record the incident time window, affected tier pair, configuration/policy
   version, and whether the coordinator reports `isRunning()`.
2. Capture `getAccessModelMetrics()` and a bounded `getRecentTransitions(limit)`
   sample. Inspect `pending_queue_depth`, `max_queue_depth`, failure counters,
   and newest-first transition records; retained history is capped at 1,000.
3. Collect access-model log entries (`access_model::eviction`,
   `access_model::promotion_decision`, `access_model::tier_transition`, and
   `access_model::coordinator_lifecycle`) for that window.
4. Check that `initialize()` returned true, all registered adapters report the
   expected tier identity and health, and both cache and storage tiers exist.
   The coordinator does not initialize adapters for the application.
5. Verify source and destination backend state independently. Promotion reads
   then writes; demotion writes then invalidates. A reported failure after
   destination placement can mean both copies remain.

Redact keys and other customer identifiers before sharing logs outside the
restricted incident channel; keys are included in structured log records.

## Symptom: no promotion decision

- Confirm `setAgePolicy()` was called before sending events. Without a policy,
  event handlers increment observation counters and return.
- Compare the event's `access_count` with `storage_promotion_threshold`.
- For cold-storage access, verify that `STORAGE_WARM` is in the caller's tier
  map; otherwise the event handler cannot queue its selected destination.
- Confirm listener setters were given live host-owned bridge pointers. The
  bridges must implement the listener interfaces and forward to a running
  coordinator; `AccessCoordinator` is not itself an `EvictionListener` or
  `PromotionListener`.

If the queue is full, a callback cannot return an admission status. The
coordinator records a failed transition/counter; reduce event ingress or raise
capacity only after workload sizing and adapter checks. For `promoteAsync()`,
inspect the completed failure future.

If these checks pass but no transition record appears, preserve the policy,
event metadata, counters, logs, and a minimal reproducer for engineering review.

## Symptom: result says success but data did not move

Inspect the result's `error_message` and the transition record. A successful
promotion means source `get()` and destination `put()` succeeded; promotion
retains the source. A successful demotion also requires source invalidation
after destination placement. If invalidation fails, the result is a failure and
both copies may remain. Compare backend contents before retrying or cleaning up;
the coordinator does not provide cross-tier transactions, retries, or
deduplication.

Disable the integration at its owner by unregistering the cache/storage
listener. If the owner has no safe live toggle, use its established deployment
rollback procedure; this module does not define one.

## Symptom: shutdown or pending work

`shutdown()` stops queue admission, wakes workers, drains accepted tasks, and
joins them. A blocked adapter call can delay it. Unregister listener producers
before shutdown. If an adapter is stuck, preserve logs/backend state and follow
the owning application's established process-level recovery procedure; the
module has no cancellation or force-drain API.

## Escalation packet

Include build/revision, policy values, tier registration summary, listener
ownership/lifetime, coordinator lifecycle state, metrics snapshot, transition
records, relevant redacted logs, and independent backend read/write evidence.
Escalate to the access-model and owning cache/storage maintainers when backend
state differs from the reported result, queue-full rejection persists, an
adapter reports partial demotion, a future fails to complete, or shutdown
exceeds the adapter's expected operation window.

For normal metric interpretation see [`OPERATIONS.md`](./OPERATIONS.md); for
source-level failure modes see [`TRUST.md`](./TRUST.md).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
