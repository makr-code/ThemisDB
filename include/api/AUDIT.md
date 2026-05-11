> ⚠️ **Header Audit** — validate findings against actual header files. Mark resolved items as `[x]`.

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/api/AUDIT.md -->

# Audit Report — API Module Public Headers

**Last Audit:** 2026-05-11
**Auditor:** Copilot
**Status:** ✅ Pass (with open items)

For the full source-level audit (implementation files, test coverage, build system) see:

→ [`../../src/api/AUDIT.md`](../../src/api/AUDIT.md)

---

## Summary

| Metric | Result |
|--------|--------|
| Total public headers | 24 |
| `#pragma once` coverage | ✅ 24/24 |
| `[[nodiscard]]` on factory/error methods | ✅ All confirmed |
| Build-conditional headers properly guarded | ✅ `THEMIS_ENABLE_GRPC`, `THEMIS_ENABLE_WEBSOCKET`, `THEMIS_ENABLE_OTEL` |
| Deprecated symbols documented | ⚠️ 4 symbols in `api_gateway_hook.h` (see below) |
| No implementation code in headers | ✅ (constexpr / template bodies only) |
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
| `themisdb_grpc_service_factory.h` | ✅ | |
| `http_handler.h` | ✅ | |
| `websocket_handler.h` | ✅ | |
| `ws_handler.h` | ✅ | Requires `THEMIS_ENABLE_WEBSOCKET` |
| `subscription_multiplexer.h` | ✅ | |
| `tracing_middleware.h` | ✅ | |
| `correlation_id.h` | ✅ | |
| `rate_limiter.h` | ✅ | `std::shared_mutex` for read path; stale-bucket TTL eviction added |
| `audit_logger.h` | ✅ | Non-blocking handler dispatch; `FileAuditLogHandler` added |
| `otlp_exporter.h` | ✅ | Persistent `CURL*` handle; `std::deque` queue; Prometheus counters |
| `persisted_queries.h` | ⚠️ | `QueryAllowList::enabled_ = false` by default; no production startup warning |
| `geo_index_hooks.h` | ✅ | |
| `api_gateway_hook.h` | ⚠️ | 4 deprecated symbols (see FINDING-HDR-001) |
| `api_version_router.h` | ✅ | |
| `aql_utils.h` | ✅ | `aqlEscapeLiteral()`, `isValidAqlIdentifier()` — added v1.9.1 |
| `federation_admin_handler.h` | ✅ | |

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
| **Action** | Add startup warning; document activation path in operations runbook (see `FUTURE_ENHANCEMENTS.md` §4) |

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
| HDR-R002 | `AuditLogger::log()` held mutex during handler callbacks | `audit_logger.h` — copy-under-lock, invoke outside critical section |
| HDR-R003 | `ResponseCache::invalidatePattern()` always cleared entire cache | `graphql_cache.h` — selective eviction by collection tag |
| HDR-R004 | `OtlpExporter` opened new CURL handle per flush | `otlp_exporter.h` / `otlp_exporter.cpp` — persistent handle |
| HDR-R005 | `GraphQLWsHandler` raw `self` pointer in CDC callback | `graphql_ws_handler.h` — `alive_` atomic flag |

---

## Compliance

- All headers use `#pragma once` — no legacy include-guard macros.
- `[[nodiscard]]` applied to all factory functions and `Result<T>`-returning methods.
- Build-conditional headers (`grpc_server.h`, `themisdb_grpc_service.h`, `ws_handler.h`, `otlp_exporter.h`) are not included unconditionally.
- No PII or secrets in header files.

---

## References

- Source-level audit: [`../../src/api/AUDIT.md`](../../src/api/AUDIT.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
