# Memory Management Policy — Core Rules

**Datum:** 2026-08-03  
**Status:** Active, Enforced  
**Primary:** src/base/resource.h, include/base/*.h  
**Bezug:** RAII, smart pointers, ownership contracts

## 🎯 Five Core Rules

### 1. RAII First
Acquire resources in constructor, release in destructor. Use `std::unique_ptr<T>` for exclusive ownership, `std::shared_ptr<T>` only when multiple owners are required. **Never** use raw `new`/`delete` unless profiling proves necessity + code review approval.

### 2. Ownership Model
- **Unique** (`std::unique_ptr<T>`): single owner, transfers on move
- **Shared** (`std::shared_ptr<T>`): reference-counted, multiple owners
- **Borrowed** (raw `T*` or `T&`): temporary, caller retains ownership (document in Doxygen)

### 3. Public API: No Raw Owning Pointers
Public headers must return `std::unique_ptr`, `std::shared_ptr`, references, or values. Raw owning pointers are internal-only.

**Good:**
```cpp
namespace themis::index {
  std::unique_ptr<IIndex> createIndex(...);  // clear ownership xfer
  void registerIndex(std::string_view name, IIndex* borrowed);  // doc: borrowed
}
```

### 4. Scope-Based Cleanup
Use `std::lock_guard`, `std::scoped_lock`, RAII wrappers for file handles/memory blocks. Exceptions trigger destructors → guaranteed cleanup.

### 5. Thread-Safety by Design
- `std::atomic<T>` for lock-free
- `std::mutex` + guards for shared mutable state
- No global mutable variables (use singletons + explicit locking)

## Approved Exception: Raw `new`/`delete`

**Only if ALL conditions met:**
1. Performance profiling shows ≥ 5% measurable improvement vs. std::make_unique
2. Code review sign-off by module owner
3. Wrapped in RAII class immediately (never exposed raw)
4. Documented in nearby comment with date + evidence

```cpp
// PROFILED: 3% overhead for 1M allocs. Approved @owner (2025-01-15).
std::unique_ptr<T[]> allocate(size_t sz) {
  return std::unique_ptr<T[]>(new T[sz]);  // wrapped
}
```

## Containers & Concurrency

| Pattern | Rule |
|---------|------|
| `std::vector<T>` | Default; use `std::make_unique` for heap allocation |
| `std::lock_guard<std::mutex>` | Automatic unlock on scope exit (preferred) |
| `std::scoped_lock` | Multi-mutex (deadlock-safe) |
| Exceptions | Strong safety: no state modification if throw |
| Destructors | Never throw; use noexcept if possible |

## Lifetime Checklist

| Issue | Pattern | Fix |
|-------|---------|-----|
| Dangling pointer | `T* p = &local; return p;` | unique_ptr or shared_ptr |
| Use-after-free | Manual delete, then access | Smart pointers, no manual cleanup |
| Double-delete | Multiple delete | unique_ptr ensures single owner |
| Memory leak | new but never delete | unique_ptr or shared_ptr |
| Race | Unsync'd shared state | std::mutex + guard or std::atomic |

## Further Reading

- Full guidance: [src/base/resource.h](../src/base/resource.h), [PERFORMANCE_EXPECTATIONS](../src/PERFORMANCE_EXPECTATIONS.md)
- Approved singletons (immutable config, append-only registries) documented separately

---

**Zuletzt geprueft (Memory Policy):** 2026-08-03
