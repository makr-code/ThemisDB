# Network Troubleshooting Guide

The `network` module implements ThemisDB's wire protocol (v1 and v2), connection pooling, QoS bandwidth management, WebSocket support, and socket timeout management for inter-node and client-to-server communication.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `WireProtocol: magic byte mismatch` | Client using wrong protocol version | Use matching client library version |
| CRC32 checksum error on message | Network packet corruption | Check NIC drivers; enable TCP checksums |
| Connection pool exhausted | Too many concurrent clients | Increase `network.connection_pool.max_connections` |
| `QosManager: rate limited` | QoS bandwidth cap too low | Increase `network.qos.max_bandwidth_mbps` |
| `SocketTimeoutManager: connection closed` | Timeout too aggressive for slow links | Increase `network.socket.idle_timeout_ms` |
| WebSocket returns 400 Bad Request | Missing `Upgrade` header or wrong path | Use correct WebSocket path and headers |
| TLS handshake timeout | Slow client or cert validation delay | Increase `network.tls.handshake_timeout_ms` |
| High latency between cluster nodes | TCP Nagle algorithm buffering | Enable `network.tcp.no_delay: true` |
| Client cannot reconnect after failover | Keep-alive not configured | Enable `network.tcp.keep_alive` |
| `WireProtocolV2: flow control stall` | Receiver window too small | Increase `network.v2.recv_window_size` |

## Common Issues

### Issue 1: Protocol Version Mismatch

**Description:** Client connections are rejected because of wire protocol version incompatibility.

**Symptoms:**
- Log: `WireProtocolServer: magic byte mismatch: expected 0x54484D53, got 0x54484D52`
- Client receives connection reset immediately

**Cause:** Client library version does not match the server's protocol version.

**Solution:**
```bash
# Check server protocol version
themisdb-admin network protocol-version

# Update client library
pip install themisdb-client==1.5.0   # match server version
```
```yaml
network:
  wire_protocol:
    version: 2                    # "1" | "2"
    accept_v1: true               # allow legacy v1 clients during migration
    v1_deprecation_warning: true
```

---

### Issue 2: CRC32 Checksum Errors

**Description:** Messages are rejected due to CRC32 checksum failures.

**Symptoms:**
- Log: `WireProtocolHelpers: CRC32 checksum mismatch: expected=0xA1B2C3D4, got=0xA1B2C3D5`
- Intermittent connection drops with no clear pattern

**Cause:** Network infrastructure corrupting packets (faulty switch/NIC), or TCP offloading bug.

**Solution:**
```bash
# Check NIC error counters
ethtool -S eth0 | grep -i error

# Disable hardware TCP offloading as a diagnostic step
ethtool -K eth0 tx off rx off

# Check for network errors
netstat -s | grep error
```
```yaml
network:
  wire_protocol:
    checksum: crc32c              # "crc32c" | "sha256" | "none"
    retry_on_checksum_error: true
    retry_max: 3
```

---

### Issue 3: Connection Pool Exhausted

**Description:** New client connections are rejected because the pool is full.

**Symptoms:**
- Log: `WireProtocolConnectionPool: connection pool exhausted (max=500)`
- Clients receive `503 Connection refused`

**Cause:** Peak client concurrency exceeds `max_connections`.

**Solution:**
```yaml
network:
  connection_pool:
    max_connections: 2000          # increase from 500
    min_idle_connections: 10
    connection_timeout_ms: 5000
    idle_timeout_ms: 300000
    health_check_interval_ms: 30000
```
```bash
# Check current connection count
themisdb-admin network connections --stats

ss -tnp | grep :8080 | wc -l
```

---

### Issue 4: QoS Rate Limiting Throttles Bulk Imports

**Description:** Large data imports are throttled by the QoS manager.

**Symptoms:**
- Import throughput capped at unexpected rate
- Log: `QosManager: bandwidth cap reached for client=10.0.0.5 (50 Mbps)`

**Cause:** Per-client bandwidth cap too low for bulk operations.

**Solution:**
```yaml
network:
  qos:
    enabled: true
    max_bandwidth_mbps: 1000       # global cap
    per_client_bandwidth_mbps: 200 # per-client cap
    bulk_import_priority: high     # give bulk imports high priority
    priority_classes:
      - name: high
        weight: 10
      - name: normal
        weight: 5
      - name: low
        weight: 1
```

---

### Issue 5: Socket Idle Timeout Drops Long-Running Queries

**Description:** Connections are closed during long analytical queries.

**Symptoms:**
- Log: `SocketTimeoutManager: connection 10.0.0.5:50234 idle for 60000ms, closing`
- Client receives connection reset mid-query

**Cause:** `idle_timeout_ms` shorter than query execution time.

**Solution:**
```yaml
network:
  socket:
    idle_timeout_ms: 600000        # 10 minutes for analytical workloads
    read_timeout_ms: 300000
    write_timeout_ms: 60000
  tcp:
    keep_alive: true
    keep_alive_idle_seconds: 60
    keep_alive_interval_seconds: 10
    keep_alive_count: 6
```

---

### Issue 6: WebSocket Upgrade Fails with 400

**Description:** WebSocket connection upgrade returns HTTP 400.

**Symptoms:**
- Browser error: `WebSocket connection to 'ws://...' failed: Error during WebSocket handshake`
- Log: `WireProtocolServerWs: WebSocket upgrade failed: missing Upgrade header`

**Cause:** Reverse proxy (nginx/HAProxy) strips `Upgrade` and `Connection` headers.

**Solution:**
```nginx
# nginx proxy configuration
location /api/v2/changes {
    proxy_pass http://themisdb:8080;
    proxy_http_version 1.1;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
    proxy_set_header Host $host;
    proxy_read_timeout 3600s;
}
```
```yaml
network:
  websocket:
    path: /api/v2/changes
    enable_compression: true
    max_frame_size_bytes: 1048576
```

---

### Issue 7: High Inter-Node Latency Due to Nagle Algorithm

**Description:** Small messages between cluster nodes experience high latency.

**Symptoms:**
- Raft heartbeat latency > 10ms even on local network
- Log: `WireProtocolPerformance: high latency detected (p99=45ms)`

**Cause:** TCP Nagle algorithm batches small packets, adding up to 40ms delay.

**Solution:**
```yaml
network:
  tcp:
    no_delay: true                 # disable Nagle algorithm for low latency
    keep_alive: true
  inter_node:
    dedicated_port: 7000           # separate port for cluster communication
    no_delay: true
```

---

### Issue 8: TLS Handshake Timeout Under Load

**Description:** TLS handshakes time out when the server is under high load.

**Symptoms:**
- Log: `WireProtocolServer: TLS handshake timeout after 5000ms`
- Client receives `connection reset` during peak hours

**Cause:** TLS handshake timeout too short; server CPU busy with handshakes during traffic spikes.

**Solution:**
```yaml
network:
  tls:
    enabled: true
    cert_file: /etc/themisdb/tls/server.crt
    key_file: /etc/themisdb/tls/server.key
    handshake_timeout_ms: 15000    # increase from 5000
    session_cache:
      enabled: true                # cache TLS sessions to avoid full handshakes
      size: 10000
      ttl_seconds: 300
    protocols: [TLSv1.2, TLSv1.3]
    prefer_server_ciphers: true
```

## Diagnostic Commands

```bash
# Network statistics
themisdb-admin network stats

# Active connections
themisdb-admin network connections --list

# Connection pool utilisation
themisdb-admin network pool-stats

# QoS bandwidth usage
themisdb-admin network qos-stats

# Test protocol connectivity
themisdb-admin network test-connection \
  --host replica-02 \
  --port 8080 \
  --protocol v2

# Live network metrics
curl -s http://localhost:9100/metrics | grep themisdb_network

# Tail network logs
journalctl -u themisdb -f | grep -E "network|wire|socket|qos|tls|websocket"
```

## Configuration Reference

```yaml
network:
  wire_protocol:
    version: 2
    accept_v1: true
    checksum: crc32c
  connection_pool:
    max_connections: 1000
    idle_timeout_ms: 300000
  tls:
    enabled: true
    handshake_timeout_ms: 10000
    session_cache:
      enabled: true
  tcp:
    no_delay: true
    keep_alive: true
  qos:
    enabled: false
    max_bandwidth_mbps: 1000
  socket:
    idle_timeout_ms: 300000
    read_timeout_ms: 120000
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `tcp.no_delay` | `false` | `true` for cluster communication |
| `connection_pool.max_connections` | `100` | `1000+` in production |
| `socket.idle_timeout_ms` | `30000` | `300000` for analytics |
| `tls.session_cache.enabled` | `false` | `true` under high TLS load |

## Known Limitations

- Wire protocol v1 does not support streaming responses; upgrade to v2 for large result sets.
- QoS bandwidth management is approximate; actual throughput may exceed cap by up to 10%.
- TLS session cache is not shared across cluster nodes; each node maintains its own cache.
- WebSocket connections do not support mTLS client certificate authentication.

## Related Documentation

- [Network Module ROADMAP](../../src/network/ROADMAP.md)
- [Network Roadmap](../de/roadmap/network_roadmap.md)
- [Wire Protocol](../wire-protocol.md)
- [WAL & gRPC mTLS Configuration](../architecture/WAL_GRPC_MTLS_CONFIGURATION.md)
- [Network Timeout Handling](../ARCHIVED/implementation-summaries/NETWORK_TIMEOUT_HANDLING.md)
- [mTLS Shard Communication](../de/security/MTLS_SHARD_COMMUNICATION.md)
