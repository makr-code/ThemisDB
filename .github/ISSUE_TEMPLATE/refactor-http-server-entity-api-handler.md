---
title: "[REFACTOR] Implement EntityApiHandler - Extract Entity CRUD Operations from http_server.cpp"
labels: 
  - "priority:P2"
  - "type:refactoring"
  - "area:api"
  - "effort:large"
  - "good first issue"
assignees: []
---

# Refactoring Task: EntityApiHandler Implementation

## Overview

Extract and implement entity CRUD operations from `http_server.cpp` into the `EntityApiHandler` class. This is part of the http_server.cpp refactoring initiative to split the 12,845-line file into modular, maintainable handler classes.

## Handler Details

**Class:** `EntityApiHandler`  
**Files:** `include/server/entity_api_handler.h`, `src/server/entity_api_handler.cpp`  
**Lines to Extract:** ~880 lines  
**Priority:** Recommended as reference implementation (medium complexity)

## Endpoints to Implement

### 1. GET /entities/{id}
- **Method:** `handleGet(const http::request<http::string_body>& req)`
- **Source:** `http_server.cpp` ~line 5621 (`handleGetEntity`)
- **Features:** 
  - Entity retrieval with field-level encryption support
  - Secondary index integration
  - Graph edge traversal

### 2. PUT /entities
- **Method:** `handlePut(const http::request<http::string_body>& req)`
- **Source:** `http_server.cpp` ~line 5780 (`handlePutEntity`)
- **Features:**
  - Entity creation/update
  - Field-level encryption
  - Secondary index updates
  - Graph relationship management

### 3. DELETE /entities/{id}
- **Method:** `handleDelete(const http::request<http::string_body>& req)`
- **Source:** `http_server.cpp` ~line 6064 (`handleDeleteEntity`)
- **Features:**
  - Entity deletion
  - Cascade delete for graph relationships
  - Secondary index cleanup

### 4. POST /entities/batch
- **Method:** `handleBatch(const http::request<http::string_body>& req)`
- **Source:** `http_server.cpp` ~line 6178 (`handleEntitiesBatch`)
- **Features:**
  - Batch operations (get, put, delete)
  - Transaction support
  - Bulk processing

## Implementation Guide

### Step 1: Review Reference Implementation

**Reference:** `src/server/admin_api_handler.cpp` (implemented as reference example in this PR)

Study the AdminApiHandler to understand:
- Constructor pattern with dependencies
- Helper method implementation (`makeResponse`, `makeErrorResponse`)
- Error handling patterns
- Request/response flow

### Step 2: Locate Source Code

```bash
grep -n "handleGetEntity\|handlePutEntity\|handleDeleteEntity\|handleEntitiesBatch" src/server/http_server.cpp
```

### Step 3: Copy Implementations

Copy each handler method from `http_server.cpp` to `entity_api_handler.cpp`:

1. Copy method implementation
2. Update includes (nlohmann/json, logger, tracing, etc.)
3. Update member variable access (use `storage_`, `secondary_index_`, etc.)
4. Implement helper methods

### Step 4: Implement Helper Methods

```cpp
http::response<http::string_body> EntityApiHandler::makeResponse(
    http::status status, const std::string& body, 
    const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> EntityApiHandler::makeErrorResponse(
    http::status status, const std::string& message,
    const http::request<http::string_body>& req
) {
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

std::string EntityApiHandler::extractPathParam(
    const std::string& target, const std::string& prefix
) {
    if (target.size() <= prefix.size()) return "";
    if (target.substr(0, prefix.size()) != prefix) return "";
    return target.substr(prefix.size());
}
```

### Step 5: Update Dependencies

Ensure constructor receives all required dependencies:
- `std::shared_ptr<RocksDBWrapper> storage_`
- `std::shared_ptr<SecondaryIndex> secondary_index_`
- `std::shared_ptr<GraphIndex> graph_index_`
- `std::shared_ptr<TransactionManager> tx_manager_`
- `std::shared_ptr<FieldEncryption> field_encryption_`
- `std::shared_ptr<KeyProvider> key_provider_`
- `std::shared_ptr<AuthMiddleware> auth_`

## Testing Strategy

### Unit Tests

Create tests for each handler method:

```cpp
TEST(EntityApiHandlerTest, HandleGetEntity) {
    // Setup mock storage and dependencies
    // Call handleGet with test request
    // Verify response status and body
}

TEST(EntityApiHandlerTest, HandlePutEntity) {
    // Test entity creation
    // Test entity update
    // Verify field-level encryption
}

TEST(EntityApiHandlerTest, HandleDeleteEntity) {
    // Test entity deletion
    // Verify cascade delete
}

TEST(EntityApiHandlerTest, HandleBatch) {
    // Test batch operations
    // Verify transaction handling
}
```

### Integration Tests

Test with actual HTTP requests:

```bash
# Start server with EntityApiHandler integrated
./build/themis_server --config config.yaml

# Test GET
curl -X GET http://localhost:8080/entities/test-id

# Test PUT
curl -X PUT http://localhost:8080/entities \
  -H "Content-Type: application/json" \
  -d '{"id":"test-id","data":"value"}'

# Test DELETE
curl -X DELETE http://localhost:8080/entities/test-id

# Test Batch
curl -X POST http://localhost:8080/entities/batch \
  -H "Content-Type: application/json" \
  -d '{"operations":[...]}'
```

## Integration into HttpServer

### Update CMakeLists.txt

Add to `cmake/CMakeLists.txt` in the server source files section:

```cmake
../src/server/entity_api_handler.cpp
```

### Update http_server.h

```cpp
#include "server/entity_api_handler.h"

class HttpServer {
    // ...
private:
    std::shared_ptr<EntityApiHandler> entity_handler_;
    // ...
};
```

### Update http_server.cpp Constructor

```cpp
entity_handler_ = std::make_shared<EntityApiHandler>(
    storage_, secondary_index_, graph_index_, tx_manager_, 
    field_encryption_, key_provider_, auth_);
```

### Update Routing

Replace handler calls with delegations:

```cpp
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

### Remove Old Methods

After verification, delete from `http_server.cpp`:
- `handleGetEntity()`
- `handlePutEntity()`
- `handleDeleteEntity()`
- `handleEntitiesBatch()`

## Documentation

Refer to comprehensive guides:
- `docs/HANDLER_IMPLEMENTATION_GUIDE.md` - Step-by-step implementation
- `docs/INTEGRATION_GUIDE.md` - Integration into HttpServer
- `docs/HTTP_SERVER_REFACTORING.md` - Overall refactoring plan

## Acceptance Criteria

- [ ] All 4 handler methods implemented and working
- [ ] Helper methods implemented (`makeResponse`, `makeErrorResponse`, `extractPathParam`)
- [ ] Constructor properly initializes all dependencies
- [ ] Code compiles without errors or warnings
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] Handler integrated into HttpServer routing
- [ ] Old methods removed from http_server.cpp
- [ ] CMakeLists.txt updated
- [ ] Code follows project coding standards
- [ ] Doxygen documentation complete

## Related Files

**Handler Structure (already created):**
- `include/server/entity_api_handler.h`
- `src/server/entity_api_handler.cpp`

**Source Code:**
- `src/server/http_server.cpp` (search for handleGetEntity, handlePutEntity, handleDeleteEntity, handleEntitiesBatch)

**Reference Implementation:**
- `src/server/admin_api_handler.cpp`

## Estimated Effort

- **Complexity:** Medium-High
- **Lines of Code:** ~880 lines
- **Estimated Time:** 4-6 hours
- **Dependencies:** Storage, SecondaryIndex, GraphIndex, TransactionManager, FieldEncryption

## Notes

- This handler has moderate complexity due to multiple dependencies
- Field-level encryption adds complexity
- Graph relationship management requires careful handling
- Good candidate for second implementation after reviewing AdminApiHandler
- Follow the pattern established in AdminApiHandler reference implementation

---

**Part of:** http_server.cpp refactoring initiative (12,845 lines → modular handlers)  
**Related PR:** Refactor http_server.cpp: Complete foundation with structure, reference implementation, and integration guides
