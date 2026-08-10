# ThemisDB CDC Module

**Status:** PRODUCTION_CANDIDATE  
**Phase:** 6 (Documentation & Acceptance) — ✅ COMPLETE  
**Last Updated:** 2026-08-10  
**Owner:** Replication & CDC Team

---

## Module Purpose

The CDC module provides change data capture runtime surfaces for ThemisDB, including changefeed recording, stream delivery, replay, transport integration, and operational administration for change-event pipelines. Phase 1-6 complete with all release gates validated.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| changefeed.cpp | core capture/replay/event stream behavior |
| changefeed_buffer.cpp | buffering and retrieval surfaces for change events |
| tenant_buffer_manager.cpp | tenant-scoped buffer lifecycle and quotas |
| cdc_ws_handler.cpp | websocket-facing CDC stream handler |
| ws_transport.cpp | CDC transport implementation over websocket paths |
| kafka_cdc_producer.cpp | Kafka-oriented CDC transport producer integration |
| consumer_group.cpp | consumer group state and offset handling |
| delivery_tracker.cpp | delivery state tracking and acknowledgement surfaces |
| dead_letter_queue.cpp | failed delivery persistence/replay queue |
| outbox.cpp | transactional outbox integration surfaces |
| cross_collection_stream.cpp | cross-collection stream aggregation behavior |
| cdc_materialized_view.cpp | incremental materialized-view maintenance hooks |
| cdc_admin.cpp | CDC administration and operations endpoints |

## Scope

In scope:
- capture, replay, and stream delivery of change events
- transport and consumer-group delivery coordination
- CDC operational admin, buffering, and failure handling surfaces
- CDC integration paths for downstream replication and view maintenance

Out of scope:
- non-CDC business logic in domain modules
- full external pipeline orchestration outside CDC integration contracts
- storage/query execution internals not owned by CDC runtime surfaces

## Runtime Behavior and Limits

- behavior depends on configured transports, retention, and delivery settings.
- delivery semantics are bounded by configured acknowledgement/replay controls.
- degraded transport/backends produce structured runtime failure behavior.

## Sourcecode Verification (Module: cdc/readme)

- Verified files:
  - src/cdc/changefeed.cpp
  - src/cdc/changefeed_buffer.cpp
  - src/cdc/tenant_buffer_manager.cpp
  - src/cdc/cdc_ws_handler.cpp
  - src/cdc/ws_transport.cpp
  - src/cdc/kafka_cdc_producer.cpp
  - src/cdc/consumer_group.cpp
  - src/cdc/delivery_tracker.cpp
  - src/cdc/dead_letter_queue.cpp
  - src/cdc/outbox.cpp
  - src/cdc/cross_collection_stream.cpp
  - src/cdc/cdc_materialized_view.cpp
  - src/cdc/cdc_admin.cpp
- Verified behavior surfaces:
  - CDC capture/replay and transport integration paths
  - delivery tracking, buffering, and dead-letter/outbox behavior
  - admin and cross-collection/materialized-view integration hooks
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md