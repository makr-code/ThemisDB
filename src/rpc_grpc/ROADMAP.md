<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# gRPC Plugin Roadmap

## Current Status

v0.0.1 — `GRPCServer` and `GRPCPlugin` provide a fully functional gRPC server with
mTLS, service registration, and HTTP/2. The fail-closed TLS design is in place.
Tests and advanced features (health service, interceptors, streaming helpers) are planned.

---

## Completed ✅

- [x] `GRPCServer` implementing `IRPCServer` (init, start, stop, register, stats)
- [x] `GRPCPlugin` implementing `IRPCPlugin` + `IThemisPlugin`
- [x] Fail-closed TLS: throws on cert load failure; no insecure fallback
- [x] mTLS via `GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY`
- [x] Server-side TLS (client cert optional mode)
- [x] `InsecureServerCredentials()` for development/test environments
- [x] Service registration: `registerService(void*)` with null guard
- [x] `std::atomic<bool> running_` for lock-free `isRunning()`
- [x] `std::mutex stats_mutex_` protecting `getStats()` / `resetStats()`
- [x] Max message size: 100 MB receive + 100 MB send
- [x] `PluginCapabilities`: `supports_streaming=true`, `supports_batching=true`, `thread_safe=true`
- [x] `extern "C"` export: `createPlugin()` / `destroyPlugin()`

---

## In Progress [~]

- [~] Unit tests for server lifecycle (insecure, TLS, mTLS modes)
- [~] Unit tests for GRPCPlugin (getName, getVersion, getType, getCapabilities, initialize, createServer, getProtocol, getDefaultPort)
- [~] tests/test_grpc_plugin.cpp registered as standalone target (THEMIS_ENABLE_GRPC guard)

---

## Planned Features

### v0.1.0 — Tests and Uptime Tracking (Target: Q3 2026)

- [~] Unit tests: `initialize`, `start` (insecure), `stop`, `registerService` (Target: Q3 2026)
- [~] Fail-closed TLS test: start() returns false on bad cert path (Target: Q3 2026)
- [ ] Integration tests: mTLS round-trip with real gRPC service (Target: Q3 2026)
- [ ] `RPCServerStats::uptime_seconds` incremental tracking with `std::chrono` (Target: Q3 2026)

### v0.2.0 — Health and Observability (Target: Q4 2026)

- [ ] Auto-register `grpc.health.v1.Health` service on `start()` (Target: Q4 2026)
- [ ] Server-side gRPC interceptor for per-method latency and error rate (Target: Q4 2026)
- [ ] Prometheus metrics: `grpc_requests_total`, `grpc_latency_seconds`, `grpc_active_connections` (Target: Q4 2026)
- [ ] Structured access log: method, status code, duration, client CN (Target: Q4 2026)

### v0.3.0 — Advanced Features (Target: Q1 2027)

- [ ] Bidirectional streaming helper: `BidiStreamAdapter<Req, Resp>` (Target: Q1 2027)
- [ ] Server-side TLS certificate hot-reload (SIGHUP trigger) (Target: Q1 2027)
- [ ] Connection keepalive tuning: `grpc.keepalive_time_ms`, `grpc.keepalive_timeout_ms` config keys (Target: Q1 2027)
- [ ] Multi-port binding for admin vs client traffic separation (Target: Q1 2027)

---

## Implementation Phases

### Phase 1: Design / API Contract ✅
- [x] Define `IRPCServer` and `IRPCPlugin` compliance points
- [x] Define `RPCServerConfig` fields for TLS and mTLS
- [x] Define fail-closed credential model

### Phase 2: Core Implementation ✅
- [x] `GRPCServer` server builder integration
- [x] `configureCredentials()` with TLS / mTLS / insecure branches
- [x] Service registration and lifecycle (start/stop)

### Phase 3: Error Handling & Edge Cases ✅
- [x] Already-running guard in `start()`
- [x] Null service guard in `registerService()`
- [x] Cert file load failure → fail-closed throw

### Phase 4: Tests [ ]
- [~] `tests/test_grpc_plugin.cpp` — GRPCPlugin + GRPCServer unit tests
- [~] Fail-closed TLS: start() returns false (no insecure fallback)
- [~] Registered in `tests/CMakeLists.txt` as `test_grpc_plugin` (THEMIS_ENABLE_GRPC guard)
- [ ] Integration test: mTLS round-trip with echo service
- [ ] Unit tests (Target: Q3 2026)
- [ ] Integration tests (Target: Q3 2026)

### Phase 5: Performance / Hardening [ ]
- [ ] Health service (Target: Q4 2026)
- [ ] Interceptors + Prometheus (Target: Q4 2026)

### Phase 6: Documentation & Acceptance ✅
- [x] README, ARCHITECTURE, AUDIT, CHANGELOG, ROADMAP, SECURITY, FUTURE_ENHANCEMENTS

---

## Production Readiness Checklist

| Area | Status | Notes |
|------|--------|-------|
| Core RPC transport | ✅ | HTTP/2 + Protocol Buffers via gRPC C++ |
| TLS / mTLS | ✅ | Fail-closed; mTLS requires and verifies client cert |
| Service registration | ✅ | Any `grpc::Service*` accepted before start |
| Thread safety | ✅ | Atomic running state; mutex-protected stats |
| Unit/integration tests | ❌ | Planned Q3 2026 |
| Uptime tracking | ⚠️ | Initialised to 0; incremental tracking planned Q3 2026 |
| Health service | ❌ | Planned Q4 2026 |
| Prometheus metrics | ❌ | Planned Q4 2026 |

---

## Known Issues & Limitations

- `RPCServerStats::uptime_seconds` is set to 0 at start and never incremented.
- No gRPC health-check service is auto-registered; load balancers requiring it need manual registration.
- Single-port binding only; separate admin port requires a second `GRPCServer` instance.
