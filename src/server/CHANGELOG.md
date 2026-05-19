> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Server Module

All notable changes to the Server module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Fixed
- **Server API/HTTP reliability hardening** (`src/server/http_server.cpp`, `src/server/query_api_handler.cpp`, `src/server/monitoring_api_handler.cpp`, `src/server/content_api_handler.cpp`, `src/server/changefeed_api_handler.cpp`)
  - Replaced all remaining `catch (...)` handlers in the edited server handler files with typed `catch (const std::exception&)`.
  - Targeted delta in this block: **99 → 0** catch-all handlers (`http_server`: 51→0, `query_api_handler`: 25→0, `monitoring_api_handler`: 10→0, `content_api_handler`: 7→0, `changefeed_api_handler`: 6→0).

## [1.9.0] — 2026-03-23
### Added
- **MQTT Client Service** (`include/server/mqtt_client_service.h`, `src/server/mqtt_client_service.cpp`) — bidirectional MQTT integration for real-time environments
  - `MqttClientConfig` — broker host/port, credentials, TLS paths, keepalive, CDC topic prefix/QoS, retry policy, outbound-queue limit
  - `MqttClientStats` — atomic counters: messages_published, messages_received, bytes_sent/received, connect_count, reconnect_count, publish_errors, subscribe_count, is_connected
  - `IMqttMessageHandler` — callback interface with `onMessage()` (required), `onConnected()` and `onDisconnected()` (default no-ops); all `noexcept`
  - `MqttCDCTransport : ICDCTransport` — CDC → MQTT bridge; maps `Changefeed::ChangeEvent` to `{prefix}{collection}/{EVENT_TYPE}` topics; configurable prefix and QoS at runtime
  - `MqttClientService` — Boost.Asio async I/O client; background thread; automatic reconnect with `MqttRetryConfig` exponential back-off; `publish()` / `subscribe()` / `unsubscribe()` thread-safe via `asio::post()`; `registerWithServiceRegistry()` / `unregisterFromServiceRegistry()` for service discovery
  - 31 unit tests in `tests/test_mqtt_client_service.cpp`
  - CI: `.github/workflows/mqtt-client-service-ci.yml`
  - No-op stubs compiled when `THEMIS_ENABLE_MQTT` is absent (zero overhead)

## [1.5.0] — 2026-03-12
### Added
- HTTP/3 (QUIC) transport support
- MCP (Model Context Protocol) server for AI tool integration
- WasmHandlerRegistry for WebAssembly request handlers (v2.1.0)
- Redis-backed TokenBucketRateLimiter (`Backend::REDIS` via EVALSHA Lua script)
- Per-client rate limiting with `PerClientRateLimiter`
- PostgreSQL wire protocol server for SQL compatibility
- gRPC server with Protobuf serialization
- MQTT broker endpoint for IoT device connectivity
- GraphQL WebSocket handler (graphql-transport-ws protocol) with subscription management
- Force-run endpoint window override for maintenance scheduling

### Changed
- Rate limiter v2 replaces v1 (local fallback preserved)
- Admin PII eviction endpoint wired to cache module (`AdminCachePiiEvictDelete`)

### Fixed
- HMAC validation in distributed cache coordinator

## [1.0.0] — 2024-01-01
### Added
- HTTP/1.1 and HTTP/2 server built on Boost.Beast/Asio
- RESTful routing with 40+ specialized endpoints
- TLS/SSL termination with certificate management
- JWT authentication middleware
- Request/response pipeline with middleware chain
