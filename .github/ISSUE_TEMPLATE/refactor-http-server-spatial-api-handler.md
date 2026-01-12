---
title: "[REFACTOR] Implement SpatialApiHandler - Extract Spatial Operations from http_server.cpp"
labels: 
  - "priority:P2"
  - "type:refactoring"
  - "area:spatial"
  - "effort:small"
  - "good first issue"
assignees: []
---

# Refactoring Task: SpatialApiHandler Implementation

## Overview

Extract and implement spatial/geospatial operations from `http_server.cpp` into the `SpatialApiHandler` class.

## Handler Details

**Class:** `SpatialApiHandler`  
**Files:** `include/server/spatial_api_handler.h`, `src/server/spatial_api_handler.cpp`  
**Lines to Extract:** ~200 lines  
**Complexity:** Low-Medium  
**Recommended:** Good candidate for early implementation

## Endpoints to Implement

1. **POST /spatial/index/create** - `handleIndexCreate()`
2. **POST /spatial/index/rebuild** - `handleIndexRebuild()`
3. **GET /spatial/index/stats** - `handleIndexStats()`
4. **GET /spatial/metrics** - `handleMetrics()`

## Key Dependencies

- `std::shared_ptr<RocksDBWrapper> storage_`
- `std::shared_ptr<SpatialIndex> spatial_index_`
- `std::shared_ptr<AuthMiddleware> auth_`

## Features

- R-tree indexing
- Geospatial queries
- Index statistics
- Spatial metrics

## Documentation

- See `docs/HANDLER_IMPLEMENTATION_GUIDE.md`
- See `docs/INTEGRATION_GUIDE.md`
- Reference: `src/server/admin_api_handler.cpp`

## Acceptance Criteria

- [ ] All 4 handler methods implemented
- [ ] Spatial operations working
- [ ] R-tree indexing working
- [ ] Tests pass
- [ ] Integrated into HttpServer

---

**Effort:** Small | **Priority:** P2 | **Complexity:** Low-Medium | **Good First Issue**
