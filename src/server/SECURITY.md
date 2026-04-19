> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Server Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Unauthenticated API access | JWT/OIDC validation on every request; auth middleware mandatory |
| Brute force / credential stuffing | Token bucket rate limiting per client (local + Redis backends) |
| DDoS / request flood | Per-client rate limiter; connection limits; HTTP/3 flow control |
| Path traversal in API routes | Strict route matching; no filesystem path construction from request |
| WebSocket hijacking | Origin validation; JWT authentication on WS upgrade |
| Malicious WASM handlers | WasmHandlerRegistry sandbox; memory isolation per handler |
| TLS downgrade attacks | TLS 1.3 minimum; HSTS enforced; certificate pinning available |
| SSRF via webhook endpoints | Allowlist validation for outbound URLs |
| Header injection | Request header sanitization in middleware |
| Cross-site request forgery | CSRF token validation for state-changing endpoints |

## Security Controls

- TLS 1.3 enforced on all external endpoints
- JWT validation with JWKS caching (RS256, ES256)
- Rate limiting: token bucket per IP/client with Redis-backed distributed state
- WASM handlers run in isolated sandboxes via `WasmHandlerRegistry`
- All admin endpoints require elevated privilege claims
- Audit logging for all write operations and auth events
- CORS policy enforced on all API responses

## Data Handling

- Request bodies are not logged by default (configurable)
- PII fields in admin cache endpoints require explicit operator action
- TLS certificates stored outside the application data directory

## Known Limitations

- HTTP/3 requires UDP port availability (may be blocked by some firewalls)
- WASM sandbox does not yet enforce CPU time quotas

## Dependency Security

- Boost.Beast/Asio: network I/O — kept up to date via vcpkg
- OpenSSL: TLS — version pinned; CVE monitoring active
- libwebsockets / WebSocket++ for WebSocket transport
