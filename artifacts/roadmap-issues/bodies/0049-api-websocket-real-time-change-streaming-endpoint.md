### Context

This issue implements the roadmap item 'WebSocket Real-Time Change Streaming Endpoint' for the api domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: WebSocket Real-Time Change Streaming Endpoint

### Goal

Deliver the scoped changes for WebSocket Real-Time Change Streaming Endpoint in src/api/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### WebSocket Real-Time Change Streaming Endpoint
**Priority:** High
**Target Version:** v1.7.0

Add a dedicated WebSocket endpoint `/v2/changes` that multiplexes multiple `cdc::Changefeed` subscriptions over a single connection. This is distinct from the GraphQL subscription path and targets clients that need raw change events without the GraphQL envelope.

**Implementation Notes:**
- `[x]` Create `ws_handler.cpp` (`src/api/ws_handler.cpp`); register route `WS /v2/changes` in `src/server/http_server.cpp`.
- `[x]` Frame format: newline-delimited JSON, each frame matching `Changefeed::ChangeEvent::toJson()` output. (`WebSocketSession::pollCDCEvents` emits JSON via `ev.toJson()` / `buildEventFrame`; legacy path uses `cdc_message["type"]="cdc_event"`)
- `[x]` Client subscribes/unsubscribes by sending `{"action":"subscribe","collection":"orders","filter":{"type":"PUT"}}` control frames. (`WebSocketSession::processMessage` handles `type="subscribe"/"unsubscribe"` for `/v2/changes`; `CdcWebSocketHandler::handleFrame` handles `action="subscribe"/"unsubscribe"/"ack"` for `/v2/cdc/stream`)
- `[x]` Implement per-connection back-pressure: if the outbound frame queue exceeds 1,000 entries, close with `1011 Internal Error` and log tenant/connection ID. (`WebSocketSession::kMaxQueueDepth = 1000`)
- `[x]` Reuse `auth::JWTValidator` middleware already wired for HTTP; extract Bearer token from the WebSocket upgrade `Authorization` header. (`WsChangeHandler::validate()` requires `cdc:subscribe` scope)
- `[ ]` `WsChangeHandler::validate()` in `ws_handler.cpp` parses query-string parameters (`from_sequence`, `key_prefix`) with ad-hoc string search using `std::string::find`. URL-encoded characters (e.g., `key_prefix=orders%3A`) are never decoded, so clients that percent-encode the query string will receive incorrect filter values. Replace with a proper URL-decoding step (e.g., using `boost::urls` or a small `url_decode()` utility) before extracting parameter values.

**Performance Targets:**
- ≥ 10,000 concurrent WebSocket connections on a single node with < 50 MB additional RSS.
- Frame delivery latency p99 < 30 ms under 5,000 events/sec aggregate throughput.

---

### Acceptance Criteria

- [ ] Create `ws_handler.cpp` (`src/api/ws_handler.cpp`); register route `WS /v2/changes` in `src/server/http_server.cpp`.
- [ ] Frame format: newline-delimited JSON, each frame matching `Changefeed::ChangeEvent::toJson()` output. (`WebSocketSession::pollCDCEvents` emits JSON via `ev.toJson()` / `buildEventFrame`; legacy path uses `cdc_message["type"]="cdc_event"`)
- [ ] Client subscribes/unsubscribes by sending `{"action":"subscribe","collection":"orders","filter":{"type":"PUT"}}` control frames. (`WebSocketSession::processMessage` handles `type="subscribe"/"unsubscribe"` for `/v2/changes`; `CdcWebSocketHandler::handleFrame` handles `action="subscribe"/"unsubscribe"/"ack"` for `/v2/cdc/stream`)
- [ ] Implement per-connection back-pressure: if the outbound frame queue exceeds 1,000 entries, close with `1011 Internal Error` and log tenant/connection ID. (`WebSocketSession::kMaxQueueDepth = 1000`)
- [ ] Reuse `auth::JWTValidator` middleware already wired for HTTP; extract Bearer token from the WebSocket upgrade `Authorization` header. (`WsChangeHandler::validate()` requires `cdc:subscribe` scope)
- [ ] `WsChangeHandler::validate()` in `ws_handler.cpp` parses query-string parameters (`from_sequence`, `key_prefix`) with ad-hoc string search using `std::string::find`. URL-encoded characters (e.g., `key_prefix=orders%3A`) are never decoded, so clients that percent-encode the query string will receive incorrect filter values. Replace with a proper URL-decoding step (e.g., using `boost::urls` or a small `url_decode()` utility) before extracting parameter values.
- [ ] ≥ 10,000 concurrent WebSocket connections on a single node with < 50 MB additional RSS.
- [ ] Frame delivery latency p99 < 30 ms under 5,000 events/sec aggregate throughput.

### Relationships

- Roadmap row: #49 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/api/FUTURE_ENHANCEMENTS.md#websocket-real-time-change-streaming-endpoint
- Source key: roadmap:49:api:v1.7.0:websocket-real-time-change-streaming-endpoint

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:49:api:v1.7.0:websocket-real-time-change-streaming-endpoint -->
<!-- roadmap-ref: row=49;module=api;target=v1.7.0 -->
<!-- roadmap-detail: src/api/FUTURE_ENHANCEMENTS.md#websocket-real-time-change-streaming-endpoint -->
