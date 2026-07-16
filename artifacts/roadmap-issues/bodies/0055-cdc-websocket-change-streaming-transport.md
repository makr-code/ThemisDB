### Context

This issue implements the roadmap item 'WebSocket Change Streaming Transport' for the cdc domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: WebSocket Change Streaming Transport

### Goal

Deliver the scoped changes for WebSocket Change Streaming Transport in src/cdc/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### WebSocket Change Streaming Transport
**Priority:** High
**Target Version:** v1.7.0

Replace or supplement the SSE transport with a bidirectional WebSocket endpoint (`/v2/cdc/stream`) that supports both server-push change events and client-sent subscription management frames. WebSocket allows the client to change subscriptions without reconnecting.

**Implementation Notes:**
- `[x]` Create `cdc_ws_handler.cpp`; register `WS /v2/cdc/stream` in `src/server/http_server.cpp`. (transport implemented as `cdc/ws_transport.cpp`; endpoint wiring is a follow-up)
- `[x]` Protocol: JSON control frames for subscribe/unsubscribe; change event frames matching `ChangeEvent::toJson()` output.
- `[x]` Subscribe frame: `{"action":"subscribe","id":"sub-1","collection":"orders","key_prefix":"US-","event_types":["PUT","DELETE"]}`.
- `[x]` Unsubscribe frame: `{"action":"unsubscribe","id":"sub-1"}`.
- `[x]` Back-pressure: per-connection outbound queue capped at 1,000 pending frames; on overflow, close with code `1011` and record `cdc_ws_overflow_total` metric.
- `[x]` Reuse `Changefeed::subscribe()` with the same filter model as SSE; each WebSocket subscription ID maps to one `Changefeed` subscription handle.
- `[x]` TLS handshake reuses existing Beast SSL context; no new cert management needed.

**Performance Targets:**
- ≥ 5,000 concurrent WebSocket connections per node with < 100 MB additional RSS.
- Event delivery latency p99 < 20 ms from `Changefeed` emit to WebSocket frame write.

**API Sketch:**
```json
// Client → Server: subscribe
{"action":"subscribe","id":"sub-1","collection":"inventory","event_types":["PUT"]}

// Server → Client: change event (matches ChangeEvent::toJson())
{"sequence":10042,"type":"PUT","key":"inventory:SKU-9918","value":"{\"qty\":5}","timestamp_ms":1740000000000}

// Server → Client: subscription ack
{"action":"subscribed","id":"sub-1"}
```

---

### Acceptance Criteria

- [x] Create `cdc_ws_handler.cpp`; register `WS /v2/cdc/stream` in `src/server/http_server.cpp`. (transport implemented as `cdc/ws_transport.cpp`; endpoint wiring is a follow-up)
- [x] Protocol: JSON control frames for subscribe/unsubscribe; change event frames matching `ChangeEvent::toJson()` output.
- [x] Subscribe frame: `{"action":"subscribe","id":"sub-1","collection":"orders","key_prefix":"US-","event_types":["PUT","DELETE"]}`.
- [x] Unsubscribe frame: `{"action":"unsubscribe","id":"sub-1"}`.
- [x] Back-pressure: per-connection outbound queue capped at 1,000 pending frames; on overflow, close with code `1011` and record `cdc_ws_overflow_total` metric.
- [x] Reuse `Changefeed::subscribe()` with the same filter model as SSE; each WebSocket subscription ID maps to one `Changefeed` subscription handle.
- [x] TLS handshake reuses existing Beast SSL context; no new cert management needed.
- [x] ≥ 5,000 concurrent WebSocket connections per node with < 100 MB additional RSS.
- [x] Event delivery latency p99 < 20 ms from `Changefeed` emit to WebSocket frame write.

### Relationships

- Roadmap row: #55 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cdc/FUTURE_ENHANCEMENTS.md#websocket-change-streaming-transport
- Source key: roadmap:55:cdc:v1.7.0:websocket-change-streaming-transport

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:55:cdc:v1.7.0:websocket-change-streaming-transport -->
<!-- roadmap-ref: row=55;module=cdc;target=v1.7.0 -->
<!-- roadmap-detail: src/cdc/FUTURE_ENHANCEMENTS.md#websocket-change-streaming-transport -->
