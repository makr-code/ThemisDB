---
name: Error Code Migration - Phase 3 (Medium Priority)
about: Migrate medium priority error logging locations to structured error codes
title: '[TASK] Error Code Migration - Phase 3: Medium Priority Errors'
labels: 'enhancement, error-handling, refactoring, priority-low'
assignees: ''
---

# Error Code Migration - Phase 3: Medium Priority

## 📋 Summary

Migrate medium priority error logging locations from `spdlog::error()` to the new structured error code system. This is Phase 3 focusing on MCP errors and remaining LLM/LoRA errors.

## 🎯 Objectives

Migrate the following medium priority error locations:

### 🟢 MEDIUM (Mid-term - Phase 3)

1. **MCP Transport Errors** - Important for MCP functionality (3 locations)
2. **Remaining LLM/LoRA Errors** - Various edge cases

**Total locations in Phase 3:** ~3 primary error logging statements

## 📝 Detailed Migration List

### 1. MCP Transport Failed (ERR_MCP_TRANSPORT_FAILED - 3000)

**File: `src/server/mcp_server.cpp`**

| Line | Current Code | Target Code |
|------|--------------|-------------|
| 193 | `spdlog::error("Error handling MCP request: {}", e.what());` | `errors::logError(ErrorCode::ERR_MCP_TRANSPORT_FAILED, e.what());` |
| 1656 | `spdlog::error("Error handling WebSocket message from session {}: {}", session_id, e.what());` | `errors::logError(ErrorCode::ERR_MCP_TRANSPORT_FAILED, e.what());` |

### 2. New Error Code Required: ERR_MCP_STDIO_INIT_FAILED (3004)

**File: `src/server/mcp_server.cpp`**

| Line | Current Code | Proposed Target |
|------|--------------|-----------------|
| 1312 | `spdlog::error("Failed to get stdin handle");` | `errors::logError(ErrorCode::ERR_MCP_STDIO_INIT_FAILED, "stdin handle");` |

**Note:** This requires defining `ERR_MCP_STDIO_INIT_FAILED` in the error registry first (see separate issue).

## 🔧 Implementation Steps

### Step 1: Define New Error Code (if needed)

Add to `include/utils/error_registry.h`:
```cpp
// Additional MCP Errors
ERR_MCP_STDIO_INIT_FAILED = 3004,
```

Add metadata in `src/utils/error_registry.cpp`:
```cpp
registerError({
    ErrorCode::ERR_MCP_STDIO_INIT_FAILED,
    "MCP",
    "Error",
    "Failed to initialize stdio transport: {}",
    "The MCP stdio transport could not be initialized.",
    "1. Check if stdin/stdout are available\n"
    "2. Verify process has proper file descriptor access\n"
    "3. Check if running in appropriate environment (not detached)",
    {"/docs/mcp/troubleshooting.md"},
    {"mcp", "stdio", "init", "failed"}
});
```

### Step 2: Add Required Include

Add to affected files:
```cpp
#include "utils/error_registry.h"
```

### Step 3: Replace spdlog::error Calls

Follow the migration pattern from Phase 1 and 2.

## ✅ Acceptance Criteria

- [ ] New error code `ERR_MCP_STDIO_INIT_FAILED` defined (if applicable)
- [ ] All medium priority error locations migrated
- [ ] Include statements added to all affected files
- [ ] Code compiles without errors or warnings
- [ ] Existing unit tests pass
- [ ] Error codes are visible in logs
- [ ] MCP tool `get_error_info` returns correct metadata

## 📚 Related Documents

- **Migration Guide:** `docs/ERROR_CODE_MIGRATION_LIST.md`
- **Error Registry:** `include/utils/error_registry.h`
- **Phase 1 Issue:** Link to Phase 1
- **Phase 2 Issue:** Link to Phase 2

## 🔄 Dependencies

- Phase 2 must be completed and merged
- New error codes issue (if separate)

## 📊 Estimated Effort

- **Development:** 1 hour
- **Testing:** 30 minutes
- **Code Review:** 30 minutes

**Total:** ~2 hours

---

**Priority:** 🟢 MEDIUM  
**Phase:** 3 of 4  
**Blocked by:** Phase 2 completion
