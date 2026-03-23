<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — gRPC RPC Plugin

All notable changes to this module are documented here.  
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), newest first.

---

## [Unreleased]

- Unit and integration tests for server lifecycle and TLS modes
- `RPCServerStats::uptime_seconds` incremental tracking
- gRPC health-check service auto-registration
- Server-side interceptors for metrics and tracing

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
