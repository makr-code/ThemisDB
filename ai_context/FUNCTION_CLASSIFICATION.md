# Function Classification Framework

Datum: 2026-08-03  
**Status:** Active  
**Primary:** src/*/ROADMAP.md, include/**/*.h  
**Bezug:** API-Ebenen, Aufgaben, Performance-Kritikalität

## 1. API Levels (Visibility & Stability)

| Level | Scope | Stability | Use Case | Example |
|-------|-------|-----------|----------|---------|
| **Public** | include/<module>/<public>.h | Frozen (v1.x+) | External clients, SDKs | `themis::llm::createInferenceEngine()` |
| **Internal** | include/<module>/<internal>.h or src/**/*.h | Fluid (may change) | Cross-module within core | `themis::query::detail::estimateCost()` |
| **Private** | detail::, unnamed namespace | Very fluid | Single module only | `namespace detail { class Scanner {} }` |
| **Test** | tests/**/*.cpp or <module>::test:: | Volatile | Tests only, never prod | `MockIndexFactory` |

## 2. Function Task Classification

| Category | Task Type | Stability | Example |
|----------|-----------|-----------|---------|
| **Business Logic** | Core feature implementation | High | Search, insert, join operators |
| **Infrastructure** | Data flow (pipes, formats, buffering) | Medium | Serialization, streaming, buffering |
| **Plumbing** | Cross-cutting (logging, metrics, error codes) | Medium | `emitMetric()`, `logWarning()` |
| **Validation** | Input/precondition checks | High | `validateSchema()`, range checks |
| **Helper/Util** | Generic (not domain-specific) | High | String split, base64 encode |

**Usage:** Use during code review to identify task clarity and appropriate API level.

## 3. Performance Criticality (P0, P1, P2)

| Level | Frequency | SLA | Benchmark | Example |
|-------|-----------|-----|-----------|---------|
| **P0 (Hot)** | Per-query or per-request | ≤100 µs | GATE-*-0{1..6} | Index search, query parse |
| **P1 (Normal)** | Occasional (per-txn/per-batch) | ≤1 ms | Not gated; spot-checked | Transaction commit, schema update |
| **P2 (Cold)** | Rare (startup, shutdown, admin) | ≤1 s | Not gated | Init, graceful shutdown |

**Marking:** Benchmark names follow pattern `bench_<module>_release_gates.cpp` with `GATE-<MOD>-0{1..6}`.

Examples:
- `GATE-API-01`: GET parse & routing ≤5 µs
- `GATE-LLM-01`: Inference dispatch ≤100 µs
- `GATE-INDEX-01`: HNSW point search ≤10 µs

## 4. Error Handling Patterns

| Pattern | When | Example |
|---------|------|---------|
| **Return Result<T>** | Recoverable error, caller must handle | `Result<std::vector<V>> search(...)` |
| **Throw exception** | Truly exceptional (should never happen) | `IndexCorruptionException` |
| **noexcept + error code** | Performance-critical path, no errors | RAII destructors, rarely used otherwise |
| **Out-param + bool** | Legacy; avoid in new code | `bool getData(T& out)` |

**Guideline:** Public APIs return `Result<>`, internal can use exceptions for safety.

## 5. Concurrency Safety Classification

| Class | Guarantee | Locking | Example |
|-------|-----------|---------|---------|
| **Thread-Unsafe** | Single-threaded use only | None | Temporary local objects |
| **Thread-Safe (reads)** | Multiple readers, single writer | RW-lock or atomic snapshots | `Config` (immutable after init) |
| **Thread-Safe (full)** | Concurrent read/write | Mutex + guard per operation | `QueryCache::get()`, `ConcurrentHashMap` |
| **Lock-Free** | No locks, atomic operations | std::atomic<T> | Reference counters, flags |

**Document in Doxygen:** `@thread_safety` tag or comments.

```cpp
/// @class QueryCache
/// @thread_safety Multiple threads may call get() concurrently; write via put() serialized with internal mutex.
class QueryCache { /* ... */ };
```

## 6. Preconditions & Postconditions (Contract)

**Pattern:** Doxygen comments + assertions

```cpp
/// Searches the index for a key.
/// @param key Non-empty key to search (precondition: must be ASCII-safe)
/// @return Found value if key exists, empty optional otherwise
/// @throw IndexCorruptedException if internal invariant violated
/// @post Returned value is valid and immutable
std::optional<Value> search(std::string_view key) const;
```

**Validation strategy:**
- **Preconditions:** Checked via `assert()` in debug; unchecked assumptions in release (caller responsibility)
- **Postconditions:** Verified in tests, not runtime-checked
- **Invariants:** Checked internally, maintain via RAII

## 7. Timeout & Resource Limits

**For network/storage/inference operations:**

- **Default timeout:** Module-specific (see PERFORMANCE_EXPECTATIONS.md)
- **Cancellation:** Via `CancellationToken` or `std::stop_token` (C++20)
- **Resource caps:** Memory limits, connection limits, queue sizes

**Example:**
```cpp
Result<Embedding> embed(std::string_view text,
                        const EmbedOptions& opts,
                        CancellationToken cancel) noexcept;
// opts.timeout_ms: defaults to 30000, overridable
// cancel: allows early termination
```

## 8. Versioning & Deprecation

**Public API versioning:**
- Breaking changes require major version bump
- Deprecations marked with C++20 `[[deprecated]]` + Doxygen `@deprecated`
- Deprecation window: minimum 1 major version (e.g., 2.x → 3.x)

```cpp
/// @deprecated Use embed() instead. Removed in ThemisDB 3.0.
[[deprecated("Use embed() instead")]]
Result<Embedding> legacyEmbed(std::string_view text);
```

## 9. Const-Correctness Levels

| Level | Methods marked | Use |
|-------|-----------------|-----|
| **Full** | All non-mutating as const | Immutable-by-default classes (Config, Model) |
| **Partial** | Only top-level getters const | Mutable state (Cache, Repository) |
| **None** | No const marking | Rare; legacy code |

**New code:** Use Full.

## 10. Side Effects Classification

| Type | Allowed | Example |
|------|---------|---------|
| **Functional (pure)** | Yes | `hash(x)`, `validate(y)` |
| **I/O (file/net)** | Only in dedicated handlers | `readFile()`, `postMetric()` |
| **State mutation** | Guarded by mutex if shared | `cache.put(k, v)` |
| **Observable (metrics/logs)** | Document in Doxygen | `emitMetric("query.count", 1)` |

---

**Zuletzt geprueft (Function Classification):** 2026-08-03
