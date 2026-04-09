<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — API Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The API module is the primary network entry point for ThemisDB. Security concerns include: authentication enforcement, transport security, rate limiting, request validation, tenant isolation in routing, and prevention of resource exhaustion via subscriptions or long-running queries.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Unauthenticated access to endpoints | Authentication and authorization middleware enforced on all routes; no endpoint is accessible without a valid JWT/API key |
| TLS downgrade or MITM | TLS/SSL enforced with certificate configuration; no plaintext fallback in production mode |
| Brute force or credential stuffing | Rate limiting middleware with per-client token bucket (Issue #1495); delegated to auth module for lockout |
| GraphQL subscription fan-out (DoS) | `QueryLimits::max_subscriptions` per-connection cap prevents unbounded WebSocket subscription creation |
| Long-running query resource exhaustion | Async job API enforces per-tenant query time limits; configurable job TTL |
| Cross-tenant data leakage via namespace routing | Multi-tenant namespace routing isolates tenants at the routing layer; AQL executor enforces tenant scope |
| Request injection via AQL execution endpoint | AQL is parsed and validated before execution; raw query strings are never interpolated into backend calls |
| Correlation ID spoofing | `TracingMiddleware` accepts `X-Correlation-ID` from trusted upstream headers only; untrusted values are sanitized |
| gRPC reflection exposure | gRPC reflection service is disabled in production mode; enabled only in development environments |
| Oversized request payloads | HTTP server enforces configurable max request body size; large uploads are rejected before processing |

## Security Controls

### Transport Security
- TLS/SSL with configurable certificate paths; all production deployments must configure certificates.
- mTLS is supported at the server level for service-to-service communication.

### Authentication Middleware
- Every route passes through `auth_middleware.cpp` which validates JWT tokens or API keys before dispatching.
- Token validation is delegated to the auth module (JWT validator, JWKS caching).

### Rate Limiting
- Per-client token bucket rate limiter implemented in the middleware pipeline.
- GraphQL subscription count capped via `QueryLimits::max_subscriptions`.
- Async job queue depth limited per tenant.

### Request Tracing
- `X-Correlation-ID` propagated through all log lines for audit trail correlation.
- OTLP trace export for distributed tracing; no sensitive request data included in trace attributes.

### WebSocket Security
- WebSocket upgrade only permitted after authentication is validated.
- `graphql-transport-ws` protocol enforces message framing; raw frame injection is not possible.

## Data Handling

- The API module is a routing and protocol layer; it does not persist data.
- Request and response payloads are held in memory only for the duration of request processing.
- Correlation IDs and trace data written to logs do not include document content or PII.
- AQL query strings may contain user-supplied values; these are passed to the AQL executor which enforces parameterization.

## Known Limitations

- OpenAPI specification may be incomplete for newer endpoints added after v1.6.0 (Issue #1491 in progress).
- API versioning routing (`/v1/`, `/v2/` prefixes with unversioned redirect) is implemented; deprecation-header governance policy remains in progress (Issue #1497).
- gRPC reflection is controlled by configuration; operators must explicitly disable it for production deployments.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| Crow / Boost.Beast | HTTP server | Keep patched; TLS via Boost.Asio+OpenSSL |
| gRPC / Protobuf | gRPC surface | Pin versions; reflection disabled in production |
| OpenSSL | TLS | System-provided; must be kept up to date |
| nlohmann/json | JSON parsing | Input validation before deserialization |
