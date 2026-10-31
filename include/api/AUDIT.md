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

> ⚠️ **Header Audit** — validate findings against actual header files. Mark resolved items as `[x]`.

<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/api/AUDIT.md -->

# Audit Report — API Module Public Headers

**Header inventory checked:** 2026-10-31
**Last detailed audit:** 2026-05-11
**Auditor:** Copilot
**Status:** Review required; detailed findings below have not been fully revalidated.

The current inventory is source-checked. Earlier compliance and finding statements remain
historical until a full header audit is repeated.

For the full source-level audit (implementation files, test coverage, build system) see:

→ [`../../src/api/AUDIT.md`](../../src/api/AUDIT.md)

---

## Summary

| Metric | Result |
|--------|--------|
| Total public headers | 28 |
| `#pragma once` coverage | ✅ 28/28 |
| `[[nodiscard]]` on factory/error methods | Not revalidated in this inventory pass |
| Build-conditional headers | `THEMIS_ENABLE_GRPC`, `THEMIS_ENABLE_WEBSOCKET`; Prometheus integration uses `THEMIS_HAS_PROMETHEUS` |
| Deprecated symbols documented | ⚠️ 4 symbols in `api_gateway_hook.h` (historical finding; revalidate) |
| Inline implementation | Present in several headers; see `ARCHITECTURE.md` |
| Open stubs / pure-virtual with no impl | ⚠️ `IGRPCBridge` — no concrete implementation |

---

## Headers Audited

| Header | Status | Notes |
|--------|--------|-------|
| `graphql.h` | ✅ | `Value::VariableRef` added v2.0.0; `Parser::error()` marked `@deprecated` |
| `graphql_cache.h` | ✅ | `ResponseCache::invalidatePattern()` uses selective eviction |
| `graphql_metrics.h` | ✅ | |
| `graphql_schema_builder.h` | ✅ | |
| `graphql_aql_resolver.h` | ✅ | |
| `graphql_ws_handler.h` | ✅ | `alive_` flag for CDC callback lifetime safety |
| `grpc_server.h` | ✅ | Requires `THEMIS_ENABLE_GRPC` |
| `grpc_bridge.h` | ⚠️ | Pure-virtual `IGRPCBridge`; no concrete implementation in codebase |
| `themisdb_grpc_service.h` | ✅ | Requires `THEMIS_ENABLE_GRPC` |
| `themisdb_grpc_service_factory.h` | ✅ | `ThemisDBGrpcServiceFactory` |
| `http_handler.h` | ✅ | |
| `websocket_handler.h` | ✅ | |
| `ws_handler.h` | ✅ | Requires `THEMIS_ENABLE_WEBSOCKET` |
| `subscription_multiplexer.h` | ✅ | |
| `tracing_middleware.h` | ✅ | |
| `correlation_id.h` | ✅ | |
| `rate_limiter.h` | ✅ | `std::shared_mutex` for read path; stale-bucket TTL eviction added |
| `graphql_audit_logger.h` | ✅ | `themis::graphql` audit logger and file handler |
| `otlp_exporter.h` | ✅ | Persistent `CURL*` handle; `std::deque` queue; Prometheus counters |
| `persisted_queries.h` | ⚠️ | `QueryAllowList::enabled_ = false` by default; no production startup warning |
| `geo_index_hooks.h` | ✅ | |
| `api_gateway_hook.h` | ⚠️ | 4 deprecated symbols (see FINDING-HDR-001) |
| `api_version_router.h` | ✅ | |
| `aql_utils.h` | ✅ | `aqlEscapeLiteral()`, `isValidAqlIdentifier()` — added v1.9.1 |
| `federation_admin_handler.h` | ✅ | |
| `api_transport_contracts.h` | ✅ | Transport versions, capabilities, failure classes, and validator |
| `api_transport_policy.h` | ✅ | Policy middleware with bounded payload/path configuration |
| `api_error_taxonomy.h` | ✅ | Transport failure to error/status mapping |

---

## Findings

### FINDING-HDR-001: `api_gateway_hook.h` — Deprecated Symbols Without External Callers

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | Open — tracked for removal |
| **Headers** | `api_gateway_hook.h` |
| **Symbols** | `IAPIGatewayHook::hookId()`, `IGatewayHookRegistry::registerHook()`, `IGatewayHookRegistry::unregisterHook()`, `IGatewayHookRegistry::getHooks()` |
| **Evidence** | Symbols annotated `[[deprecated]]` and `@deprecated No external callers confirmed. CANDIDATE_FOR_REMOVAL` |
| **Impact** | Virtual methods on a pure-virtual interface; cannot be removed without breaking ABI |
| **Action** | Resolve CANDIDATE_FOR_REMOVAL decision: (1) confirm no callers and remove in next major release, or (2) wire an internal caller and remove the deprecation notice |

---

### FINDING-HDR-002: `grpc_bridge.h` — `IGRPCBridge` Has No Concrete Implementation

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | Open — tracked in `FUTURE_ENHANCEMENTS.md` |
| **Header** | `grpc_bridge.h` |
| **Evidence** | Pure-virtual interface with no implementing class in `src/api/` |
| **Action** | Implement `GrpcBridgeImpl` (see `FUTURE_ENHANCEMENTS.md` §3) |

---

### FINDING-HDR-003: `persisted_queries.h` — `QueryAllowList` Disabled by Default

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | Open |
| **Header** | `persisted_queries.h` |
| **Evidence** | `QueryAllowList` constructor sets `enabled_ = false`; no startup warning emitted in NDEBUG builds |
| **Action** | Verify request-enforcement integration before describing the enabled flag as a production control (see `FUTURE_ENHANCEMENTS.md` §4) |

---

### FINDING-HDR-004: `graphql.h` — `Parser::error()` Deprecated but Not Yet Removed

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | Open |
| **Header** | `graphql.h` |
| **Evidence** | Method annotated `@deprecated ("Use Result<T> return types instead of error() method")` |
| **Action** | Migrate remaining call sites in `graphql.cpp` to `Result<T>`; remove method in v2.1.0 |

---

## Resolved Findings

| ID | Description | Resolved In |
|----|-------------|------------|
| HDR-R001 | `RateLimiter::allow()` nested lock contention (`OperationRateLimiter` outer mutex + `RateLimiter` inner mutex) | `rate_limiter.h` — `std::shared_mutex` migration |
| HDR-R002 | `AuditLogger::log()` held mutex during handler callbacks | `graphql_audit_logger.h` — copy-under-lock, invoke outside critical section |
| HDR-R003 | `ResponseCache::invalidatePattern()` always cleared entire cache | `graphql_cache.h` — selective eviction by collection tag |
| HDR-R004 | `OtlpExporter` opened new CURL handle per flush | `otlp_exporter.h` / `otlp_exporter.cpp` — persistent handle |
| HDR-R005 | `GraphQLWsHandler` raw `self` pointer in CDC callback | `graphql_ws_handler.h` — `alive_` atomic flag |

---

## Compliance

- All headers use `#pragma once` — no legacy include-guard macros.
- The `[[nodiscard]]` and secret/PII checks from the 2026-05 audit have not been repeated.
- The current source guards gRPC and WebSocket header surfaces with `THEMIS_ENABLE_GRPC` and `THEMIS_ENABLE_WEBSOCKET`; `otlp_exporter.h` directly includes libcurl and is not guarded by `THEMIS_ENABLE_OTEL`.

## Revalidation TODO

- [ ] Re-run the detailed audit of all 28 headers, including `[[nodiscard]]`, build guards, ABI risks, and the four findings above.
- [ ] Confirm which deprecated gateway-hook symbols still have no external callers before making removal claims.

---

## References

- Source-level audit: [`../../src/api/AUDIT.md`](../../src/api/AUDIT.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
