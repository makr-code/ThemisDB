---
title: "[REFACTOR] Implement PromptApiHandler - Extract Prompt Template Operations from http_server.cpp"
labels: 
  - "priority:P2"
  - "type:refactoring"
  - "area:llm"
  - "effort:small"
  - "good first issue"
assignees: []
---

# Refactoring Task: PromptApiHandler Implementation

## Overview

Extract and implement LLM prompt template operations from `http_server.cpp` into the `PromptApiHandler` class.

## Handler Details

**Class:** `PromptApiHandler`  
**Files:** `include/server/prompt_api_handler.h`, `src/server/prompt_api_handler.cpp`  
**Lines to Extract:** ~250 lines  
**Complexity:** Low  
**Recommended:** Good candidate for early implementation

## Endpoints to Implement

1. **POST /prompts** - `handleCreate()`
2. **GET /prompts/{id}** - `handleGet()`
3. **GET /prompts** - `handleList()`
4. **PUT /prompts/{id}** - `handleUpdate()`

## Key Dependencies

- `std::shared_ptr<RocksDBWrapper> storage_`
- `std::shared_ptr<PromptManager> prompt_manager_`
- `std::shared_ptr<AuthMiddleware> auth_`

## Features

- LLM prompt management
- Template CRUD operations
- Variable substitution

## Documentation

- See `docs/HANDLER_IMPLEMENTATION_GUIDE.md`
- See `docs/INTEGRATION_GUIDE.md`
- Reference: `src/server/admin_api_handler.cpp`

## Acceptance Criteria

- [ ] All 4 handler methods implemented
- [ ] Prompt CRUD working
- [ ] Template management working
- [ ] Tests pass
- [ ] Integrated into HttpServer

---

**Effort:** Small | **Priority:** P2 | **Complexity:** Low | **Good First Issue**
