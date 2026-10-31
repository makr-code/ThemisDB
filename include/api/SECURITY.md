---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

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
| `rate_limiter.h` | `graphql::RateLimiter::allow()` — per-key bucket map and periodic idle-bucket eviction | ⚠️ Active distinct keys are not capped; `RateLimiter` uses a mutex, while `OperationRateLimiter` uses a shared mutex |
| `graphql_audit_logger.h` | `graphql::AuditLogger::log()` — handler dispatch and in-memory event buffer | ✅ Handlers are copied under the buffer lock and invoked after releasing it |
| `persisted_queries.h` | `graphql::QueryAllowList::enabled_` defaults to false | ⚠️ `isAllowed()` checks membership independently of this flag; production enforcement is unverified |
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
| `THEMIS_ENABLE_WEBSOCKET` | `ws_handler.h`, `graphql_ws_handler.h` | Exposes WebSocket API types and requires the WebSocket dependencies |

`otlp_exporter.h` includes libcurl directly and is not guarded by `THEMIS_ENABLE_OTEL`.

Production builds should enable only the protocols required by the deployment profile.

---

## Interface Contract Security Notes

### `graphql.h` — `QueryLimits`

```cpp
// Secure default for production deployments:
auto limits = graphql::QueryLimits::production();
// Disables introspection; the other limits retain the QueryLimits defaults.

// Development-only permissive limits (do NOT use in production):
auto limits = graphql::QueryLimits::defaults();
```

`QueryLimits::production()` disables GraphQL schema introspection (`__schema`, `__type`) to prevent schema enumeration attacks. Always use `production()` limits in deployments exposed to untrusted clients.

### `persisted_queries.h` — `QueryAllowList`

`QueryAllowList` exposes a disabled-by-default state flag. `isAllowed()` performs only a membership
check; the flag does not change its result. The public header does not establish a production
request-enforcement call site.

```cpp
themis::graphql::QueryAllowList::instance().setEnabled(true);
themis::graphql::QueryAllowList::instance().allow("<query-hash>");
const bool allowed = themis::graphql::QueryAllowList::instance().isAllowed("<query-hash>");
```

TODO: Verify and document where query membership is enforced before relying on `enabled_` as a
security control.

### `rate_limiter.h` — `RateLimiter`

```cpp
// Per-tenant rate limiting (recommended):
themis::graphql::RateLimiter::Config config;
themis::graphql::OperationRateLimiter op_limiter;
op_limiter.setLimit("queries", config);

if (!op_limiter.allow("queries", tenant_id)) {
    // Return HTTP 429
}
```

The token-bucket rate limiter provides per-key burst protection. Keys are typically tenant IDs or client IP addresses.

### `graphql_audit_logger.h` — `AuditLogger`

Audit events should be registered for all security-relevant API operations:

```cpp
graphql::AuditLogger logger;
graphql::AuditLogEntry entry{};
entry.event_type = graphql::AuditLogEntry::EventType::MutationExecution;
entry.operation_name = "document.delete";
entry.tenant_id = tenant_id;
entry.success = true;
logger.log(entry);
```

For persistent audit trails, register a `FileAuditLogHandler`:

```cpp
logger.addFileHandler("/var/log/themisdb/audit.jsonl");
```

---

## Known Limitations

- `persisted_queries.h::QueryAllowList::enabled_` defaults to false, but `isAllowed()` does not consult the flag. Verify the production enforcement call site before treating it as a control or adding a warning.
- `api_gateway_hook.h` deprecated symbols (`hookId`, `registerHook`, `unregisterHook`, `getHooks`) are marked `[[deprecated]]` but remain in the ABI until the major-version removal decision is made.
- `grpc_bridge.h::IGRPCBridge` has no concrete implementation; all gRPC service registration routes through `GrpcApiServer::registerService()` until `GrpcBridgeImpl` is implemented.

---

## References

- Full security documentation: [`../../src/api/SECURITY.md`](../../src/api/SECURITY.md)
- Project security policy: [`../../SECURITY.md`](../../SECURITY.md)
- Audit report: [`AUDIT.md`](AUDIT.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
