---
title: "[REFACTOR] Implement ChangefeedApiHandler - Extract Changefeed Operations from http_server.cpp"
labels: 
  - "priority:P2"
  - "type:refactoring"
  - "area:changefeed"
  - "effort:medium"
assignees: []
---

# Refactoring Task: ChangefeedApiHandler Implementation

## Overview

Extract and implement changefeed/CDC operations from `http_server.cpp` into the `ChangefeedApiHandler` class.

## Handler Details

**Class:** `ChangefeedApiHandler`  
**Files:** `include/server/changefeed_api_handler.h`, `src/server/changefeed_api_handler.cpp`  
**Lines to Extract:** ~400 lines  
**Complexity:** Medium-High

## Endpoints to Implement

1. **GET /changefeed** - `handleGet()`
2. **GET /changefeed/stream** - `handleStream()` (SSE)
3. **GET /changefeed/stats** - `handleStats()`
4. **POST /changefeed/retention** - `handleRetention()`

## Key Dependencies

- `std::shared_ptr<RocksDBWrapper> storage_`
- `std::shared_ptr<Changefeed> changefeed_`
- `std::shared_ptr<SSEManager> sse_manager_`
- `std::shared_ptr<AuthMiddleware> auth_`

## Features

- Change Data Capture (CDC)
- Server-Sent Events (SSE) streaming
- Real-time change notifications
- Retention management

## Documentation

- See `docs/HANDLER_IMPLEMENTATION_GUIDE.md`
- See `docs/INTEGRATION_GUIDE.md`
- Reference: `src/server/admin_api_handler.cpp`

## Acceptance Criteria

- [ ] All 4 handler methods implemented
- [ ] CDC working
- [ ] SSE streaming working
- [ ] Tests pass
- [ ] Integrated into HttpServer

---

**Effort:** Medium | **Priority:** P2 | **Complexity:** Medium-High
