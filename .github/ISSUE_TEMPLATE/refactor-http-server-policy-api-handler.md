---
title: "[REFACTOR] Implement PolicyApiHandler - Extract Ranger Policy Operations from http_server.cpp"
labels: 
  - "priority:P2"
  - "type:refactoring"
  - "area:security"
  - "effort:small"
assignees: []
---

# Refactoring Task: PolicyApiHandler Implementation

## Overview

Extract and implement Apache Ranger policy operations from `http_server.cpp` into the `PolicyApiHandler` class.

## Handler Details

**Class:** `PolicyApiHandler`  
**Files:** `include/server/policy_api_handler.h`, `src/server/policy_api_handler.cpp`  
**Lines to Extract:** ~200 lines  
**Complexity:** Low-Medium

## Endpoints to Implement

1. **POST /policies/ranger/import** - `handleRangerImport()`
2. **GET /policies/ranger/export** - `handleRangerExport()`

## Key Dependencies

- `std::shared_ptr<RocksDBWrapper> storage_`
- `std::shared_ptr<RangerAdapter> ranger_adapter_`
- `std::shared_ptr<AuthMiddleware> auth_`

## Features

- Apache Ranger integration
- Policy import/export
- Access control policies

## Documentation

- See `docs/HANDLER_IMPLEMENTATION_GUIDE.md`
- See `docs/INTEGRATION_GUIDE.md`
- Reference: `src/server/admin_api_handler.cpp`

## Acceptance Criteria

- [ ] Both handler methods implemented
- [ ] Ranger integration working
- [ ] Policy import/export working
- [ ] Tests pass
- [ ] Integrated into HttpServer

---

**Effort:** Small | **Priority:** P2 | **Complexity:** Low-Medium
