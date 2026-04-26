> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md -->

# Future Enhancements — gRPC Plugin

---

## 1. gRPC Health Check Service

### Scope
Auto-register the standard `grpc.health.v1.Health` service when `GRPCServer::start()`
is called, enabling Kubernetes readiness/liveness probes and load-balancer health checks.

### Design Constraints
- Service state managed by `grpc::health::experimental::HealthCheckServiceInterface`.
- Per-service health state settable via `GRPCServer::setServiceHealth(name, status)`.
- Auto-set to `SERVING` on start; `NOT_SERVING` on stop.

### Required Interfaces
- `GRPCServer::setServiceHealth(const std::string& service_name, bool serving)`
- Health service added to `services_` internally before `BuildAndStart()`.

### Test Strategy
- Unit: health service registered and returns `SERVING` after start.
- Integration: `grpc_health_probe` CLI returns exit code 0 on healthy server.

---

## 2. Server-Side Interceptors for Metrics and Tracing

### Scope
Add a gRPC server interceptor that records per-method request count, latency histogram,
and error codes, and injects trace context (OpenTelemetry) into each RPC.

### Design Constraints
- Interceptor registered via `grpc::ServerBuilder::experimental().SetInterceptorCreators()`.
- Metrics exported to Prometheus via ThemisDB's existing registry.
- OpenTelemetry trace context propagated via `grpc-trace-bin` metadata key.

### Planned Metrics

| Metric | Type | Labels |
|--------|------|--------|
| `grpc_server_requests_total` | Counter | `method`, `status_code` |
| `grpc_server_latency_seconds` | Histogram | `method` |
| `grpc_server_active_calls` | Gauge | `method` |

### Test Strategy
- Unit: interceptor increments counter on each RPC.
- Integration: Prometheus scrape returns all expected metric names.

---

## 3. Bidirectional Streaming Helper ✅ Implemented (v0.2.0)

### Scope
`BidiStreamAdapter<Req, Resp>` is a typed header-only wrapper (`src/rpc_grpc/bidi_stream_adapter.h`)
that simplifies writing bidirectional streaming service implementations on top of `GRPCServer`.

### Design Constraints
- Header-only template class; no additional compilation unit required.
- Thread-safe: `Read()` and `Write()` may be called from different threads.
- `onMessage(Req&&)` callback invoked for each inbound message.
- Backpressure: `Write()` blocks if the outbound queue exceeds a configurable depth (default: 100).

### Required Interfaces
```cpp
template <typename Req, typename Resp>
class BidiStreamAdapter {
public:
    void onMessage(std::function<void(Req&&)> handler);
    bool write(Resp response);
    void finish(grpc::Status status);
    void run();
    bool isFinished() const;
    std::size_t queueDepth() const;
    grpc::Status finishStatus() const;
};
```

### Test Strategy
- 20 unit tests (BSA-01 … BSA-20) in `tests/test_bidi_stream_adapter.cpp`.

---

## 4. TLS Certificate Hot-Reload ✅ Implemented (v0.2.0)

### Scope
TLS certificates can be reloaded without restarting the gRPC server via
`GRPCServer::reloadTls(cert_path, key_path, ca_path)`.

### Design Constraints
- New certificates applied to new connections only; existing TLS sessions continue
  with their negotiated parameters.
- Reload triggered by explicit `GRPCServer::reloadTls()` call.
- If new cert files are invalid, the old credentials remain active (fail-safe).
- SIGHUP-triggered hot-reload is planned for Q1 2027.

### Test Strategy
- `tests/test_grpc_plugin.cpp` — `GRPCServerTlsReloadTest` suite (11 tests).
- Failure test: supply invalid cert path; verify old credentials remain (`false` returned).

### Security
- New cert must pass the same `configureCredentials()` validation as initial load.
- Uses `buildSslCredentials()` private helper factored out from `configureCredentials()`.
