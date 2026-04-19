<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md -->

# Security Notes — Server Module (Public Headers)

## Scope

This document covers security considerations for the public headers in `include/server/`.
For implementation-level security analysis see `../../src/server/SECURITY.md`.

---

## Threat Model

| Threat | Impact | Mitigation |
|--------|--------|-----------|
| Authentication bypass | Critical | `auth_middleware.h` is mandatory in the gateway pipeline; no handler can be reached without it |
| Tenant data leakage | Critical | `tenant_manager.h` isolates context before any handler is invoked; cross-tenant references fail at dispatch |
| Rate limit bypass | High | `rate_limiter_v2.h` enforces limits at the gateway before handler execution; `cost_based_rate_limiter.h` caps expensive queries |
| Request injection via malformed input | High | `request_validation_middleware.h` schema-validates all requests before routing |
| gRPC-Web CORS misconfiguration | High | `grpc_web_proxy_handler.h` requires explicit CORS allowlist; no wildcard origin permitted |
| mTLS certificate spoofing | High | `auth_middleware.h` `AuthContext::trust_level` is set only after successful mTLS handshake via `transport_security_checker.h` |
| DoS via connection exhaustion | High | `load_shedder.h` sheds excess load; `sse_connection_manager.h` caps SSE connections per tenant |
| HTTP/3 datagram amplification | Medium | `http3_production_config.h` enforces QUIC address validation before datagram flow |
| OAuth2 token leakage via logs | Medium | `oauth2_provider.h` tokens are opaque handles; raw bearer strings never appear in public API types |
| Policy evaluation bypass via OPA downtime | Medium | `opa_adapter.h` fails closed (deny) when the OPA sidecar is unreachable |
| API key brute force | Medium | `api_key_mgmt_handler.h` keys are rate-limited independently via `cost_based_rate_limiter.h` |
| GraphQL introspection data exposure | Low | `graphql_api_handler.h` requires `Permission::INTROSPECT` for schema introspection |

---

## Security Controls

- **Auth-first middleware chain** — `auth_middleware.h` runs before all domain handlers; there
  is no opt-out path through `api_gateway.h`.
- **Tenant isolation at dispatch** — `tenant_manager.h` sets tenant context before handler
  invocation; handlers cannot access cross-tenant storage without an explicit escalation
  permission.
- **Rate limiting layered** — three rate limiters (`rate_limiter_v2.h`,
  `adaptive_rate_limiter.h`, `cost_based_rate_limiter.h`) can be stacked; the gateway applies
  all active limiters in order.
- **Input validation mandatory** — `request_validation_middleware.h` validates against the
  registered OpenAPI schema from `openapi_route_registry.h`; unvalidated routes are rejected at
  registration time.
- **Fail-closed policy evaluation** — both `opa_adapter.h` and `policy_engine.h` deny by default
  on error or timeout.
- **Audit trail** — `api_security_audit.h` records every authenticated request with principal,
  tenant, action, and outcome; integrated with `SecurityEvidenceCollector` from
  `include/security/`.
- **gRPC-Web proxy CORS** — explicit allowlist required; wildcard origin (`*`) is rejected by
  `GrpcWebProxyHandler` at startup.

---

## Known Limitations

- `smart_routing.h` ML routing does not enforce security boundaries; routing decisions are
  advisory and do not bypass `auth_middleware.h`.
- `mqtt_session.h` MQTT-over-TLS is enforced at connection time, but MQTT topic ACLs are
  implemented in the handler, not in the session header contract.
- `postgres_session.h` wire-protocol adapter trusts the upstream TLS terminator for client
  identity; direct TCP connections without mTLS are not supported in production.
- `wasm_handler_registry.h` WASM sandbox isolation is implementation-dependent; the public
  interface does not guarantee memory isolation between WASM modules.
