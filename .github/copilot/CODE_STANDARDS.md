# ThemisDB - Code Standards & Style Guide

## Language Standards

- **C++ Version:** C++17/20
- **Build System:** CMake 3.20+
- **Package Manager:** vcpkg

## Coding Style

### Modern C++ Patterns

**Prefer:**
- ✅ Modern C++17/20 features
- ✅ RAII patterns for resource management
- ✅ Smart pointers over raw pointers
- ✅ `auto` for type deduction (when clear)
- ✅ Range-based for loops
- ✅ Structured bindings (C++17)

**Avoid:**
- ❌ Manual memory management (new/delete)
- ❌ Raw pointers for ownership
- ❌ C-style casts
- ❌ Macros (use constexpr when possible)

### Code Examples

```cpp
// ✅ Good: Modern C++ with RAII
auto result = database->query("SELECT * FROM users");
std::unique_ptr<Connection> conn = pool->acquire();

// ✅ Good: Structured bindings
for (const auto& [key, value] : map) {
    process(key, value);
}

// ❌ Bad: Manual memory management
Connection* conn = new Connection();  // Don't do this
delete conn;                          // Don't do this
```

## Naming Conventions

### Classes and Structs

**PascalCase** for classes, structs, and enums:

```cpp
class VectorIndex {};
class DatabaseEngine {};
struct QueryResult {};
enum class IndexType {};
```

### Functions and Methods

**camelCase** for functions and methods:

```cpp
void processQuery() {};
auto executeTransaction() -> bool;
std::string getUserName();
```

### Variables

**snake_case** with trailing underscore for member variables:

```cpp
class Database {
private:
    int max_connections_{100};      // Member variable
    std::string db_path_;           // Member variable
    
public:
    void setMaxConnections(int max_connections);  // Parameter
};
```

### Constants

**UPPER_CASE** for compile-time constants:

```cpp
const int MAX_CONNECTIONS = 100;
constexpr size_t BUFFER_SIZE = 4096;
inline constexpr int DEFAULT_TIMEOUT = 30;
```

## Code Quality Tools

### Clang-Tidy

Configuration: `.clang-tidy` in project root

Run before committing:
```bash
clang-tidy src/**/*.cpp -- -std=c++17
```

### Clang-Format

Configuration: `.clang-format` in project root

Format code:
```bash
clang-format -i src/**/*.cpp
```

### Cppcheck

Configuration: `.cppcheck` in project root

Static analysis:
```bash
cppcheck --enable=all --project=compile_commands.json
```

## Documentation

### Header Comments

```cpp
/**
 * @brief Executes a query against the database
 * 
 * @param query The AQL query string
 * @param timeout Query timeout in milliseconds
 * @return QueryResult containing matched records
 * @throws QueryException if query syntax is invalid
 */
auto executeQuery(const std::string& query, int timeout) -> QueryResult;
```

### Inline Comments

- Use comments sparingly - prefer self-documenting code
- Explain **why**, not **what** (code shows what)
- Document complex algorithms or non-obvious decisions

```cpp
// ✅ Good: Explains why
// Use thread_local to avoid lock contention in hot path
thread_local ConnectionPool pool;

// ❌ Bad: States the obvious
// Increment counter
counter++;
```

## Thread Safety

### Document Threading Assumptions

```cpp
class ThreadSafeCache {
    // Thread-safe: All public methods use mutex_
    std::mutex mutex_;
    std::unordered_map<std::string, Data> cache_;
    
public:
    void insert(const std::string& key, Data data);
};
```

### Prefer Standard Patterns

```cpp
// ✅ Good: Use standard synchronization
std::lock_guard<std::mutex> lock(mutex_);

// ✅ Good: Use atomic for simple counters
std::atomic<int> request_count_{0};

// ❌ Avoid: Custom synchronization primitives (unless necessary)
```

## Error Handling

### Use Exceptions for Exceptional Cases

```cpp
// ✅ Good: Exception for truly exceptional condition
if (!file.exists()) {
    throw FileNotFoundException(path);
}

// ✅ Good: Return value for expected failures
auto connect() -> std::optional<Connection>;
```

### Prefer Expected/Optional

```cpp
// ✅ Good: Optional for nullable results
auto findUser(int id) -> std::optional<User>;

// ✅ Good: Expected for operations that can fail
auto parseConfig(const std::string& path) 
    -> std::expected<Config, ParseError>;  // C++23
```

## Performance Guidelines

### Avoid Unnecessary Copies

```cpp
// ✅ Good: Pass by const reference
void process(const std::string& data);

// ✅ Good: Move when transferring ownership
return std::move(large_object);

// ❌ Bad: Unnecessary copy
void process(std::string data);  // Copies unless caller moves
```

### Use Reserve for Containers

```cpp
// ✅ Good: Reserve capacity upfront
std::vector<int> results;
results.reserve(expected_size);

for (const auto& item : items) {
    results.push_back(process(item));
}
```

## Build Integration

### CMake Target Properties

Targets should enforce standards:

```cmake
target_compile_features(themis_core PUBLIC cxx_std_17)
target_compile_options(themis_core PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
)
```

## Architecture Documentation

Document architectural decisions in:
- `docs/architecture.md` - Overall architecture
- `docs/design.md` - AQL semantics, indexing, storage layout

## Additional Resources

- C++ Core Guidelines: https://isocpp.github.io/CppCoreGuidelines/
- Project Architecture: `ARCHITECTURE.md`
- Contributing Guidelines: `CONTRIBUTING.md`
