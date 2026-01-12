---
title: "[REFACTOR] Implement IndexApiHandler - Extract Index Operations from http_server.cpp"
labels: 
  - "priority:P2"
  - "type:refactoring"
  - "area:indexing"
  - "effort:medium"
assignees: []
---

# Refactoring Task: IndexApiHandler Implementation

## Overview

Extract and implement index management operations from `http_server.cpp` into the `IndexApiHandler` class.

## Handler Details

**Class:** `IndexApiHandler`  
**Files:** `include/server/index_api_handler.h`, `src/server/index_api_handler.cpp`  
**Lines to Extract:** ~400 lines  
**Complexity:** Medium

## Endpoints to Implement

1. **POST /indexes/create** - `handleCreate()`
2. **POST /indexes/drop** - `handleDrop()`
3. **POST /indexes/rebuild** - `handleRebuild()`
4. **GET /indexes/stats** - `handleStats()`
5. **POST /indexes/suggestions** - `handleSuggestions()`
6. **GET /indexes/patterns** - `handlePatterns()`

## Key Dependencies

- `std::shared_ptr<RocksDBWrapper> storage_`
- `std::shared_ptr<SecondaryIndex> secondary_index_`
- `std::shared_ptr<AdaptiveIndex> adaptive_index_`
- `std::shared_ptr<AuthMiddleware> auth_`

## Features

- Secondary index management
- Adaptive indexing
- Index statistics
- Query pattern analysis
- Index suggestions

## Documentation

- See `docs/HANDLER_IMPLEMENTATION_GUIDE.md`
- See `docs/INTEGRATION_GUIDE.md`
- Reference: `src/server/admin_api_handler.cpp`

## Acceptance Criteria

- [ ] All 6 handler methods implemented
- [ ] Secondary index operations working
- [ ] Adaptive indexing working
- [ ] Tests pass
- [ ] Integrated into HttpServer

---

**Effort:** Medium | **Priority:** P2 | **Complexity:** Medium
