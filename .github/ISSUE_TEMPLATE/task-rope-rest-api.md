---
name: "RoPE Enhancement: REST API Endpoints"
about: Implement HTTP endpoints for RoPE configuration and operations
title: '[RoPE] REST API Endpoints for Configuration and Operations'
labels: 'enhancement, priority:P2, area:api, component:rotary-embeddings, effort:small'
assignees: ''
---

## Feature Description

Expose Rotary Position Embeddings (RoPE) functionality through RESTful HTTP endpoints. This enables remote configuration, management, and usage of RoPE features via standard HTTP requests.

## Problem Statement

Current RoPE integration is C++-only and requires direct library usage. Users cannot:
- Configure RoPE remotely via HTTP API
- Query rotation-enabled vector indices via REST
- Manage RoPE configurations across distributed deployments
- Integrate RoPE with web applications or microservices

## Proposed Solution

### API Endpoints

#### 1. Configuration Management

**POST /api/v1/vector-index/{index_name}/rope/config**
```json
{
  "hidden_dim": 768,
  "num_rotation_pairs": 384,
  "base_theta": 10000.0,
  "normalize_after": false
}
```

**Response:**
```json
{
  "status": "success",
  "message": "RoPE configuration enabled for index 'documents'",
  "config": {
    "hidden_dim": 768,
    "num_rotation_pairs": 384,
    "base_theta": 10000.0,
    "normalize_after": false,
    "theta_cache_size": 384
  }
}
```

**GET /api/v1/vector-index/{index_name}/rope/config**

Returns current RoPE configuration or 404 if not enabled.

**DELETE /api/v1/vector-index/{index_name}/rope/config**

Disables RoPE for the specified index.

#### 2. Entity Operations

**POST /api/v1/vector-index/{index_name}/rope/add**
```json
{
  "entity": {
    "id": "doc123",
    "embedding": [0.1, 0.2, ..., 0.768],
    "content": "Document text",
    "metadata": {}
  },
  "vector_field": "embedding",
  "position": 42
}
```

**POST /api/v1/vector-index/{index_name}/rope/add-relational**
```json
{
  "entity": {
    "id": "entity_A",
    "embedding": [0.1, 0.2, ..., 0.768],
    "name": "Entity A"
  },
  "vector_field": "embedding",
  "relation_type": "parent_of"
}
```

#### 3. Search Operations

**POST /api/v1/vector-index/{index_name}/rope/search**
```json
{
  "query": [0.1, 0.2, ..., 0.768],
  "k": 10,
  "position": 100,
  "filters": {
    "field": "category",
    "value": "medical"
  }
}
```

**Response:**
```json
{
  "status": "success",
  "results": [
    {
      "id": "doc123",
      "distance": 0.123,
      "position": 42,
      "metadata": {}
    }
  ],
  "query_time_ms": 5.2,
  "rotation_enabled": true
}
```

#### 4. Batch Operations

**POST /api/v1/vector-index/{index_name}/rope/batch-add**
```json
{
  "entities": [
    {
      "entity": { "id": "doc1", "embedding": [...] },
      "position": 0
    },
    {
      "entity": { "id": "doc2", "embedding": [...] },
      "position": 1
    }
  ],
  "vector_field": "embedding"
}
```

#### 5. Statistics & Monitoring

**GET /api/v1/vector-index/{index_name}/rope/stats**
```json
{
  "enabled": true,
  "config": { "hidden_dim": 768, ... },
  "statistics": {
    "total_rotated_entities": 10000,
    "avg_rotation_time_us": 1.5,
    "relational_rotations": 500,
    "adapters_loaded": ["medical", "legal"]
  }
}
```

## Technical Implementation

### REST Handler (`src/rest/rope_handler.cpp`)

```cpp
class RopeHandler : public RestHandler {
public:
    RopeHandler(std::shared_ptr<VectorIndexManager> vim) 
        : vector_index_mgr_(vim) {}
    
    // Configure RoPE
    Response handleConfigPost(const Request& req) {
        auto index_name = req.getPathParam("index_name");
        auto body = json::parse(req.body());
        
        RotationConfig config;
        config.hidden_dim = body["hidden_dim"];
        config.num_rotation_pairs = body["num_rotation_pairs"];
        config.base_theta = body.value("base_theta", 10000.0);
        config.normalize_after = body.value("normalize_after", false);
        config.computeThetaCache();
        
        auto status = vector_index_mgr_->setRotaryEmbeddingConfig(config);
        
        if (status.ok) {
            return Response::success(json{
                {"status", "success"},
                {"message", "RoPE enabled"},
                {"config", configToJson(config)}
            });
        } else {
            return Response::error(400, status.message);
        }
    }
    
    // Add entity with rotation
    Response handleAddPost(const Request& req) {
        // Parse entity, position, call addEntityWithRotation
    }
    
    // Search with rotation
    Response handleSearchPost(const Request& req) {
        // Parse query, position, call searchWithRotation
    }
    
private:
    std::shared_ptr<VectorIndexManager> vector_index_mgr_;
};
```

### OpenAPI Specification

```yaml
openapi: 3.0.0
info:
  title: ThemisDB RoPE API
  version: 1.0.0
  description: REST API for Rotary Position Embeddings

paths:
  /api/v1/vector-index/{index_name}/rope/config:
    post:
      summary: Enable RoPE for vector index
      parameters:
        - name: index_name
          in: path
          required: true
          schema:
            type: string
      requestBody:
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/RotationConfig'
      responses:
        '200':
          description: RoPE configuration applied
        '400':
          description: Invalid configuration
        '404':
          description: Index not found

components:
  schemas:
    RotationConfig:
      type: object
      required:
        - hidden_dim
        - num_rotation_pairs
      properties:
        hidden_dim:
          type: integer
          minimum: 2
          description: Embedding dimension (must be even)
        num_rotation_pairs:
          type: integer
          minimum: 1
          description: Number of 2D rotation pairs
        base_theta:
          type: number
          default: 10000.0
          description: Base frequency for rotation
        normalize_after:
          type: boolean
          default: false
          description: Apply L2 normalization after rotation
```

## Implementation Considerations

### Dependencies
- Existing ThemisDB REST API framework
- JSON library (nlohmann/json)
- OpenAPI spec generator

### CMakeLists.txt Changes
```cmake
add_library(rest_rope_handler
    src/rest/rope_handler.cpp
)
target_link_libraries(rest_rope_handler
    rotary_embeddings
    vector_index
    rest_framework
    nlohmann_json::nlohmann_json
)
```

### Authentication & Authorization
- Reuse existing ThemisDB auth middleware
- Require `vector:write` permission for configuration
- Require `vector:read` for search operations

### Rate Limiting
- Apply standard rate limits per API endpoint
- Higher limits for batch operations
- Separate quota for configuration vs. operations

### Error Handling
```json
{
  "error": {
    "code": "INVALID_CONFIG",
    "message": "hidden_dim must be even",
    "details": {
      "field": "hidden_dim",
      "value": 127,
      "constraint": "even number"
    }
  }
}
```

## Use Cases

1. **Web Application Integration**: Frontend apps can configure and use RoPE via AJAX
2. **Microservices Architecture**: Services can interact with RoPE without C++ SDK
3. **Cloud Deployments**: Remote configuration across distributed ThemisDB instances
4. **Monitoring & Observability**: Prometheus metrics via REST stats endpoint

## Example Usage

### cURL

```bash
# Enable RoPE
curl -X POST http://localhost:8529/api/v1/vector-index/documents/rope/config \
  -H "Content-Type: application/json" \
  -d '{
    "hidden_dim": 768,
    "num_rotation_pairs": 384,
    "base_theta": 10000.0
  }'

# Add document with rotation
curl -X POST http://localhost:8529/api/v1/vector-index/documents/rope/add \
  -H "Content-Type: application/json" \
  -d '{
    "entity": {
      "id": "doc123",
      "embedding": [0.1, 0.2, ...],
      "content": "Medical document"
    },
    "vector_field": "embedding",
    "position": 42
  }'

# Search with rotation
curl -X POST http://localhost:8529/api/v1/vector-index/documents/rope/search \
  -H "Content-Type: application/json" \
  -d '{
    "query": [0.1, 0.2, ...],
    "k": 10,
    "position": 100
  }'
```

### Python Client

```python
import requests

# Configure RoPE
response = requests.post(
    'http://localhost:8529/api/v1/vector-index/documents/rope/config',
    json={
        'hidden_dim': 768,
        'num_rotation_pairs': 384,
        'base_theta': 10000.0
    }
)

# Search with rotation
results = requests.post(
    'http://localhost:8529/api/v1/vector-index/documents/rope/search',
    json={
        'query': embedding_vector,
        'k': 10,
        'position': 100
    }
).json()
```

## Alternative Solutions

1. **GraphQL API**: More flexible but higher complexity
2. **gRPC API**: Better performance but requires protobuf
3. **WebSocket API**: Real-time but more complex client code

## Related Features

- Vector Index REST API ([#existing_issue])
- OpenAPI Documentation ([#existing_issue])
- API Authentication ([#existing_issue])

## Additional Context

**References:**
- ThemisDB REST API: `src/rest/rest_server.cpp`
- OpenAPI Generator: https://swagger.io/tools/swagger-codegen/
- RESTful API Best Practices: https://restfulapi.net/

**API Versioning:** Use `/api/v1/` prefix for backward compatibility

**Priority:** P2 (Medium) - Enables remote usage and web integration  
**Effort:** 1-2 weeks  
**Complexity:** Low (standard REST API development)

---

**Checklist:**
- [ ] I have searched existing issues to ensure this is not a duplicate
- [ ] I have clearly described the problem this feature solves
- [ ] I have provided a detailed description of the proposed solution
- [ ] I have considered authentication and authorization requirements
- [ ] I have specified OpenAPI/Swagger documentation
- [ ] I have provided example usage in multiple languages
