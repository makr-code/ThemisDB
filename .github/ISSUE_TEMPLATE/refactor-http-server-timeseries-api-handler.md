---
title: "[REFACTOR] Implement TimeSeriesApiHandler - Extract Time Series Operations from http_server.cpp"
labels: 
  - "priority:P2"
  - "type:refactoring"
  - "area:timeseries"
  - "effort:medium"
assignees: []
---

# Refactoring Task: TimeSeriesApiHandler Implementation

## Overview

Extract and implement time series operations from `http_server.cpp` into the `TimeSeriesApiHandler` class.

## Handler Details

**Class:** `TimeSeriesApiHandler`  
**Files:** `include/server/timeseries_api_handler.h`, `src/server/timeseries_api_handler.cpp`  
**Lines to Extract:** ~350 lines  
**Complexity:** Medium

## Endpoints to Implement

1. **POST /timeseries/put** - `handlePut()`
2. **POST /timeseries/query** - `handleQuery()`
3. **POST /timeseries/aggregate** - `handleAggregate()`
4. **GET /timeseries/config** - `handleConfig()`
5. **POST /timeseries/retention** - `handleRetention()`

## Key Dependencies

- `std::shared_ptr<RocksDBWrapper> storage_`
- `std::shared_ptr<TimeSeriesStore> ts_store_`
- `std::shared_ptr<AggregateManager> agg_manager_`
- `std::shared_ptr<AuthMiddleware> auth_`

## Features

- Gorilla compression
- Continuous aggregates
- Retention policies
- Time-based queries

## Documentation

- See `docs/HANDLER_IMPLEMENTATION_GUIDE.md`
- See `docs/INTEGRATION_GUIDE.md`
- Reference: `src/server/admin_api_handler.cpp`

## Acceptance Criteria

- [ ] All 5 handler methods implemented
- [ ] Time series operations working
- [ ] Compression working
- [ ] Aggregates working
- [ ] Tests pass
- [ ] Integrated into HttpServer

---

**Effort:** Medium | **Priority:** P2 | **Complexity:** Medium
