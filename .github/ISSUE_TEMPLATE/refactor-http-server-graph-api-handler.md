---
title: "[REFACTOR] Implement GraphApiHandler - Extract Graph Operations from http_server.cpp"
labels: 
  - "priority:P2"
  - "type:refactoring"
  - "area:graph"
  - "effort:small"
  - "good first issue"
assignees: []
---

# Refactoring Task: GraphApiHandler Implementation

## Overview

Extract and implement graph traversal operations from `http_server.cpp` into the `GraphApiHandler` class.

## Handler Details

**Class:** `GraphApiHandler`  
**Files:** `include/server/graph_api_handler.h`, `src/server/graph_api_handler.cpp`  
**Lines to Extract:** ~150 lines  
**Complexity:** Low-Medium  
**Recommended:** Good candidate for early implementation

## Endpoints to Implement

1. **POST /graph/traverse** - `handleTraverse()`
2. **POST /graph/edges** - `handleEdgeCreate()`
3. **DELETE /graph/edges** - `handleEdgeDelete()`

## Key Dependencies

- `std::shared_ptr<RocksDBWrapper> storage_`
- `std::shared_ptr<GraphIndex> graph_index_`
- `std::shared_ptr<AuthMiddleware> auth_`

## Features

- Graph traversal
- Property graphs
- Edge management

## Documentation

- See `docs/HANDLER_IMPLEMENTATION_GUIDE.md`
- See `docs/INTEGRATION_GUIDE.md`
- Reference: `src/server/admin_api_handler.cpp`

## Acceptance Criteria

- [ ] All 3 handler methods implemented
- [ ] Graph traversal working
- [ ] Edge operations working
- [ ] Tests pass
- [ ] Integrated into HttpServer

---

**Effort:** Small | **Priority:** P2 | **Complexity:** Low-Medium | **Good First Issue**
