# Change Data Capture (CDC) Module

Change Data Capture and changefeed implementation for ThemisDB.

## Module Purpose

Implements Change Data Capture for ThemisDB, providing real-time change notifications via SSE streaming, filtered subscriptions, change log management, and historical change replay.

## Subsystem Scope

**In scope:** Changefeed engine, SSE event streaming, per-collection/per-key filtering, change log persistence, historical replay, subscription lifecycle management.

**Out of scope:** Message broker integration (Kafka planned), WebSocket transport (in progress), consumer offset tracking (planned).

## Relevant Interfaces

- `changefeed.cpp` — core change capture engine
- `sse_streamer.cpp` — Server-Sent Events transport
- `change_log.cpp` — persistent change log
- `subscription_manager.cpp` — subscription lifecycle

## Current Delivery Status

**Maturity:** 🟡 Beta — SSE-based changefeeds and filtered subscriptions operational; WebSocket and Kafka integration planned.

## Components

- Changefeed implementation
- Server-Sent Events (SSE) streaming
- Change log management
- Subscription management

## Features

- Real-time change notifications
- SSE-based event streaming
- Filtered change subscriptions
- Historical change replay

## Documentation

For CDC documentation, see:
- [Changefeed](../../docs/src/cdc/changefeed.cpp.md)
- [Change Data Capture](../../docs/change_data_capture.md)
- [CDC Documentation](../../docs/cdc.md)
- [Changefeed Development](../../docs/development/changefeed/)
