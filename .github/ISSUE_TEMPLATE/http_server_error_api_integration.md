---
name: HTTP Server Integration for Error API
about: Register Error API routes in HTTP server
title: '[TASK] HTTP Server Integration for Error API Endpoints'
labels: 'enhancement, error-handling, http-server, priority-low'
assignees: ''
---

# HTTP Server Integration for Error API Endpoints

## 📋 Summary

Integrate the Error API handlers into the HTTP server by registering routes for error introspection endpoints. The handlers are already implemented in `src/server/error_api_handler.cpp` and ready for integration.

## 🎯 Objectives

Register the following REST API endpoints in the HTTP server:

1. `GET /api/v1/errors` - List all registered errors
2. `GET /api/v1/errors/:code` - Get specific error by code
3. `GET /api/v1/errors/categories` - List all error categories
4. `GET /api/v1/errors/search` - Search errors by keyword

## 📝 Implementation Details

### Step 1: Review Existing HTTP Server Structure

**File: `src/server/http_server.cpp`**

Understand how other API handlers are registered (e.g., LLM API, Buffer API, etc.).

### Step 2: Create Error API Handler Instance

Add to HTTP server initialization:

```cpp
#include "server/error_api_handler.h"

// In HttpServer constructor or registerRoutes method
auto error_handler = std::make_shared<ErrorApiHandler>();
```

### Step 3: Register Routes

Add route registrations (adapt to existing HTTP server framework):

```cpp
// List all errors
server_.Get("/api/v1/errors", [error_handler](const Request& req, Response& res) {
    error_handler->handleGetErrors(req, res);
});

// Get specific error by code
server_.Get("/api/v1/errors/:code", [error_handler](const Request& req, Response& res) {
    error_handler->handleGetError(req, res);
});

// Get all categories
server_.Get("/api/v1/errors/categories", [error_handler](const Request& req, Response& res) {
    error_handler->handleGetCategories(req, res);
});

// Search errors
server_.Get("/api/v1/errors/search", [error_handler](const Request& req, Response& res) {
    error_handler->handleSearchErrors(req, res);
});
```

### Step 4: Handle Request/Response Types

The `ErrorApiHandler` uses simplified `Request` and `Response` structs. You may need to create adapter functions to convert between the HTTP server's request/response types and the handler's types.

**Example adapter:**

```cpp
void handleErrorsRoute(const HttpRequest& http_req, HttpResponse& http_res) {
    // Convert HttpRequest to simplified Request
    server::Request req;
    req.method = http_req.method;
    req.path = http_req.path;
    req.params = http_req.params;
    req.query = http_req.query_params;
    req.body = http_req.json_body;
    
    // Call handler
    server::Response res;
    error_handler->handleGetErrors(req, res);
    
    // Convert Response back to HttpResponse
    http_res.status = res.status_code;
    http_res.set_content(res.body.dump(), "application/json");
}
```

### Step 5: Add Middleware/Authentication (Optional)

Decide if error endpoints should:
- Require authentication
- Be rate-limited
- Have CORS enabled

## 📋 API Endpoint Specifications

### 1. GET /api/v1/errors

**Description:** List all registered errors

**Query Parameters:**
- `category` (optional): Filter by category (e.g., "LLM", "LoRA", "MCP")

**Response:**
```json
{
  "status": "success",
  "total_errors": 20,
  "categories": ["Storage", "LLM", "LoRA", "MCP", "Schema", "Network"],
  "errors": [
    {
      "code": 2000,
      "category": "LLM",
      "severity": "Error",
      "message_template": "Model file not found: {}",
      "cause": "...",
      "solution": "...",
      "related_docs": [...],
      "keywords": [...]
    }
  ]
}
```

### 2. GET /api/v1/errors/:code

**Description:** Get details for a specific error code

**URL Parameters:**
- `code`: Error code (e.g., 2000, 2004)

**Response:**
```json
{
  "status": "success",
  "error": {
    "code": 2004,
    "category": "LLM",
    "severity": "Critical",
    "message_template": "GPU out of memory: {} MB required, {} MB available",
    "cause": "Insufficient GPU VRAM...",
    "solution": "1. Use a smaller model...",
    "related_docs": ["/docs/llm/gpu_management.md"],
    "keywords": ["gpu", "oom", "vram"]
  }
}
```

### 3. GET /api/v1/errors/categories

**Description:** List all error categories

**Response:**
```json
{
  "status": "success",
  "categories": ["Storage", "LLM", "LoRA", "MCP", "Schema", "Network"],
  "count": 6
}
```

### 4. GET /api/v1/errors/search

**Description:** Search errors by keyword

**Query Parameters:**
- `q` (required): Search query (e.g., "gpu", "model", "lora")

**Response:**
```json
{
  "status": "success",
  "query": "gpu",
  "errors": [...],
  "count": 5
}
```

## ✅ Acceptance Criteria

- [ ] All 4 endpoints registered in HTTP server
- [ ] Request/Response adapters implemented (if needed)
- [ ] Endpoints return correct JSON responses
- [ ] HTTP status codes are appropriate (200, 404, 400, 500)
- [ ] Query parameters are properly parsed
- [ ] URL parameters are properly extracted
- [ ] Error handling for invalid requests
- [ ] CORS headers configured (if applicable)
- [ ] Authentication/authorization implemented (if required)
- [ ] API documentation updated
- [ ] Endpoints tested with curl or Postman
- [ ] Integration tests added

## 🧪 Testing

### Manual Testing

```bash
# List all errors
curl http://localhost:8080/api/v1/errors

# Filter by category
curl http://localhost:8080/api/v1/errors?category=LLM

# Get specific error
curl http://localhost:8080/api/v1/errors/2004

# List categories
curl http://localhost:8080/api/v1/errors/categories

# Search by keyword
curl "http://localhost:8080/api/v1/errors/search?q=gpu"

# Test error cases
curl http://localhost:8080/api/v1/errors/9998  # Non-existent code
curl "http://localhost:8080/api/v1/errors/search?q="  # Empty query
```

### Integration Tests

Add tests to verify:
- Successful requests return 200
- Invalid error codes return 404
- Missing parameters return 400
- JSON structure is correct
- Filtering and searching work as expected

## 📚 Related Documents

- **Error API Handler:** `include/server/error_api_handler.h`, `src/server/error_api_handler.cpp`
- **HTTP Server:** `include/server/http_server.h`, `src/server/http_server.cpp`
- **Design Doc:** `docs/research/ERROR_AWARENESS_AND_INTROSPECTION.md` (Phase 2: REST API)

## 🔄 Dependencies

- Error Registry PR must be merged first
- HTTP server must be functional

## 📊 Estimated Effort

- **Development:** 2-3 hours
- **Testing:** 1-2 hours
- **Documentation:** 1 hour
- **Code Review:** 30 minutes

**Total:** ~4-7 hours

## 🎯 Success Metrics

- All endpoints accessible via HTTP
- API responses match specification
- Error handling is robust
- Performance is acceptable (<100ms response time)
- Documentation is complete

---

**Priority:** 🔵 LOW  
**Blocked by:** Error Registry PR merge  
**Type:** Integration task
