---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "module-behavior"
Canonical source references: "include/cdc/cdc_admin.h; include/cdc/cdc_delivery_contract.h; include/cdc/dead_letter_queue.h; include/cdc/outbox.h; src/cdc/SECURITY.md"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC incident runbook

Use this runbook to gather evidence and select a safe recovery path. It
documents CDC library surfaces, not deployment-specific commands or an
independent authorization policy. See [`OPERATIONS.md`](./OPERATIONS.md) for
routine monitoring. Avoid purge, reset, retention changes, or DLQ removal until
the affected stream and recovery point are understood.

## 1. Establish impact and current state

1. Use the hosting service's authorized admin path to call
   `CDCAdmin::healthCheck()` and `getDiagnostics()`. Capture health components,
   error counters, event totals, and low/high watermarks from the returned
   values.
2. Call `getRetentionStatus()` and record the active retention policy,
   cleanup-thread state, event count, size, and oldest/newest timestamps.
3. Correlate the affected feed/group/consumer, sequence range, deployment
   version, and transport. Preserve relevant logs with credentials and
   sensitive payloads removed.

## 2. Consumer lag, duplicate delivery, or replay gap

1. Compare the consumer's committed offset with the feed's available
   watermarks. Check in-flight counts/statistics through the relevant
   `ConsumerGroupManager` or `DeliveryTracker` surface.
2. Verify acknowledgement handling. At-least-once delivery permits duplicates;
   consumers should make processing idempotent where required.
3. For replay, confirm the requested lower/upper sequence semantics and whether
   retention has already removed the requested events. `from_sequence` is
   exclusive and `to_sequence` inclusive when nonzero.
4. Do not advance offsets or purge events to make a lag alert disappear.
   Capture a reproducible sequence range and investigate before changing
   consumer state.

## 3. Transport failure or backpressure

1. Capture the adapter's start/publish result and structured error/diagnostic
   context. Confirm build-time adapter availability; Kafka requires the
   configured Kafka build and librdkafka.
2. Classify the fault using `CdcTransportFailureClass` in
   [`cdc_delivery_contract.h`](../../include/cdc/cdc_delivery_contract.h).
   The contract requires fail-closed treatment for backend unreachable,
   authentication failure, malformed payload, backpressure, and internal
   error classes. Do not silently advance acknowledgement state.
3. Restore the backend or capacity according to the host deployment's
   procedure. Verify replay/ack parity and observe the backlog before resuming
   normal traffic.

## 4. DLQ or outbox accumulation

1. Inspect DLQ entries with `DeadLetterQueue::listEntries`/`getEntry`, or
   outbox records with `OutboxRelay::listRecords`/`listAllRecords`. Preserve
   failure reason, attempts, timestamps and record identifiers.
2. Identify whether the failing path is delivery to a transport or relay from
   the outbox. Check the owning backend and the relevant error class before
   retrying.
3. Test recovery on a bounded sample. A DLQ replay records the event into the
   changefeed with a new changefeed sequence; it is not restoration of the
   original delivery cursor.
4. Remove/drain records only under an approved recovery plan with evidence
   retained. The module roadmap still tracks additional DLQ/outbox contract
   hardening.

## 5. Retention or data-subject operation

Retention, purge, compaction, and redaction can affect replay availability or
stored values. Confirm the target key/sequence range, operational approval,
and downstream consequences before invoking admin methods. For redaction,
inspect the result and audit evidence; do not assume that purging and redacting
have equivalent semantics.

## Escalation evidence

Provide the relevant code/build profile, error class, feed/group/consumer
identifiers, sequence/watermark range, retention status, redacted diagnostics,
and minimal reproduction. Do not include event payload secrets in tickets or
logs.

## Source references

- [Admin and diagnostic APIs](../../include/cdc/cdc_admin.h)
- [Delivery failure and replay contract](../../include/cdc/cdc_delivery_contract.h)
- [DLQ API](../../include/cdc/dead_letter_queue.h)
- [Outbox API](../../include/cdc/outbox.h)
- [CDC security summary](./SECURITY.md)

**Exact model backend not verifiable from final document text alone.**
