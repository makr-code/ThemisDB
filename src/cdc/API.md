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
SOT domain: "api-contract"
Canonical source references: "include/cdc/cdc_delivery_contract.h; include/cdc/changefeed.h; include/cdc/icdc_replay_controller.h; include/cdc/delivery_tracker.h; include/cdc/consumer_group.h; include/cdc/icdc_transport.h; include/cdc/cdc_admin.h"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC public API guide

The public C++ declarations under
[`include/cdc/`](../../include/cdc/README.md) are normative. This guide
summarizes their main contracts and is not a replacement for the headers or
Doxygen comments.

## API areas

| Concern | Primary interface | Contract notes |
|---|---|---|
| Capture, event listing, retention and subscriptions | [`Changefeed`](../../include/cdc/changefeed.h) | Records a `ChangeEvent`; lists events by sequence/filter options; exposes watermarks and retention controls. |
| Replay | [`ICDCReplayController`](../../include/cdc/icdc_replay_controller.h) | Creates replay sessions with batch, state, cancellation and delivered-count methods. |
| Per-consumer delivery | [`DeliveryTracker`](../../include/cdc/delivery_tracker.h) | Tracks pending events and exposes acknowledgement and redelivery methods. |
| Group offsets and partitions | [`ConsumerGroupManager`](../../include/cdc/consumer_group.h) | Manages group configuration, committed offsets, key partitions and at-least-once fetch/ack operations. |
| Transport adapters | [`ICDCTransport`](../../include/cdc/icdc_transport.h) | Defines `start`, `stop`, and single-event `publish`; concrete adapters add their own configuration. |
| Admin and diagnostics | [`CDCAdmin`](../../include/cdc/cdc_admin.h) | Exposes purge/replay/compaction/redaction, retention status, health and diagnostics methods. |

## Contract details

- The normative contract is
  [`cdc_delivery_contract.h`](../../include/cdc/cdc_delivery_contract.h).
  It defines sequence constraints, at-least-once delivery, replay terminal
  states, transport failure classes, fail-closed cases, consumer-group rules,
  and diagnostic obligations.
- `Changefeed::ListOptions::from_sequence` is an exclusive lower bound;
  `to_sequence` is an inclusive upper bound when nonzero. A zero upper bound
  means no upper sequence bound.
- `DeliveryTracker` defaults include a 30-second acknowledgement timeout and
  a 10,000-event pending cap per consumer. The contract declares that the cap
  must trigger backpressure rather than silent dropping.
- Transport method success/failure does not establish application-level
  authorization. The hosting API/server must enforce its own access policy.
- Kafka support depends on `THEMIS_ENABLE_KAFKA` and librdkafka; consult the
  concrete adapter header and build configuration.

## Compatibility

The delivery contract header states that it is stable within the active v1.x
line and that breaking changes require a v2.0 contract bump, migration notes,
and a changelog entry. Review the module
[`CHANGELOG.md`](./CHANGELOG.md) and root release/versioning policy before
changing public behavior.

## Source references

- [Public CDC header index](../../include/cdc/README.md)
- [Normative delivery contract](../../include/cdc/cdc_delivery_contract.h)
- [Changefeed API](../../include/cdc/changefeed.h)
- [Replay API](../../include/cdc/icdc_replay_controller.h)
- [Delivery API](../../include/cdc/delivery_tracker.h)
- [Admin API](../../include/cdc/cdc_admin.h)

**Exact model backend not verifiable from final document text alone.**
