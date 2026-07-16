### Context

This issue implements the roadmap item 'UDP Protocol Support' for the network domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: UDP Protocol Support

### Goal

Deliver the scoped changes for UDP Protocol Support in src/network/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### UDP Protocol Support
**Priority:** Medium  
**Target Version:** v1.8.0

Add UDP support for low-latency, fire-and-forget operations.

**Features:**
- UDP server for high-throughput ingestion
- Connection-less protocol (no handshake overhead)
- Suitable for metrics, logs, events
- Optional reliability via application-level ACKs
- Packet loss tolerance

**Use Cases:**
- Metrics ingestion (Prometheus remote write, StatsD)
- Log aggregation (syslog, structured logs)
- IoT sensor data ingestion
- Real-time event streaming
- High-frequency trading data feeds

**API:**
```cpp
UDPServer::Config config;
config.port = 8768;
config.max_packet_size = 65507;  // Max UDP packet size
config.enable_batching = true;    // Batch small packets
config.batch_interval_ms = 100;
config.enable_acks = false;       // Fire-and-forget (no reliability)

UDPServer udp_server(config, storage, index_mgr);
udp_server.start();

// Client-side
UDPClient client("server.example.com:8768");

// Send metric (fire-and-forget)
client.send_metric("cpu.usage", 85.5, timestamp);

// Send with optional ACK
client.send_metric("important.metric", 123.4, timestamp, 
    /*wait_for_ack=*/true,
    /*timeout_ms=*/1000
);
```

**Packet Format:**
```
UDP Packet:
┌──────────┬─────────┬──────────┬─────────┐
│ Magic    │ Version │ OpCode   │ Payload │
│ (2 bytes)│ (1 byte)│ (1 byte) │ (N byte)│
└──────────┴─────────┴──────────┴─────────┘

- Magic: 0x544D ("TM")
- Version: 1
- OpCode: METRIC=1, LOG=2, EVENT=3, BATCH=4

Payload:
- Protobuf or MessagePack encoded data
```

**Trade-offs:**
- ✅ Lower latency (no handshake)
- ✅ Higher throughput (no flow control)
- ✅ Lower CPU overhead
- ❌ No reliability guarantee (packets may be lost)
- ❌ No ordering guarantee
- ❌ No congestion control

**Implementation:**
- Use Boost.Asio UDP sockets
- Implement application-level ACKs (optional)
- Add packet deduplication (sequence numbers)
- Implement rate limiting per source IP
- Add metrics for packet loss monitoring

---

### Acceptance Criteria

- [ ] UDP server for high-throughput ingestion
- [ ] Connection-less protocol (no handshake overhead)
- [ ] Suitable for metrics, logs, events
- [ ] Optional reliability via application-level ACKs
- [ ] Packet loss tolerance
- [ ] Metrics ingestion (Prometheus remote write, StatsD)
- [ ] Log aggregation (syslog, structured logs)
- [ ] IoT sensor data ingestion
- [ ] Real-time event streaming
- [ ] High-frequency trading data feeds
- [ ] ✅ Lower latency (no handshake)
- [ ] ✅ Higher throughput (no flow control)
- [ ] ✅ Lower CPU overhead
- [ ] ❌ No reliability guarantee (packets may be lost)
- [ ] ❌ No ordering guarantee
- [ ] ❌ No congestion control
- [ ] Use Boost.Asio UDP sockets
- [ ] Implement application-level ACKs (optional)
- [ ] Add packet deduplication (sequence numbers)
- [ ] Implement rate limiting per source IP
- [ ] Add metrics for packet loss monitoring

### Relationships

- Roadmap row: #189 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/network/FUTURE_ENHANCEMENTS.md#udp-protocol-support
- Source key: roadmap:189:network:v1.8.0:udp-protocol-support

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:189:network:v1.8.0:udp-protocol-support -->
<!-- roadmap-ref: row=189;module=network;target=v1.8.0 -->
<!-- roadmap-detail: src/network/FUTURE_ENHANCEMENTS.md#udp-protocol-support -->
