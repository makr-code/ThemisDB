# Architecture - CDC Module

<!-- Status: current | validated: 2026-05-31 -->
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

## Sourcecode Verification (Module: cdc/architecture)

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