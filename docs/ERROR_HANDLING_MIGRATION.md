# Error Handling Migration Guide

## Overview

ThemisDB is migrating from inconsistent error handling patterns (null returns, Status structs, optional types) to a standardized `Result<T>` and `ErrorCode` infrastructure.

## Established Infrastructure

### Result<T> Pattern

Located in `include/utils/expected.h`:

```cpp
// Result<T> is an alias for tl::expected<T, Error>
template<typename T>
using Result = tl::expected<T, Error>;

// Error class wraps ErrorCode with optional context
class Error {
public:
    Error(errors::ErrorCode code, std::string context = "");
    errors::ErrorCode code() const;
    const std::string& context() const;
    std::string message() const;  // Full error message with template + context
};
```

### Helper Functions

```cpp
// Create success results
auto result = Ok(value);             // Result<T> with value
auto result = OkVoid();              // Result<void> for success

// Create error results
auto result = Err<T>(ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, "path/to/file");
auto result = ErrVoid(ErrorCode::ERR_STORAGE_TRANSACTION_FAILED, "Already open");
```

### ErrorCode Enum

Located in `include/utils/error_registry.h`:

- **Storage Errors**: 1000-1999 (e.g., `ERR_STORAGE_FILE_NOT_FOUND`, `ERR_STORAGE_CORRUPTION`)
- **LLM Errors**: 2000-2099 (e.g., `ERR_LLM_MODEL_NOT_FOUND`, `ERR_LLM_GPU_OOM`)
- **Index Errors**: 6000-6099 (e.g., `ERR_INDEX_NOT_FOUND`, `ERR_INDEX_CREATION_FAILED`)
- **Query Errors**: 6100-6199 (e.g., `ERR_QUERY_PARSE_FAILED`, `ERR_QUERY_EXECUTION_FAILED`)
- **Plugin Errors**: 6300-6399 (e.g., `ERR_PLUGIN_NOT_FOUND`, `ERR_PLUGIN_LOAD_FAILED`)

Over 150+ error codes covering all subsystems.

## Migration Patterns

### Pattern 1: Bool Return → Result<void>

**Before:**
```cpp
bool open(const std::string& db_path) {
    if (already_open) {
        return false;
    }
    // ... initialize ...
    return true;
}
```

**After:**
```cpp
Result<void> open(const std::string& db_path) {
    if (already_open) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED, 
                       "Storage engine already open");
    }
    // ... initialize ...
    return OkVoid();
}
```

### Pattern 2: std::optional<T> → Result<T>

**Before:**
```cpp
std::optional<std::string> get(const std::string& key) {
    if (!is_open) {
        return std::nullopt;  // Lost: WHY it failed
    }
    // ... fetch value ...
    if (!found) {
        return std::nullopt;  // Lost: Key not found vs storage error
    }
    return value;
}
```

**After:**
```cpp
Result<std::string> get(const std::string& key) {
    if (!is_open) {
        return Err<std::string>(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                "Storage engine not open");
    }
    // ... fetch value ...
    if (!found) {
        return Err<std::string>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                                fmt::format("Key '{}' not found", key));
    }
    return Ok(value);
}
```

### Pattern 3: Raw Pointer → Result<T*>

**Before:**
```cpp
ISecondaryIndex* createSecondaryIndex(std::string_view name) {
    if (invalid_name) {
        return nullptr;  // Lost: Why creation failed
    }
    // ... create index ...
    return index_ptr;
}
```

**After:**
```cpp
Result<ISecondaryIndex*> createSecondaryIndex(std::string_view name) {
    if (invalid_name) {
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_CREATION_FAILED,
                                     fmt::format("Invalid index name: {}", name));
    }
    // ... create index ...
    return Ok(index_ptr);
}
```

### Pattern 4: Custom Status Struct → Result<T>

**Before:**
```cpp
struct Status {
    bool ok = true;
    std::string message;
    static Status OK() { return {}; }
    static Status Error(std::string msg) { return {false, msg}; }
};

std::pair<Status, std::vector<Result>> searchKnn(...) {
    if (error) {
        return {Status::Error("Search failed"), {}};
    }
    return {Status::OK(), results};
}
```

**After:**
```cpp
Result<std::vector<SearchResult>> searchKnn(...) {
    if (error) {
        return Err<std::vector<SearchResult>>(
            errors::ErrorCode::ERR_INDEX_NOT_FOUND,
            "Index not initialized"
        );
    }
    return Ok(results);
}
```

### Pattern 5: Calling Migrated Methods

**Checking Results:**
```cpp
auto result = storage->get(key);
if (result) {
    // Success - use *result
    std::string value = *result;
    processValue(value);
} else {
    // Error - check error details
    spdlog::error("Get failed: {}", result.error().message());
    // Can also check specific error code
    if (result.error().code() == errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND) {
        // Handle key not found specifically
    }
}
```

**Propagating Errors:**
```cpp
Result<MyData> fetchData(const std::string& key) {
    auto result = storage->get(key);
    if (!result) {
        // Propagate error with additional context
        return Err<MyData>(result.error().code(), 
                          fmt::format("Failed to fetch data: {}", result.error().context()));
    }
    return Ok(parseData(*result));
}
```

## Migration Status

### ✅ Completed

- **IStorageEngine Interface** (`include/themis/base/interfaces/storage_interface.h`)
  - `open()`: `bool` → `Result<void>`
  - `put()`: `bool` → `Result<void>`
  - `get()`: `std::optional<std::string>` → `Result<std::string>`
  - `del()`: `bool` → `Result<void>`

- **StorageEngine Implementation** (`src/storage/storage_engine.cpp`)
  - All methods updated with proper ErrorCodes

- **PluginManager** (`include/plugins/plugin_manager.h`)
  - Already uses `Result<T>` pattern throughout

- **IndexManager Interface** (`include/index/index_manager.h`)
  - Partially migrated - all methods return `Result<T>`

### 🔄 In Progress

- **VectorIndexManager** - Uses custom Status struct
- **SecondaryIndexManager** - Uses custom Status struct
- **GraphIndexManager** - Uses custom Status struct
- **QueryEngine** - Mixed: some methods use `std::pair<Status, T>`, some use `Result<T>`

### ⏳ Pending

- Query subsystem method signatures
- Various Manager classes
- RPC handlers
- API handlers

## Migration Checklist

When migrating a class/interface:

1. ✅ Include `utils/expected.h` header
2. ✅ Replace bool returns with `Result<void>`
3. ✅ Replace `std::optional<T>` returns with `Result<T>`
4. ✅ Replace nullable pointer returns with `Result<T*>`
5. ✅ Choose appropriate ErrorCode from error_registry.h
6. ✅ Provide meaningful context strings (file paths, resource names, etc.)
7. ✅ Update calling code to check results properly
8. ✅ Add/update tests to verify error handling
9. ✅ Mark old Status structs as `[[deprecated]]` if keeping temporarily

## Benefits

1. **Type Safety**: Compiler enforces error checking
2. **Rich Context**: ErrorCode + custom message explain exactly what went wrong
3. **No Information Loss**: Unlike bool/null, errors carry full details
4. **Standardization**: Consistent pattern across entire codebase
5. **Composition**: Easy error propagation with monadic operations
6. **Forward Compatible**: Uses tl::expected (compatible with C++23 std::expected)

## Example: Complete Migration

**Before:**
```cpp
// Header
class MyManager {
public:
    bool initialize(const std::string& path);
    std::optional<Data> getData(int id);
};

// Implementation
bool MyManager::initialize(const std::string& path) {
    if (path.empty()) return false;
    // ...
    return true;
}

std::optional<Data> MyManager::getData(int id) {
    if (!initialized_) return std::nullopt;
    // ...
    if (!found) return std::nullopt;
    return data;
}

// Usage
MyManager mgr;
if (!mgr.initialize("/path")) {
    // No idea why it failed
    return;
}
auto data = mgr.getData(42);
if (!data) {
    // Was it not initialized? Or data not found?
    return;
}
```

**After:**
```cpp
// Header
#include "utils/expected.h"

class MyManager {
public:
    Result<void> initialize(const std::string& path);
    Result<Data> getData(int id);
};

// Implementation
Result<void> MyManager::initialize(const std::string& path) {
    if (path.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                       "Path cannot be empty");
    }
    // ...
    return OkVoid();
}

Result<Data> MyManager::getData(int id) {
    if (!initialized_) {
        return Err<Data>(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                        "Manager not initialized");
    }
    // ...
    if (!found) {
        return Err<Data>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                        fmt::format("Data with id {} not found", id));
    }
    return Ok(data);
}

// Usage
MyManager mgr;
auto init_result = mgr.initialize("/path");
if (!init_result) {
    spdlog::error("Initialization failed: {}", init_result.error().message());
    return;
}

auto data_result = mgr.getData(42);
if (!data_result) {
    // Now we know exactly what went wrong
    spdlog::error("Get data failed: {}", data_result.error().message());
    if (data_result.error().code() == errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND) {
        // Handle missing data specifically
    }
    return;
}
// Use *data_result
```

## Legacy Pattern Converters

For gradual migration, helper functions are available in `expected.h`:

```cpp
// Convert nullable pointer
T* ptr = legacyFunction();
auto result = fromNullable(ptr, ErrorCode::ERR_PLUGIN_NOT_FOUND, "Plugin xyz");

// Convert bool status
bool ok = legacyFunction();
auto result = fromBoolStatus(ok, "Operation failed", ErrorCode::ERR_STORAGE_TRANSACTION_FAILED);

// Convert optional
std::optional<T> opt = legacyFunction();
auto result = fromOptional(std::move(opt), ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, "Key not found");
```

## Testing

When testing error handling:

```cpp
TEST(MyManagerTest, GetDataReturnsErrorWhenNotInitialized) {
    MyManager mgr;
    // Don't initialize
    
    auto result = mgr.getData(42);
    
    ASSERT_FALSE(result);  // Should be error
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED);
    EXPECT_THAT(result.error().message(), testing::HasSubstr("not initialized"));
}

TEST(MyManagerTest, GetDataReturnsErrorWhenNotFound) {
    MyManager mgr;
    ASSERT_TRUE(mgr.initialize("/path"));
    
    auto result = mgr.getData(999);  // Non-existent ID
    
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
}

TEST(MyManagerTest, GetDataReturnsValueWhenFound) {
    MyManager mgr;
    ASSERT_TRUE(mgr.initialize("/path"));
    // Insert data with id 42
    
    auto result = mgr.getData(42);
    
    ASSERT_TRUE(result);  // Should succeed
    EXPECT_EQ(result->id, 42);
}
```

## References

- Error Registry: `include/utils/error_registry.h`
- Result<T> Implementation: `include/utils/expected.h`
- tl::expected Documentation: https://github.com/TartanLlama/expected
- C++23 std::expected: https://en.cppreference.com/w/cpp/utility/expected
