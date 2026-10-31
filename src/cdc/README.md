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
Canonical source references: "include/cdc/changefeed.h; include/cdc/cdc_delivery_contract.h; src/cdc/changefeed.cpp; src/cdc/consumer_group.cpp; src/cdc/outbox.cpp; tests/cdc/CMakeLists.txt"
Related milestone: "DOC-WEEKLY-2026-44"
---

# ThemisDB CDC module

CDC provides a RocksDB-backed changefeed and separate APIs for subscriptions,
replay, delivery tracking, consumer groups, transports, buffering, and
administration. It does not install a universal storage commit hook: the host
application determines when to call `Changefeed::recordEvent` and how to expose
the module's operations.

## Runtime behavior

- `Changefeed::recordEvent` assigns a sequence, fills an unset timestamp,
  serializes the event, persists it in RocksDB, and notifies push subscribers.
- `Changefeed::listEvents` reads by sequence bounds and optional filters.
  `ConsumerGroupManager` persists group configuration and committed offsets,
  while in-flight delivery records are held in process memory.
- Delivery is at-least-once where specified by the contract; consumers should
  handle duplicate delivery. Replay, group acknowledgement, and direct
  subscriptions are distinct mechanisms.
- `OutboxWriter` stores an outbox record using the supplied RocksDB transaction.
  `OutboxRelay` later writes the event to the feed and updates outbox state in
  separate operations; those relay operations are not atomic together.
- Kafka and WebSocket transports depend on `THEMIS_ENABLE_KAFKA` and
  `THEMIS_ENABLE_WEBSOCKET`, respectively. Verify the configured build before
  relying on an optional adapter.

## Source map

| Concern | Implementation |
|---|---|
| Feed, sequence, persistence, retention | [`changefeed.cpp`](./changefeed.cpp) |
| Buffer and tenant buffer management | [`changefeed_buffer.cpp`](./changefeed_buffer.cpp), [`tenant_buffer_manager.cpp`](./tenant_buffer_manager.cpp) |
| Replay, delivery tracking, consumer groups | [`delivery_tracker.cpp`](./delivery_tracker.cpp), [`consumer_group.cpp`](./consumer_group.cpp) |
| WebSocket and Kafka | [`ws_transport.cpp`](./ws_transport.cpp), [`kafka_cdc_producer.cpp`](./kafka_cdc_producer.cpp) |
| Outbox and dead-letter queue | [`outbox.cpp`](./outbox.cpp), [`dead_letter_queue.cpp`](./dead_letter_queue.cpp) |
| Admin and stream integrations | [`cdc_admin.cpp`](./cdc_admin.cpp), [`cross_collection_stream.cpp`](./cross_collection_stream.cpp), [`cdc_materialized_view.cpp`](./cdc_materialized_view.cpp) |

Public declarations and normative delivery invariants are in
[`../../include/cdc/`](../../include/cdc/README.md) and
[`../../include/cdc/cdc_delivery_contract.h`](../../include/cdc/cdc_delivery_contract.h).
Example deployment configuration is under
[`../../config/data_management/`](../../config/data_management/cdc_retention.yaml):
`cdc_retention.yaml` covers retention/compaction and `cdc_kafka.yaml` covers the
optional Kafka producer. Resolve Kafka secrets through `ConfigPathResolver`;
do not commit credentials.
See [`ARCHITECTURE.md`](./ARCHITECTURE.md) for the data flow,
[`API.md`](./API.md) for API details, and [`ROADMAP.md`](./ROADMAP.md) for
implementation status and open work.

## Focused tests

[`tests/cdc/CMakeLists.txt`](../../tests/cdc/CMakeLists.txt) discovers each
`test_*.cpp` and registers a `module_cdc_<test-name>_focused` build target and
corresponding CTest entry. Configure a build with tests enabled, then build and
run the selected target; the test registration itself does not imply a
successful build or execution.

The generated [`DOXYGEN.md`](./DOXYGEN.md) is a 2026-09-20 snapshot; its source
commit was not recorded and it was not regenerated during the current review
because Doxygen is unavailable. Do not rely on its extracted signatures or
counts as current until the documented generator is run.

**Exact model backend not verifiable from final document text alone.**
