---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "module-behavior"
Canonical source references: "include/cdc/README.md; include/cdc/changefeed.h; include/cdc/outbox.h; include/cdc/icdc_transport.h; include/cdc/cdc_ws_handler.h; include/cdc/kafka_cdc_producer.h"
Related milestone: "DOC-WEEKLY-2026-44"
---

# Embedding CDC in a ThemisDB application

Here “embedding” means integrating CDC into a host application or service; it
does not refer to vector embeddings. Use the public contracts under
[`include/cdc/`](../../include/cdc/README.md). The host remains responsible for
its transaction policy, authorization, lifecycle, and transport deployment.

## Integration choices

| Need | CDC surface | Integration boundary |
|---|---|---|
| Record and query changes | [`Changefeed`](../../include/cdc/changefeed.h) | Construct with the host's RocksDB `TransactionDB`; the feed records events in RocksDB. |
| Coordinate consumer progress | [`ConsumerGroupManager`](../../include/cdc/consumer_group.h) | Persist group config/offsets and explicitly acknowledge delivered sequences. |
| Publish from a transaction | [`OutboxWriter`](../../include/cdc/outbox.h) and `OutboxRelay` | Write the outbox record using the caller's RocksDB transaction, then run/manage the relay lifecycle. |
| Provide a transport | [`ICDCTransport`](../../include/cdc/icdc_transport.h) | Implement `start`, `stop`, and `publish`; report success/failure to the caller. |
| Serve WebSocket subscriptions | [`CdcWebSocketHandler`](../../include/cdc/cdc_ws_handler.h) | Integrate at the service boundary and preserve that boundary's authz checks. |
| Use Kafka | [`KafkaCDCProducer`](../../include/cdc/kafka_cdc_producer.h) | Confirm `THEMIS_ENABLE_KAFKA` and librdkafka availability for the actual build. |

## Recommended integration sequence

1. Choose one durable source of event state and define how event content and
   sequence cursors are retained. Apply explicit retention limits suitable for
   the deployment.
2. For ordinary recording, construct the feed with the configured RocksDB
   database and call `recordEvent` only when the host can provide the intended
   event semantics. For transactional writes requiring a coordinated event
   record, use the outbox transaction API rather than assuming an independent
   transport publish is atomic with application data.
3. Choose direct subscription, replay, or a consumer group based on the
   consumer's cursor/acknowledgement requirements. CDC delivery is
   at-least-once; consumers should be prepared for duplicate delivery.
4. Start and stop any relay/transport components as part of the owning
   application's lifecycle. Check return values and surface diagnostics rather
   than treating a started component as proof of successful delivery.
5. Keep authentication/authorization at the server or application boundary.
   The minimal `ICDCTransport` interface itself does not define an identity or
   authorization policy.

## Integration constraints

- `Changefeed` and the storage objects passed to CDC have external lifetimes;
  keep them alive for as long as the CDC component uses them.
- A group’s committed offset is durable, while its in-flight delivery state is
  in memory. Design reconnect/restart handling with that distinction in mind.
- Kafka support is build-dependent. Without `THEMIS_ENABLE_KAFKA`, the header
  provides a fallback whose `start()` and `publish()` return `false` unless its
  injectable callbacks are configured; it does not connect to Kafka. The
  current sources do not specify a removal milestone for that fallback.
- Avoid changing event identity, sequence, and acknowledgement handling in an
  adapter without checking
  [`cdc_delivery_contract.h`](../../include/cdc/cdc_delivery_contract.h).

## Source references

- [Public CDC headers and build notes](../../include/cdc/README.md)
- [Event recording and subscription API](../../include/cdc/changefeed.h)
- [Transactional outbox API](../../include/cdc/outbox.h)
- [Transport contract](../../include/cdc/icdc_transport.h)
- [Kafka build guard](../../include/cdc/kafka_cdc_producer.h)
- [CDC delivery contract](../../include/cdc/cdc_delivery_contract.h)

**Exact model backend not verifiable from final document text alone.**
