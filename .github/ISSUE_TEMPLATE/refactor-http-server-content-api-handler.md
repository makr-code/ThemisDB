---
title: "[REFACTOR] Implement ContentApiHandler - Extract Content Operations from http_server.cpp"
labels: 
  - "priority:P2"
  - "type:refactoring"
  - "area:content"
  - "effort:x-large"
assignees: []
---

# Refactoring Task: ContentApiHandler Implementation

## Overview

Extract and implement content processing operations from `http_server.cpp` into the `ContentApiHandler` class.

## Handler Details

**Class:** `ContentApiHandler`  
**Files:** `include/server/content_api_handler.h`, `src/server/content_api_handler.cpp`  
**Lines to Extract:** ~900 lines (largest handler)  
**Complexity:** High

## Endpoints to Implement

1. **POST /content/import** - `handleImport()`
2. **GET /content** - `handleGet()`
3. **POST /content/search/hybrid** - `handleSearchHybrid()`
4. **POST /content/search/fusion** - `handleSearchFusion()`
5. **POST /content/search/fulltext** - `handleSearchFulltext()`
6. **GET /content/config** - `handleConfig()`

## Key Dependencies

- `std::shared_ptr<RocksDBWrapper> storage_`
- `std::shared_ptr<ContentManager> content_manager_`
- `std::shared_ptr<ContentProcessor> content_processor_`
- `std::shared_ptr<AuthMiddleware> auth_`

## Features

- Multi-format content ingestion
- Text chunking
- Embedding generation
- Hybrid search (vector + fulltext)
- Fusion search algorithms

## Documentation

- See `docs/HANDLER_IMPLEMENTATION_GUIDE.md`
- See `docs/INTEGRATION_GUIDE.md`
- Reference: `src/server/admin_api_handler.cpp`

## Acceptance Criteria

- [ ] All 6 handler methods implemented
- [ ] Content import working
- [ ] Search operations working
- [ ] Tests pass
- [ ] Integrated into HttpServer

---

**Effort:** X-Large | **Priority:** P2 | **Complexity:** High
