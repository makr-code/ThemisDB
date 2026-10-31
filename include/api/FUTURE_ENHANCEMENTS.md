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

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/api/FUTURE_ENHANCEMENTS.md -->

# API Module — Public Header Future Enhancements

**Module Path:** `include/api/`
**Canonical implementation enhancements:** [`../../src/api/FUTURE_ENHANCEMENTS.md`](../../src/api/FUTURE_ENHANCEMENTS.md)

---

## Scope

This document covers planned enhancements to the **public header contract** in `include/api/` — new types, interface additions, deprecation removals, and header-level API improvements. Enhancements that touch both headers and implementation files are tracked primarily in the canonical source-level document:

→ [`../../src/api/FUTURE_ENHANCEMENTS.md`](../../src/api/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Headers must remain backward-compatible within a major version; new capabilities are added via new methods or versioned types, not by modifying existing signatures.
- `[x]` `#pragma once` guard required on every header; no include-guard macros.
- `[x]` No implementation code in headers (exception: `constexpr` helpers, template bodies, and header-only utilities explicitly documented as such).
- `[x]` All factory functions and error-returning methods must be `[[nodiscard]]`.
- `[x]` gRPC and WebSocket header surfaces use `THEMIS_ENABLE_GRPC` and `THEMIS_ENABLE_WEBSOCKET`. `otlp_exporter.h` has no `THEMIS_ENABLE_OTEL` guard and includes libcurl directly.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `graphql::Parser::parse()` | `graphql.h` | GraphQL HTTP handler, tests | ✅ Stable |
| `graphql::Executor::execute()` | `graphql.h` | GraphQL HTTP handler, tests | ✅ Stable |
| `api::GrpcApiServer::initialize() / start() / stop()` | `grpc_server.h` | gRPC bootstrap | ✅ Stable |
| `api::ThemisDBGrpcServiceFactory::build()` | `themisdb_grpc_service_factory.h` | gRPC wiring | ✅ Stable |
| `api::WsChangeHandler::validate()` | `ws_handler.h` | WebSocket upgrade path | ✅ Stable |
| `api::TracingMiddleware::processRequest()` | `tracing_middleware.h` | HTTP middleware pipeline | ✅ Stable |
| `graphql::RateLimiter::allow()` | `rate_limiter.h` | Caller-selected request paths | ✅ Stable |
| `graphql::AuditLogger::log()` | `graphql_audit_logger.h` | Security event path | ✅ Stable |
| `api::OtlpExporter::enqueue()` | `otlp_exporter.h` | Tracing middleware | ✅ Stable |
| `api::IGRPCBridge::registerService()` | `grpc_bridge.h` | gRPC service registry | ⚠️ Interface only — no concrete impl |
| `graphql::QueryAllowList::isAllowed()` | `persisted_queries.h` | Caller-owned query validation | ⚠️ Membership check; `enabled_` is separate state |

---

## Planned Header Enhancements

### 1. `graphql.h` — Remaining Fragment and Directive Syntax

**Priority:** Medium
**Target Version:** v2.1.0
**Tracks:** `src/api/FUTURE_ENHANCEMENTS.md` — GraphQL Schema Completion section

The current parser already reads fragment definitions and expands named fragment spreads that
appear after their definition. It also parses the field directives `@include` and `@skip`.
Inline fragments and other directives are rejected. Any extension must preserve the existing
`FragmentDefinition` and `Field::Directive` representations.

```cpp
// Potential representation for the remaining inline-fragment syntax.
struct InlineFragment {
    std::string on_type;           // empty = applies to any type
    SelectionSet selection_set;
    std::vector<std::string> directives;
};
```

TODO: Confirm the required type-condition and directive semantics before finalizing a public AST
contract; the current source does not establish those semantics.

**Acceptance criteria:**
- `Parser::parse()` continues to accept named fragments and `@include`/`@skip`, and supports inline fragments under an explicitly defined contract.
- `Document` preserves fragment definitions and execution behavior without changing existing named-fragment handling.
- Existing callers that do not use fragments are unaffected.

---

### 2. `graphql.h` — Remove Deprecated `Parser::error()`

**Priority:** Low
**Target Version:** v2.1.0
**Tracks:** `src/api/FUTURE_ENHANCEMENTS.md` — GraphQL Schema Completion section

`Parser::error()` is marked `@deprecated` in favour of `Result<T>` return types. Once all internal call sites in `graphql.cpp` are migrated, the method will be removed from the public header.

**Migration path for consumers:**
```cpp
// Before (deprecated path)
auto doc = parser.parse(query, limits);
if (!parser.error().empty()) { /* handle */ }

// After (Result<T> path)
auto result = parser.parse(query, limits);
if (!result) { auto err = result.error(); /* handle */ }
```

---

### 3. `grpc_bridge.h` — Concrete `GrpcBridgeImpl`

**Priority:** Low
**Target Version:** v2.1.0
**Tracks:** `src/api/FUTURE_ENHANCEMENTS.md` — gRPC Bridge Interface section

`IGRPCBridge` is a pure-virtual interface with no concrete implementation in the current codebase. A `GrpcBridgeImpl` class will be added with:

```cpp
// New class in grpc_bridge.h (or a new grpc_bridge_impl.h)
class GrpcBridgeImpl : public IGRPCBridge {
public:
    bool registerService(ServiceDescriptor desc) override;
    bool route(const std::string& method, GRPCRequest& req) override;
    GRPCMetadata getMetadata(const std::string& service_name) const override;
    std::vector<ServiceDescriptor> listServices() const override;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, ServiceDescriptor> services_;
};
```

---

### 4. `persisted_queries.h` — Production `QueryAllowList` Activation

**Priority:** Medium
**Target Version:** v2.1.0
**Tracks:** `src/api/FUTURE_ENHANCEMENTS.md` — Security/Reliability section

`QueryAllowList` defaults to `enabled_ = false`. The current `isAllowed()` method checks set
membership independently of that flag, and no public method couples the flag to enforcement.
Define and verify the production call-site contract before adding a startup warning or describing
the flag as a security control.

**Header change:**
```cpp
// In QueryAllowList:
/// Set the allow-list enabled-state flag.
/// TODO: Define how this flag participates in request enforcement before documenting it as a control.
void setEnabled(bool enabled);
[[nodiscard]] bool isEnabled() const noexcept;
```

---

### 5. `grpc_server.h` — Runtime Message-Size Configuration (Implemented)

**Priority:** Low
**Target Version:** v2.1.0
**Tracks:** `src/api/FUTURE_ENHANCEMENTS.md` — gRPC API Surface section

`GrpcServerConfig::max_message_size_bytes` is already a runtime field with a 100 MiB default.
`GrpcApiServer` reads `grpc.max_message_size_mb` from `config/networking/grpc.yaml`; invalid
values are normalized by the implementation. No additional public-header field is pending.

---

## Test Strategy

| Test Type | Target | Notes |
|-----------|--------|-------|
| Compile-time | All headers compile in isolation with each build-conditional combination | CMake `check_headers` target |
| Unit | `graphql::Parser` fragment/directive parsing | `tests/test_graphql_fragments.cpp` (planned) |
| Unit | `GrpcBridgeImpl` registration and routing | `tests/test_grpc_bridge.cpp` (planned) |
| Unit | `QueryAllowList` production startup warning | `tests/test_persisted_queries.cpp` extension |
| ABI | No unexpected virtual table changes between patch releases | ABI checker in CI |

---

## Performance Targets

| Enhancement | Overhead Budget | Measurement |
|-------------|----------------|-------------|
| Fragment parsing in `Parser::parse()` | ≤ 10% additional parse time vs. flat query | `BM_GraphQL_Parse_Complex_Uncached` |
| `GrpcBridgeImpl::route()` | ≤ 500 ns per call (hash map lookup under shared lock) | Dedicated microbenchmark |
| `QueryAllowList::isAllowed()` (enabled) | ≤ 200 ns per call (hash map lookup) | `BM_PersistedQuery_AllowList` |

---

## Security / Reliability

- `[!]` `QueryAllowList::enabled_` defaults to false, but `isAllowed()` does not consult it; establish and verify a production enforcement call site before treating it as a security control.
- `[x]` `RateLimiter::allow()` uses `std::shared_mutex` (shared for reads, exclusive for writes) — no nested lock contention.
- `[x]` `AuditLogger::log()` copies handlers under lock and invokes them outside the critical section.
- `[x]` `GrpcApiServer::start()` releases mutex before `BuildAndStart()`; `stop()` applies a 30-second deadline.

---

## References

- Canonical implementation enhancements: [`../../src/api/FUTURE_ENHANCEMENTS.md`](../../src/api/FUTURE_ENHANCEMENTS.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Security: [`SECURITY.md`](SECURITY.md)
- Module overview: [`README.md`](README.md)
