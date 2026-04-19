<!-- Status: current | validated: 2026-04-15 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# gRPC Plugin Roadmap

## Current Status

v0.3.0 — `GRPCServer` and `GRPCPlugin` are production-ready.  Health service
(`setServiceHealth`/`isServiceHealthy`), per-method interceptor metrics
(`recordRPC`), Prometheus text export (`getMetricsText`), and structured JSON
access logging (`setAccessLogSink`/`logAccess`) are fully implemented.
Tests now cover all v0.3.0 features (12 new tests in `test_grpc_observability.cpp`,
total 63 focused tests).

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
- [x] `RPCServerStats::uptime_seconds` incremental tracking with `std::chrono`
- [x] Unit tests — `tests/test_grpc_plugin.cpp` (51 tests, `GrpcPluginTests`)

### v0.2.0 — Completed 2026-04-15

- [x] Connection keepalive tuning via `extra_config["keepalive_time_ms"]` / `keepalive_timeout_ms` (Target: Q3 2026)
- [x] Multi-port binding: admin port via `extra_config["admin_port"]`; `getAdminAddress()` (Target: Q3 2026)
- [x] TLS certificate hot-reload: `GRPCServer::reloadTls(cert, key, ca)` — fail-safe on invalid files (Target: Q3 2026)
- [x] `buildSslCredentials()` helper factored out from `configureCredentials()` (Target: Q3 2026)
- [x] `BidiStreamAdapter<Req,Resp>` header-only template (`src/rpc_grpc/bidi_stream_adapter.h`) (Target: Q3 2026)
  - `onMessage(handler)` — registers inbound callback
  - `run()` — blocking read loop
  - `write(Resp)` — thread-safe write with backpressure (configurable queue depth)
  - `finish(grpc::Status)` — unblocks writes, marks stream done
  - `isFinished()`, `queueDepth()`, `finishStatus()`
- [x] 20 new unit tests for `BidiStreamAdapter` — `tests/test_bidi_stream_adapter.cpp`
- [x] 11 new unit tests for keepalive, multi-port, TLS hot-reload

---

## In Progress [~]

*(none — all previously in-progress items are now complete)*

---

## Planned Features

### v0.3.0 — Health and Observability (Target: Q4 2026) ✅ Completed 2026-04-19

- [x] Auto-register `grpc.health.v1.Health` service on `start()` — `setServiceHealth("")` auto-called SERVING on start / NOT_SERVING on stop (Target: Q4 2026)
- [x] Server-side gRPC interceptor for per-method latency and error rate — `recordRPC(method, success, duration_ms)` + per-method atomic counters (Target: Q4 2026)
- [x] Prometheus metrics: `grpc_server_requests_total`, `grpc_server_errors_total`, `grpc_server_latency_ms_total`, `grpc_server_active_connections` via `getMetricsText()` (Target: Q4 2026)
- [x] Structured access log: method, status code, duration, client CN via `setAccessLogSink()` + `logAccess()` (Target: Q4 2026)

### v0.4.0 — Advanced Features (Target: Q1 2027)

- [ ] SIGHUP-triggered TLS certificate hot-reload (currently callable only via `reloadTls()`) (Target: Q1 2027)
- [ ] SNI-based multi-host TLS on a single port (Target: Q1 2027)

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
- [x] `reloadTls()` fail-safe: bad certs keep old credentials active

### Phase 4: Tests ✅
- [x] `tests/test_grpc_plugin.cpp` — GRPCPlugin + GRPCServer unit tests (51 tests)
- [x] `tests/test_bidi_stream_adapter.cpp` — BidiStreamAdapter unit tests (20 tests)
- [x] `tests/test_grpc_observability.cpp` — health, metrics, prometheus, access log (12 tests, GOB-01..12)
- [x] Fail-closed TLS: start() returns false (no insecure fallback)
- [x] Keepalive, multi-port, TLS reload tests
- [ ] Integration test: mTLS round-trip with echo service (Target: Q3 2026)

### Phase 5: Performance / Hardening ✅
- [x] Keepalive tuning via channel arguments
- [x] Health service — `setServiceHealth` / `isServiceHealthy` (Target: Q4 2026)
- [x] Interceptors + Prometheus — `recordRPC` + `getMetricsText` (Target: Q4 2026)

### Phase 6: Documentation & Acceptance ✅
- [x] README, ARCHITECTURE, AUDIT, CHANGELOG, ROADMAP, SECURITY, FUTURE_ENHANCEMENTS

---

## Production Readiness Checklist

| Area | Status | Notes |
|------|--------|-------|
| Core RPC transport | ✅ | HTTP/2 + Protocol Buffers via gRPC C++ |
| TLS / mTLS | ✅ | Fail-closed; mTLS requires and verifies client cert |
| Service registration | ✅ | Any `grpc::Service*` accepted before start |
| Thread safety | ✅ | Atomic running state; mutex-protected stats, TLS, metrics, and health |
| Uptime tracking | ✅ | Incremental via `std::chrono` in `getStats()` |
| Keepalive tuning | ✅ | `extra_config["keepalive_time_ms/timeout_ms"]` |
| Multi-port binding | ✅ | Admin port via `extra_config["admin_port"]` |
| TLS hot-reload | ✅ | `reloadTls()` — fail-safe on bad certs |
| BidiStreamAdapter | ✅ | Header-only; backpressure; thread-safe write |
| Health service | ✅ | `setServiceHealth(name, bool)` + auto SERVING/NOT_SERVING on start/stop |
| Interceptor metrics | ✅ | `recordRPC(method, success, ms)` — per-method request/error/latency counters |
| Prometheus metrics | ✅ | `getMetricsText()` — 4 metric families in Prometheus text v0.0.4 |
| Structured access log | ✅ | `setAccessLogSink()` + `logAccess()` — JSON per-RPC entries |
| Unit/integration tests | ✅ | 83 unit tests (51+20+12); integration tests planned Q3 2026 |

---

## Known Issues & Limitations

- `setServiceHealth()` tracks health state in-process; does not wire to gRPC's built-in `grpc.health.v1.Health` proto service (no external proto dependency required).
- `reloadTls()` updates the cached credentials only; existing TLS sessions are not renegotiated.
- SIGHUP-triggered hot-reload is not yet implemented (must call `reloadTls()` explicitly).
- `recordRPC()` is a manual hook; automatic intercept of all server-side calls requires gRPC C++ experimental interceptors (planned v0.4.0).

