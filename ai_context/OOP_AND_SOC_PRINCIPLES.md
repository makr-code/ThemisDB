# OOP & Separation of Concerns Principles

**Datum:** 2026-08-03  
**Status:** Active  
**Primary:** include/**/*.h, src/*/ARCHITECTURE.md  
**Bezug:** Design patterns, interfaces, module boundaries

## 1. Ownership & RAII (See memory_management_policy.md)

- Bind resource lifetime to object lifetime via destructors
- Use `std::unique_ptr<T>` for exclusive ownership
- Return `std::unique_ptr` / `std::shared_ptr` from factories, never raw owning pointers

## 2. Abstract Interfaces (SOC Boundary)

**Pattern:** Each public subsystem exports pure-virtual interface in include/<module>/<iface>.h

```cpp
namespace themis::index {
  class IIndex {
   public:
    virtual ~IIndex() = default;
    virtual bool insert(const Key& k, const Value& v) = 0;
    virtual bool search(const Key& k, Value& out) = 0;
  };

  // Factory in include/index/index.h (or _factory.h)
  std::unique_ptr<IIndex> createIndex(const IndexSpec&);
}
```

**Benefit:** Implementation swappable, testing mockable, no circular deps.

## 3. Template & Concept Constraints (C++20)

**Preferred:** Use `requires` clauses & concepts over SFINAE

```cpp
// Good: C++20 concepts
template <typename T>
requires std::ranges::range<T> && std::equality_comparable<T::value_type>
class Container { /* ... */ };

// Avoid: SFINAE (old pattern, harder to read)
template <typename T, typename = std::enable_if_t<...>>
class Container { /* ... */ };
```

**Common concepts in ThemisDB:**
- `Serializable`: `serialize()/deserialize()`
- `Comparable`: `operator<`, `operator==`
- `ThreadSafe`: `lock()` / `unlock()` available
- `Copyable`/`Movable`: as per standard

## 4. Adapter Pattern (Cross-Module Bridges)

**Pattern:** Module A → Bridge → Module B API

Used in:
- `aql/aql_model_router.cpp` bridges AQL → LLM
- `api/api_version_router.h` bridges HTTP → internal query executor
- `chimera/plugin_interface.h` bridges external plugins → core

**Structure:**
```cpp
namespace themis::aql {
  // bridge in impl file (src/aql/llm_aql_bridge.cpp)
  class LLMAQLBridge {
    const llm::IModelRouter& router_;  // borrowed ref to llm subsystem
   public:
    // Bridge method: AQL types → LLM types
    std::vector<llm::Embedding> queryLLM(...);
  };
}
```

## 5. Const-Correctness

- Mark methods `const` if they don't modify state
- Use `const T&` for parameters (avoid copies)
- Use `const_cast` only as last resort + document why

```cpp
class Index {
  int size() const { return entries_.size(); }  // doesn't modify
  void insert(const Key& k) { entries_[k] = ...; }  // modifies
};
```

## 6. Module Boundaries (SOC)

**Rule:** One module = one primary concern + well-defined API surface.

| Layer | Module Group | Boundary |
|-------|--------------|----------|
| **Transport** | server, api, network | HTTP/gRPC/WebSocket only |
| **Query** | aql, query, execution | SQL/AQL → execution plans |
| **Indexing** | index, cache, search, retrieval | All queries use index APIs |
| **Storage** | storage, sharding, replication | All persists go through storage |
| **AI/ML** | llm, ai, training, rag | LLM calls via unified interface |

**Crossing boundaries:** Use adapters/bridges, never internal details.

## 7. Error Handling (Result-Based)

**Preferred:** `std::expected<T, E>` or custom `Result<T, Err>`

```cpp
// Good: explicit error type
namespace themis::index {
  Result<std::vector<Value>, IndexError> search(const Key& k);
}

// OK: exceptions for truly exceptional cases (rare)
void setupDBConnection() noexcept(false);  // may throw
```

**In new code:** Use Result types, avoids hidden error propagation.

## 8. Copy/Move Semantics

**Default:** = delete copy, allow move (for ownership types)

```cpp
class UniqueResource {
 public:
  UniqueResource(const UniqueResource&) = delete;  // no copy
  UniqueResource(UniqueResource&&) noexcept = default;  // move
  UniqueResource& operator=(UniqueResource&&) noexcept = default;
};
```

**Shared/Value types:** Default copy/move OK

```cpp
struct Point { double x, y; };  // value type, copy freely
```

## 9. Constructor Patterns

| Pattern | Use Case | Example |
|---------|----------|---------|
| **Default** | Stateless, or safe zero-init | `Vec<T>{}` |
| **Explicit single** | Single mandatory param | `Index(IndexSpec)` |
| **Builder** | Complex setup, optional fields | `IndexBuilder().setName(...).build()` |
| **Factory** | Conditional creation, error handling | `createIndex(spec)` → `Result<>` |

## 10. Singleton Anti-Pattern Prevention

**Allowed ONLY for:**
1. Immutable configuration (read-only after init)
2. Append-only registries (guarded by mutex)

**Example (good):**
```cpp
class Config {
  static Config& instance() {
    static Config cfg;  // Meyer's singleton, thread-safe C++11
    return cfg;
  }
 private:
  Config() = default;
  // immutable after ctor
};
```

**NOT allowed:** Mutable global state (data mutations).

## 11. Dependency Injection (Testing)

**Pattern:** Pass dependencies as constructor params, enable mocking

```cpp
namespace themis::query {
  class Executor {
    const index::IIndex& idx_;  // injected (borrowed)
   public:
    explicit Executor(const index::IIndex& idx) : idx_(idx) {}
  };
}

// Test:
MockIndex mock;
Executor exec(mock);
```

## 12. Plugin Architecture (Chimera Adapter)

**Rule:** Plugins = dynamic libraries loaded via Chimera interface

- Plugins MUST use stable C-compatible interfaces in `include/chimera/`
- Plugins MUST NOT use ThemisDB internal headers directly
- Plugin lifecycle managed by `chimera::PluginManager`

See [src/chimera/ARCHITECTURE.md](../src/chimera/ARCHITECTURE.md) for details.

---

**Zuletzt geprueft (OOP/SOC):** 2026-08-03
