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
   sample. The method limits its returned sample, but retained history may grow
   beyond 1,000 records because worker-completion paths do not apply the same
   trimming as event handlers.
3. Collect access-model log entries (`access_model::eviction`,
   `access_model::promotion_decision`, `access_model::tier_transition`, and
   `access_model::coordinator_lifecycle`) for that window.
4. Check the caller's tier map and listener wiring. `initialize()` currently
   accepts an empty or incomplete map; it is not a registration health check.
5. Verify the destination tier's real backend state independently. Current
   worker code reports bookkeeping outcomes without invoking tier `get`/`put`.

Redact keys and other customer identifiers before sharing logs outside the
restricted incident channel; keys are included in structured log records.

## Symptom: no promotion decision

- Confirm `setAgePolicy()` was called before sending events. Without a policy,
  event handlers increment observation counters and return.
- Compare the event's `access_count` with `storage_promotion_threshold`.
- For cold-storage access, verify that `STORAGE_WARM` is in the caller's tier
  map; otherwise the event handler cannot queue its selected destination.
- Confirm listener setters were given a live coordinator pointer and that the
  coordinator has not been shut down.

If these checks pass but no transition record appears, preserve the policy,
event metadata, counters, logs, and a minimal reproducer for engineering review.

## Symptom: result says success but data did not move

Treat this as a module limitation, not a storage recovery condition. The worker
currently creates a successful result without invoking a tier operation; the
demotion path similarly returns a result after checking registry keys. Do not
retry blindly or delete the source copy based on this status. Keep authoritative
data in its existing tier and route reads/writes through the owning backend.

Disable the integration at its owner by unregistering the cache/storage
listener. If the owner has no safe live toggle, use its established deployment
rollback procedure; this module does not define one.

## Symptom: shutdown or pending work

`shutdown()` sets the running state false, wakes workers, and joins them. Workers
exit on that state and do not drain the queue. Do not use shutdown as a queue
flush. If work is pending, preserve logs and backend state, disable new listener
events, and escalate before restart or retry decisions. The queue depth is not
available through the public API.

## Escalation packet

Include build/revision, policy values, tier registration summary, listener
ownership/lifetime, coordinator lifecycle state, metrics snapshot, transition
records, relevant redacted logs, and independent backend read/write evidence.
Escalate to the access-model and owning cache/storage maintainers when observed
success diverges from backend state, when futures fail to complete, or when
events disappear during shutdown.

For normal metric interpretation see [`OPERATIONS.md`](./OPERATIONS.md); for
source-level failure modes see [`TRUST.md`](./TRUST.md).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
