# Network-Modul — Übersicht

<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/network/ -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MISSING_IMPLEMENTATIONS.md -->

**Stand:** 6. April 2026  
**Version:** 1.1  
**Kategorie:** Netzwerk / Transportunfrastruktur  
**Validated:** 2026-03-09 (Reality-Check gegen Sourcecode; siehe [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md))

---

## Übersicht

Das Network-Modul implementiert ThemisDBs hochperformante, sichere Netzwerkschicht für verteilte Kommunikation, Client-Verbindungen und Inter-Node-Messaging. Es stellt ein binäres Wire-Protocol als Alternative zu HTTP/REST bereit, das für geringe Latenz und hohen Durchsatz optimiert ist.

**Wichtigste Eigenschaften:**

- Binäres TCP Wire-Protocol (Port 8766) mit dedizierten I/O- und Worker-Thread-Pools
- TLS 1.3 und mutual TLS (mTLS) für alle externen Verbindungen
- Per-IP-Rate-Limiting und Connection-Limits als erste Verteidigungslinie
- Wire Protocol V2: Multi-Stream-Multiplexing, Flow-Control, Server Push
- WebSocket-Upgrade auf Port 8766 (JSON-Text-Frames, `THEMIS_ENABLE_WEBSOCKET`)
- UDP-Fast-Path für Read-only-Queries (Port 8769)
- QUIC/HTTP3-Transport (Port 8770, `THEMIS_ENABLE_HTTP3`)
- gRPC Native-Transport (Port 8771, `THEMIS_ENABLE_GRPC`)
- Geo-Topology-Router für geo-verteilte Cluster
- Service-Mesh-Integration (Istio/Envoy, `THEMIS_ENABLE_SERVICE_MESH`)
- Per-Tenant-Bandbreiten-Quotas und QoS-Manager (Token-Bucket, Prioritätswarteschlangen)
- Connection-Level-Kompression (LZ4, Zstd)
- **IPv6 Dual-Stack** (v1.9.0): `Config::enable_ipv6` + `Config::ipv6_dual_stack` (IPV6_V6ONLY=0)

---

## Source-Code-Referenz

### Implementierung (`src/network/`)

| Datei / Komponente | Rolle | Status |
|---|---|---|
| `wire_protocol_server.cpp` | Kern-TCP-Server: Accept, Rate-Limiting, Frame-Dispatch, V1-Opcode-Handler | ✅ Vollständig implementiert |
| `wire_protocol_connection_pool.cpp` | Client-seitiger Connection-Pool, RAII-Handles | ✅ Implementiert |
| `wire_protocol_helpers.cpp` | Frame-Parsing und -Serialisierung (ohne libprotobuf) | ✅ Implementiert |
| `wire_protocol_v2.cpp` | V2-Protokoll: Multi-Stream, Flow-Control, Server-Push | ✅ Implementiert |
| `wire_protocol_server_ws.cpp` | WebSocket-Upgrade auf Port 8766 | ✅ (`THEMIS_ENABLE_WEBSOCKET`) |
| `wire_protocol_performance.cpp` | Performance-Monitoring und Benchmarking | ✅ Implementiert |
| `socket_timeout_manager.cpp` | Socket-Timeout, Circuit-Breaker | ✅ Implementiert |
| `qos_manager.cpp` | Per-Tenant-Bandbreite, Token-Bucket, Prioritäts-Queue | ✅ Implementiert |
| `udp_fast_path.cpp` | UDP Read-only-Fast-Path, Port 8769 | ✅ Implementiert |
| `quic_transport.cpp` | QUIC/HTTP3-Transport, Port 8770 | ✅ (`THEMIS_ENABLE_HTTP3`) |
| `grpc_transport.cpp` | gRPC Native-Transport, Port 8771 | ✅ (`THEMIS_ENABLE_GRPC`) |
| `geo_topology_router.cpp` | Netzwerk-Topology-Routing für geo-verteilte Cluster | ✅ Implementiert |
| `service_mesh.cpp` | Istio/Envoy Probe-Server | ✅ (`THEMIS_ENABLE_SERVICE_MESH`) |
| `envoy_xds.cpp` | Envoy xDS v3 REST-Polling-Client | ✅ (`THEMIS_ENABLE_SERVICE_MESH`) |
| `themis_wire_v1.proto` | Protobuf-Schema für Wire-Protocol-V1-Messages | ✅ Referenz |

### Öffentliche Header (`include/network/`)

| Header | Rolle |
|---|---|
| `wire_protocol_server.h` | WireProtocolServer: Config, Stats, Lifecycle |
| `wire_protocol_connection_pool.h` | ConnectionPool, SocketWrapper, ConnectionHandle |
| `wire_protocol_helpers.h` | ProtobufParser, ProtobufSerializer |
| `wire_protocol_performance.h` | WireProtocolPerformanceMonitor |
| `wire_protocol_websocket.h` | WireProtocolWebSocketSession |
| `socket_timeout_manager.h` | SocketTimeoutManager, SocketHealthState |
| `qos_manager.h` | QoSManager, TokenBucket, TrafficPriority |
| `udp_fast_path.h` | UDPFastPath, UDP-Konstanten |
| `quic_transport.h` | QuicTransport, QuicTransport::Config |
| `grpc_transport.h` | GrpcTransport, GrpcTransport::Config |
| `geo_topology_router.h` | GeoTopologyRouter, RoutingStrategy |
| `service_mesh.h` | ServiceMeshIntegration |
| `envoy_xds.h` | EnvoyXdsClient |
| `connection_compression.h` | compressLZ4, decompressLZ4, compressZstd, decompressZstd (header-only) |

---

## Port-Übersicht

| Port | Protokoll | Feature-Flag | Komponente |
|---|---|---|---|
| 8766 | TCP Binary + WebSocket | (immer aktiv) + `THEMIS_ENABLE_WEBSOCKET` | WireProtocolServer |
| 8769 | UDP | (immer aktiv) | UDPFastPath |
| 8770 | QUIC/UDP | `THEMIS_ENABLE_HTTP3` | QuicTransport |
| 8771 | gRPC/HTTP2 | `THEMIS_ENABLE_GRPC` | GrpcTransport |
| 8082 | HTTP (Probe) | `THEMIS_ENABLE_SERVICE_MESH` | ServiceMeshIntegration |

---

## Status und bekannte Einschränkungen

### ⚠️ Bekannte Einschränkungen (kein vollständiger Block mehr)

1. **QUERY_AQL und GEO_QUERY noch nicht mit Wire-Protocol integriert** — Beide Handler geben strukturierte Fehlerantworten zurück (`error_code: "AQL_NOT_INTEGRATED"` / `"GEO_NOT_INTEGRATED"`) und verweisen Clients auf die HTTP-REST-API (`POST /api/v1/query` / `GET /api/v1/geo/query`). Vollständige Engine-Integration geplant.
2. **WebSocket-Binary-Frames nicht dispatched** — Nur JSON-Text-Frames funktionieren vollständig.

Vollständige Details: → [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md)

### ✅ Vollständig implementiert

- TCP-Wire-Protocol-Server (Port 8766): Accept, TLS, Rate-Limiting, Frame-Parsing
- **V1-Opcode-Handler (2026-03-10)**: HELLO, AUTH, GET, PUT, DELETE, VECTOR_SEARCH — vollständig implementiert; Auth-Flag wird nach erfolgreicher Validierung gesetzt
- Wire-Protocol-V2: Multiplexing, Flow-Control, Server-Push (29 Unit-Tests)
- WebSocket-Upgrade: Protokollerkennung, JSON-Text-Frames (13 Unit-Tests)
- UDP-Fast-Path: Read-only-Opcodes, Ratenlimitierung (Unit-Tests)
- QUIC/HTTP3-Transport: TLS 1.3, 0-RTT, Connection Migration (17 Unit-Tests)
- gRPC-Native-Transport: Bidirectional-Streaming, optionales TLS (16 Unit-Tests)
- QoS-Manager: Token-Bucket, Prioritätswarteschlangen (Unit-Tests)
- Geo-Topology-Router: PREFER_LOCAL, LOWEST_LATENCY, ROUND_ROBIN (26 Unit-Tests)
- Service-Mesh-Integration: Probe-Server, Envoy-xDS-Polling
- Connection-Level-Kompression: LZ4 + Zstd (Unit-Tests)

---

## Tests

| Testdatei | Beschreibung |
|---|---|
| `tests/test_wire_protocol_v1_handlers.cpp` | V1-Opcode-Handler: Config-Defaults, Auth-Logik (3 Modi), HELLO-Capabilities, GET/PUT/DELETE-Key-Format, VECTOR_SEARCH-Shape, QUERY_AQL/GEO_QUERY-Error-Kontrakt |
| `tests/test_wire_protocol_ipv6.cpp` | IPv6-Unterstützung: Config-Defaults (`enable_ipv6`, `ipv6_dual_stack`), Address-Promotion-Logik, Boost.Asio-IPv6-Parsing, Dual-Stack-Semantik (18 Tests) |
| `tests/test_wire_protocol_connection_pool.cpp` | Connection-Pool-Lifecycle, RAII-Handles |
| `tests/test_wire_protocol_v2.cpp` | V2-Protokoll-Konstanten, Frame-Header, Stream-State (29 Tests) |
| `tests/test_wire_protocol_websocket.cpp` | WebSocket-Upgrade, Protokollerkennung (13 Tests) |
| `tests/test_wire_protocol_performance.cpp` | Performance-Monitoring |
| `tests/test_udp_fast_path.cpp` | UDP-Konfiguration, Opcode-Filter, Response-Builder |
| `tests/test_quic_transport.cpp` | QUIC-Konfiguration, Port-Validierung, Stats (17 Tests) |
| `tests/test_grpc_transport.cpp` | gRPC-Konfiguration, TLS-Flags, Port-Validierung (16 Tests) |
| `tests/test_qos_manager.cpp` | Token-Bucket, QoS-Prioritäten, Backpressure |
| `tests/test_geo_topology_router.cpp` | Routing-Strategien, Zone/DC-Affinität (26 Tests) |
| `tests/test_service_mesh.cpp` | Service-Mesh-Probe, xDS-Integration |
| `tests/test_connection_compression.cpp` | LZ4/Zstd Round-Trip, Minimum-Größe, Fehlerbehandlung |
| `tests/test_network_timeout.cpp` | Network-Timeout-Szenarien |
| `tests/test_themis_wire_protocol_server.cpp` | Wire-Protocol-Server-Integration |

---

## Einstieg und Beispiele

### Wire-Protocol-Server starten

```cpp
#include "network/wire_protocol_server.h"

WireProtocolServer::Config config;
config.port = 8766;
config.num_io_threads = 4;
config.num_worker_threads = 16;
config.enable_tls = true;
config.tls_cert_path = "/etc/themisdb/server.crt";
config.tls_key_path  = "/etc/themisdb/server.key";

WireProtocolServer server(config, storage, secondary_index, ...);
server.start();
```

### Wire-Protocol-Server mit IPv6 starten

```cpp
#include "network/wire_protocol_server.h"

WireProtocolServer::Config config;
config.port          = 8766;
// IPv6 aktivieren — "0.0.0.0" wird automatisch auf "::" promoted
config.enable_ipv6   = true;
// Dual-Stack: eine einzige IPv6-Socket akzeptiert auch IPv4-mapped Verbindungen
// (IPV6_V6ONLY=0). Auf false setzen, um ausschließlich native IPv6 zu akzeptieren.
config.ipv6_dual_stack = true;           // Standard (true)
// Explizite IPv6-Adressen funktionieren direkt ohne enable_ipv6:
// config.host       = "::1";            // nur Loopback
// config.host       = "fe80::1";        // Link-Local

config.enable_tls    = true;
config.tls_cert_path = "/etc/themisdb/server.crt";
config.tls_key_path  = "/etc/themisdb/server.key";

WireProtocolServer server(config, storage, secondary_index, ...);
server.start();
```

### UDP-Fast-Path konfigurieren

```cpp
#include "network/udp_fast_path.h"

UDPFastPath::Config cfg;
cfg.port = 8769;
cfg.rate_limit_pps = 10000;   // Pakete/s pro Quell-IP

UDPFastPath udp(cfg, storage);
udp.start();
```

### QUIC-Transport starten

```cpp
#include "network/quic_transport.h"

#ifdef THEMIS_ENABLE_HTTP3
QuicTransport::Config cfg;
cfg.port = 8770;
cfg.tls_cert_path = "/etc/themisdb/server.crt";
cfg.tls_key_path  = "/etc/themisdb/server.key";

QuicTransport quic(cfg, storage);
quic.start();
#endif
```

---

## Links zur Primärdokumentation

- [Sourcecode-README](../../../src/network/README.md)
- [Architekturguide](../../../src/network/ARCHITECTURE.md)
- [Roadmap](../../../src/network/ROADMAP.md)
- [Future Enhancements](../../../src/network/FUTURE_ENHANCEMENTS.md)
- [Header-README](../../../include/network/README.md)
- [Fehlende Implementierungen](MISSING_IMPLEMENTATIONS.md) ← Dieser Report
