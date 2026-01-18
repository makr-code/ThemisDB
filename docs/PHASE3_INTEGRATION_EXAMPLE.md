# Phase 3: Integration Example - Breaking Circular Initialization

## Problem: Circular Initialization Dependencies

Before Phase 3, there was a circular initialization problem:

```
QueryEngine needs → StorageEngine
                  ↓
StorageEngine needs → QueryEngine's ExpressionEvaluator
                  ↓
ExpressionEvaluator is → part of QueryEngine
                  ↓
        🔄 Circular dependency!
```

## Solution: Late Binding with Dependency Injection

Phase 3 solves this with constructor injection + late binding via setters.

## Complete Server Initialization Example

```cpp
// File: src/server/server.cpp

#include "server/themis_server.h"
#include "query/query_engine.h"
#include "core/query_engine_builder.h"
#include "storage/storage_engine.h"
#include "storage/storage_engine_builder.h"
#include "index/index_manager.h"
#include "security/field_encryption.h"
#include "security/vault_key_provider.h"

class ThemisServer {
private:
    std::shared_ptr<QueryEngine> query_engine_;
    std::shared_ptr<StorageEngine> storage_engine_;
    std::shared_ptr<IndexManager> index_manager_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<SpatialIndexManager> spatial_index_;
    
public:
    bool initialize() {
        try {
            // ============================================================
            // Step 1: Create Security Layer (no dependencies)
            // ============================================================
            auto key_provider = std::make_shared<VaultKeyProvider>(vault_config_);
            auto field_encryption = std::make_shared<FieldEncryption>(key_provider);
            
            logger_.info("✓ Security layer initialized");
            
            // ============================================================
            // Step 2: Create Index Managers (no QueryEngine dependency yet)
            // ============================================================
            index_manager_ = std::make_shared<IndexManager>();
            graph_index_ = std::make_shared<GraphIndexManager>();
            vector_index_ = std::make_shared<VectorIndexManager>();
            spatial_index_ = std::make_shared<SpatialIndexManager>();
            
            logger_.info("✓ Index managers created");
            
            // ============================================================
            // Step 3: Create QueryEngine (storage = nullptr for late binding)
            // ============================================================
            // Note: We pass nullptr for storage because StorageEngine needs
            // QueryEngine's expression evaluator, creating a circular dependency.
            // We'll inject storage later via setStorage().
            
            query_engine_ = QueryEngineBuilder()
                .withStorage(nullptr)  // ← Late binding!
                .withIndexManager(index_manager_)
                .build();
            
            logger_.info("✓ QueryEngine created (storage pending)");
            
            // ============================================================
            // Step 4: Create StorageEngine (needs Evaluator + Encryption + Index)
            // ============================================================
            // Now QueryEngine exists, so we can get its expression evaluator.
            // StorageEngine needs this evaluator to filter results.
            
            auto evaluator = query_engine_->get_expression_evaluator();
            
            storage_engine_ = StorageEngineBuilder::standard()
                .withEvaluator(evaluator)           // ← From QueryEngine
                .withEncryption(field_encryption)
                .withKeyProvider(key_provider)
                .withIndexManager(index_manager_)
                .withPath(config_.db_path)
                .build();
            
            logger_.info("✓ StorageEngine created with QueryEngine evaluator");
            
            // ============================================================
            // Step 5: Inject Storage into QueryEngine (late binding)
            // ============================================================
            // Now that StorageEngine is created, inject it back into QueryEngine.
            // This completes the circular initialization chain.
            
            query_engine_->setStorage(storage_engine_);
            
            logger_.info("✓ Storage injected into QueryEngine - circular dependency resolved!");
            
            // ============================================================
            // Step 6: Inject Evaluator into Index Managers
            // ============================================================
            // Index managers also need the expression evaluator for filtering.
            
            index_manager_->setExpressionEvaluator(evaluator);
            graph_index_->setExpressionEvaluator(evaluator);
            vector_index_->setExpressionEvaluator(evaluator);
            spatial_index_->setExpressionEvaluator(evaluator);
            
            logger_.info("✓ Expression evaluator injected into all index managers");
            
            // ============================================================
            // Initialization Complete!
            // ============================================================
            logger_.info("✅ ThemisDB server initialized successfully");
            logger_.info("   Query → Storage: Connected");
            logger_.info("   Query → Indexes: Connected");
            logger_.info("   Storage → Evaluator: Connected");
            logger_.info("   Indexes → Evaluator: Connected");
            logger_.info("   🎉 All circular dependencies resolved!");
            
            return true;
            
        } catch (const std::exception& e) {
            logger_.error("❌ Failed to initialize server: {}", e.what());
            return false;
        }
    }
    
    void shutdown() {
        logger_.info("Shutting down ThemisDB server...");
        
        // Clean shutdown order (reverse of initialization)
        query_engine_.reset();
        storage_engine_.reset();
        index_manager_.reset();
        graph_index_.reset();
        vector_index_.reset();
        spatial_index_.reset();
        
        logger_.info("✓ ThemisDB server shut down");
    }
};

int main() {
    ThemisServer server;
    
    if (!server.initialize()) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }
    
    // Run server...
    
    server.shutdown();
    return 0;
}
```

## Initialization Flow Diagram

```
Time →

T1:  Create Security (VaultKeyProvider, FieldEncryption)
     └─→ No dependencies
     
T2:  Create Index Managers (IndexManager, GraphIndex, VectorIndex, SpatialIndex)
     └─→ No dependencies yet
     
T3:  Create QueryEngine with nullptr storage
     └─→ Needs: IndexManager ✓
     └─→ Needs: Storage ✗ (defer via late binding)
     
T4:  Create StorageEngine with QueryEngine's evaluator
     └─→ Needs: ExpressionEvaluator (from QueryEngine) ✓
     └─→ Needs: Encryption ✓
     └─→ Needs: IndexManager ✓
     
T5:  Inject Storage into QueryEngine
     └─→ query_engine->setStorage(storage_engine)
     └─→ Circular dependency resolved! ✅
     
T6:  Inject Evaluator into Index Managers
     └─→ index_manager->setExpressionEvaluator(evaluator)
     └─→ All dependencies satisfied! ✅
```

## Key Techniques

### 1. Constructor Injection with Nullable Parameters

```cpp
// Allow nullptr for storage - will be set later
QueryEngine(IStorageEnginePtr storage, IIndexManagerPtr index_manager);
```

### 2. Setter for Late Binding

```cpp
// Inject storage after construction
void setStorage(IStorageEnginePtr storage);
```

### 3. Expression Evaluator Export

```cpp
// Export evaluator as lightweight interface
IExpressionEvaluatorPtr get_expression_evaluator();
```

### 4. Builder Pattern for Complex Construction

```cpp
auto storage = StorageEngineBuilder::standard()
    .withEvaluator(evaluator)
    .withEncryption(encryption)
    .build();
```

## Benefits

1. **No Circular Includes**: Header files don't include each other
2. **Testable**: Each component can be tested in isolation with mocks
3. **Flexible**: Late binding enables complex initialization scenarios
4. **Clean**: Dependencies flow in one direction (downward in dependency graph)
5. **Maintainable**: Clear ownership and initialization order

## Testing with Mocks

The same pattern works with mocks for testing:

```cpp
TEST(ServerInitializationTest, CompleteFlow) {
    // Mocks
    auto mock_storage = std::make_shared<MockStorageEngine>();
    auto mock_index = std::make_shared<MockIndexManager>();
    
    // Step 1: Create QueryEngine with nullptr storage
    auto query = std::make_shared<QueryEngine>(nullptr, mock_index);
    
    // Step 2: Get evaluator
    auto evaluator = query->get_expression_evaluator();
    
    // Step 3: "Create" storage (it's a mock, so just use it)
    
    // Step 4: Inject storage
    query->setStorage(mock_storage);
    
    // Verify initialization succeeded
    EXPECT_NE(query, nullptr);
    EXPECT_NE(evaluator, nullptr);
}
```

## Migration Path

### Phase 3 (Current):
- ✅ QueryEngine refactored with DI
- ✅ Late binding supported
- ✅ Legacy constructors still work

### Phase 4 (Next):
- Refactor StorageEngine to use DI
- Refactor IndexManager to use DI
- Complete circular dependency removal

### Phase 5 (Future):
- Remove legacy constructors
- Pure interface-based architecture
- 100% mockable for testing

## References

- Architecture: `docs/PHASE3_QUERYENGINE_DI_ARCHITECTURE.md`
- Tests: `tests/test_query_engine_di.cpp`
- Builder: `include/core/query_engine_builder.h`
