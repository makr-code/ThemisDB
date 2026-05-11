> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/api/SECURITY.md -->

# Security — API Module Public Headers

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../SECURITY.md).

For the full implementation-level security document (threat model, transport security, audit logging configuration) see:

→ [`../../src/api/SECURITY.md`](../../src/api/SECURITY.md)

---

## Scope

This document covers security considerations that are specific to the **public header contract** in `include/api/` — type safety, interface contracts, build-conditional exposure, and known limitations in header-defined types.

---

## Security-Relevant Headers

| Header | Security Concern | Status |
|--------|-----------------|--------|
| `graphql.h` | `QueryLimits` — depth/complexity/introspection guards | ✅ `QueryLimits::production()` disables introspection |
| `graphql.h` | `Parser::error()` deprecated — dual error path risk | ⚠️ Tracked for removal in v2.1.0 |
| `rate_limiter.h` | `RateLimiter::allow()` — unbounded bucket map, nested lock | ✅ TTL eviction + `std::shared_mutex` added |
| `audit_logger.h` | `AuditLogger::log()` — handler dispatch under mutex | ✅ Non-blocking handler dispatch fixed |
| `persisted_queries.h` | `QueryAllowList::enabled_ = false` by default | ⚠️ No startup warning in production; tracked in `FUTURE_ENHANCEMENTS.md` |
| `grpc_server.h` | `GrpcApiServer` TLS enforcement | ✅ Fails closed on cert load failure |
| `grpc_bridge.h` | `IGRPCBridge` — no concrete implementation | ⚠️ Extension point only; all gRPC routing currently via `GrpcApiServer::registerService()` |
| `otlp_exporter.h` | Trace data must not include PII | ✅ `SpanData` fields do not include document content |
| `api_gateway_hook.h` | `IAPIGatewayHook` lifecycle — deprecated methods | ⚠️ CANDIDATE_FOR_REMOVAL; see `AUDIT.md` FINDING-HDR-001 |

---

## Build Conditional Security

Headers that expose security-relevant surfaces are guarded by build conditionals:

| Symbol | Headers | Risk if accidentally included |
|--------|---------|-------------------------------|
| `THEMIS_ENABLE_GRPC` | `grpc_server.h`, `themisdb_grpc_service.h` | Exposes gRPC types; binds against gRPC library |
| `THEMIS_ENABLE_OTEL` | `otlp_exporter.h` | Enables outbound trace data export; requires libcurl |

Production builds should enable only the protocols required by the deployment profile.

---

## Interface Contract Security Notes

### `graphql.h` — `QueryLimits`

```cpp
// Secure default for production deployments:
auto limits = graphql::QueryLimits::production();
// Sets: allow_introspection = false, maxDepth = 10, maxComplexity = 1000, max_subscriptions = 100

// Development-only permissive limits (do NOT use in production):
auto limits = graphql::QueryLimits::defaults();
```

`QueryLimits::production()` disables GraphQL schema introspection (`__schema`, `__type`) to prevent schema enumeration attacks. Always use `production()` limits in deployments exposed to untrusted clients.

### `persisted_queries.h` — `QueryAllowList`

The allow-list is disabled by default. Enable it in production to restrict accepted queries to a known set:

```cpp
api::QueryAllowList::instance().setEnabled(true);
api::QueryAllowList::instance().registerQuery("sha256:<hash>", query_string);
```

> ⚠️ Without the allow-list, any well-formed GraphQL query is accepted by the parser. In high-security deployments, always enable the allow-list.

### `rate_limiter.h` — `RateLimiter`

```cpp
// Per-tenant rate limiting (recommended):
api::OperationRateLimiter op_limiter;
op_limiter.setLimit("queries", std::make_shared<api::RateLimiter>(config));

if (!op_limiter.allow("queries", tenant_id)) {
    // Return HTTP 429
}
```

The token-bucket rate limiter provides per-key burst protection. Keys are typically tenant IDs or client IP addresses.

### `audit_logger.h` — `AuditLogger`

Audit events should be registered for all security-relevant API operations:

```cpp
api::AuditLogger::instance().log({
    .actor = tenant_id,
    .action = "document.delete",
    .resource = collection + "/" + key,
    .outcome = api::AuditOutcome::SUCCESS,
});
```

For persistent audit trails, register a `FileAuditLogHandler`:

```cpp
api::AuditLogger::instance().addFileHandler("/var/log/themisdb/audit.jsonl");
```

---

## Known Limitations

- `persisted_queries.h::QueryAllowList::enabled_ = false` by default. In production deployments, `setEnabled(true)` must be called explicitly. A startup warning will be added in v2.1.0 (see `FUTURE_ENHANCEMENTS.md`).
- `api_gateway_hook.h` deprecated symbols (`hookId`, `registerHook`, `unregisterHook`, `getHooks`) are marked `[[deprecated]]` but remain in the ABI until the major-version removal decision is made.
- `grpc_bridge.h::IGRPCBridge` has no concrete implementation; all gRPC service registration routes through `GrpcApiServer::registerService()` until `GrpcBridgeImpl` is implemented.

---

## References

- Full security documentation: [`../../src/api/SECURITY.md`](../../src/api/SECURITY.md)
- Project security policy: [`../../SECURITY.md`](../../SECURITY.md)
- Audit report: [`AUDIT.md`](AUDIT.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
