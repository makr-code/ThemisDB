> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-05-11 -->
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
- `[x]` Build-conditional headers (`THEMIS_ENABLE_GRPC`, `THEMIS_ENABLE_WEBSOCKET`, `THEMIS_ENABLE_OTEL`) must not be included unconditionally by other headers.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `graphql::Parser::parse()` | `graphql.h` | GraphQL HTTP handler, tests | ✅ Stable |
| `graphql::Executor::execute()` | `graphql.h` | GraphQL HTTP handler, tests | ✅ Stable |
| `api::GrpcApiServer::initialize() / start() / stop()` | `grpc_server.h` | gRPC bootstrap | ✅ Stable |
| `api::ThemisDBServiceFactory::build()` | `themisdb_grpc_service_factory.h` | gRPC wiring | ✅ Stable |
| `api::WsChangeHandler::validate()` | `ws_handler.h` | WebSocket upgrade path | ✅ Stable |
| `api::TracingMiddleware::processRequest()` | `tracing_middleware.h` | HTTP middleware pipeline | ✅ Stable |
| `api::RateLimiter::allow()` | `rate_limiter.h` | All request handlers | ✅ Stable |
| `api::AuditLogger::log()` | `audit_logger.h` | Security event path | ✅ Stable |
| `api::OtlpExporter::enqueue()` | `otlp_exporter.h` | Tracing middleware | ✅ Stable |
| `api::IGRPCBridge::registerService()` | `grpc_bridge.h` | gRPC service registry | ⚠️ Interface only — no concrete impl |
| `api::QueryAllowList::isAllowed()` | `persisted_queries.h` | Query validation path | ⚠️ `enabled_=false` by default |

---

## Planned Header Enhancements

### 1. `graphql.h` — Fragment and Directive Support

**Priority:** Medium
**Target Version:** v2.1.0
**Tracks:** `src/api/FUTURE_ENHANCEMENTS.md` — GraphQL Schema Completion section

The `Parser` class documents "Not yet supported: Fragments, Directives, Inline fragments." Adding these requires new public types:

```cpp
// New types in graphql.h
struct FragmentDefinition {
    std::string name;
    std::string on_type;
    SelectionSet selection_set;
};

struct InlineFragment {
    std::string on_type;           // empty = applies to any type
    SelectionSet selection_set;
    std::vector<std::string> directives;
};
```

**Acceptance criteria:**
- `Parser::parse()` no longer rejects queries containing `fragment Foo on Bar { ... }` or `... on Bar { ... }`.
- `Document` carries a `fragments` map; inline fragments are embedded in `SelectionSet`.
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

`QueryAllowList` defaults to `enabled_ = false`. A startup warning must be emitted when the allow-list is disabled in a production build.

**Header change:**
```cpp
// In QueryAllowList:
/// Enable or disable allow-list enforcement.
/// @warning Production builds should call setEnabled(true) to prevent ad-hoc query injection.
///          A THEMIS_WARN is emitted at startup if disabled in a NDEBUG build.
void setEnabled(bool enabled);
[[nodiscard]] bool isEnabled() const noexcept;
```

---

### 5. `grpc_server.h` — `GrpcServerConfig::max_message_size_bytes` Runtime Knob

**Priority:** Low
**Target Version:** v2.1.0
**Tracks:** `src/api/FUTURE_ENHANCEMENTS.md` — gRPC API Surface section

`GrpcServerConfig::max_message_size_bytes` is currently a compile-time constant. Exposing it as a runtime configuration field allows operators to tune without recompilation.

**Header change:**
```cpp
struct GrpcServerConfig {
    // ...existing fields...
    /// Maximum gRPC message size in bytes. Default: 4 MiB. Set via config key grpc.max_message_size_mb.
    int max_message_size_bytes = 4 * 1024 * 1024;
};
```

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

- `[ ]` `QueryAllowList::enabled_ = false` by default; see enhancement #4 above for the production activation plan.
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
