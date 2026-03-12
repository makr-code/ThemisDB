### Context

This issue implements the roadmap item 'Process Graph Visit Timestamp TODO' for the network domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.8.0.

Primary detail section: `WireProtocolServer`: ProcessGraph Visit Timestamp TODO

### Goal

Deliver the scoped changes for Process Graph Visit Timestamp TODO in src/network/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `WireProtocolServer`: ProcessGraph Visit Timestamp TODO
**Priority:** Low
**Target Version:** v1.8.0

`wire_protocol_server.cpp` line 2306 has: "TODO: `ProcessGraphManager` doesn't store individual visit timestamps per node" — the temporal-graph traversal path in the wire server cannot reconstruct visit ordering for graph result pages.

**Implementation Notes:**
- `[ ]` Add `ProcessGraphVisitLog` (map from node ID to `std::chrono::system_clock::time_point`) to `ProcessGraphManager` state; update on each traversal step.
- `[ ]` Expose via `ProcessGraphManager::getVisitTimestamp(node_id)` for use in the wire protocol server response encoding.
- `[ ]` Add unit test verifying visit timestamps are populated and ordered correctly across multi-hop traversals.

---


**Priority:** High  
**Target Version:** v1.7.0

Add WebSocket support for real-time bidirectional communication.

**Features:**
- WebSocket server for browser-based clients
- Unified protocol (HTTP upgrade to WebSocket)
- Server-push notifications (document changes, query results)
- Streaming query results
- Real-time collaborative editing support

**Benefits:**
- Enables browser-based real-time applications
- Lower latency than HTTP polling
- Bidirectional communication (server can push to client)
- Compatible with existing web infrastructure

**API:**
```cpp
WebSocketServer::Config config;
config.port = 8767;
config.enable_tls = true;
config.max_connections = 10000;
config.enable_compression = true;  // Per-message deflate

WebSocketServer ws_server(config, storage, index_mgr);
ws_server.start();

// Client-side
WebSocketClient client("wss://server.example.com:8767");
client.connect();

// Subscribe to document changes
client.subscribe("documents/articles", [](const auto& change) {
    std::cout << "Document changed: " << change.uuid << std::endl;
});

// Send query, receive streaming results
client.query("FOR doc IN articles FILTER doc.category == 'tech' RETURN doc",
    [](const auto& result) {
        // Called for each result document as it arrives
        std::cout << "Result: " << result << std::endl;
    }
);
```

**Protocol:**
```
WebSocket Frame Format:
┌──────────┬─────────┬──────────┬─────────┐
│ FIN/RSV  │ OpCode  │ Mask     │ Payload │
│ (1 byte) │ (4 bits)│ (1-9 b)  │ (N byte)│
└──────────┴─────────┴──────────┴─────────┘

OpCodes:
- 0x1: Text frame (JSON)
- 0x2: Binary frame (Protobuf)
- 0x8: Close
- 0x9: Ping
- 0xA: Pong

ThemisDB Message Format (JSON):
{
    "id": "req_12345",
    "type": "query" | "subscribe" | "unsubscribe" | "update",
    "payload": { ... }
}
```

**Implementation:**
- Use Boost.Beast for WebSocket protocol
- Reuse existing authentication and authorization
- Share storage/index managers with wire protocol server
- Implement backpressure for slow clients

---

### Acceptance Criteria

- [ ] Add `ProcessGraphVisitLog` (map from node ID to `std::chrono::system_clock::time_point`) to `ProcessGraphManager` state; update on each traversal step.
- [ ] Expose via `ProcessGraphManager::getVisitTimestamp(node_id)` for use in the wire protocol server response encoding.
- [ ] Add unit test verifying visit timestamps are populated and ordered correctly across multi-hop traversals.
- [ ] WebSocket server for browser-based clients
- [ ] Unified protocol (HTTP upgrade to WebSocket)
- [ ] Server-push notifications (document changes, query results)
- [ ] Streaming query results
- [ ] Real-time collaborative editing support
- [ ] Enables browser-based real-time applications
- [ ] Lower latency than HTTP polling
- [ ] Bidirectional communication (server can push to client)
- [ ] Compatible with existing web infrastructure
- [ ] Use Boost.Beast for WebSocket protocol
- [ ] Reuse existing authentication and authorization
- [ ] Share storage/index managers with wire protocol server
- [ ] Implement backpressure for slow clients

### Relationships

- Roadmap row: #255 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/network/FUTURE_ENHANCEMENTS.md#wireprotocolserver-processgraph-visit-timestamp-todo
- Source key: roadmap:255:network:v1.8.0:wireprotocolserver-processgraph-visit-timestamp-todo

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:255:network:v1.8.0:wireprotocolserver-processgraph-visit-timestamp-todo -->
<!-- roadmap-ref: row=255;module=network;target=v1.8.0 -->
<!-- roadmap-detail: src/network/FUTURE_ENHANCEMENTS.md#wireprotocolserver-processgraph-visit-timestamp-todo -->
