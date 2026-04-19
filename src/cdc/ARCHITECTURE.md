> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# CDC Module — Architecture Guide
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/cdc/ -->

**Version:** 1.1
**Last Updated:** 2026-04-06
**Status:** current
**Module Path:** `src/cdc/`

---

## 1. Overview

The Change Data Capture (CDC) module intercepts mutations written to ThemisDB and streams
them to subscribers in real time. Consumers use CDC to drive downstream analytics pipelines,
cache invalidation, audit trails, replication, and event-driven architectures.

The primary delivery mechanisms are Server-Sent Events (SSE), WebSocket, and Apache Kafka
(via `KafkaCDCProducer`, built with `THEMIS_ENABLE_KAFKA`).  All transport backends implement
the `ICDCTransport` abstract interface (`include/cdc/icdc_transport.h`).

---

## 2. Design Principles

- **Append-Only Change Log** – every mutation is first appended to a durable change log
  before being streamed, guaranteeing durability and enabling historical replay.
- **Filtered Subscriptions** – clients subscribe with fine-grained filters (collection,
  key prefix, operation type) to receive only relevant events.
- **At-Least-Once Delivery** – events may be re-delivered on reconnect; consumers must be
  idempotent.
- **Tenant Isolation** – each tenant's change buffer is independent; a noisy tenant cannot
  starve others.
- **Backpressure** – slow consumers cause the server to buffer up to a configurable high-water
  mark, then drop oldest events (with a gap marker).

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `changefeed.cpp` | Core change capture engine: intercepts writes, routes to buffers |
| `changefeed_buffer.cpp` | Per-tenant in-memory ring buffer for pending events |
| `tenant_buffer_manager.cpp` | Manages per-tenant buffer lifecycle and quota enforcement |
| `cdc_admin.cpp` | Admin API: list/create/delete subscriptions, flush buffers |
| `ws_transport.cpp` | WebSocket transport backend (`WsTransport`, implements `ICDCTransport`) |
| `cdc_ws_handler.cpp` | WebSocket HTTP handler: wires CDC streams into the HTTP server |
| `consumer_group.cpp` | Consumer group semantics with durable offset tracking (`ConsumerGroupManager`) |
| `delivery_tracker.cpp` | At-least-once delivery with redelivery and acknowledgement (`DeliveryTracker`) |
| `dead_letter_queue.cpp` | Persistence for events that exhaust delivery retries (`DeadLetterQueue`) |
| `outbox.cpp` | Transactional outbox pattern for atomic CDC + application data publishing (`OutboxWriter`, `OutboxRelay`) |
| `cross_collection_stream.cpp` | Cross-collection change aggregation (`CrossCollectionStream`) |
| `cdc_materialized_view.cpp` | CDC-driven incremental materialized view maintenance: bridges `Changefeed::ChangeEvent` to `analytics::IncrementalViewManager` |
| `kafka_cdc_producer.cpp` | Kafka transport backend: polls changefeed and publishes events to Apache Kafka topics via librdkafka (opt-in, `THEMIS_ENABLE_KAFKA`) |
| `icdc_transport.h` | Abstract transport interface (`ICDCTransport`) implemented by all CDC delivery backends |

### 3.2 Component Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│               Write Path (storage / transaction)                 │
│   rocksdb_wrapper.put() / transaction.commit()                   │
└───────────────────────────┬──────────────────────────────────────┘
                            │ CDC hook (post-commit)
┌───────────────────────────▼──────────────────────────────────────┐
│                     ChangeFeed Engine                            │
│  captures: operation, collection, key, before/after values       │
└───────────────────────────┬──────────────────────────────────────┘
                            │
           ┌────────────────┴──────────────────┐
           │                                   │
┌──────────▼─────────────┐        ┌────────────▼──────────────────┐
│   Persistent ChangeLog  │        │   TenantBufferManager          │
│   (RocksDB WAL-backed)  │        │   per-tenant ring buffers      │
└──────────┬──────────────┘        └────────────┬──────────────────┘
           │                                    │ fan-out to subscribers
           │                       ┌────────────┴──────────────────┐
           │                       │     SSE Streamer               │
           │                       │  (one goroutine-like strand    │
           │                       │   per active subscription)     │
           │                       └────────────┬──────────────────┘
           │                                    │
           └────────────────────────────────────┘
                        historical replay
```

---

## 4. Data Flow

### 4.1 New Subscription

```
Client: GET /changefeed?collection=users&op=insert,update
    │
    ▼
Server validates subscription params + auth (tenant scope)
    │
    ▼
TenantBufferManager: register subscription filter
    │
    ▼
SSE connection established; HTTP response stays open
    │
    ▼
Ongoing: ChangeFeed → filter match? → buffer → SSE frame → client
```

### 4.2 Historical Replay

```
Client: GET /changefeed?collection=users&since=<cursor>
    │
    ▼
ChangeLog: iterator from cursor position
    │
    ▼
Replay historical events (filtered) → SSE frames
    │
    ▼
Transition seamlessly to live feed at log tail
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Receives events from** | `src/storage/` | Post-commit write hooks |
| **Receives events from** | `src/transaction/` | Transaction commit hooks |
| **Delivers to** | `src/server/` | SSE HTTP handlers |
| **Delivers to** | `src/cache/` | Cache invalidation events |
| **Delivers to** | `src/analytics/` | `IncrementalViewManager::applyChange()` via `CDCMaterializedViewMaintainer` |
| **Uses** | `src/observability/` | CDC throughput and lag metrics |

---

## 6. Threading & Concurrency Model

- `ChangeFeed` hook runs on the storage commit thread; it must be fast and non-blocking
  (enqueue to buffer and return immediately).
- `TenantBufferManager` uses per-tenant mutex to serialize buffer access.
- Each active SSE subscription runs on a dedicated I/O strand (Boost.Asio or equivalent).
- `ChangeLog` persistence uses a separate background writer thread to avoid blocking the
  commit path.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Lock-free ring buffer | Per-tenant event buffer uses lock-free MPSC queue |
| Backpressure | High-water mark triggers oldest-event eviction + gap marker |
| Batch SSE frames | Multiple events bundled into a single SSE frame under load |
| ChangeLog compaction | Old change log segments are compacted and archived |

---

## 8. Security Considerations

- Subscription requests are authenticated and scoped to the tenant's visible collections.
- Cross-tenant event leakage is prevented by per-tenant buffer isolation.
- `before` field values are masked for fields with `encryption` or `pii` annotations.
- Admin API (`cdc_admin.cpp`) requires admin role.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `cdc.buffer.max_events_per_tenant` | 100000 | Ring buffer capacity per tenant |
| `cdc.buffer.high_watermark_pct` | 80 | % full before oldest-event eviction |
| `cdc.changelog.retention_days` | 7 | Change log retention period |
| `cdc.sse.keepalive_interval_s` | 30 | SSE keepalive ping interval |
| `cdc.max_subscriptions_per_tenant` | 100 | Max concurrent subscriptions |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Buffer overflow (high-water mark) | Drop oldest events; inject gap marker in SSE stream |
| ChangeLog write failure | Log error; continue in-memory; warn on replay gaps |
| Slow SSE client | Backpressure: buffer; eventually evict subscription |
| Client disconnect | Clean up subscription; buffer events briefly for reconnect |

---

## 11. Known Limitations & Future Work

- WebSocket transport is implemented (`cdc_ws_handler.cpp`); HTTP server wiring is a follow-up item.
- Kafka transport (`kafka_cdc_producer.cpp`) is implemented; requires `THEMIS_ENABLE_KAFKA` build flag and a librdkafka installation.
- Consumer offset tracking (durable cursor persistence per consumer) is available via `ConsumerGroupManager`.
- Per-subscription delivery guarantees are at-least-once; exactly-once requires consumer
  idempotency.

---

## 12. References

- `src/cdc/README.md` — module overview
- `src/cdc/FUTURE_ENHANCEMENTS.md` — roadmap
- `docs/CDC_IMPLEMENTATION_SUMMARY.md` — implementation history
- `docs/CDC_OPERATIONS_RUNBOOK.md` — operational runbook
- `ARCHITECTURE.md` (root) — full system architecture
