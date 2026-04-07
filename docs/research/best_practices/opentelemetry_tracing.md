# OpenTelemetry Span Instrumentation at Every API Handler Entry Point

**Metadaten:**
- Source: OpenTelemetry Specification (CNCF) — Tracing API
- URL: https://opentelemetry.io/docs/specs/otel/trace/api/
- Tags: observability, tracing
- ThemisDB-Versionen: v1.9.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

Distributed tracing provides end-to-end visibility into request flows across microservice boundaries. Without consistent span creation at every API entry point, correlation of latency, errors, and retries across the ThemisDB server and its downstream components (storage, index, chimera) becomes impossible. The OpenTelemetry specification defines a vendor-neutral tracing API; the `Tracer::startSpan("handleXxx")` pattern applied uniformly at handler entry is the minimal requirement for actionable trace data.

ThemisDB v1.9.0 standardised on this pattern across all 64 API handler files via the `utils/tracing.h` helper header, which wraps the OpenTelemetry C++ SDK into a thin RAII-based scope guard (`ScopedSpan`) that automatically closes and records status on scope exit.

## 🎯 Core Principles

- **Span at every handler entry**: Every public API handler (REST, WebSocket, MQTT, admin) creates a span at its first line; the span name follows the convention `"handle<OperationName>"` (e.g., `"handleQueryExecute"`).
- **Context propagation**: Incoming W3C `traceparent` / `tracestate` headers are extracted and used as the parent context; spans without an incoming context start a new root trace.
- **RAII span lifecycle**: The `ScopedSpan` guard starts the span in its constructor and calls `span->End()` in its destructor, ensuring spans are always closed even on exception paths.
- **Semantic conventions**: Span attributes follow OpenTelemetry semantic conventions (`db.system`, `db.operation`, `http.method`, `http.status_code`, `net.peer.ip`).
- **Error recording**: On exception, `span->SetStatus(StatusCode::kError, e.what())` is called before rethrowing; on success, `StatusCode::kOk` is set.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `utils/tracing.h` — `ScopedSpan` RAII class; `getTracer()` singleton; `extractContext(headers)` helper.
- All 64 handler files under `src/server/handlers/` — Each opens with `ScopedSpan span(getTracer(), "handleXxx", extractContext(request.headers));`
- `src/chimera/` — Async retry iterations annotate the active span with `retry.attempt` and `retry.delay_ms` attributes.
- `src/index/` — Index read and write operations create child spans (`"indexLookup"`, `"indexWrite"`) under the handler span.

### What Was Adopted?

- `ScopedSpan` constructor: calls `tracer->StartSpan(name, options)` with parent context; stores `std::unique_ptr<opentelemetry::trace::Span>`.
- `ScopedSpan` destructor: calls `span_->SetStatus(ok_ ? StatusCode::kOk : StatusCode::kError, message_)` then `span_->End()`.
- `ScopedSpan::setAttr(key, value)` forwards to `span_->SetAttribute(key, value)` for handler-specific attributes.
- `ScopedSpan::recordError(exception)` records exception details and marks `ok_ = false`.
- OTLP gRPC exporter is configured via `OTEL_EXPORTER_OTLP_ENDPOINT` environment variable; default localhost:4317 for development.
- Sampling rate is configurable via `OTEL_TRACES_SAMPLER=parentbased_traceidratio` and `OTEL_TRACES_SAMPLER_ARG=0.1` (10% in production).

### Deviations & Rationale

- **Thin wrapper rather than raw SDK**: The OpenTelemetry C++ SDK API is verbose; `ScopedSpan` reduces boilerplate from ~8 lines to 1 line per handler. This is a purely ergonomic wrapper with no semantic deviation.
- **No baggage propagation**: OpenTelemetry baggage is not used; all cross-cutting context (user-id, tenant-id) is passed as span attributes rather than baggage to keep the context model simple.
- **Single global tracer**: `getTracer()` returns a process-global `Tracer` instance. Multi-tenancy tracing isolation (per-tenant tracers) is tracked for v2.2.0.

## ⚠️ Trade-offs & Limitations

- **Overhead at high request rates**: Creating a span per request adds ~1–3 µs of overhead per handler invocation. At the current production sampling rate (10%), the effective overhead per request is ~0.1–0.3 µs. At 100% sampling (development mode), total tracing overhead is ~2–5% of request processing time.
- **Span cardinality explosion**: If span names include dynamic identifiers (e.g., `"handleQuery/userXxx"`), backend systems struggle with high-cardinality span names. ThemisDB uses fixed operation names; dynamic identifiers go into attributes only.
- **OTLP exporter backpressure**: Under heavy load the OTLP exporter queue can fill; dropped spans are counted in `otel.exporter.otlp.dropped_spans` metric. The queue size is configurable but defaults to 2048.
- **SDK version pinning**: The OpenTelemetry C++ SDK API is not yet stable (v1.x); minor version upgrades occasionally require updating the wrapper. The `utils/tracing.h` abstraction isolates these changes to a single file.

## 🔬 Validation

- [x] Code reviewed against OpenTelemetry tracing specification (trace API + semantic conventions)
- [x] Unit tests in `tests/server/tracing_test.cpp` verify span creation, attribute recording, and error status propagation
- [x] Integration tests verify W3C `traceparent` propagation across handler → index → storage call chain
- [x] Performance benchmark confirms < 5% overhead at 100% sampling
- [x] Module README linked (`src/server/README.md`, `utils/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [Exponential Backoff Retry](exponential_backoff_retry.md)
- [Token Bucket Rate Limiting](token_bucket_rate_limiting.md)

---
**Last Updated:** 2026-04-06
