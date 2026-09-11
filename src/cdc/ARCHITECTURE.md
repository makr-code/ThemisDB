# Architecture - CDC Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The cdc module composes event capture, buffering, replay, and transport delivery into a unified change-stream runtime. It provides structured reliability and operational control for downstream consumers and integration planes.

## Main Execution Planes

1. Capture and event plane
- change capture and event construction paths
- event buffering and retention-oriented retrieval behavior

2. Delivery and transport plane
- SSE/WebSocket/Kafka-facing transport integration surfaces
- consumer-group and acknowledgement/retry control paths

3. Reliability and recovery plane
- dead-letter handling and replay-aware delivery tracking
- transactional outbox and backpressure-aware buffering support

4. Integration and operations plane
- cross-collection stream and materialized-view integration
- CDC admin controls and operational diagnostics surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| capture/replay interfaces | record and replay change events deterministically |
| delivery interfaces | manage acknowledgement, redelivery, and ordering contracts |
| transport interfaces | expose CDC events to external/internal consumers |
| admin interfaces | provide retention/control and operational visibility |

## Failure Semantics

- malformed or unsupported CDC artifacts fail with structured errors.
- transport/backend degradation remains explicit and observable.
- delivery/replay failure classes remain bounded by configured controls.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| storage | `include/storage/rocksdb_wrapper.h` | Reads committed write batches from RocksDB to construct change events |
| utils | `include/utils/` (time, serialization, concurrency helpers) | Event timestamping, payload serialisation, and buffer synchronisation primitives |
| sharding | `include/sharding/` (shard routing) | Routes change events to the correct shard-local consumer partitions |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/cdc/cdc_ws_handler.h`, `include/cdc/changefeed.h`, `include/cdc/consumer_group.h`, `include/cdc/delivery_tracker.h`, `include/cdc/icdc_transport.h` | WebSocket CDC push, changefeed management, consumer-group registration, and transport dispatch at the HTTP/WS boundary |
| transaction | `include/cdc/changefeed.h` | Transactional changefeed integration; MVCC commit triggers CDC event emission |
| storage | `include/cdc/changefeed.h` | Storage-layer write events feed the CDC capture pipeline |
| replication | `include/cdc/schema_registry.h` | Schema registry consumed by replication to deserialise CDC event payloads |

## Integration Points

### Critical Integration: ChangeFeed ↔ Storage Write Path
**Files:** `include/cdc/changefeed.h` ↔ `src/storage/` write commit path
**Contract:** `ChangeFeed::onCommit(write_batch)` is called synchronously during the storage commit finalisation phase; the change event is appended to `ChangeBuffer` before the commit acknowledgement is returned to the transaction layer.
**Thread Safety:** `ChangeFeed::onCommit()` is called under the storage commit lock; the `ChangeBuffer` append is lock-free via a SPSC queue per shard.
**Failure Mode:** If `ChangeBuffer` is full the commit still proceeds but the change event is dropped and a `cdc.buffer_overflow` counter is incremented; downstream consumers must handle sequence gaps via resume-token replay.

### Critical Integration: ConsumerGroup ↔ Server WebSocket Handler
**Files:** `include/cdc/consumer_group.h`, `include/cdc/cdc_ws_handler.h` ↔ `src/server/`
**Contract:** `ConsumerGroup::subscribe(session_id, filter)` registers a WebSocket session for filtered change delivery; `DeliveryTracker` tracks per-session ack positions and handles redelivery on reconnect.
**Thread Safety:** Consumer registration and event fan-out run on separate thread pools; access to the consumer registry is protected by a `std::shared_mutex`.
**Failure Mode:** Client disconnection triggers graceful consumer removal; unacknowledged events within the retention window are replayed on reconnect. Events beyond the retention window produce a `CURSOR_EXPIRED` error.

### Critical Integration: SchemaRegistry ↔ Replication
**Files:** `include/cdc/schema_registry.h` ↔ `src/replication/`
**Contract:** The replication layer calls `SchemaRegistry::getSchema(schema_id)` to decode CDC event payloads at the replica; schema IDs embedded in events must be resolvable before the payload is applied.
**Thread Safety:** `SchemaRegistry` caches resolved schemas in a read-optimised concurrent map.
**Failure Mode:** Unresolvable schema ID halts replication for the affected partition and emits a `SCHEMA_NOT_FOUND` error to the replication error log.



- Verified files:
  - src/cdc/changefeed.cpp
  - src/cdc/changefeed_buffer.cpp
  - src/cdc/consumer_group.cpp
  - src/cdc/delivery_tracker.cpp
  - src/cdc/dead_letter_queue.cpp
  - src/cdc/ws_transport.cpp
  - src/cdc/kafka_cdc_producer.cpp
  - src/cdc/outbox.cpp
  - src/cdc/cdc_admin.cpp
- Verified architecture claims:
  - explicit capture, delivery, reliability, and operations planes
  - bounded failure behavior for transport and delivery paths
  - dedicated CDC-layer runtime composition for stream pipelines