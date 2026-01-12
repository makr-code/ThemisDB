---
title: "[REFACTOR] Implement QueryApiHandler - Extract Query Operations from http_server.cpp"
labels: 
  - "priority:P2"
  - "type:refactoring"
  - "area:query"
  - "effort:x-large"
assignees: []
---

# Refactoring Task: QueryApiHandler Implementation

## Overview

Extract and implement query operations from `http_server.cpp` into the `QueryApiHandler` class. This is part of the http_server.cpp refactoring initiative.

## Handler Details

**Class:** `QueryApiHandler`  
**Files:** `include/server/query_api_handler.h`, `src/server/query_api_handler.cpp`  
**Lines to Extract:** ~850 lines  
**Complexity:** High (complex query processing logic)

## Endpoints to Implement

### 1. POST /query
- **Method:** `handleQuery(const http::request<http::string_body>& req)`
- **Source:** `http_server.cpp` (`handleQuery`)
- **Features:** Basic query execution

### 2. POST /query/aql
- **Method:** `handleQueryAql(const http::request<http::string_body>& req)`
- **Source:** `http_server.cpp` ~line 5990 (`handleQueryAql`)
- **Features:** AQL (Advanced Query Language) processing
- **Note:** Large handler (~573 lines)

### 3. POST /query/enhanced
- **Method:** `handleEnhancedQuery(const http::request<http::string_body>& req)`
- **Source:** `http_server.cpp` (`handleEnhancedQuery`)
- **Features:** LLM-enhanced query processing

## Key Dependencies

- `std::shared_ptr<RocksDBWrapper> storage_`
- `std::shared_ptr<SecondaryIndex> secondary_index_`
- `std::shared_ptr<QueryEngine> query_engine_`
- `std::shared_ptr<QueryOptimizer> query_optimizer_`
- `std::shared_ptr<SemanticCache> semantic_cache_`
- `std::shared_ptr<LLMStore> llm_store_`
- `std::shared_ptr<PromptManager> prompt_manager_`
- `std::shared_ptr<AuthMiddleware> auth_`

## Implementation Notes

- `handleQueryAql` is one of the largest handlers (573 lines)
- Complex query optimization logic
- Semantic caching integration
- LLM enhancement for natural language queries

## Documentation

- See `docs/HANDLER_IMPLEMENTATION_GUIDE.md`
- See `docs/INTEGRATION_GUIDE.md`
- Reference: `src/server/admin_api_handler.cpp`

## Acceptance Criteria

- [ ] All 3 handler methods implemented
- [ ] Query engine integration working
- [ ] Semantic cache integration working
- [ ] LLM enhancement working
- [ ] Tests pass
- [ ] Integrated into HttpServer

---

**Effort:** X-Large | **Priority:** P2 | **Complexity:** High
