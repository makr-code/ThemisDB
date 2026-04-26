# Phase 3: QueryEngine Refactoring - Breaking Circular Dependencies

## Overview

Phase 3 refactors the QueryEngine to use Dependency Injection, breaking circular dependencies between Query ↔ Storage ↔ Index components.

## Before: Circular Dependency Problem

```
┌─────────────┐         ┌──────────────────┐
│ QueryEngine │ ──────> │ RocksDBWrapper   │
│             │         │ (Storage)        │
└─────────────┘         └──────────────────┘
      │                        │
      │                        │
      v                        v
┌──────────────────────────────────────┐
│     SecondaryIndexManager            │
│     GraphIndexManager                │
│     VectorIndexManager               │
│     SpatialIndexManager              │
└──────────────────────────────────────┘
      │
      │ (Circular Dependency!)
      v
┌─────────────┐
│ QueryEngine │
│  (again!)   │
└─────────────┘

All depend on each other - creating tight coupling!
```

**Problems:**
1. ❌ Query knows concrete Storage implementation
2. ❌ Query knows concrete Index implementations
3. ❌ Circular dependency: Query ↔ Storage ↔ Index
4. ❌ Query cannot be tested in isolation
5. ❌ Alternative Storage/Index implementations are impossible

## After: Clean Dependency Injection

```
┌─────────────┐         ┌──────────────────────┐
│ QueryEngine │ ──────> │ IStorageEngine       │ (Interface)
│             │         │   (abstract)         │
└─────────────┘         └──────────────────────┘
      │                        ↑
      │                        │ implements
      v                        │
┌──────────────────────┐       │
│ IIndexManager        │       │
│   (abstract)         │  ┌────┴───────────┐
└──────────────────────┘  │ RocksDBWrapper │
      ↑                   │  (concrete)    │
      │ implements        └────────────────┘
      │
┌─────┴──────────────────┐
│ SecondaryIndexManager  │ (concrete)
│ GraphIndexManager      │ (concrete)
│ VectorIndexManager     │ (concrete)
│ SpatialIndexManager    │ (concrete)
└────────────────────────┘
```

**Benefits:**
✅ **Query only depends on abstractions** (IStorageEngine, IIndexManager)
✅ **Storage can be injected or mocked** for testing
✅ **Index managers can be injected or mocked** for testing
✅ **No circular includes** - cleaner compilation dependencies
✅ **Late binding supported** via setStorage() for complex initialization
✅ **Alternative implementations possible** without changing QueryEngine

## Key Design Patterns

### 1. Constructor Injection

```cpp
// New DI constructor (Phase 3)
QueryEngine(
    IStorageEnginePtr storage,
    IIndexManagerPtr index_manager
);

// Legacy constructors (still supported)
QueryEngine(RocksDBWrapper& db, SecondaryIndexManager& secIdx);
```

### 2. Late Binding with Setter

For complex initialization scenarios where QueryEngine and Storage have circular runtime needs:

```cpp
// Step 1: Create QueryEngine with nullptr storage
auto query = std::make_shared<QueryEngine>(nullptr, index_manager);

// Step 2: Create Storage that needs QueryEngine's evaluator
auto storage = createStorageWithEvaluator(query->get_expression_evaluator());

// Step 3: Inject storage back into QueryEngine
query->setStorage(storage);
```

### 3. Builder Pattern

```cpp
auto query = QueryEngineBuilder()
    .withStorage(my_storage)
    .withIndexManager(my_index_mgr)
    .build();
```

### 4. Expression Evaluator Interface

Breaking circular dependency for expression evaluation:

```cpp
class QueryEngine {
public:
    // Export evaluator as interface
    IExpressionEvaluatorPtr get_expression_evaluator();
    
private:
    // Internal implementation wraps QueryEngine's logic
    class QueryExpressionEvaluator : public IExpressionEvaluator {
        // Delegates to parent QueryEngine
    };
};
```

## Usage Examples

### Option 1: Factory (recommended for production)
```cpp
auto query = QueryEngine::createDefault();
```

### Option 2: Builder (recommended for testing)
```cpp
auto query = QueryEngineBuilder()
    .withStorage(my_storage)
    .withIndexManager(my_index)
    .build();
```

### Option 3: Direct DI with late binding (for complex scenarios)
```cpp
auto index_mgr = std::make_shared<IndexManager>();
auto query = std::make_shared<QueryEngine>(nullptr, index_mgr);

// Later, after storage is created...
auto storage = std::make_shared<StorageEngine>();
query->setStorage(storage);
```

### Option 4: Legacy (backward compatible)
```cpp
RocksDBWrapper db;
SecondaryIndexManager idx;
QueryEngine query(db, idx);  // Still works!
```

## Testing with Mocks

The DI architecture enables isolated unit testing:

```cpp
// Create mock dependencies
auto mock_storage = std::make_shared<MockStorageEngine>();
auto mock_index = std::make_shared<MockIndexManager>();

// Inject mocks
auto query = std::make_shared<QueryEngine>(mock_storage, mock_index);

// Test without real storage or indexes!
auto result = query->execute("SELECT * FROM users WHERE age > 18");
```

See `tests/test_query_engine_di.cpp` for complete mock implementations.

## Migration Guide

### For Existing Code

**No changes required!** The legacy constructors still work:

```cpp
// This still works exactly as before:
RocksDBWrapper db;
SecondaryIndexManager idx;
QueryEngine engine(db, idx);
```

### For New Code

Use the DI constructors and builder pattern:

```cpp
// New style with DI:
auto storage = std::make_shared<RocksDBWrapper>();
auto index_mgr = std::make_shared<SecondaryIndexManager>();
auto engine = QueryEngineBuilder()
    .withStorage(storage)
    .withIndexManager(index_mgr)
    .build();
```

### For Tests

Use mock implementations:

```cpp
auto mock_storage = std::make_shared<MockStorageEngine>();
auto mock_index = std::make_shared<MockIndexManager>();
auto engine = std::make_shared<QueryEngine>(mock_storage, mock_index);
```

## Implementation Details

### Type Aliases

```cpp
using IStorageEnginePtr = std::shared_ptr<IStorageEngine>;
using IIndexManagerPtr = std::shared_ptr<IIndexManager>;
using IQueryEnginePtr = std::shared_ptr<IQueryEngine>;
using IExpressionEvaluatorPtr = std::shared_ptr<IExpressionEvaluator>;
```

### Member Variables

QueryEngine now supports both legacy pointers and interface pointers:

```cpp
private:
    // Legacy concrete dependencies (for backward compatibility)
    RocksDBWrapper* db_ = nullptr;
    SecondaryIndexManager* secIdx_ = nullptr;
    GraphIndexManager* graphIdx_ = nullptr;
    VectorIndexManager* vectorIdx_ = nullptr;
    SpatialIndexManager* spatialIdx_ = nullptr;
    
    // New interface-based dependencies
    IStorageEnginePtr storage_;
    IIndexManagerPtr index_manager_;
```

**Note:** When using DI constructors, only the interface pointers are set. When using legacy constructors, only the concrete pointers are set. The implementation checks which set is available at runtime.

## Interface Definitions

All interfaces are defined in `include/themis/base/interfaces/`:

- `storage_interface.h` - IStorageEngine, ITransaction
- `index_interface.h` - IIndexManager, ISecondaryIndex, IVectorIndex, IGraphIndex
- `query_interface.h` - IQueryEngine, IExpressionEvaluator

## Future Phases

**Phase 4 (planned):** Refactor RocksDBWrapper to implement IStorageEngine interface directly, eliminating the need for legacy pointers.

**Phase 5 (planned):** Refactor Index managers to implement IIndexManager interface directly.

**Phase 6 (planned):** Remove legacy constructors once all consumers are migrated to DI constructors.

## References

- Problem Statement: PR #3 specification
- Related PRs: 
  - PR #1: DIP Interfaces
  - PR #2: Generic Plugin System
  - PR #2.5: StorageEngine DI
- Architecture Docs: `docs/architecture/dependency_injection.md`
