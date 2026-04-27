---
description: "Use when editing C/C++ files in this repository; apply modern C++ safety, RAII, threading, and performance best practices."
applyTo: "**/*.{c,cc,cpp,cxx,h,hh,hpp,hxx,ipp,tpp}"
---

# C++ Best Practices

Apply the following guidance for C++ generation, review, and refactoring in this repository.

```yaml
cpp_best_practices:
  modern_cpp_features:
    - "Use 'auto' for type inference to improve readability."
    - "Prefer 'nullptr' over NULL or 0."
    - "Use 'constexpr' for compile-time computations."
    - "Use range-based for loops for container iteration."
    - "Prefer smart pointers (std::unique_ptr, std::shared_ptr) over raw pointers."

  cpp_attributes:
    - "Use [[nodiscard]] on all functions whose return value must not be silently ignored: Result<T>, bool success flags, factory methods returning pointers/smart-pointers, getters used for decisions, and any function where ignoring the return value is a bug."
    - "Use [[deprecated(\"reason\")]] on any function, type, or variable that is superseded or should no longer be used. Always include a migration hint in the reason string (e.g., 'Use foo() instead')."
    - "Use [[maybe_unused]] to suppress unused-variable/-parameter warnings for intentionally unused entities (e.g., platform-specific branches, callback parameters)."
    - "Use [[noreturn]] on functions that unconditionally throw, call std::terminate(), or std::abort(). Never use [[noreturn]] on functions that may return."
    - "Use [[likely]] / [[unlikely]] on branch conditions that are performance-critical and statistically skewed (e.g., fast-path cache hits, error branches)."
    - "Use [[fallthrough]] in switch-case blocks where intentional fall-through is required; omitting it triggers -Wimplicit-fallthrough warnings."
    - "Apply [[nodiscard]] at the interface level (virtual declarations) so overrides inherit the warning automatically."
    - "Pair [[deprecated]] with a Doxygen @deprecated tag for documentation-tool visibility."

  resource_management_raii:
    - "Use RAII to bind resource lifetime to object lifetime."
    - "Use std::lock_guard or std::unique_lock for mutex locking."
    - "Avoid manual new/delete; prefer smart pointers."
    - "Ensure resources are released automatically when objects go out of scope."

  avoid_unnecessary_copies:
    - "Pass large objects by const reference (const &)."
    - "Use move semantics (std::move) for efficient transfer of ownership."
    - "Implement copy and move constructors appropriately."

  clear_and_safe_interfaces:
    - "Mark member functions that do not modify state as 'const'."
    - "Document side effects and exceptions clearly."
    - "Avoid global variables and mutable shared state."

  threading_and_synchronization:
    - "Use std::mutex with std::lock_guard for critical sections."
    - "Keep critical sections as short as possible."
    - "Use atomic operations (std::atomic) when feasible."
    - "Avoid deadlocks by consistent lock ordering or using std::lock."

  error_handling:
    - "Use exceptions for errors that cannot be handled locally."
    - "Write exception-safe code."
    - "Avoid silent failures and undefined behavior."

  performance:
    - "Profile code before optimizing."
    - "Avoid premature optimization."
    - "Use cache-friendly data structures and algorithms."
    - "Avoid false sharing in multithreaded code."

  style_and_readability:
    - "Use meaningful and descriptive names."
    - "Keep functions short and focused."
    - "Follow a consistent coding style guide."

  copilot_guidance:
    - "When generating C++ code, use modern language features like 'auto', smart pointers, and 'constexpr'."
    - "Avoid manual memory management; prefer RAII."
    - "Synchronize threads with std::mutex and std::lock_guard; keep critical sections short."
    - "Prevent deadlocks by consistent locking order."
    - "Write clear, const-correct, exception-safe functions."
    - "Optimize only after profiling and consider cache friendliness."
    - "If stubs, mocks, or simulation paths are introduced in source code, document them explicitly (purpose, activation conditions, production delta, and removal plan)."
    - "Apply [[nodiscard]], [[deprecated]], [[maybe_unused]], [[noreturn]], [[likely]], [[unlikely]], and [[fallthrough]] consistently per the cpp_attributes rules above."
```

  Use this comment template directly above the stub/mock/simulation code path:

  ```cpp
  // STUB/SIMULATION NOTE:
  // Purpose: <why this non-production path exists>
  // Activation: <build flag/runtime condition/test-only gate>
  // Production Delta: <how behavior differs from production>
  // Removal Plan: <when/how this path will be removed>
  ```

## C++ Attribute Quick Reference

| Attribute | When to use |
|---|---|
| `[[nodiscard]]` | All functions returning `Result<T>`, `bool` (success flag), factory pointers, getters used in decisions |
| `[[deprecated("use X instead")]]` | Superseded APIs, research-only code not used in production; always pair with `@deprecated` Doxygen tag |
| `[[maybe_unused]]` | Intentionally unused parameters/variables (platform guards, callbacks, debug-only vars) |
| `[[noreturn]]` | Functions that always throw, call `std::terminate()`, or `std::abort()` |
| `[[likely]]` / `[[unlikely]]` | Performance-critical branches with strong statistical bias (cache hit paths, error branches) |
| `[[fallthrough]]` | Intentional fall-through in `switch` cases (required to silence `-Wimplicit-fallthrough`) |
