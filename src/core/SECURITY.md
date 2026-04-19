> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Core Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Core module provides the dependency injection framework, structured logging, distributed tracing, metrics, and context propagation used by all other ThemisDB modules. Security concerns focus on: preventing credential exposure through logging, securing trace context propagation, protecting the circuit breaker from abuse, and ensuring audit event integrity.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Credential exposure in logs | `SpdlogLoggerAdapter` does not log `IContext` values; log messages are caller-controlled; PII/credentials must not be included in log messages by convention |
| Trace context injection via untrusted headers | W3C `traceparent`/`tracestate` parsing is validation-only; malformed values are silently dropped; parsed values propagated in internal context only |
| SSRF via OTLP/Jaeger/Zipkin exporter | Exporter endpoints are configured by operators, not by request headers; circuit breaker prevents cascade from unreachable exporters |
| Metrics label injection | Metric labels are defined at registration time using static strings; no user-supplied label values are accepted |
| Log level escalation | Dynamic log level adjustment requires privileged API access; not exposed via public HTTP endpoints |
| Feature flag abuse | Feature flags are read-only from untrusted callers; flag state is set only by operators |
| Context corruption across async boundaries | `W3CTraceContextPropagator` creates immutable context copies for async propagation; shared mutable state is not used |
| Audit event tampering | Audit event interface emits to an append-only audit sink; events cannot be modified after emission |

## Security Controls

### Structured Logging
- `SpdlogLoggerAdapter` supports plain-text and structured JSON modes; trace/span/request IDs are injected automatically via `logWithTrace()`.
- Log messages contain no request body content; only correlation IDs and severity-qualified messages.
- `NoopLogger` is used for performance-critical paths where logging overhead must be zero.

### Distributed Tracing
- W3C TraceContext standard (`traceparent`/`tracestate`) is used for trace propagation; B3 and `uber-trace-id` headers also supported via Zipkin/Jaeger adapters.
- Inbound trace context from untrusted HTTP headers is parsed and validated; malformed values are silently ignored.
- Circuit breaker prevents trace exporter failures from affecting request processing.

### Health and Readiness
- `/health/live` and `/health/ready` endpoints return aggregate per-concern health status.
- Health checks do not expose internal configuration, connection strings, or credential status.
- Readiness checks fail closed: a degraded tracer or metrics sink does not mark the service as unhealthy unless configured to do so.

### Audit Events
- Audit event interface emits to the audit sink configured at `ConcernsContext` construction time.
- Audit events are structured (event type, principal, timestamp, resource) — no free-text injection possible.

## Data Handling

- Core module does not persist any data; all state is held in memory for the lifetime of the `ConcernsContext` instance.
- Trace and span IDs included in log lines are correlation identifiers, not personal data.
- Prometheus metric labels contain only statically defined names; no user-supplied values.
- Secrets injection interface (`ISecrets`, planned Issue #1417) will accept credentials via environment or vault; secrets will never be included in logs or traces.

## Known Limitations

- Secrets interface (`ISecrets`) is planned but not yet fully implemented (Issue #1417); currently credentials are injected via environment variables at component construction time.
- Plugin-based adapter loading (Issue #1706) is not yet implemented; adapter selection requires recompilation.
- Log message content is caller-controlled; there is no automatic PII scrubbing at the core logger level.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| spdlog | Structured logging | Keep patched; no async sink by default |
| OpenTelemetry C++ SDK | OTLP trace export | Keep patched; circuit-breaker guarded |
| Prometheus C++ client | Metrics export | Keep patched |
