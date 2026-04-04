# HTTP Server Refactoring - Critical Issue Summary
**Date:** 2026-01-13  
**Severity:** CRITICAL (Code does not compile)  
**Affected Files:** src/server/http_server.cpp, src/server/{diff,snapshot}_api_handler.cpp

## Critical Problem

The codebase has **incompatible type definitions** that prevent compilation:

### Type Mismatch Evidence

**File:** `src/server/http_server.cpp` (line 1931)
```cpp
// Main server calls with Beast types:
if (snapshot_api_handler_) {
    snapshot_api_handler_->handleCreateTag(req, response);
    //                                      ^^^  ^^^^^^^^
    //                                      |    |
    //                                      |    http::response<http::string_body>& (Beast)
    //                                      http::request<http::string_body>& (Beast)
}
```

**File:** `include/server/snapshot_api_handler.h` (line 59)
```cpp
// Handler expects cpp-httplib types:
void handleCreateTag(const httplib::Request& req, httplib::Response& res);
//                          ^^^^^^^^^^^^^^^^       ^^^^^^^^^^^^^^^^^
//                          cpp-httplib type       cpp-httplib type
```

**Result:** Compilation error - incompatible types

### Verification Test

Created test program `/tmp/test_types.cpp` that simulates the issue:
```cpp
// Simulate the type mismatch
boost::beast::http::request<boost::beast::http::string_body> beast_req;
boost::beast::http::response<boost::beast::http::string_body> beast_res;

Handler h;  // Expects httplib::Request/Response
h.handle(beast_req, beast_res);  // COMPILE ERROR
```

**Compiler Output:**
```
error: cannot convert 'boost::beast::http::request<boost::beast::http::string_body>' 
       to 'const httplib::Request&'
```

## Affected Endpoints

All Snapshot API endpoints cannot work:
- ❌ POST /api/v1/snapshots/tags (line 1931)
- ❌ GET /api/v1/snapshots/tags (line 1939)
- ❌ GET /api/v1/snapshots/tags/:name (line 1947)
- ❌ DELETE /api/v1/snapshots/tags/:name (line 1955)
- ❌ GET /api/v1/snapshots/stats (line 1963)

All Diff API endpoints would have the same issue (if integrated):
- ❌ GET /api/v1/diff
- ❌ GET /api/v1/diff/cache/stats
- ❌ DELETE /api/v1/diff/cache

## Root Cause

1. **Incomplete Refactoring:** Migration to cpp-httplib was started but not completed
2. **Missing Dependency:** cpp-httplib not added to vcpkg.json
3. **No Adapter Layer:** No bridge between Beast and cpp-httplib types
4. **Recent Addition:** Handlers added in commit bd436a6 (2026-01-13) without full integration

## Why This Wasn't Caught

Likely reasons:
1. **No CI Build:** Changes not tested in continuous integration
2. **Feature Disabled:** Snapshot manager might not be initialized in some builds
3. **Partial Build:** Only some compilation units tested
4. **Header-Only Issue:** cpp-httplib might be header-only, so linker doesn't fail

## Immediate Actions Required

### Option 1: Quick Fix with Adapter (2 days) ⭐ RECOMMENDED

Create a type adapter to bridge Beast ↔ cpp-httplib types:

**Create:** `src/server/http_type_adapter.h`
```cpp
namespace themis::server {
class HttpTypeAdapter {
public:
    static httplib::Request beastToHttplib(
        const http::request<http::string_body>& beast_req
    ) {
        httplib::Request httplib_req;
        // Convert method
        httplib_req.method = std::string(http::to_string(beast_req.method()));
        // Convert target/path
        httplib_req.path = std::string(beast_req.target());
        // Convert headers
        for (auto& field : beast_req) {
            std::string name(field.name_string());
            std::string value(field.value());
            httplib_req.headers.insert({name, value});
        }
        // Convert body
        httplib_req.body = beast_req.body();
        return httplib_req;
    }
    
    static http::response<http::string_body> httplibToBeast(
        const httplib::Response& httplib_res,
        unsigned version = 11
    ) {
        http::response<http::string_body> beast_res;
        beast_res.version(version);
        beast_res.result(httplib_res.status);
        // Convert headers
        for (auto& [name, value] : httplib_res.headers) {
            beast_res.set(name, value);
        }
        // Convert body
        beast_res.body() = httplib_res.body;
        beast_res.prepare_payload();
        return beast_res;
    }
};
}
```

**Update:** `src/server/http_server.cpp` (line 1929-1935)
```cpp
case Route::SnapshotsTagsPost:
    if (snapshot_api_handler_) {
        // Convert Beast → cpp-httplib
        auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
        httplib::Response httplib_res;
        snapshot_api_handler_->handleCreateTag(httplib_req, httplib_res);
        // Convert cpp-httplib → Beast
        response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
    } else {
        response = makeErrorResponse(http::status::service_unavailable, 
            "Snapshot API not available (requires CDC feature)", req);
    }
    break;
```

**Pros:**
- ✅ Quick fix (1-2 days)
- ✅ Allows code to compile and run
- ✅ Minimal risk
- ✅ Can be deployed quickly

**Cons:**
- ⚠️ Adds performance overhead (type conversions)
- ⚠️ Technical debt (temporary solution)
- ⚠️ Must be removed later

### Option 2: Revert to Beast Types (1 day)

Convert new handlers back to Beast types:

**Update:** `include/server/snapshot_api_handler.h`
```cpp
// Change from:
void handleCreateTag(const httplib::Request& req, httplib::Response& res);

// To:
void handleCreateTag(
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res
);
```

**Pros:**
- ✅ Very quick fix (1 day)
- ✅ No adapter overhead
- ✅ Consistent with rest of codebase

**Cons:**
- ❌ Stays with monolithic Beast architecture
- ❌ Misses cpp-httplib modernization benefits
- ❌ No progress toward refactoring goal

### Option 3: Complete cpp-httplib Migration (4-6 weeks)

Full refactoring as described in `HTTP_SERVER_REFACTORING_ACTION_PLAN.md`

**Pros:**
- ✅ Achieves original refactoring goals
- ✅ Modern, maintainable architecture
- ✅ Reduces http_server.cpp from 7,410 to < 500 LOC

**Cons:**
- ⚠️ Long timeline (4-6 weeks)
- ⚠️ High risk of breaking changes
- ⚠️ Requires extensive testing

## Recommended Action Plan

**Phase 1: Immediate Fix (This Week)**
1. ✅ Implement Option 1 (Adapter) to restore compilation
2. ✅ Add unit tests for adapter
3. ✅ Verify all affected endpoints work
4. ✅ Deploy adapter fix

**Phase 2: Long-term Solution (Next Sprint)**
1. 📋 Decide on Option 2 vs Option 3
2. 📋 If Option 3 chosen, follow `HTTP_SERVER_REFACTORING_ACTION_PLAN.md`
3. 📋 If Option 2 chosen, update handlers and remove adapter

## Testing Checklist

Before considering this issue resolved:
- [ ] Code compiles without errors
- [ ] All unit tests pass
- [ ] Integration tests pass for affected endpoints:
  - [ ] POST /api/v1/snapshots/tags
  - [ ] GET /api/v1/snapshots/tags
  - [ ] GET /api/v1/snapshots/tags/:name
  - [ ] DELETE /api/v1/snapshots/tags/:name
  - [ ] GET /api/v1/snapshots/stats
  - [ ] GET /api/v1/diff (if integrated)
- [ ] Performance impact measured (< 5% regression acceptable for adapter)
- [ ] Memory leaks checked
- [ ] Load testing completed

## Related Documents

- `HTTP_SERVER_REFACTORING_REVIEW.md` - Full review of refactoring status
- `HTTP_SERVER_REFACTORING_ACTION_PLAN.md` - 9-week implementation plan for complete refactoring
- `.github/ISSUE_TEMPLATE/refactoring_http_server_cpp_httplib.md` - Original refactoring plan

## Git History

**Commit:** bd436a6 (2026-01-13)
**Message:** "Refactor PITR implementation to use RocksDBWrapper abstraction (#461)"
**Issue:** Handlers created with cpp-httplib types but not integrated with Beast-based main server

---

**Priority:** P0 - Critical (Blocks compilation)  
**Severity:** High (Feature completely broken)  
**Fix Timeline:** 1-2 days (Option 1)  
**Owner:** Backend Team
