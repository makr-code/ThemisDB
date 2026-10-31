---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "module-behavior"
Canonical source references: "include/cdc/changefeed.h; include/cdc/consumer_group.h; include/cdc/outbox.h; include/cdc/cdc_delivery_contract.h; src/cdc/changefeed.cpp; src/cdc/consumer_group.cpp; src/cdc/outbox.cpp"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC module architecture

The CDC module records change events in a RocksDB-backed feed and provides
separate interfaces for subscriptions, replay, consumer-group delivery,
transport adapters, buffering, and administration. The source does not establish
a universal storage commit hook or a deployment-specific server topology; host
applications decide when to record events and how to expose administrative or
transport operations.

## Data flow and state

1. A host calls `themis::Changefeed::recordEvent`. The feed assigns a sequence,
   supplies a timestamp when needed, serializes the event, persists it in
   RocksDB, and notifies registered push subscribers. `listEvents` reads events
   using sequence bounds and optional filters. See
   [`changefeed.h`](../../include/cdc/changefeed.h) and
   [`changefeed.cpp`](./changefeed.cpp).
2. Replay controllers and consumer groups read from the feed. A
   `ConsumerGroupManager` persists group configuration and committed offsets in
   RocksDB; its per-consumer in-flight delivery records are process-local and
   are not restored after restart. See
   [`consumer_group.h`](../../include/cdc/consumer_group.h) and
   [`consumer_group.cpp`](./consumer_group.cpp).
3. Transport adapters publish or poll feed events independently of capture.
   Kafka support is guarded by `THEMIS_ENABLE_KAFKA`; `WsTransport` is guarded
   by `THEMIS_ENABLE_WEBSOCKET`. Their presence in the public headers does not
   prove that a particular build enables the backend.
4. `OutboxWriter` writes an outbox record through the caller-provided RocksDB
   transaction. `OutboxRelay` later forwards pending records to the changefeed
   and updates their state in separate operations. The relay therefore does not
   make the feed append and outbox state update one atomic operation. See
   [`outbox.h`](../../include/cdc/outbox.h) and [`outbox.cpp`](./outbox.cpp).

## Main implementation areas

| Area | Source |
|---|---|
| Feed persistence, sequence assignment, subscriptions, retention | [`changefeed.cpp`](./changefeed.cpp) |
| Buffering and tenant-scoped buffers | [`changefeed_buffer.cpp`](./changefeed_buffer.cpp), [`tenant_buffer_manager.cpp`](./tenant_buffer_manager.cpp) |
| Replay, delivery tracking, and consumer groups | [`delivery_tracker.cpp`](./delivery_tracker.cpp), [`consumer_group.cpp`](./consumer_group.cpp) |
| WebSocket and optional Kafka adapters | [`ws_transport.cpp`](./ws_transport.cpp), [`kafka_cdc_producer.cpp`](./kafka_cdc_producer.cpp) |
| Outbox and dead-letter handling | [`outbox.cpp`](./outbox.cpp), [`dead_letter_queue.cpp`](./dead_letter_queue.cpp) |
| Admin, cross-collection streams, and materialized views | [`cdc_admin.cpp`](./cdc_admin.cpp), [`cross_collection_stream.cpp`](./cross_collection_stream.cpp), [`cdc_materialized_view.cpp`](./cdc_materialized_view.cpp) |

`Changefeed` is declared in namespace `themis`; most other public CDC types
are in `themis::cdc`. The public headers and
[`cdc_delivery_contract.h`](../../include/cdc/cdc_delivery_contract.h) define
the interface and delivery invariants. Consult those sources before inferring
behavior from a transport or host integration.

## Focused tests

CDC tests are under [`tests/cdc/`](../../tests/cdc/). Its CMake file discovers
each `test_*.cpp` and registers a `module_cdc_<test-name>_focused` target with
CTest. The target links `themis_core` and the shared test libraries; runtime
execution still depends on the configured build and its dependencies.

**Exact model backend not verifiable from final document text alone.**
