---
name: Phase 3 - Migrate API Layer to Result<T>
about: Migrate API Layer endpoints from legacy error patterns to Result<T>
title: '[Phase 3] Migrate API Layer to Result<T>'
labels: ['enhancement', 'error-handling', 'phase-3', 'api-layer', 'high-priority']
assignees: ''
---

## 📋 Overview

Migrate API Layer endpoints from legacy error patterns to `Result<T>` for better user-facing error messages and consistent error handling.

**Current Status:** ~5% complete (1 of 20+ endpoints improved)  
**Target:** 100% complete  
**Priority:** 🔴 **CRITICAL** (user-facing)

## 🎯 Goals

Provide structured, detailed error messages to API consumers through type-safe error handling in all API endpoints.

## ✅ Already Improved

- [x] Export API handler (`export_api_handler.cpp`) - Enhanced error message for plugin loading

## 🔨 Remaining Work

### API Endpoints to Migrate (~20+ endpoints)

#### Query API
- [ ] Query execution endpoint
- [ ] Query validation endpoint
- [ ] Query planning endpoint

#### Collection API
- [ ] Collection create
- [ ] Collection delete
- [ ] Collection update
- [ ] Collection list

#### Document API
- [ ] Document insert
- [ ] Document update
- [ ] Document delete
- [ ] Document get
- [ ] Bulk operations

#### Index API
- [ ] Index create (already partially done via IndexManager)
- [ ] Index drop
- [ ] Index rebuild
- [ ] Index statistics

#### Plugin API
- [ ] Plugin load (already partially done)
- [ ] Plugin unload
- [ ] Plugin list
- [ ] Plugin status

#### Export/Import API
- [ ] Export endpoint (partially done)
- [ ] Import endpoint
- [ ] Backup endpoint
- [ ] Restore endpoint

## 📝 Implementation Strategy

### 1. Identify API Handler Functions
```bash
# Find API handlers
find src/server -name "*handler*.cpp" -o -name "*api*.cpp"
```

### 2. Migration Pattern

**Before:**
```cpp
if (!result) {
    return errorResponse(http::status::internal_server_error, "Operation failed");
}
```

**After:**
```cpp
auto result = operation();
if (!result.has_value()) {
    return errorResponse(
        http::status::internal_server_error,
        fmt::format("Operation failed: {}", result.error().message())
    );
}
```

### 3. Error Code to HTTP Status Mapping

Create helper function:
```cpp
http::status errorCodeToHttpStatus(errors::ErrorCode code) {
    switch (code) {
        case ERR_NOT_FOUND: return http::status::not_found;
        case ERR_INVALID_REQUEST: return http::status::bad_request;
        case ERR_PERMISSION_DENIED: return http::status::forbidden;
        // ... etc
        default: return http::status::internal_server_error;
    }
}
```

## 📋 Implementation Checklist

### Phase 1: Core Infrastructure
- [ ] Create error code to HTTP status mapping helper
- [ ] Create standardized error response formatter
- [ ] Update error response structure to include error codes

### Phase 2: High-Priority Endpoints
- [ ] Query execution (highest traffic)
- [ ] Document operations (CRUD)
- [ ] Collection operations

### Phase 3: Secondary Endpoints
- [ ] Index operations
- [ ] Plugin operations
- [ ] Export/Import operations

### Phase 4: Validation & Testing
- [ ] Integration tests for all endpoints
- [ ] Error response format validation
- [ ] API documentation updates

## 🧪 Testing Requirements

- [ ] Update integration tests for each endpoint
- [ ] Test all error code paths
- [ ] Verify HTTP status codes are correct
- [ ] Test error message formatting
- [ ] Validate JSON error response structure

### Sample Test Structure
```cpp
TEST(APILayerTest, QueryExecutionWithError) {
    auto response = executeQuery("invalid query");
    EXPECT_EQ(response.status, http::status::bad_request);
    EXPECT_TRUE(response.body.contains("error_code"));
    EXPECT_TRUE(response.body.contains("message"));
}
```

## 📚 Documentation Updates

- [ ] Update API documentation with error codes
- [ ] Document error response format
- [ ] Update OpenAPI/Swagger specs
- [ ] Add error handling examples to API docs
- [ ] Update client library documentation

### Error Response Format
```json
{
  "error": {
    "code": 6200,
    "name": "ERR_API_INVALID_REQUEST",
    "message": "Invalid query syntax: unexpected token at line 5",
    "category": "API",
    "severity": "Error",
    "solution": "Check query syntax and try again",
    "docs": ["/docs/query-language.md"]
  }
}
```

## 🎯 Success Criteria

- [ ] All API endpoints use `Result<T>` internally
- [ ] Error responses include error codes and detailed messages
- [ ] HTTP status codes correctly map to error codes
- [ ] All integration tests pass
- [ ] API documentation updated
- [ ] Zero breaking changes to existing clients

## 📊 Progress Tracking

**Expected Effort:** 2-3 weeks  
**Priority:** 🔴 Critical (user-facing impact)

### Milestones
- [ ] Week 1: Infrastructure + Query/Document APIs
- [ ] Week 2: Collection/Index APIs
- [ ] Week 3: Plugin/Export APIs + Testing

## 🔗 Related

- **Parent Issue:** #XXX (Error Handling Migration - Master Tracking)
- **Documentation:** ERROR_HANDLING_MIGRATION_STATUS.md
- **Error Codes:** include/utils/error_registry.h

## 💡 Notes

- **User Impact:** This is the most visible change to end users
- **Client Libraries:** May need updates if error format changes
- **Backward Compatibility:** Consider v1/v2 API versioning if needed
- **Monitoring:** Update error tracking/monitoring after migration
