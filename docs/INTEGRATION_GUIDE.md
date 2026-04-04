# Phase 3: Integration Guide

## Overview

This guide explains how to integrate the extracted handler classes into the existing `HttpServer` infrastructure after implementing the handler methods.

## Integration Steps

### Step 1: Update CMakeLists.txt

Add all handler source files to the build system in `cmake/CMakeLists.txt` (around line 1021):

```cmake
# Existing http_server.cpp
../src/server/http_server.cpp

# Add new API handlers
../src/server/entity_api_handler.cpp
../src/server/query_api_handler.cpp
../src/server/index_api_handler.cpp
../src/server/vector_api_handler.cpp
../src/server/content_api_handler.cpp
../src/server/transaction_api_handler.cpp
../src/server/timeseries_api_handler.cpp
../src/server/changefeed_api_handler.cpp
../src/server/spatial_api_handler.cpp
../src/server/cache_api_handler.cpp
../src/server/prompt_api_handler.cpp
../src/server/graph_api_handler.cpp
../src/server/admin_api_handler.cpp
../src/server/monitoring_api_handler.cpp
../src/server/policy_api_handler.cpp
../src/server/wal_api_handler.cpp
```

### Step 2: Update http_server.h

Add handler member variables and includes:

```cpp
// Add to includes section
#include "server/entity_api_handler.h"
#include "server/query_api_handler.h"
#include "server/index_api_handler.h"
#include "server/vector_api_handler.h"
#include "server/content_api_handler.h"
#include "server/transaction_api_handler.h"
#include "server/timeseries_api_handler.h"
#include "server/changefeed_api_handler.h"
#include "server/spatial_api_handler.h"
#include "server/cache_api_handler.h"
#include "server/prompt_api_handler.h"
#include "server/graph_api_handler.h"
#include "server/admin_api_handler.h"
#include "server/monitoring_api_handler.h"
#include "server/policy_api_handler.h"
#include "server/wal_api_handler.h"

// Add to HttpServer class private members
class HttpServer {
    // ... existing members ...
    
private:
    // Handler instances
    std::shared_ptr<EntityApiHandler> entity_handler_;
    std::shared_ptr<QueryApiHandler> query_handler_;
    std::shared_ptr<IndexApiHandler> index_handler_;
    std::shared_ptr<VectorApiHandler> vector_handler_;
    std::shared_ptr<ContentApiHandler> content_handler_;
    std::shared_ptr<TransactionApiHandler> transaction_handler_;
    std::shared_ptr<TimeSeriesApiHandler> timeseries_handler_;
    std::shared_ptr<ChangefeedApiHandler> changefeed_handler_;
    std::shared_ptr<SpatialApiHandler> spatial_handler_;
    std::shared_ptr<CacheApiHandler> cache_handler_;
    std::shared_ptr<PromptApiHandler> prompt_handler_;
    std::shared_ptr<GraphApiHandler> graph_handler_;
    std::shared_ptr<AdminApiHandler> admin_handler_;
    std::shared_ptr<MonitoringApiHandler> monitoring_handler_;
    std::shared_ptr<PolicyApiHandler> policy_handler_;
    std::shared_ptr<WALApiHandler> wal_handler_;
    
    // ... existing members ...
};
```

### Step 3: Initialize Handlers in HttpServer Constructor

In `http_server.cpp`, update the constructor to create handler instances:

```cpp
HttpServer::HttpServer(/* constructor parameters */) 
    : /* existing initializers */
{
    // ... existing initialization ...
    
    // Initialize API handlers
    entity_handler_ = std::make_shared<EntityApiHandler>(
        storage_, secondary_index_, graph_index_, tx_manager_, 
        field_encryption_, key_provider_, auth_);
    
    query_handler_ = std::make_shared<QueryApiHandler>(
        storage_, secondary_index_, query_engine_, query_optimizer_,
        semantic_cache_, llm_store_, prompt_manager_, auth_);
    
    index_handler_ = std::make_shared<IndexApiHandler>(
        storage_, secondary_index_, adaptive_index_, auth_);
    
    vector_handler_ = std::make_shared<VectorApiHandler>(
        storage_, vector_index_, auth_);
    
    content_handler_ = std::make_shared<ContentApiHandler>(
        storage_, content_manager_, content_processor_, auth_);
    
    transaction_handler_ = std::make_shared<TransactionApiHandler>(
        storage_, tx_manager_, auth_);
    
    timeseries_handler_ = std::make_shared<TimeSeriesApiHandler>(
        storage_, ts_store_, agg_manager_, auth_);
    
    changefeed_handler_ = std::make_shared<ChangefeedApiHandler>(
        storage_, changefeed_, sse_manager_, auth_);
    
    spatial_handler_ = std::make_shared<SpatialApiHandler>(
        storage_, spatial_index_, auth_);
    
    cache_handler_ = std::make_shared<CacheApiHandler>(
        semantic_cache_, auth_);
    
    prompt_handler_ = std::make_shared<PromptApiHandler>(
        storage_, prompt_manager_, auth_);
    
    graph_handler_ = std::make_shared<GraphApiHandler>(
        storage_, graph_index_, auth_);
    
    admin_handler_ = std::make_shared<AdminApiHandler>(
        storage_, auth_);
    
    monitoring_handler_ = std::make_shared<MonitoringApiHandler>(
        storage_, auth_, request_count_);
    
    policy_handler_ = std::make_shared<PolicyApiHandler>(
        storage_, ranger_adapter_, auth_);
    
    wal_handler_ = std::make_shared<WALApiHandler>(
        storage_, wal_applier_, wal_manager_, replication_coordinator_, auth_);
    
    // ... rest of initialization ...
}
```

### Step 4: Update Route Handling

Replace handler method calls with delegations to handler instances. In the routing logic (around line 1600):

#### Example: Entity Operations

```cpp
// Before:
case Route::EntitiesGet:
    response = handleGetEntity(req);
    break;
case Route::EntitiesPut:
    response = handlePutEntity(req);
    break;
case Route::EntitiesDelete:
    response = handleDeleteEntity(req);
    break;
case Route::EntitiesBatch:
    response = handleEntitiesBatch(req);
    break;

// After:
case Route::EntitiesGet:
    response = entity_handler_->handleGet(req);
    break;
case Route::EntitiesPut:
    response = entity_handler_->handlePut(req);
    break;
case Route::EntitiesDelete:
    response = entity_handler_->handleDelete(req);
    break;
case Route::EntitiesBatch:
    response = entity_handler_->handleBatch(req);
    break;
```

#### Example: Admin Operations

```cpp
// Before:
case Route::AdminBackup:
    response = handleAdminBackup(req);
    break;
case Route::AdminRestore:
    response = handleAdminRestore(req);
    break;

// After:
case Route::AdminBackup:
    response = admin_handler_->handleBackup(req);
    break;
case Route::AdminRestore:
    response = admin_handler_->handleRestore(req);
    break;
```

### Step 5: Remove Old Handler Methods

After delegating all routes, remove the old handler method implementations from `http_server.cpp`:

```cpp
// DELETE these methods after delegation:
// http::response<http::string_body> HttpServer::handleGetEntity(...)
// http::response<http::string_body> HttpServer::handlePutEntity(...)
// http::response<http::string_body> HttpServer::handleAdminBackup(...)
// ... etc
```

**Important**: Keep any helper methods that are still used elsewhere in HttpServer:
- `makeResponse()`
- `makeErrorResponse()`
- `requireAccess()`
- `applyGovernanceHeaders()`
- `extractAuthContext()`
- etc.

### Step 6: Test Build

After each handler integration:

```bash
cd build
cmake ..
make -j$(nproc)
```

Fix any compilation errors related to:
- Missing includes
- Missing dependencies
- Signature mismatches

### Step 7: Run Tests

```bash
# Run server tests
./build/tests/server_test

# Run integration tests
./build/tests/integration_test

# Start server and test manually
./build/themis_server --config config.yaml
```

## Complete Integration Example: AdminApiHandler

Since AdminApiHandler is already implemented, here's a complete integration example:

### 1. Already in CMakeLists.txt (add if not present):
```cmake
../src/server/admin_api_handler.cpp
```

### 2. In http_server.h:
```cpp
#include "server/admin_api_handler.h"

class HttpServer {
    // ...
private:
    std::shared_ptr<AdminApiHandler> admin_handler_;
    // ...
};
```

### 3. In http_server.cpp constructor:
```cpp
admin_handler_ = std::make_shared<AdminApiHandler>(storage_, auth_);
```

### 4. In http_server.cpp routing (find around line 1623 and 1626):
```cpp
// Replace these lines:
// Line ~1623:
//     response = handleAdminBackup(req);
// Line ~1626:
//     response = handleAdminRestore(req);

// With:
case Route::AdminBackup:
    response = admin_handler_->handleBackup(req);
    break;
case Route::AdminRestore:
    response = admin_handler_->handleRestore(req);
    break;
```

### 5. Remove from http_server.cpp (lines ~9813-9852):
```cpp
// DELETE these method implementations:
// http::response<http::string_body> HttpServer::handleAdminBackup(...) { ... }
// http::response<http::string_body> HttpServer::handleAdminRestore(...) { ... }
```

## Integration Checklist

For each handler:

- [ ] Handler implementation complete (Phase 2)
- [ ] Added to CMakeLists.txt
- [ ] Include added to http_server.h
- [ ] Member variable added to HttpServer class
- [ ] Initialized in HttpServer constructor
- [ ] All routes delegated to handler
- [ ] Old methods removed from http_server.cpp
- [ ] Code compiles without errors
- [ ] Tests pass
- [ ] Manual testing performed

## Migration Order (Recommended)

Follow this order to minimize integration issues:

1. ✅ AdminApiHandler (example above)
2. MonitoringApiHandler (minimal dependencies)
3. CacheApiHandler
4. PromptApiHandler
5. GraphApiHandler
6. SpatialApiHandler
7. PolicyApiHandler
8. WALApiHandler
9. EntityApiHandler
10. IndexApiHandler
11. VectorApiHandler
12. TransactionApiHandler
13. TimeSeriesApiHandler
14. QueryApiHandler
15. ChangefeedApiHandler
16. ContentApiHandler

## Common Integration Issues

### Issue 1: Missing Dependencies

**Error**: `'SomeClass' was not declared in this scope`

**Solution**: Add forward declarations or includes in handler header:
```cpp
#include "path/to/some_class.h"
```

### Issue 2: Circular Dependencies

**Error**: Recursive includes or incomplete types

**Solution**: Use forward declarations in headers, includes in .cpp files:
```cpp
// In .h file:
class SomeClass;  // Forward declaration

// In .cpp file:
#include "some_class.h"  // Full include
```

### Issue 3: Constructor Parameter Mismatch

**Error**: No matching constructor

**Solution**: Check handler constructor signature matches HttpServer initialization

### Issue 4: Helper Method Access

**Error**: `makeResponse` is not a member of handler

**Solution**: 
- Copy helper methods to handler (if handler-specific)
- Or keep in HttpServer and pass as callbacks
- Or create shared utilities class

## Verification

After full integration:

1. **Line count verification**:
   ```bash
   wc -l src/server/http_server.cpp
   # Should be ~6,800 lines (down from 12,845)
   ```

2. **Build verification**:
   ```bash
   make clean && make -j$(nproc)
   # Should build without errors
   ```

3. **Functionality verification**:
   - All endpoints still work
   - No regression in performance
   - Tests pass

## Next Phase

After Phase 3 integration is complete:

**Phase 4: Testing & Optimization**
- Comprehensive testing of all handlers
- Performance benchmarking
- Code review
- Documentation updates
- Prepare for merge

## Questions?

Refer to:
- `docs/HTTP_SERVER_REFACTORING.md` - Overall plan
- `docs/HANDLER_IMPLEMENTATION_GUIDE.md` - Implementation guide
- `src/server/admin_api_handler.cpp` - Reference implementation
