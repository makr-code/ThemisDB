## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


# ThemisDB Coding Standards

## Overview
This document establishes coding standards for ThemisDB to ensure consistency, maintainability, and code quality across the project.

**Version:** 1.0  
**Date:** 2026-01-03  
**Status:** Active

---

## C++ Language Standards

### Language Version
- **Required:** C++20
- Use modern C++ features appropriately
- Avoid deprecated features from older standards

### Header Files
- Use `#pragma once` instead of include guards
- Include order: Project headers → Third-party → Standard library
- Forward declarations preferred when possible

---

## Memory Management

### Smart Pointers (Mandatory)
```cpp
// ✅ Preferred: RAII with smart pointers
auto resource = std::make_unique<Resource>();
std::shared_ptr<Service> service = std::make_shared<Service>();

// ❌ Avoid: Manual memory management
Resource* resource = new Resource();  // Avoid unless necessary
delete resource;
```

### RAII Patterns
- Always use RAII for resource management
- Prefer stack allocation when lifetime is clear
- Use `std::vector` over raw arrays with `new[]`

---

## Concurrency & Thread Safety

### Atomic Operations
```cpp
// ✅ Thread-safe: Use atomics for shared counters
std::atomic<size_t> counter_{0};
counter_.fetch_add(1, std::memory_order_relaxed);

// ❌ Avoid: Non-atomic shared variables
static size_t counter = 0;  // Race condition!
counter++;
```

### Synchronization
- Use `std::mutex` with `std::lock_guard` or `std::unique_lock`
- Document lock ordering to prevent deadlocks
- Minimize critical sections
- Use `std::atomic` for simple flags and counters

---

## Error Handling

### Logging Levels
Use consistent logging levels:

```cpp
THEMIS_ERROR("...")   // Errors requiring immediate attention
THEMIS_WARN("...")    // Warnings about potential issues
THEMIS_INFO("...")    // Informational messages
THEMIS_DEBUG("...")   // Debug information (verbose)
```

**Guidelines:**
- `ERROR`: System failures, data corruption, security issues
- `WARN`: Recoverable errors, deprecated usage, configuration issues
- `INFO`: Startup/shutdown, important state changes
- `DEBUG`: Detailed diagnostic information

### Exception Handling
```cpp
// ✅ Specific exception types with detailed messages
try {
    // operation
} catch (const std::exception& e) {
    THEMIS_ERROR("Operation failed: {}", e.what());
    return false;
} catch (...) {
    THEMIS_ERROR("Unknown error in operation");
    return false;
}

// ❌ Avoid: Generic catch-all without logging
try {
    // operation
} catch (...) { return false; }  // Silent failure!
```

---

## Naming Conventions

### Variables
- `snake_case` for local variables and parameters
- `camelCase` for member variables with trailing underscore: `memberVar_`
- `UPPER_CASE` for constants and macros

### Functions
- `camelCase` for member functions
- `snake_case` for free functions (optional, project discretion)
- Descriptive names that indicate purpose

### Classes
- `PascalCase` for class names
- `IPascalCase` for interface classes (with 'I' prefix)

### Examples
```cpp
class DatabaseManager {
private:
    std::atomic<size_t> connectionCount_{0};  // Member with trailing _
    std::mutex mutex_;
    
public:
    bool openConnection(const std::string& connection_string);  // camelCase method
    size_t getConnectionCount() const;
};

constexpr int MAX_CONNECTIONS = 100;  // UPPER_CASE constant
```

---

## Type Conversions

### Static Casts
```cpp
// ✅ Document assumptions for static_cast
// Safe: size() returns size_t which fits in int for typical dimensions
int dimension = static_cast<int>(vector.size());  

// ✅ Add runtime checks for critical conversions
if (value > std::numeric_limits<int>::max()) {
    throw std::overflow_error("Value too large");
}
int converted = static_cast<int>(value);
```

### Dynamic Casts
```cpp
// ✅ Always check result of dynamic_cast
auto* derived = dynamic_cast<DerivedClass*>(base_ptr);
if (!derived) {
    THEMIS_ERROR("Failed to cast to DerivedClass");
    return false;
}
```

---

## Defensive Programming

### Null Pointer Checks
```cpp
// ✅ Check pointers before use
auto* db = getDatabase();
if (!db) {
    THEMIS_ERROR("Database is null");
    return false;
}

// ✅ Use if-init pattern with weak_ptr
if (auto session = sessionWeak.lock()) {
    session->doSomething();
}
```

### Preconditions
- Document preconditions in comments
- Add assertions in debug builds
- Validate inputs at API boundaries

---

## Comments & Documentation

### Code Comments
- Explain *why*, not *what* (code should be self-documenting)
- Document non-obvious algorithms or optimizations
- Mark TODOs with context: `// TODO(username): description`

### Function Documentation
```cpp
/**
 * @brief Executes query with snapshot isolation
 * 
 * @param query The SQL-like query to execute
 * @param timeout_ms Maximum execution time in milliseconds
 * @return Query results or error status
 * 
 * @throws std::runtime_error if database is not open
 */
Status executeQuery(const std::string& query, int timeout_ms);
```

---

## Signal Handlers

### Async-Signal-Safe Operations
Signal handlers **must only** use async-signal-safe operations:

```cpp
// ✅ Async-signal-safe
std::atomic<bool> g_shutdown{false};
void signalHandler(int signal) {
    g_shutdown.store(true, std::memory_order_release);
    write(STDERR_FILENO, "Shutting down\n", 14);
}

// ❌ NOT async-signal-safe
void signalHandler(int signal) {
    logger.info("Shutdown");  // Undefined behavior!
    shared_ptr->stop();        // Undefined behavior!
}
```

See: POSIX async-signal-safe functions list

---

## Testing

### Unit Tests
- One test file per source file: `foo.cpp` → `test_foo.cpp`
- Use descriptive test names: `TEST(ComponentName, DescriptiveBehavior)`
- Test error paths, not just happy paths

### Integration Tests
- Place in `tests/integration/` directory
- Document setup requirements
- Use fixtures for common setup

---

## Code Review Checklist

Before submitting PR, verify:
- [ ] No raw `new`/`delete` (use smart pointers)
- [ ] Thread-safe shared state (atomics/mutexes)
- [ ] Error handling with specific exceptions
- [ ] Null pointer checks after dynamic operations
- [ ] Consistent logging levels
- [ ] Comments explain non-obvious code
- [ ] Tests cover new functionality
- [ ] No compiler warnings

---

## Static Analysis

### Required Tools
Run before committing:
```bash
# Clang-tidy
clang-tidy src/**/*.cpp

# Cppcheck
cppcheck --enable=all src/
```

### CI Integration
- Static analysis runs on all PRs
- No new warnings policy
- Security scans with CodeQL

---

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [POSIX Async-Signal-Safe Functions](https://man7.org/linux/man-pages/man7/signal-safety.7.html)

---

**Last Updated:** 2026-04-06  
**Maintained By:** ThemisDB Development Team
