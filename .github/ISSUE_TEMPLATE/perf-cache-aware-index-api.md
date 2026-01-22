---
name: ⚡ Performance: REST API for Cache-Aware Index Suggestions
about: Expose cache-aware index suggestions via REST API endpoints
title: "[PERF] REST API for Cache-Aware Index Suggestions"
labels: priority:P2, type:performance, area:api, effort:medium, phase:integration
assignees: ''
---

## 📊 Performance Enhancement - Phase 2 Follow-up

**Current Status:** Cache-aware indexing implemented, REST API exposure pending  
**Priority:** P2 (Medium)  
**Effort:** 1 week  
**Target Version:** v1.4.1  
**Parent PR:** #XXX (Scaling Optimizations to 10B Records)  
**Related Files:**
- `include/index/adaptive_index.h`
- `src/index/adaptive_index.cpp`
- `src/server/index_api_handler.h`

---

## 📋 Problem Description

The cache-aware adaptive index selection system has been implemented with comprehensive functionality:
- `AdaptiveIndexManager` with cache metrics tracking
- `SelectivityAnalyzer::analyzeCacheAware()` for L3 cache fit ratio estimation
- `IndexSuggestionEngine::generateCacheAwareIndexes()` for smart index recommendations

However, these features are **not exposed via REST API**, limiting their usability for:
- Database administrators who want index recommendations
- Monitoring tools that need index optimization insights
- Automated index tuning systems

**Performance Impact:** Missing **+35% throughput improvement** due to lack of visibility and actionable recommendations.

---

## 🎯 Requirements

### Must Have (P2)

- [ ] **REST API Endpoints**
  
  **GET /api/v1/index/suggestions**
  - Query parameters: `collection`, `min_score`, `limit`, `cache_aware`
  - Returns list of index suggestions with scores
  - Supports both standard and cache-aware suggestions
  
  **GET /api/v1/index/patterns**
  - Query parameters: `collection`, `limit`
  - Returns query patterns with cache metrics
  - Shows which queries benefit most from indexing
  
  **GET /api/v1/index/cache-analysis**
  - Query parameters: `collection`, `field`
  - Returns cache fit analysis for specific field
  - Shows L3 cache fit ratio and estimated miss rate
  
  **POST /api/v1/index/analyze**
  - Body: `{ "collection": "users", "field": "email", "cache_aware": true }`
  - Returns detailed selectivity and cache analysis
  - Recommendations for index type and configuration

- [ ] **Response Format**
  ```json
  {
    "suggestions": [
      {
        "collection": "users",
        "field": "email",
        "index_type": "hash",
        "score": 0.85,
        "reason": "High selectivity (95%), fits in L3 cache",
        "queries_affected": 15420,
        "estimated_speedup_ms": 2340,
        "cache_metrics": {
          "l3_cache_fit_ratio": 1.0,
          "estimated_cache_miss_rate": 0.0,
          "current_cache_hit_rate": 0.65
        }
      }
    ],
    "metadata": {
      "total_suggestions": 5,
      "target_cache_hit_rate": 0.70,
      "l3_cache_size_mb": 20
    }
  }
  ```

- [ ] **API Handler Implementation**
  - Extend `IndexApiHandler` with new endpoints
  - Input validation and error handling
  - Authentication/authorization support
  - Rate limiting for expensive operations

### Should Have (P2)

- [ ] **API Documentation**
  - OpenAPI/Swagger spec for new endpoints
  - Example requests and responses
  - Integration guide for monitoring tools

- [ ] **Monitoring Integration**
  - Metrics for API usage
  - Performance tracking (response times)
  - Error rate monitoring

### Nice to Have (P3)

- [ ] **Advanced Features**
  - Bulk analysis endpoint (multiple collections)
  - Historical trend data (cache hit rate over time)
  - "What-if" analysis (simulate index impact)
  - Export recommendations as SQL DDL

---

## 🔧 Implementation Plan

### Phase 1: Core Endpoints (Days 1-3)
1. Add REST endpoints to `IndexApiHandler`
2. Implement request/response serialization
3. Wire up to `AdaptiveIndexManager`
4. Basic error handling

### Phase 2: Advanced Features (Days 3-4)
1. Add cache analysis endpoint
2. Implement bulk analysis
3. Add filtering and sorting options

### Phase 3: Documentation & Testing (Day 5)
1. Write OpenAPI spec
2. Add integration tests
3. Update API documentation
4. Performance testing

---

## 📝 Implementation Notes

### Code Locations

**API Handler Extension:**
```cpp
// src/server/index_api_handler.cpp
void IndexApiHandler::handleGetSuggestions(const HttpRequest& req, HttpResponse& res) {
    // Parse query parameters
    std::string collection = req.getQueryParam("collection");
    double min_score = std::stod(req.getQueryParam("min_score", "0.5"));
    size_t limit = std::stoul(req.getQueryParam("limit", "10"));
    bool cache_aware = req.getQueryParam("cache_aware", "true") == "true";
    
    // Get suggestions
    std::vector<IndexSuggestion> suggestions;
    if (cache_aware) {
        suggestions = index_manager_->generateCacheAwareIndexes(
            collection, 0.70f, min_score, limit
        );
    } else {
        suggestions = index_manager_->getSuggestions(
            collection, min_score, limit
        );
    }
    
    // Serialize response
    json response = serializeSuggestions(suggestions);
    res.setBody(response.dump());
    res.setStatus(200);
}
```

**Endpoint Registration:**
```cpp
// Register new endpoints
server.addRoute("GET", "/api/v1/index/suggestions", 
                [this](auto& req, auto& res) { handleGetSuggestions(req, res); });
server.addRoute("GET", "/api/v1/index/patterns",
                [this](auto& req, auto& res) { handleGetPatterns(req, res); });
server.addRoute("GET", "/api/v1/index/cache-analysis",
                [this](auto& req, auto& res) { handleCacheAnalysis(req, res); });
server.addRoute("POST", "/api/v1/index/analyze",
                [this](auto& req, auto& res) { handleAnalyze(req, res); });
```

---

## ✅ Testing Requirements

- [ ] Unit tests for API handlers
- [ ] Integration tests with HTTP client
- [ ] Performance tests (response time < 100ms)
- [ ] Error handling tests (invalid inputs)
- [ ] Authentication/authorization tests
- [ ] Load tests (concurrent requests)

### API Test Examples

```bash
# Get cache-aware suggestions
curl "http://localhost:8080/api/v1/index/suggestions?collection=users&cache_aware=true"

# Get query patterns
curl "http://localhost:8080/api/v1/index/patterns?collection=orders&limit=20"

# Analyze specific field
curl -X POST http://localhost:8080/api/v1/index/analyze \
  -H "Content-Type: application/json" \
  -d '{"collection": "products", "field": "category_id", "cache_aware": true}'
```

---

## 📚 References

- Parent PR: Scaling Optimizations to 10B Records
- Implementation: `include/index/adaptive_index.h`
- Configuration: `config/scaling_optimizations.yaml`
- Existing API: `include/server/index_api_handler.h`

---

## ⚠️ Risks & Considerations

1. **Performance Impact**
   - Analysis can be expensive for large collections
   - Mitigation: Add caching, rate limiting, async processing

2. **API Versioning**
   - New endpoints should be properly versioned
   - Consider backward compatibility

3. **Security**
   - Ensure proper authentication/authorization
   - Validate all inputs to prevent injection attacks
   - Rate limit to prevent DoS

---

## 🎯 Success Criteria

- [ ] All 4 core endpoints implemented and functional
- [ ] API response time < 100ms for typical requests
- [ ] OpenAPI documentation complete
- [ ] Integration tests pass (>95% coverage)
- [ ] Successfully integrated with monitoring tools
- [ ] User documentation with examples published
