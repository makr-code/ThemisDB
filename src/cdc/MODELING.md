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
Canonical source references: "include/cdc/changefeed.h; include/cdc/cdc_delivery_contract.h; include/cdc/consumer_group.h; include/cdc/outbox.h; include/cdc/dead_letter_queue.h"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC data and state model

This page describes the data shapes declared by the CDC public headers. The
declarations and delivery contract are authoritative; this guide does not
define a second serialization or persistence contract.

## Change event

[`Changefeed::ChangeEvent`](../../include/cdc/changefeed.h) is the common event
record. It carries a sequence, operation type, key, optional value, timestamp,
metadata, optional before/after snapshots, and a redaction marker. `recordEvent`
assigns a sequence and fills a missing timestamp before persisting the
serialized event. Event type includes PUT, DELETE, transaction commit, and
transaction rollback.

The delivery contract defines sequence zero as “not assigned,” requires
monotonic sequence use as the resume cursor, and sets maximum key/value sizes.
`Changefeed::ListOptions` uses an exclusive lower `from_sequence`, an inclusive
`to_sequence` when nonzero, and supports a result limit and key/event filters.
See [`changefeed.cpp`](./changefeed.cpp) and
[`cdc_delivery_contract.h`](../../include/cdc/cdc_delivery_contract.h).

## Consumer-group state

[`ConsumerGroupConfig`](../../include/cdc/consumer_group.h) identifies a group
and its consumer count. The group’s committed sequence is durable offset state;
`ConsumerGroupManager` tracks in-flight records separately in memory. Thus a
committed offset and a live in-flight delivery are different state:

| State | Purpose | Persistence declared by source |
|---|---|---|
| Group configuration | Group identity and consumer count | RocksDB |
| Committed sequence | Resume position after acknowledgement | RocksDB |
| In-flight delivery records | Per-consumer pending sequence and timeout tracking | Process memory |

The delivery contract requires deterministic key partitioning and persistence
of committed offsets before acknowledgement returns. It does not imply that
the in-flight map survives process restart.

## Outbox and dead-letter records

- [`OutboxRecord`](../../include/cdc/outbox.h) has its own sequence and state
  (`PENDING`, `PUBLISHED`, or `FAILED`), event content, timestamps, attempts,
  failure reason, and metadata. `OutboxWriter::writeToOutbox` accepts the
  caller's RocksDB transaction. `OutboxRelay` forwards pending records to the
  `Changefeed`; consult its implementation before assuming a broader
  transaction boundary.
- [`DLQEntry`](../../include/cdc/dead_letter_queue.h) wraps an original change
  event, failure reason, attempt count, enqueue timestamp, and a DLQ-local
  sequence. `DeadLetterQueue::replay` records the event in a changefeed; the
  new record has the changefeed's sequence.

These are separate state machines and sequence spaces. Do not treat outbox,
DLQ, group offset, and changefeed sequence numbers as interchangeable.

## Source references

- [Change event and feed API](../../include/cdc/changefeed.h)
- [Normative delivery contract](../../include/cdc/cdc_delivery_contract.h)
- [Consumer-group model](../../include/cdc/consumer_group.h)
- [Outbox model](../../include/cdc/outbox.h)
- [DLQ model](../../include/cdc/dead_letter_queue.h)
- [Replay and acknowledgement tests](../../tests/cdc/test_cdc_replay_ack_hardening.cpp)

**Exact model backend not verifiable from final document text alone.**
