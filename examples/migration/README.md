> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Error Handling Migration Guide

This directory contains example code demonstrating the migration from legacy error handling patterns to the new production-ready `tl::expected`-based error handling system.

## Overview

ThemisDB is migrating to a unified, type-safe error handling system using `tl::expected<T, E>` (forward-compatible with C++23 `std::expected`). This provides:

- **Type-safe error propagation** with zero overhead (no exceptions)
- **Structured error codes** with machine-readable metadata
- **Rich error context** embedded in error objects
- **Composable error handling** with monadic operations
- **Compiler-enforced error checking**

## Migration Examples

### 1. IndexManager: `nullptr` → `Result<T*>`

**File**: `index_manager_migration_example.cpp`

**Pattern**: Methods returning pointers that may be null

**Before**:
```cpp
ISecondaryIndex* index = manager.createSecondaryIndex("my_index", "field", "{}");
if (!index) {
    // Lost error context - why did it fail?
    spdlog::error("Failed to create index");
    return;
}
// Use index...
```

**After**:
```cpp
auto result = manager.createSecondaryIndex("my_index", "field", "{}");
if (result) {
    ISecondaryIndex* index = *result;
    // Use index...
} else {
    const Error& err = result.error();
    spdlog::error("Failed to create index: {} (code: {})", 
                 err.message(), static_cast<int>(err.code()));
    
    // Get detailed solution
    auto metadata = err.metadata();
    spdlog::info("Solution:\n{}", metadata.solution);
    
    // Handle specific error types
    switch (err.code()) {
        case ErrorCode::ERR_INDEX_NOT_INITIALIZED:
            // Take specific action for this error
            break;
        case ErrorCode::ERR_INDEX_CREATION_FAILED:
            // Different action for creation failure
            break;
    }
}
```

**Benefits**:
- Distinguish between "not initialized", "creation failed", "invalid request", etc.
- Error context includes details (e.g., index name)
- Structured solutions from error registry
- Type-safe error codes

---

### 2. ContentFS: `Status{ok, message}` → `Result<T>`

**File**: `contentfs_migration_example.cpp`

**Pattern**: Custom Status struct with bool + message

**Before**:
```cpp
auto status = contentfs.put("doc123", data, "text/plain");
if (!status.ok) {
    spdlog::error("Put failed: {}", status.message);  // Just a string!
    return;
}
```

**After**:
```cpp
auto result = contentfs.put("doc123", data, "text/plain");
if (!result) {
    const Error& err = result.error();
    spdlog::error("Put failed: {}", err.message());
    
    // Handle specific errors programmatically
    if (err.code() == ErrorCode::ERR_STORAGE_DISK_FULL) {
        // Cleanup and retry
        cleanup_old_data();
        result = contentfs.put("doc123", data, "text/plain");
    }
}
```

**Benefits**:
- Eliminates duplicate Status struct definitions across modules
- Machine-readable error codes (vs. parsing string messages)
- Richer error metadata (cause, solution, related docs)
- Type-safe composition with `and_then`

---

### 3. TSStore: `std::optional<T>` → `Result<T>`

**File**: `tsstore_migration_example.cpp`

**Pattern**: std::optional for operations that may fail

**Before**:
```cpp
auto components = ts_store.parseKey("ts:metrics:123:cpu");
if (!components) {
    // Why did it fail? Wrong prefix? Invalid timestamp? Empty field?
    spdlog::error("Parse failed");
    return;
}
// Use components...
```

**After**:
```cpp
auto result = ts_store.parseKey("ts:metrics:123:cpu");
if (result) {
    const auto& comp = *result;
    // Use components...
} else {
    const Error& err = result.error();
    spdlog::error("Parse failed: {}", err.message());
    
    // Different handling based on error type
    switch (err.code()) {
        case ErrorCode::ERR_API_INVALID_REQUEST:
            // Missing prefix - not a TS key
            break;
        case ErrorCode::ERR_SCHEMA_INVALID_TYPE:
            // Invalid timestamp format - corrupted?
            break;
        case ErrorCode::ERR_QUERY_PARSE_FAILED:
            // Malformed key structure
            break;
    }
}
```

**Benefits**:
- Detailed error information (vs. just "nullopt")
- Error context preserved through operation chains
- Specific error codes for each failure mode
- Composable with monadic operations

---

## Core Infrastructure

### Error Class

**Location**: `include/utils/expected.h`

```cpp
class Error {
    ErrorCode code_;        // Structured error code
    std::string context_;   // Dynamic context (paths, IDs, etc.)
    
    std::string message() const;           // Formatted message
    ErrorMetadata metadata() const;        // Rich metadata from registry
};
```

### Result<T> Type Alias

```cpp
template<typename T>
using Result = tl::expected<T, Error>;
```

### Helper Functions

```cpp
// Create success result
Result<int> Ok(42);

// Create error result
Result<int> Err<int>(ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, "/tmp/config.yaml");

// For void operations
Result<void> OkVoid();
Result<void> ErrVoid(ErrorCode::ERR_STORAGE_DISK_FULL, "Insufficient space");

// Convert legacy patterns
Result<T*> fromNullable(ptr, ErrorCode::ERR_INDEX_NOT_FOUND);
Result<T> fromOptional(opt, ErrorCode::ERR_QUERY_PARSE_FAILED);
Result<void> fromBoolStatus(ok, message, ErrorCode::ERR_STORAGE_CORRUPTION);
```

---

## Error Codes

Error codes are organized by category:

| Category | Range | Examples |
|----------|-------|----------|
| **Storage** | 1000-1999 | FILE_NOT_FOUND, PERMISSION_DENIED, DISK_FULL |
| **LLM** | 2000-2099 | MODEL_NOT_FOUND, CONTEXT_CREATION_FAILED, GPU_OOM |
| **LoRA** | 2100-2199 | NOT_LOADED, FUSION_FAILED, WEIGHT_MISMATCH |
| **MCP** | 3000-3999 | TRANSPORT_FAILED, INVALID_REQUEST, TOOL_NOT_FOUND |
| **Schema** | 4000-4999 | TABLE_NOT_FOUND, INVALID_TYPE, CACHE_MISS |
| **Network** | 5000-5999 | CONNECTION_REFUSED, TIMEOUT, DNS_FAILURE |
| **Index** | 6000-6099 | NOT_INITIALIZED, CREATION_FAILED, NOT_FOUND |
| **Query** | 6100-6199 | PARSE_FAILED, INVALID_SYNTAX, EXECUTION_FAILED |
| **API** | 6200-6299 | INVALID_REQUEST, UNAUTHORIZED, RATE_LIMIT |
| **Plugin** | 6300-6399 | NOT_FOUND, LOAD_FAILED, INCOMPATIBLE |

Each error code includes:
- **Category** (e.g., "Storage", "Query")
- **Severity** ("Critical", "Error", "Warning")
- **Message template** (with placeholders)
- **Cause** (detailed explanation)
- **Solution** (step-by-step resolution)
- **Related docs** (documentation links)
- **Keywords** (for searching)

---

## Monadic Operations

Result<T> supports monadic composition:

```cpp
// Chain operations with automatic error propagation
auto result = readConfig("/etc/config.yaml")
    .and_then([](const std::string& content) { 
        return parseYaml(content); 
    })
    .and_then([](const Config& cfg) { 
        return validateConfig(cfg); 
    })
    .and_then([](const Config& cfg) { 
        return applyConfig(cfg); 
    });

if (result) {
    spdlog::info("Configuration applied successfully");
} else {
    // Error from any step is automatically propagated
    spdlog::error("Configuration failed: {}", result.error().message());
}
```

---

## Migration Strategy

### Phase 1: Foundation (✅ Complete)
- ✅ Add `tl-expected` dependency
- ✅ Create `include/utils/expected.h` wrapper
- ✅ Add error codes for Index, Query, API, Plugin
- ✅ Register error codes in ErrorRegistry
- ✅ Add unit tests

### Phase 2: Migration Examples (✅ Complete)
- ✅ IndexManager migration example
- ✅ ContentFS migration example
- ✅ TSStore migration example

### Phase 3: Gradual Migration (Not in this PR)
- Migrate high-value/high-traffic code paths first
- Keep conversion helpers for gradual migration
- Update tests alongside code migration
- Maintain backward compatibility during transition

### Phase 4: Full Migration (Future Work)
- Migrate remaining ~300+ error sites
- Remove legacy error patterns
- Update all documentation
- Deprecate conversion helpers

---

## Testing

Unit tests for the error handling infrastructure:

**File**: `tests/test_expected.cpp`

Tests cover:
- Error class construction and metadata
- Result<T> success and error cases
- Result<void> for void operations
- Conversion helpers (fromNullable, fromOptional, fromBoolStatus)
- Monadic operations (and_then, value_or)
- Error code metadata retrieval

**Run tests**:
```bash
ctest -R ErrorHandlingTests --output-on-failure
```

---

## References

### Scientific Foundation

- **P0709R4**: "Zero-overhead deterministic exceptions" (Herb Sutter, 2019)
  - Foundation for `std::expected` proposal
  - Demonstrates zero-overhead error propagation

- **P1886R0**: "Error Speed Benchmarking" (Ben Craig, 2019)
  - Performance analysis of expected vs exceptions
  - Shows expected is faster in error-heavy codepaths

### Libraries

- **tl::expected**: C++11/14/17 implementation of expected
  - Header-only library
  - Forward-compatible with C++23 `std::expected`
  - Zero-overhead abstraction

### Documentation

- **Error Registry**: `include/utils/error_registry.h`
- **Expected Wrapper**: `include/utils/expected.h`
- **Migration Examples**: `examples/migration/`
- **Tests**: `tests/test_expected.cpp`

---

## Questions?

For questions about migration patterns or error handling best practices, see:

- Migration examples in this directory
- Error Registry documentation: `include/utils/error_registry.h`
- Unit tests: `tests/test_expected.cpp`

---

**Note**: These are EXAMPLE files showing migration patterns. Actual migration involves:
1. Updating method signatures in header files
2. Updating implementations in source files
3. Updating all call sites
4. Updating tests
5. Maintaining backward compatibility during transition
