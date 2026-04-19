# Handler Implementation Guide

## Overview

This guide provides step-by-step instructions for implementing the remaining API handlers following the pattern established by `AdminApiHandler`.

## Reference Implementation: AdminApiHandler

The `AdminApiHandler` has been fully implemented as a reference example. Review the following files:
- `include/server/admin_api_handler.h` - Class declaration
- `src/server/admin_api_handler.cpp` - Implementation

### Key Implementation Steps

1. **Locate handler methods in http_server.cpp**
   ```bash
   grep -n "handleYourMethod" src/server/http_server.cpp
   ```

2. **Copy implementation** from `http_server.cpp` to your handler's `.cpp` file

3. **Update includes** - Add necessary headers (nlohmann/json, logger, tracing, etc.)

4. **Implement helper methods**:
   - `makeResponse()` - Creates HTTP response with proper headers
   - `makeErrorResponse()` - Creates error response with JSON error body
   - Any handler-specific helpers

5. **Update constructor** - Ensure all dependencies are passed and stored

## Implementation Pattern

### Example: EntityApiHandler (Next Recommended Handler)

**Location in http_server.cpp:**
```bash
grep -n "handleGetEntity\|handlePutEntity\|handleDeleteEntity\|handleEntitiesBatch" src/server/http_server.cpp
```

**Steps:**

1. Find the implementations:
   - `handleGetEntity` - Line ~5621
   - `handlePutEntity` - Line ~5780
   - `handleDeleteEntity` - Line ~6064
   - `handleEntitiesBatch` - Line ~6178

2. Copy each method to `src/server/entity_api_handler.cpp`

3. Update helper methods:
   ```cpp
   std::string extractPathParam(const std::string& target, const std::string& prefix) {
       if (target.size() <= prefix.size()) return "";
       if (target.substr(0, prefix.size()) != prefix) return "";
       return target.substr(prefix.size());
   }
   ```

4. Verify dependencies are available via constructor parameters

## Helper Method Patterns

### makeResponse()
```cpp
http::response<http::string_body> YourHandler::makeResponse(
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
```

### makeErrorResponse()
```cpp
http::response<http::string_body> YourHandler::makeErrorResponse(
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
```

## Handler Priority (Recommended Order)

Based on complexity and dependencies:

1. ✅ **AdminApiHandler** (COMPLETE - Reference Implementation)
2. **MonitoringApiHandler** - Simple, self-contained (health, version, stats)
3. **CacheApiHandler** - Few dependencies
4. **PromptApiHandler** - Standalone LLM functionality
5. **GraphApiHandler** - Graph-specific operations
6. **SpatialApiHandler** - Geospatial operations
7. **PolicyApiHandler** - Ranger integration
8. **WALApiHandler** - Replication logic
9. **EntityApiHandler** - Core CRUD (more complex)
10. **QueryApiHandler** - Query engine integration
11. **IndexApiHandler** - Index management
12. **VectorApiHandler** - Vector operations
13. **TransactionApiHandler** - Transaction management
14. **TimeSeriesApiHandler** - Time series specific
15. **ChangefeedApiHandler** - CDC with SSE
16. **ContentApiHandler** - Content processing (most complex)

## Testing Strategy

After implementing each handler:

1. **Build verification**:
   ```bash
   # Add handler to CMakeLists.txt first
   mkdir -p build && cd build
   cmake .. && make
   ```

2. **Unit test** (if test infrastructure exists):
   ```bash
   ./build/tests/server_test --gtest_filter="*YourHandler*"
   ```

3. **Integration test** - Start server and test endpoints

## CMakeLists.txt Integration

Add each implemented handler to `cmake/CMakeLists.txt`:

```cmake
# Around line 1021, add:
../src/server/admin_api_handler.cpp        # Already added (reference)
../src/server/monitoring_api_handler.cpp   # Add when implemented
../src/server/cache_api_handler.cpp        # Add when implemented
# ... etc
```

## Common Patterns

### Authorization Checks
```cpp
if (auth_ && auth_->isEnabled()) {
    std::string path_only = std::string(req.target());
    auto qpos = path_only.find('?');
    if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
    // Check access rights
}
```

### JSON Parsing
```cpp
try {
    nlohmann::json body = nlohmann::json::parse(req.body());
    // Process body
} catch (const nlohmann::json::exception& e) {
    return makeErrorResponse(http::status::bad_request, 
        std::string("Invalid JSON: ") + e.what(), req);
}
```

### Tracing
```cpp
auto span = Tracer::startSpan("operation_name");
span.setAttribute("key", "value");
// ... perform operation
span.setStatus(true);  // or false on error
```

> **PII note:** `Tracer::Span::setAttribute(key, string)` automatically redacts PII
> via `PIIRedactionPolicy` before forwarding to OpenTelemetry — no manual redaction
> needed for values passed directly.  However, if you **pre-format** the value into
> a string before calling `setAttribute` (e.g. `fmt::format("user={}", user_email)`),
> the PII is already embedded and cannot be intercepted.  Either pass raw values,
> or pre-redact with `PIIRedactionPolicy::get().redactForLog(value)`.

### PII-Safe Logging
```cpp
#include "utils/logger.h"          // provides THEMIS_INFO / THEMIS_ERROR etc.
// PII-safe: THEMIS_INFO routes through Logger which installs PIIRedactingSink.
// Every log message is auto-scanned and PII substrings are masked before
// reaching console or file sinks – no manual redaction needed.

// ✅ Safe – PII auto-redacted by PIIRedactingSink:
THEMIS_INFO("Request processed: collection={} count={}", collection_name, count);

// ✅ Safe – even if the value contains PII, the sink masks it before output:
THEMIS_INFO("Contact: {}", contact_info);

// ⚠️ Requires care – pre-formatted strings bypass per-token detection:
//   std::string msg = "User " + user_email + " updated record";  // email now embedded
//   THEMIS_INFO("{}", msg);  // whole string is still scanned, but prefer…
//   THEMIS_INFO("User {} updated record", user_email);            // …passing separately

// ✅ Explicit pre-redaction (use for bare spdlog:: calls before Logger::init()):
auto safe = PIIRedactionPolicy::get().redactForLog(user_email);
spdlog::info("Contact: {}", safe);
```

> **PII note:** See [PII Redaction Policy](en/security/PII_REDACTION_POLICY.md)
> for the full developer checklist, PII categories, and configuration.

## Next Steps

1. Review AdminApiHandler implementation
2. Choose next handler from priority list
3. Follow implementation pattern
4. Add to CMakeLists.txt
5. Test and verify
6. Repeat for remaining handlers

## Notes

- Keep handler implementations focused on their specific domain
- Reuse helper methods where possible
- Maintain consistent error handling patterns
- Follow existing code style and conventions
- Add appropriate logging and tracing
- **PII Compliance:** Never log or trace user-supplied data (names, emails, phone numbers, IBANs, etc.) verbatim. Use `THEMIS_INFO` / `THEMIS_ERROR` macros (auto-redacted by `PIIRedactingSink`) and pass values as separate format arguments rather than pre-formatting them into strings. See [PII Redaction Policy](en/security/PII_REDACTION_POLICY.md) for the full checklist.
- Document any deviations from the pattern

## Questions?

Refer to:
- `docs/HTTP_SERVER_REFACTORING.md` - Overall refactoring plan
- `src/server/admin_api_handler.cpp` - Reference implementation
- Existing handlers: `audit_api_handler.cpp`, `pki_api_handler.cpp` - Additional examples
- `docs/en/security/PII_REDACTION_POLICY.md` - PII compliance for logs/traces/metrics
