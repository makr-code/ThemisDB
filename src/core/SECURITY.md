> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Core Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Core module provides the dependency injection framework, structured logging, distributed tracing, metrics, and context propagation used by all other ThemisDB modules. Security concerns focus on: preventing credential exposure through logging, securing trace context propagation, protecting the circuit breaker from abuse, and ensuring audit event integrity.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Credential exposure in logs | Log message content is caller-controlled; secrets/PII must not be logged by convention and code review |
| Trace context injection via untrusted headers | Trace extraction is adapter-driven (`ITracer::startSpanFromHeaders()` / propagator helpers); invalid or malformed context must be ignored by adapters |
| SSRF via OTLP/Jaeger/Zipkin exporter | Exporter endpoints are configured by operators, not by request headers; circuit breaker prevents cascade from unreachable exporters |
| Metrics label injection | Metric labels are defined at registration time using static strings; no user-supplied label values are accepted |
| Log level escalation | Dynamic log level adjustment requires privileged API access; not exposed via public HTTP endpoints |
| Feature flag abuse | Feature flags are read-only from untrusted callers; flag state is set only by operators |
| Context corruption across async boundaries | `ContextPropagation` / `ContextScope` propagate context via thread-local state with child-context copies; shared mutable request context is not used |
| Audit event tampering | Integrity and durability depend on selected `IAuditLog` adapter (`noop`/`inmemory` by default); compliance-grade append-only sinks must be provided by deployment |

## Security Controls

### Structured Logging
- `SpdlogLoggerAdapter` supports plain-text and structured JSON modes; trace/span/request IDs are injected automatically via `logWithTrace()`.
- Log messages contain no request body content; only correlation IDs and severity-qualified messages.
- `NoopLogger` is used for performance-critical paths where logging overhead must be zero.

### Distributed Tracing
- W3C TraceContext is supported by tracer interfaces (`startSpanFromHeaders`, `injectContext`) and adapter implementations.
- Zipkin/Jaeger adapters additionally support B3 / `uber-trace-id` interoperability paths where applicable.
- Circuit breaker prevents trace exporter failures from affecting request processing.

### Health and Readiness
- Core exposes per-concern health/readiness aggregation APIs; HTTP endpoint exposure is owned by the server module.
- Health checks do not expose internal configuration, connection strings, or credential status.
- Readiness checks fail closed: a degraded tracer or metrics sink does not mark the service as unhealthy unless configured to do so.

### Audit Events
- Audit event interface emits to the audit sink configured at `ConcernsContext` construction time.
- Audit events are structured (event type, principal, timestamp, resource) — no free-text injection possible.

## Data Handling

- Core module does not persist any data; all state is held in memory for the lifetime of the `ConcernsContext` instance.
- Trace and span IDs included in log lines are correlation identifiers, not personal data.
- Prometheus metric labels contain only statically defined names; no user-supplied values.
- Secrets injection interface (`ISecrets`) is implemented (Issue #1417); `InMemorySecrets` and `EnvSecretsProvider` accept credentials via environment variables or in-memory injection; secrets are never included in logs or traces.

## Known Limitations

- Secrets interface (`ISecrets`) is implemented (Issue #1417): `InMemorySecrets` (map-backed, thread-safe) and `EnvSecretsProvider` (environment-variable-backed with configurable prefix) are available; config-driven selection via `Config::secretsAdapter` (`"noop"`, `"inmemory"`, `"env"`).
- Plugin-based adapter loading (Issue #1706) is not yet implemented; adapter selection requires recompilation.
- Log message content is caller-controlled; there is no automatic PII scrubbing at the core logger level.
- Core security bootstrap is fail-closed by design in production mode, but deployment correctness still depends on operator-supplied Vault/HSM/JWT configuration values.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| spdlog | Structured logging | Keep patched; no async sink by default |
| OpenTelemetry C++ SDK | OTLP trace export | Keep patched; circuit-breaker guarded |
| Prometheus C++ client | Metrics export | Keep patched |
