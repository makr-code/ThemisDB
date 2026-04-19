> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Base Module — Public Headers

This directory contains the public C++ header files for the `base` module: plugin-loading
infrastructure, hot-reload, OS-level sandboxing, WASM isolation, remote registry client,
plugin dependency graph, A/B test manager, platform export macros, and the abstract
dependency-inversion interfaces.

## Header Files

| Header | Purpose |
|--------|---------|
| `module_loader.h` | Secure cross-platform shared-library loading, signature verification, trust levels, per-plugin audit trail |
| `hot_reload_manager.h` | Zero-downtime hot-reload and rollback |
| `module_sandbox.h` | OS-level resource limits (CPU/memory) and ABI compatibility checking |
| `wasm_plugin_sandbox.h` | WASM-based memory-safe isolation for untrusted plugins |
| `remote_registry_client.h` | Authenticated download and installation of marketplace plugins |
| `plugin_dependency_graph.h` | Dependency declaration, visualization, and topological ordering |
| `ab_test_manager.h` | Traffic-split A/B testing via module swapping |
| `export.h` | Platform-specific `THEMIS_BASE_API` export/import macros |

## `interfaces/` Subdirectory

This subdirectory contains **abstract dependency-inversion interfaces** that break circular
dependencies between ThemisDB's core components and enable testability and modularity.

ThemisDB has historically had circular dependencies between core components:
- **Query ↔ Storage ↔ Index**: Query needs Storage, Storage needs Query (for filtering), Index needs Query (for expression evaluation)
- **Storage ↔ Security**: Storage needs Security for encryption, Security needs Storage for configuration
- **Index ↔ Query**: Index needs Query for WHERE clause evaluation, Query needs Index for optimizations

These interfaces apply the **Dependency Inversion Principle (DIP)** to break these cycles:
- High-level modules depend on abstractions (interfaces), not concrete implementations
- Low-level modules implement abstractions
- Abstractions don't depend on details; details depend on abstractions

## Interface Files in `interfaces/`

### `interfaces/storage_interface.h`
Abstract interface for storage backends.

**Key Types:**
- `IStorageEngine`: Core key-value storage operations (Put, Get, Delete, Scan)
- `IStorageEngine::ITransaction`: Transaction interface for ACID guarantees
- `IStorageEngineFactory`: Factory for creating storage engines

**Dependencies:** None (pure abstraction)

**Use Cases:**
- Query engine depends on `IStorageEngine` instead of concrete `RocksDBWrapper`
- Enables mock storage for unit testing
- Allows swapping RocksDB for alternative backends

### `interfaces/query_interface.h`
Abstract interface for query execution and expression evaluation.

**Key Types:**
- `IExpressionEvaluator`: Evaluates filter expressions against row data
- `IQueryEngine`: Executes queries and returns result sets
- `QueryResult`: Standard result container
- `IQueryEngineFactory`: Factory for creating query engines

**Dependencies:** None (pure abstraction)

**Use Cases:**
- Index manager depends on `IExpressionEvaluator` for WHERE clause filtering
- Storage engine depends on `IQueryEngine` for query processing
- Enables mock query engine for testing
- Expression evaluator can be injected into indexes without circular dependencies

### `interfaces/index_interface.h`
Abstract interfaces for various index types.

**Key Types:**
- `ISecondaryIndex`: B-tree/hash indexes for field lookups
- `IVectorIndex`: Vector similarity search (HNSW, IVF)
- `IGraphIndex`: Graph traversal operations
- `IIndexManager`: Coordinates all indexes

**Dependencies:**
- `query_interface.h` (for `IExpressionEvaluator` injection)

**Use Cases:**
- Query engine depends on `IIndexManager` instead of concrete implementations
- Indexes receive `IExpressionEvaluator` for filtering without depending on query engine
- Enables mock indexes for testing
- Supports multiple index implementations

### `interfaces/security_interface.h`
Abstract interfaces for encryption and key management.

**Key Types:**
- `IKeyProvider`: Abstract key storage (memory, HSM, KMS, Vault)
- `IFieldEncryption`: Field-level encryption operations
- `EncryptedData`: Standard encrypted data container
- `IFieldEncryptionFactory`: Factory for creating encryption services
- `IKeyProviderFactory`: Factory for creating key providers

**Dependencies:** None (pure abstraction)

**Use Cases:**
- Storage engine depends on `IFieldEncryption` for encrypting field values
- Key providers can be swapped (HSM vs. memory vs. cloud KMS)
- Enables mock encryption for testing
- Decouples storage from cryptographic implementation details

## Architecture Benefits

### 1. **No Circular Dependencies**
```
Before:
Query → Storage → Query (circular!)
Index → Query → Index (circular!)

After:
Query → IStorageEngine ← StorageImpl
Index → IExpressionEvaluator ← QueryImpl
```

### 2. **Isolated Unit Testing**
```cpp
// Mock storage for query engine tests
class MockStorageEngine : public IStorageEngine {
    bool put(string_view key, string_view value) override {
        mock_data[key] = value;
        return true;
    }
    // ... implement other methods
};

TEST(QueryEngine, BasicSelect) {
    auto mock_storage = std::make_unique<MockStorageEngine>();
    auto query_engine = createQueryEngine(std::move(mock_storage));
    // Test query engine in isolation
}
```

### 3. **Clear Dependency Graph**
```
IStorageEngine ← RocksDBWrapper
     ↑
     |
QueryEngine

IExpressionEvaluator ← QueryEngine
         ↑
         |
   IndexManager
```

### 4. **Pluggable Implementations**
- Swap RocksDB for alternative storage (LMDB, etc.)
- Use different query parsers (AQL, SQL, GraphQL)
- Support multiple index implementations
- Use different encryption backends (OpenSSL, HSM, KMS)

## Usage Patterns

### Dependency Injection
```cpp
// Create components with injected dependencies
auto storage = std::make_unique<RocksDBWrapper>(config);
auto query_engine = createQueryEngine(std::move(storage));

auto key_provider = std::make_shared<VaultKeyProvider>(vault_config);
auto encryption = createFieldEncryption(config, key_provider);
```

### Expression Evaluation in Indexes
```cpp
// Query engine creates evaluator
auto evaluator = query_engine->createExpressionEvaluator();

// Index receives evaluator and uses it for filtering
auto results = vector_index->search(
    query_vector,
    k,
    evaluator.get()  // No circular dependency!
);
```

### Mock Testing
```cpp
// Create mock implementations for testing
class MockKeyProvider : public IKeyProvider {
    std::optional<std::vector<uint8_t>> getKey(
        string_view key_id, uint32_t version) override {
        return std::vector<uint8_t>{0x00, 0x11, 0x22, ...};
    }
};

TEST(FieldEncryption, EncryptDecrypt) {
    auto mock_keys = std::make_shared<MockKeyProvider>();
    auto encryption = createFieldEncryption(config, mock_keys);
    // Test encryption in isolation
}
```

## Implementation Phases

### Phase 1: Interface Definition ✅ (Current)
- Define abstract interfaces with comprehensive documentation
- No implementation changes
- No breaking changes

### Phase 2: Storage Engine Refactoring (Next PR)
- `RocksDBWrapper` implements `IStorageEngine`
- Inject dependencies via constructors
- Update tests to use interfaces

### Phase 3: Query Engine Refactoring
- Query engine implements `IQueryEngine` and `IExpressionEvaluator`
- Remove direct dependencies on storage/index implementations
- Use dependency injection

### Phase 4: Index Manager Refactoring
- Index implementations extend interface types
- Accept `IExpressionEvaluator` via dependency injection
- Remove circular dependencies on query engine

### Phase 5: Security Layer Refactoring
- Implement `IKeyProvider` for various backends
- `FieldEncryption` implements `IFieldEncryption`
- Storage uses injected `IFieldEncryption` instead of direct calls

## Design Principles

### Interface Segregation
Each interface defines a focused, cohesive set of operations. Clients depend only on methods they use.

### Dependency Inversion
High-level policy modules (Query, Index) depend on abstractions. Low-level detail modules (Storage, Security) implement abstractions.

### Open/Closed Principle
Interfaces are closed for modification (stable), open for extension (new implementations).

### Single Responsibility
Each interface has one reason to change: its contract with clients.

## Guidelines for Future Interfaces

1. **Pure Virtual Methods**: Interfaces should have no implementation (except virtual destructor)
2. **No Dependencies on Implementations**: Interfaces should not include headers of concrete classes
3. **Forward Declarations**: Use forward declarations where possible
4. **Factory Patterns**: Provide factory interfaces for creating instances
5. **Export Macros**: Use `THEMIS_BASE_API` for all interface classes
6. **Comprehensive Documentation**: Document purpose, usage, and threading guarantees

## Testing

Each interface should have:
1. **Compilation tests**: Verify interface compiles standalone
2. **Mock implementations**: For testing components that depend on interface
3. **Contract tests**: Verify implementations satisfy interface contract

Example:
```cpp
TEST(StorageInterface, Compiles) {
    // This test just needs to compile
    static_assert(std::is_abstract_v<IStorageEngine>);
}

TEST(MockStorage, ImplementsInterface) {
    MockStorageEngine mock;
    IStorageEngine* iface = &mock;
    EXPECT_TRUE(iface->put("key", "value"));
}
```

## Related Documentation

- [src/base README](../../../src/base/README.md) — module overview and usage guide
- [src/base ARCHITECTURE](../../../src/base/ARCHITECTURE.md) — detailed architecture guide
- [src/base ROADMAP](../../../src/base/ROADMAP.md) — feature roadmap and status
- [src/base FUTURE_ENHANCEMENTS](../../../src/base/FUTURE_ENHANCEMENTS.md) — planned features and design constraints
- [Architecture Decision Records](../../../docs/architecture/)
- [Modularization Plan](../../../docs/architecture/MODULARIZATION_PLAN.md)
- [Testing Strategy](../../../tests/README.md)

## Questions?

For questions about these interfaces or suggestions for improvements, please open an issue or contact the ThemisDB Core Team.
