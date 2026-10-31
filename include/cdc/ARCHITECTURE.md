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
SOT domain: "api-contract"
Canonical source references: "include/cdc/README.md; include/cdc/cdc_delivery_contract.h; include/cdc/changefeed.h; include/cdc/consumer_group.h; include/cdc/outbox.h"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC public-header architecture

`include/cdc/` declares interfaces used by the CDC feed and its surrounding
delivery components. The implementation and data flow are described in
[`../../src/cdc/ARCHITECTURE.md`](../../src/cdc/ARCHITECTURE.md); the complete
header inventory is in [`README.md`](./README.md).

## Interface groups

- **Feed and persistence:** `Changefeed` is declared in namespace `themis`;
  `ChangefeedBuffer` shares the feed event type and buffering APIs.
- **Delivery and replay:** `DeliveryTracker`, `ConsumerGroupManager`, and
  `ICDCReplayController` expose separate acknowledgement, group-offset, and
  replay-session mechanisms.
- **Transport:** `ICDCTransport` defines `start`, `stop`, and `publish`.
  `KafkaCDCProducer` and `WsTransport` are conditional on their build flags.
- **Durable failure handling:** `OutboxWriter` writes with a caller-provided
  RocksDB transaction; `OutboxRelay` forwards records later. These stages are
  not one atomic operation. `DeadLetterQueue` exposes a separate failure queue.
- **Operations:** `CDCAdmin` and `CDCMetrics` expose control, diagnostic, and
  metric data structures; the deployment determines how these are authorized
  and exported.
- **Event transformations and integration:** schema encoding, filtering,
  fan-in, compression, cross-collection streams, and materialized-view
  maintenance are separate interfaces rather than an implicit end-to-end
  pipeline.

Most public types reside in `themis::cdc`; `Changefeed` and
`ChangefeedBuffer` are in `themis`. See the declarations for exact names,
ownership, and feature guards. `cdc_delivery_contract.h` is the canonical
source for sequence, delivery, replay, and transport-failure invariants.

## Source references

- [CDC public header inventory](./README.md)
- [Delivery contract](./cdc_delivery_contract.h)
- [Feed API](./changefeed.h)
- [Consumer-group API](./consumer_group.h)
- [Outbox API](./outbox.h)
- [Implementation architecture](../../src/cdc/ARCHITECTURE.md)

**Exact model backend not verifiable from final document text alone.**
