### Context

This issue implements the roadmap item 'Request Tracing and Correlation IDs' for the api domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Request Tracing and Correlation IDs

### Goal

Deliver the scoped changes for Request Tracing and Correlation IDs in src/api/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Request Tracing and Correlation IDs
**Priority:** Medium
**Target Version:** v1.7.0

All inbound requests must carry or receive a `X-Correlation-ID` header that propagates through the entire call stack (API → AQL → storage → cache) and appears in all log lines and error responses.

**Implementation Notes:**
- `[x]` Add `TracingMiddleware` in `src/api/tracing_middleware.cpp`; generate UUID v4 if `X-Correlation-ID` absent; inject into thread-local `RequestContext`. (**Implemented** — `TracingMiddleware::processRequest()` uses `boost::uuids::random_generator` per thread.)
- `[x]` Forward `RequestContext::correlationId` to `utils/logger.h` log macros via a structured field (`correlation_id`). (**Implemented** — `utils::Logger::setTraceContext(corr_id)` called in `processRequest()`.)
- `[x]` Echo back `X-Correlation-ID` in all responses including errors and SSE streams (implemented in `HttpServer::applyGovernanceHeaders()`).
- `[x]` Export span data to OpenTelemetry collector via OTLP HTTP exporter (configurable endpoint in `config/networking/`). Implemented in `include/api/otlp_exporter.h` + `src/api/otlp_exporter.cpp` (async queue + libcurl POST, OTLP JSON format); `TracingMiddleware` extended with `finishSpan()` and optional `OtlpExporter*`; configuration in `config/networking/otlp.yaml`.
- `[x]` Decision: retain proprietary `X-Correlation-ID` as the primary correlation header; the OTLP exporter uses the correlation-ID value as the OTLP `traceId`. A future W3C `traceparent` bridge can be added when SDK interoperability is required.

**Performance Targets:**
- Middleware overhead < 10 µs per request (UUID generation + thread-local write).
- Zero correlation ID collision probability for ≥ 1 billion requests (UUID v4 guarantee).

---

### Acceptance Criteria

- [ ] Add `TracingMiddleware` in `src/api/tracing_middleware.cpp`; generate UUID v4 if `X-Correlation-ID` absent; inject into thread-local `RequestContext`. (**Implemented** — `TracingMiddleware::processRequest()` uses `boost::uuids::random_generator` per thread.)
- [ ] Forward `RequestContext::correlationId` to `utils/logger.h` log macros via a structured field (`correlation_id`). (**Implemented** — `utils::Logger::setTraceContext(corr_id)` called in `processRequest()`.)
- [ ] Echo back `X-Correlation-ID` in all responses including errors and SSE streams (implemented in `HttpServer::applyGovernanceHeaders()`).
- [ ] Export span data to OpenTelemetry collector via OTLP HTTP exporter (configurable endpoint in `config/networking/`). Implemented in `include/api/otlp_exporter.h` + `src/api/otlp_exporter.cpp` (async queue + libcurl POST, OTLP JSON format); `TracingMiddleware` extended with `finishSpan()` and optional `OtlpExporter*`; configuration in `config/networking/otlp.yaml`.
- [ ] Decision: retain proprietary `X-Correlation-ID` as the primary correlation header; the OTLP exporter uses the correlation-ID value as the OTLP `traceId`. A future W3C `traceparent` bridge can be added when SDK interoperability is required.
- [ ] Middleware overhead < 10 µs per request (UUID generation + thread-local write).
- [ ] Zero correlation ID collision probability for ≥ 1 billion requests (UUID v4 guarantee).

### Relationships

- Roadmap row: #140 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/api/FUTURE_ENHANCEMENTS.md#request-tracing-and-correlation-ids
- Source key: roadmap:140:api:v1.7.0:request-tracing-and-correlation-ids

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:140:api:v1.7.0:request-tracing-and-correlation-ids -->
<!-- roadmap-ref: row=140;module=api;target=v1.7.0 -->
<!-- roadmap-detail: src/api/FUTURE_ENHANCEMENTS.md#request-tracing-and-correlation-ids -->
