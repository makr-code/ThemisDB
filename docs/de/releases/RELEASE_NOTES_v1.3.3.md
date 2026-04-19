# ThemisDB v1.3.3 - Network Protocol Enhancements

**Release Date:** 21. Dezember 2025  
**Focus:** Modern Network Protocols & Real-Time Communication

---

## 🎉 Overview

ThemisDB v1.3.3 introduces comprehensive network protocol enhancements including HTTP/2 with Server Push, WebSocket support, MQTT broker, HTTP/3 base implementation, PostgreSQL Wire Protocol, and Model Context Protocol (MCP) server. This release significantly expands ThemisDB's protocol support for modern distributed applications and real-time communication.

---

## 🌐 New Features

### Network Protocol Enhancements (PR #111)

#### HTTP/2 with Server Push
- **CDC/Changefeed Integration**: Proactive event delivery with ~0ms latency
- **Server Push**: Push database changes to clients without polling
- **Multiplexing**: Multiple streams over single connection
- **Header Compression**: Reduced overhead with HPACK
- **Use Cases**: Real-time dashboards, live notifications, reactive applications

#### WebSocket Support
- **CDC Streaming**: Bidirectional real-time communication
- **Long-lived Connections**: Persistent connections for continuous data flow
- **Low Latency**: Direct push notifications for database changes
- **Protocol Upgrade**: Seamless HTTP to WebSocket upgrade
- **Use Cases**: Chat applications, collaborative editing, live monitoring

#### MQTT Broker
- **WebSocket Transport**: MQTT over WebSocket for browser compatibility
- **Rate Limiting**: Configurable rate limits per client
- **Monitoring Metrics**: Built-in Prometheus metrics
- **QoS Support**: Quality of Service levels 0, 1, 2
- **Topic Filtering**: Pattern-based topic subscriptions
- **Use Cases**: IoT device communication, pub/sub messaging, event streaming

#### HTTP/3 Base Implementation
- **QUIC Protocol**: Built on ngtcp2 + nghttp3
- **Multiplexing**: Stream-level flow control
- **0-RTT**: Zero round-trip time connection establishment
- **Connection Migration**: Seamless network switching
- **Status**: Base implementation, production-ready in future release
- **Use Cases**: Mobile applications, high-latency networks

#### PostgreSQL Wire Protocol
- **SQL-to-Cypher Translation**: Automatic query translation
- **BI Tool Compatibility**: Works with Tableau, PowerBI, Metabase, etc.
- **Standard Protocol**: Uses PostgreSQL wire protocol v3.0
- **Authentication**: PostgreSQL-compatible authentication
- **Use Cases**: Business intelligence, reporting tools, SQL client compatibility

#### Model Context Protocol (MCP) Server
- **Cross-Platform Transports**: stdio, SSE, WebSocket
- **LLM Integration**: Standardized protocol for AI model communication
- **Tool Support**: Expose database operations as MCP tools
- **Resource Management**: Efficient resource handling
- **Use Cases**: AI agent integration, LLM-powered applications

#### Security Features
- **Explicit Opt-In**: Build switches for each protocol
  - `-DTHEMIS_ENABLE_HTTP2=ON`
  - `-DTHEMIS_ENABLE_WEBSOCKET=ON`
  - `-DTHEMIS_ENABLE_MQTT=ON`
  - `-DTHEMIS_ENABLE_HTTP3=ON`
  - `-DTHEMIS_ENABLE_POSTGRESQL_WIRE=ON`
  - `-DTHEMIS_ENABLE_MCP=ON`
- **TLS/mTLS Support**: Secure connections for all protocols
- **Authentication**: Per-protocol authentication mechanisms
- **Authorization**: Fine-grained access control

#### Testing
- **Google Test Framework**: Comprehensive test coverage
- **Protocol Compliance**: Standards-compliant implementations
- **Integration Tests**: Cross-protocol testing
- **Performance Tests**: Benchmarks for each protocol

---

## 🚀 Benefits

### Real-Time Communication
- **Instant Updates**: Push changes to clients immediately
- **Reduced Latency**: No polling overhead
- **Efficient**: Multiplexed connections reduce resource usage

### Broader Compatibility
- **BI Tools**: PostgreSQL wire protocol enables SQL tool compatibility
- **IoT Devices**: MQTT support for device communication
- **Modern Apps**: HTTP/2, HTTP/3, WebSocket for web applications

### Production Ready
- **Security First**: Explicit opt-in, TLS/mTLS support
- **Monitoring**: Built-in metrics and observability
- **Testing**: Comprehensive test coverage

---

## 📚 Documentation

- [HTTP/2 Server Push Guide](../apis/HTTP2_SERVER_PUSH_CDC.md)
- [WebSocket CDC Streaming](../apis/WEBSOCKET_CDC_STREAMING.md)
- [MQTT Broker Configuration](../apis/MQTT_BROKER_GUIDE.md)
- [HTTP/3 Implementation Status](../apis/HTTP3_IMPLEMENTATION.md)
- [PostgreSQL Wire Protocol](../../architecture/POSTGRESQL_WIRE_PROTOCOL.md)
- [MCP Server Documentation](../apis/MCP_PROTOCOL_SUPPORT.md)
- [Protocol Build Switches](../apis/PROTOCOL_BUILD_SWITCHES.md)

---

## 🔄 Upgrade Notes

### Build Changes
All new protocols are **optional** and require explicit build flags:

```bash
# Enable specific protocols
cmake -DTHEMIS_ENABLE_HTTP2=ON \
      -DTHEMIS_ENABLE_WEBSOCKET=ON \
      -DTHEMIS_ENABLE_MQTT=ON \
      -DTHEMIS_ENABLE_HTTP3=ON \
      -DTHEMIS_ENABLE_POSTGRESQL_WIRE=ON \
      -DTHEMIS_ENABLE_MCP=ON
```

### New Dependencies
- **nghttp2**: HTTP/2 support
- **ngtcp2 + nghttp3**: HTTP/3 support (optional)
- **mosquitto**: MQTT broker (optional)
- **PostgreSQL client libraries**: Wire protocol support (optional)

### Configuration
Add protocol configuration to `config.yaml`:

```yaml
protocols:
  http2:
    enabled: true
    port: 8443
    tls: true
    
  websocket:
    enabled: true
    port: 8080
    path: "/ws"
    
  mqtt:
    enabled: true
    port: 1883
    websocket_port: 9001
    
  postgresql_wire:
    enabled: true
    port: 5432
    
  mcp:
    enabled: true
    transports: ["stdio", "sse", "websocket"]
```

### Port Configuration
New default ports:
- HTTP/2: 8443 (TLS)
- WebSocket: 8080 (HTTP upgrade)
- MQTT: 1883 (standard), 9001 (WebSocket)
- PostgreSQL Wire: 5432
- MCP: Dynamic based on transport

---

## 📦 Compatibility

- **Backward Compatible**: Yes - 100% compatible with v1.3.2
- **Optional Features**: All new protocols are opt-in
- **Database Format**: No changes
- **API**: New protocol endpoints added (optional)
- **Configuration**: New optional protocol sections

---

## 🔗 Links

- [GitHub Release](https://github.com/makr-code/ThemisDB/releases/tag/v1.3.3)
- [Changelog](CHANGELOG.md)
- [PR #111](https://github.com/makr-code/ThemisDB/pull/111)
- [Protocol Documentation](../apis/OPTIONAL_PROTOCOLS.md)
