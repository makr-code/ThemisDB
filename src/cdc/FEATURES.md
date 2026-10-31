---
Author: "ThemisDB Contributors"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Exact model backend not verifiable from final document text alone.
---

# CDC features

The CDC module exposes change-event persistence, retrieval, subscription,
replay, delivery coordination, transport, and administration APIs. These are
module capabilities, not a claim that every optional transport is enabled or
that all production-readiness gates have passed. Current implementation
readiness and outstanding work are tracked in [`ROADMAP.md`](./ROADMAP.md).

## Implemented surfaces

- **Changefeed:** `themis::Changefeed` assigns sequence numbers, stores event
  records in RocksDB, supports filtered reads and push subscriptions, and
  provides retention and redaction operations.
- **Delivery:** `DeliveryTracker` manages in-memory pending acknowledgements and
  redelivery; `ConsumerGroupManager` persists group configuration and committed
  offsets while keeping in-flight records in process memory.
- **Replay and failure handling:** replay controllers read bounded event ranges;
  `DeadLetterQueue` and the outbox provide distinct persistence and recovery
  paths.
- **Transports:** `KafkaCDCProducer` requires `THEMIS_ENABLE_KAFKA` and
  librdkafka. `WsTransport` is available only with
  `THEMIS_ENABLE_WEBSOCKET`.
- **Administration and integrations:** CDC admin/metrics interfaces expose
  diagnostics and retention controls; separate APIs cover tenant buffers,
  cross-collection streams, Debezium formatting, schema encoding, and
  materialized-view updates.

The outbox writer records an entry through a caller-provided RocksDB
transaction. The relay later appends to the changefeed and updates outbox state
in separate operations; this is not a single atomic operation across the
application write, feed append, and relay state.

## Verification

Focused unit tests are registered from `tests/cdc/test_*.cpp` by
[`tests/cdc/CMakeLists.txt`](../../tests/cdc/CMakeLists.txt). Their presence is
not evidence that a build or test run passed. See
[`IMPLEMENTATION.md`](./IMPLEMENTATION.md) for the focused validation map and
[`include/cdc/README.md`](../../include/cdc/README.md) for build instructions.

**Exact model backend not verifiable from final document text alone.**
