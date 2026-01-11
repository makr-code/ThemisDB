---
name: LoRA Framework - REST API Endpoints
about: Implement REST API endpoints for LoRA adapter management
title: '[FEATURE] Implement REST API Endpoints for LoRA Framework'
labels: ['enhancement', 'api', 'lora-framework']
assignees: ''
---

## Description

Implement RESTful API endpoints to expose LoRA framework functionality over HTTP, enabling remote management of LLM models and LoRA adapters.

## Motivation

REST API endpoints would:
- Enable remote adapter management via HTTP
- Support web-based admin interfaces
- Allow integration with external tools and services
- Provide programmatic access to LoRA functionality
- Support multi-tenant deployments
- Enable microservices architecture

## Proposed Solution

Extend `src/api/llm_api_handler.cpp` with new LoRA-specific endpoints following the existing ThemisDB API patterns.

## API Endpoints

### LLM Model Management

#### Register Model
```http
POST /api/v1/llm/models
Content-Type: application/json

{
  "model_id": "llama-2-7b",
  "architecture": "llama",
  "parameter_count": 7000000000,
  "quantization": "Q4_K_M",
  "gguf_path": "/models/llama-2-7b-Q4.gguf",
  "description": "Llama 2 7B model with Q4 quantization",
  "metadata": {
    "context_length": 4096,
    "vocab_size": 32000
  }
}

Response: 201 Created
{
  "model_id": "llama-2-7b",
  "status": "registered",
  "timestamp": "2026-01-11T14:00:00Z"
}
```

#### Get Model
```http
GET /api/v1/llm/models/{model_id}

Response: 200 OK
{
  "model_id": "llama-2-7b",
  "architecture": "llama",
  "parameter_count": 7000000000,
  "created_at": "2026-01-11T14:00:00Z",
  "metadata": {...}
}
```

#### List Models
```http
GET /api/v1/llm/models?architecture=llama&limit=10&offset=0

Response: 200 OK
{
  "models": [...],
  "total": 42,
  "limit": 10,
  "offset": 0
}
```

#### Delete Model
```http
DELETE /api/v1/llm/models/{model_id}

Response: 204 No Content
```

### LoRA Adapter Management

#### Create Adapter
```http
POST /api/v1/llm/lora/adapters
Content-Type: application/json

{
  "adapter_id": "themis_help_lora",
  "base_model": "llama-2-7b",
  "task": "documentation_qa",
  "rank": 8,
  "alpha": 16,
  "training_data": {
    "dataset_id": "docs_v1",
    "samples": 10000
  },
  "description": "Documentation Q&A adapter"
}

Response: 201 Created
{
  "adapter_id": "themis_help_lora",
  "version": "v1.0",
  "status": "training",
  "job_id": "job_123"
}
```

#### Get Adapter
```http
GET /api/v1/llm/lora/adapters/{adapter_id}

Response: 200 OK
{
  "adapter_id": "themis_help_lora",
  "base_model": "llama-2-7b",
  "version": "v1.0",
  "status": "ready",
  "metrics": {
    "validation_accuracy": 0.92,
    "training_loss": 0.15
  },
  "created_at": "2026-01-11T14:30:00Z"
}
```

#### Update Adapter
```http
PUT /api/v1/llm/lora/adapters/{adapter_id}
Content-Type: application/json

{
  "additional_training_data": {
    "dataset_id": "feedback_v1",
    "samples": 500
  }
}

Response: 200 OK
{
  "adapter_id": "themis_help_lora",
  "version": "v1.1",
  "status": "training",
  "job_id": "job_124"
}
```

#### Delete Adapter
```http
DELETE /api/v1/llm/lora/adapters/{adapter_id}?version=v1.0

Response: 204 No Content
```

#### List Adapters
```http
GET /api/v1/llm/lora/adapters?base_model=llama-2-7b&status=ready

Response: 200 OK
{
  "adapters": [...],
  "total": 15,
  "limit": 10,
  "offset": 0
}
```

### Adapter Lifecycle

#### Load Adapter
```http
POST /api/v1/llm/lora/adapters/{adapter_id}/load

Response: 200 OK
{
  "adapter_id": "themis_help_lora",
  "status": "loaded",
  "load_time_ms": 45
}
```

#### Unload Adapter
```http
POST /api/v1/llm/lora/adapters/{adapter_id}/unload

Response: 200 OK
{
  "adapter_id": "themis_help_lora",
  "status": "unloaded"
}
```

#### Get Adapter Status
```http
GET /api/v1/llm/lora/adapters/{adapter_id}/status

Response: 200 OK
{
  "adapter_id": "themis_help_lora",
  "is_loaded": true,
  "memory_usage_mb": 32,
  "last_used": "2026-01-11T15:00:00Z"
}
```

### Inference

#### Query with LoRA
```http
POST /api/v1/llm/lora/query
Content-Type: application/json

{
  "model_id": "llama-2-7b",
  "adapter_id": "themis_help_lora",
  "prompt": "How do I enable sharding in ThemisDB?",
  "max_tokens": 500,
  "temperature": 0.7,
  "user_id": "user_42"
}

Response: 200 OK
{
  "response": "To enable sharding in ThemisDB...",
  "model_id": "llama-2-7b",
  "adapter_id": "themis_help_lora",
  "tokens_used": 145,
  "inference_time_ms": 850,
  "audit_id": "audit_789"
}
```

### Health & Monitoring

#### Get Framework Stats
```http
GET /api/v1/llm/lora/stats

Response: 200 OK
{
  "total_adapters": 15,
  "loaded_adapters": 3,
  "cache_hit_rate": 0.842,
  "total_inferences": 1234567,
  "avg_load_time_ms": 450,
  "uptime_seconds": 864000
}
```

#### Health Check
```http
GET /api/v1/llm/lora/health

Response: 200 OK
{
  "status": "healthy",
  "storage": "ok",
  "manager": "ok",
  "training": "ok",
  "checks_passed": 3,
  "checks_failed": 0
}
```

## Implementation Details

### File Structure
```
src/api/
├── llm_api_handler.cpp (extend existing)
├── lora_api_handler.cpp (new)
├── lora_api_handler.h (new)
└── api_middleware.cpp (auth, rate limiting)
```

### Integration with Existing Framework
- Use `LoRAOrchestrator` for CRUD operations
- Use `LoRAAdapterManager` for lifecycle management
- Use `LoRAStorageService` for persistence
- Use `LoRATrainingService` for training operations
- Use `LoRAAuditLogger` for request logging
- Use `LoRAMetrics` for Prometheus metrics

### Authentication & Authorization
- [ ] API key-based authentication
- [ ] JWT token support
- [ ] Role-based access control (RBAC)
- [ ] Rate limiting per user/API key
- [ ] Request/response logging

### Error Handling
- [ ] Standard HTTP status codes
- [ ] Structured error responses (RFC 7807)
- [ ] Validation error details
- [ ] Retry-After headers for rate limits

### API Versioning
- [ ] URL path versioning (`/api/v1/...`)
- [ ] Support for multiple API versions
- [ ] Deprecation warnings in response headers

## Tasks

### Core Implementation
- [ ] Extend `llm_api_handler.cpp` with model endpoints
- [ ] Create `lora_api_handler.cpp` for adapter endpoints
- [ ] Implement request validation and sanitization
- [ ] Add authentication middleware
- [ ] Implement rate limiting
- [ ] Add comprehensive error handling

### API Documentation
- [ ] Create OpenAPI 3.0 specification (`api/openapi.yaml`)
- [ ] Generate interactive API documentation (Swagger UI)
- [ ] Add request/response examples
- [ ] Document authentication flows
- [ ] Add error code reference

### Testing
- [ ] Unit tests for each endpoint
- [ ] Integration tests with real orchestrator
- [ ] Load testing (concurrent requests)
- [ ] Authentication/authorization tests
- [ ] Error case testing

### Documentation
- [ ] Update `LORA_USAGE_EXAMPLES.md` with API examples
- [ ] Add API section to main `README.md`
- [ ] Create `API_REFERENCE.md` with complete endpoint list
- [ ] Add Postman collection
- [ ] Add cURL examples

## Acceptance Criteria

- [ ] All 20+ endpoints implemented and functional
- [ ] Complete OpenAPI 3.0 specification
- [ ] Authentication and authorization working
- [ ] Rate limiting implemented
- [ ] Comprehensive error handling
- [ ] Request validation for all inputs
- [ ] Integration with existing LoRA framework
- [ ] Complete audit logging of API requests
- [ ] Prometheus metrics for API endpoints
- [ ] Unit tests with > 80% coverage
- [ ] Integration tests for critical flows
- [ ] Complete API documentation
- [ ] Postman collection provided

## Related Files

- `src/api/llm_api_handler.cpp` - Existing LLM API handler
- `src/llm/lora_framework/lora_orchestrator.h` - Orchestrator interface
- `src/llm/lora_framework/lora_adapter_manager.h` - Manager interface
- `LORA_USAGE_EXAMPLES.md` - Usage documentation

## References

- REST API best practices: https://restfulapi.net/
- OpenAPI Specification: https://swagger.io/specification/
- RFC 7807 (Problem Details): https://tools.ietf.org/html/rfc7807

## Priority

**High** - Critical for production deployments and external integrations.

## Estimated Effort

**Large** (16-24 hours)
- Core endpoints: 8-10 hours
- Authentication/authorization: 2-3 hours
- Testing: 3-4 hours
- API documentation: 2-3 hours
- Integration: 1-2 hours
- Examples and guides: 2 hours
