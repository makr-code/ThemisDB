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
Canonical source references: "include/cdc/cdc_delivery_contract.h; include/cdc/changefeed.h; src/cdc/changefeed.cpp; src/cdc/consumer_group.cpp; src/cdc/delivery_tracker.cpp; src/cdc/outbox.cpp"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC implementation guide

Use this page when changing CDC internals. Public declarations in
[`include/cdc/`](../../include/cdc/README.md) and the frozen
[`cdc_delivery_contract.h`](../../include/cdc/cdc_delivery_contract.h) define
the interfaces and invariants; implementation files provide the concrete
behavior.

## Main runtime paths

1. **Record and read.** `Changefeed::recordEvent` assigns the next sequence,
   fills an unset timestamp, serializes the event, writes it to RocksDB, and
   then notifies subscribers. `listEvents` uses sequence bounds and filters.
   See [`changefeed.cpp`](./changefeed.cpp).
2. **Replay.** `ICDCReplayController` creates bounded replay sessions with
   batch iteration and terminal `Done`/`Cancelled` states. Keep replay
   independent of live subscriber state as required by the interface.
3. **Consumer groups.** `ConsumerGroupManager` stores group configuration and
   committed offsets in RocksDB; its in-flight records are held in memory.
   Its fetch path re-fetches timed-out events and filters newly fetched events
   by key partition. See [`consumer_group.cpp`](./consumer_group.cpp).
4. **Acknowledgement/redelivery.** `DeliveryTracker` holds per-consumer pending
   events, accepts single or cumulative acknowledgements, and returns pending
   events eligible for redelivery. Its configuration includes timeout and
   pending-cap controls. See [`delivery_tracker.cpp`](./delivery_tracker.cpp).
5. **Outbox.** `OutboxWriter` writes a record through the supplied RocksDB
   transaction. `OutboxRelay` scans pending records, writes them to the
   changefeed, and updates their state. The outbox and feed writes are distinct
   operations in the current implementation; do not describe them as one
   atomic transaction. See [`outbox.cpp`](./outbox.cpp).
6. **Optional transport.** Kafka is guarded by `THEMIS_ENABLE_KAFKA` and
   librdkafka configuration. Verify the configured build rather than inferring
   support from the public type declaration.

## Invariants for changes

- Preserve the contract's sequence cursor, at-least-once acknowledgement,
  replay state transitions, transport failure classification, and consumer
  group consistency rules.
- Apply backpressure when configured pending limits are reached; do not
  silently advance delivery state without acknowledgement.
- Keep operation failures observable through the existing return/error and
  diagnostic surfaces.
- Treat the roadmap's outstanding DLQ/outbox hardening and degraded transport
  validation as open work, not as evidence of completed behavior.
- When changing a public method, update its header/API documentation and the
  focused regression tests in the same change.

## Focused validation map

| Change area | Test source |
|---|---|
| Replay bounds, draining, cancellation, cumulative ack | [`test_cdc_replay_ack_hardening.cpp`](../../tests/cdc/test_cdc_replay_ack_hardening.cpp) |
| Pending limits, timeouts, partitioning, ack rejection | [`test_cdc_transport_degradation.cpp`](../../tests/cdc/test_cdc_transport_degradation.cpp) |
| Health, watermarks, metrics and JSON diagnostics | [`test_cdc_diagnostics_hardening.cpp`](../../tests/cdc/test_cdc_diagnostics_hardening.cpp) |
| Outbox transitions | [`test_cdc_outbox.cpp`](../../tests/cdc/test_cdc_outbox.cpp) |
| Redaction and tombstones | [`test_cdc_gdpr_redaction.cpp`](../../tests/cdc/test_cdc_gdpr_redaction.cpp) |

Run the relevant `module_cdc_*_focused` target from the configured CMake build;
the module test registration is in
[`tests/cdc/CMakeLists.txt`](../../tests/cdc/CMakeLists.txt).

## Source references

- [Normative CDC delivery contract](../../include/cdc/cdc_delivery_contract.h)
- [Changefeed implementation](./changefeed.cpp)
- [Consumer-group implementation](./consumer_group.cpp)
- [Delivery tracker implementation](./delivery_tracker.cpp)
- [Outbox implementation](./outbox.cpp)
- [CDC module roadmap](./ROADMAP.md)

**Exact model backend not verifiable from final document text alone.**
