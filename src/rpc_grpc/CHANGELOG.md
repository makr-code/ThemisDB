> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-15 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — gRPC RPC Plugin

All notable changes to this module are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), newest first.

---

## [Unreleased]

- Integration tests: mTLS round-trip with real gRPC echo service
- gRPC health-check service auto-registration (`grpc.health.v1.Health`)
- Server-side interceptors for metrics and tracing
- SIGHUP-triggered TLS certificate hot-reload

---

## [0.2.0] — 2026-04-15

### Added

- **Connection keepalive tuning** (`GRPCServer::start()`):
  Reads optional `extra_config["keepalive_time_ms"]` and
  `extra_config["keepalive_timeout_ms"]` keys from `RPCServerConfig`.
  Applies `GRPC_ARG_KEEPALIVE_TIME_MS`, `GRPC_ARG_KEEPALIVE_TIMEOUT_MS`,
  and `GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS` via `ServerBuilder::AddChannelArgument()`.
  Defaults: 120 000 ms / 20 000 ms.  Invalid values are silently ignored.

- **Multi-port binding** (`GRPCServer::start()` + `getAdminAddress()`):
  When `extra_config["admin_port"]` is set, binds a second listening port on
  `InsecureServerCredentials()` for admin traffic (internal loop-back use).
  `GRPCServer::getAdminAddress()` returns `"<host>:<admin_port>"` after `start()`,
  or an empty string when no admin port is configured.

- **TLS certificate hot-reload** (`GRPCServer::reloadTls(cert, key, ca)`):
  Validates new certificate files using the same fail-closed logic as `start()`.
  On success, atomically updates the cached `SslServerCredentials` and the stored
  cert paths in `config_`.  New connections pick up the updated credentials;
  existing TLS sessions are unaffected.  Returns `false` (fail-safe) if the
  new files are invalid or TLS is not enabled — old credentials remain active.

- **`buildSslCredentials(cert, key, ca, require_client_cert)`** (private helper):
  Factored out from `configureCredentials()` and reused by `reloadTls()`.
  Eliminates credential-building duplication.

- **`BidiStreamAdapter<Req, Resp>`** (`src/rpc_grpc/bidi_stream_adapter.h`):
  Header-only bidirectional streaming helper.
  - `onMessage(MessageHandler)` — registers per-message callback.
  - `run()` — blocking read loop; dispatches to handler.
  - `write(Resp)` — thread-safe enqueue + synchronous flush; blocks on backpressure.
  - `finish(grpc::Status)` — signals end-of-stream; unblocks waiting `write()` calls.
  - `isFinished()`, `queueDepth()`, `finishStatus()` — state introspection.
  - Configurable `max_queue_depth` (default 100); `std::invalid_argument` on null stream.

- **New unit tests** (21 tests added; total 71):
  - `tests/test_grpc_plugin.cpp` — 11 new tests for keepalive tuning,
    multi-port binding, and TLS hot-reload (`GRPCServerKeepaliveTest`,
    `GRPCServerMultiPortTest`, `GRPCServerTlsReloadTest`).
  - `tests/test_bidi_stream_adapter.cpp` — 20 tests (BSA-01 … BSA-20)
    covering construction, `run()`, `write()`, `finish()`, `queueDepth()`,
    backpressure, and edge cases.  Registered in `tests/CMakeLists.txt`
    as `test_bidi_stream_adapter` under `THEMIS_ENABLE_GRPC` guard.

### Changed

- `GRPCPlugin::getVersion()` now returns `"2.0.0"`.
- `GRPCServer` private section: added `admin_address_`, `tls_mutex_`, `credentials_`
  fields; `buildSslCredentials()` private method.
- `configureCredentials()` now caches the credentials in `credentials_` for use by
  `reloadTls()`.

---

## [0.0.2] — 2026-04-08

### Added

- **Unit tests** (`tests/test_grpc_plugin.cpp`, 30 tests, `GrpcPluginTests`):
  Covers full `GRPCPlugin` and `GRPCServer` lifecycle: `initialize`, `start`
  (insecure mode), `stop`, `registerService` (null guard), `getStats`,
  `resetStats`, and all `IThemisPlugin` contract methods (`getName`,
  `getVersion`, `getType`, `getCapabilities`, `getDefaultPort`, `getProtocol`).
  Fail-closed TLS test verifies `start()` returns `false` when cert path is
  invalid. Registered in `tests/CMakeLists.txt` as standalone target
  `test_grpc_plugin` under `THEMIS_ENABLE_GRPC` compile guard.

---

## [0.0.1] — 2026-03-22

### Added

- **`GRPCServer`** (`grpc_plugin.h/.cpp`):
  Implements `IRPCServer`. Manages gRPC `grpc::Server` lifecycle via
  `grpc::ServerBuilder`. Supports binding to any host:port, TLS-only and mTLS
  modes, and up to 100 MB send/receive message sizes. `registerService(void*)`
  accepts `grpc::Service*` pointers from callers. `isRunning()` is lock-free via
  `std::atomic<bool>`. `getStats()` / `resetStats()` are mutex-protected.

- **`GRPCPlugin`** (`grpc_plugin.h/.cpp`):
  Implements `IRPCPlugin` + `IThemisPlugin`. Factory returning `GRPCServer` via
  `createServer()`. Stateless after `initialize()`. Returns default port 50051
  via `getDefaultPort()`. Reports `RPCProtocol::GRPC` and capability flags
  `supports_streaming=true`, `supports_batching=true`, `thread_safe=true`.

- **Fail-closed TLS**:
  `configureCredentials()` throws `std::runtime_error` on certificate load failure
  instead of falling back to `InsecureServerCredentials`. This ensures TLS-enabled
  servers never start without valid certificates.

- **mTLS support**:
  When `auth_required=true`, `GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY`
  is set in `SslServerCredentialsOptions` for mutual TLS authentication.

- **C export entry points**:
  `extern "C" createPlugin()` / `destroyPlugin()` for dynamic plugin loading
  by the ThemisDB plugin manager.

