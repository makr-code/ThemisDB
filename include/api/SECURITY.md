<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — API Module Public Headers

**Module Path:** `include/api/`
**Implementation Security:** `../../src/api/SECURITY.md`

---

## Scope

Security considerations for the public API header surface. Covers authentication integration
points, rate limiting, audit logging, input validation hooks, and observability data handling
across GraphQL, gRPC, HTTP, and WebSocket protocol headers.

---

## Threat Model

| Threat | Vector | Mitigation Header |
|--------|--------|------------------|
| Unauthenticated API access | Missing auth token in request | `IHTTPHandler`/`IGraphQLEngine` require auth context; `IAuditLogger` logs attempts |
| GraphQL introspection abuse | Unlimited schema traversal | `graphql.h` — `GraphQLRequest.disable_introspection` flag |
| Denial of service via query depth | Deeply nested GraphQL queries | `graphql.h` — `max_depth`, `max_complexity` limits in `GraphQLRequest` |
| Rate limit bypass | Parallel connections exceeding quota | `rate_limiter.h` — per-IP and per-tenant policies enforced at handler level |
| Injection via GraphQL variables | Unsanitised variable substitution | `IGraphQLEngine` contract requires parameterised variable handling |
| WebSocket hijacking | Missing origin validation | `IWebSocketHandler` — origin allowlist in `WebSocketConfig` |
| OTLP data exfiltration | Sensitive spans exported to untrusted collector | `IOTLPExporter::OTLPConfig` — TLS and auth token required for export endpoint |
| Correlation ID spoofing | Attacker injects trace context headers | `CorrelationContext` validates header format; rejects malformed IDs |
| Audit log tampering | Attacker suppresses `IAuditLogger` calls | Handler contracts require audit emission before response is sent |
| Persisted query cache poisoning | Malicious APQ hash collision | `IPersistedQueryStore` — SHA-256 content hashing required |

---

## Security Controls

### Rate Limiting
`IRateLimiter` is a required dependency for all request handlers; the `rate_limiter.h`
interface supports token bucket, sliding window, and fixed window policies with per-tenant
and per-IP buckets.

### Audit Logging
`IAuditLogger` must be injected into all handler implementations; `AuditEvent` captures
operation type, tenant, user, IP, and result code.

### GraphQL Security
`GraphQLRequest` exposes `max_depth`, `max_complexity`, `disable_introspection`, and
`allowed_operations` fields for defence-in-depth at the schema level.

### Tracing Data Sensitivity
`ITracingMiddleware` must be configured to redact PII fields from span attributes;
see `OTLPConfig::redacted_attributes` in `otlp_exporter.h`.

---

## Known Limitations

- 15 open security-relevant findings at implementation level; see `../../src/api/AUDIT.md`.
- GraphQL subscription DoS via unbounded long-lived connections is rate-limited but not
  connection-count limited in the current header contract; planned for v1.9.0.
- `ws_handler.h` and `websocket_handler.h` overlap may create confusion about which
  origin validation path applies; to be resolved in v1.9.0.
