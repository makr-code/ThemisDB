# ThemisDB LoRA Framework API Reference

**Version:** 1.5.0-dev  
**Last Updated:** 2026-04-06

Complete REST API reference for the LoRA (Low-Rank Adaptation) framework in ThemisDB.

## Base URL and Ports

Default base URL:
```
http://localhost:8080
```

> **📖 Port Reference:** ThemisDB uses different ports depending on deployment platform. See [docs/de/deployment/PORT_REFERENCE.md](../de/deployment/PORT_REFERENCE.md) for complete mapping.

**Default Ports:**
- `8080` - HTTP/REST API (this documentation)
- `18765` - Binary Wire Protocol/gRPC
- `4318` - OpenTelemetry/Prometheus metrics

## Table of Contents

- [Authentication](#authentication)
- [Model Management](#model-management)
- [Adapter Management](#adapter-management)
- [Adapter Lifecycle](#adapter-lifecycle)
- [Inference](#inference)
- [Monitoring](#monitoring)
- [Error Handling](#error-handling)
- [Rate Limiting](#rate-limiting)

## Authentication

All API endpoints require JWT Bearer Token authentication.

### Request Headers

```http
Authorization: Bearer <your-jwt-token>
Content-Type: application/json
```

### Obtaining a Token

Contact your ThemisDB administrator to obtain a JWT token. Tokens include user information and permissions.

## Model Management

### Register Model

Register a new LLM model in the system.

**Endpoint:** `POST /api/v1/llm/models`

**Request:**
```json
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
```

**Response:** `201 Created`
```json
{
  "model_id": "llama-2-7b",
  "status": "registered",
  "timestamp": "2026-01-11T14:00:00Z"
}
```

**cURL Example:**
```bash
curl -X POST http://localhost:8080/api/v1/llm/models \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "llama-2-7b",
    "architecture": "llama",
    "parameter_count": 7000000000,
    "quantization": "Q4_K_M",
    "gguf_path": "/models/llama-2-7b-Q4.gguf"
  }'
```

### Get Model

Retrieve details about a specific model.

**Endpoint:** `GET /api/v1/llm/models/{model_id}`

**Response:** `200 OK`
```json
{
  "model_id": "llama-2-7b",
  "architecture": "llama",
  "parameter_count": 7000000000,
  "created_at": "2026-01-11T14:00:00Z",
  "metadata": {}
}
```

**cURL Example:**
```bash
curl -X GET http://localhost:8080/api/v1/llm/models/llama-2-7b \
  -H "Authorization: Bearer $TOKEN"
```

### List Models

List all registered models with optional filters.

**Endpoint:** `GET /api/v1/llm/models`

**Query Parameters:**
- `architecture` (optional): Filter by architecture
- `limit` (optional, default: 10): Maximum results
- `offset` (optional, default: 0): Pagination offset

**Response:** `200 OK`
```json
{
  "models": [
    {
      "model_id": "llama-2-7b",
      "architecture": "llama",
      "parameter_count": 7000000000
    }
  ],
  "total": 42,
  "limit": 10,
  "offset": 0
}
```

**cURL Example:**
```bash
curl -X GET "http://localhost:8080/api/v1/llm/models?architecture=llama&limit=10" \
  -H "Authorization: Bearer $TOKEN"
```

### Delete Model

Delete a model from the registry.

**Endpoint:** `DELETE /api/v1/llm/models/{model_id}`

**Response:** `204 No Content`

**cURL Example:**
```bash
curl -X DELETE http://localhost:8080/api/v1/llm/models/llama-2-7b \
  -H "Authorization: Bearer $TOKEN"
```

## Adapter Management

### Create Adapter

Create a new LoRA adapter through training.

**Endpoint:** `POST /api/v1/llm/lora/adapters`

**Request:**
```json
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
```

**Response:** `201 Created`
```json
{
  "adapter_id": "themis_help_lora",
  "version": "v1.0",
  "status": "training",
  "job_id": "job_123"
}
```

**cURL Example:**
```bash
curl -X POST http://localhost:8080/api/v1/llm/lora/adapters \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "adapter_id": "themis_help_lora",
    "base_model": "llama-2-7b",
    "task": "documentation_qa",
    "rank": 8,
    "alpha": 16,
    "training_data": {
      "dataset_id": "docs_v1",
      "samples": 10000
    }
  }'
```

### Get Adapter

Retrieve details about a specific adapter.

**Endpoint:** `GET /api/v1/llm/lora/adapters/{adapter_id}`

**Response:** `200 OK`
```json
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

**cURL Example:**
```bash
curl -X GET http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora \
  -H "Authorization: Bearer $TOKEN"
```

### Update Adapter

Update an adapter with additional training data.

**Endpoint:** `PUT /api/v1/llm/lora/adapters/{adapter_id}`

**Request:**
```json
{
  "additional_training_data": {
    "dataset_id": "feedback_v1",
    "samples": 500
  }
}
```

**Response:** `200 OK`
```json
{
  "adapter_id": "themis_help_lora",
  "version": "v1.1",
  "status": "training",
  "job_id": "job_124"
}
```

**cURL Example:**
```bash
curl -X PUT http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "additional_training_data": {
      "dataset_id": "feedback_v1",
      "samples": 500
    }
  }'
```

### Delete Adapter

Delete an adapter and optionally all its versions.

**Endpoint:** `DELETE /api/v1/llm/lora/adapters/{adapter_id}`

**Query Parameters:**
- `version` (optional): Specific version to delete (omit to delete all)

**Response:** `204 No Content`

**cURL Example:**
```bash
# Delete specific version
curl -X DELETE "http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora?version=v1.0" \
  -H "Authorization: Bearer $TOKEN"

# Delete all versions
curl -X DELETE http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora \
  -H "Authorization: Bearer $TOKEN"
```

### List Adapters

List all adapters with optional filters.

**Endpoint:** `GET /api/v1/llm/lora/adapters`

**Query Parameters:**
- `base_model` (optional): Filter by base model
- `status` (optional): Filter by status (ready, stored, training)
- `limit` (optional, default: 10): Maximum results
- `offset` (optional, default: 0): Pagination offset

**Response:** `200 OK`
```json
{
  "adapters": [
    {
      "adapter_id": "themis_help_lora",
      "base_model": "llama-2-7b",
      "status": "ready",
      "is_loaded": true
    }
  ],
  "total": 15,
  "limit": 10,
  "offset": 0
}
```

**cURL Example:**
```bash
curl -X GET "http://localhost:8080/api/v1/llm/lora/adapters?base_model=llama-2-7b&status=ready" \
  -H "Authorization: Bearer $TOKEN"
```

## Adapter Lifecycle

### Load Adapter

Load an adapter into memory for use.

**Endpoint:** `POST /api/v1/llm/lora/adapters/{adapter_id}/load`

**Response:** `200 OK`
```json
{
  "adapter_id": "themis_help_lora",
  "status": "loaded",
  "load_time_ms": 45
}
```

**cURL Example:**
```bash
curl -X POST http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora/load \
  -H "Authorization: Bearer $TOKEN"
```

### Unload Adapter

Unload an adapter from memory.

**Endpoint:** `POST /api/v1/llm/lora/adapters/{adapter_id}/unload`

**Response:** `200 OK`
```json
{
  "adapter_id": "themis_help_lora",
  "status": "unloaded"
}
```

**cURL Example:**
```bash
curl -X POST http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora/unload \
  -H "Authorization: Bearer $TOKEN"
```

### Get Adapter Status

Get the current status of an adapter.

**Endpoint:** `GET /api/v1/llm/lora/adapters/{adapter_id}/status`

**Response:** `200 OK`
```json
{
  "adapter_id": "themis_help_lora",
  "is_loaded": true,
  "memory_usage_mb": 32,
  "last_used": "2026-01-11T15:00:00Z"
}
```

**cURL Example:**
```bash
curl -X GET http://localhost:8080/api/v1/llm/lora/adapters/themis_help_lora/status \
  -H "Authorization: Bearer $TOKEN"
```

## Inference

### Query with LoRA

Execute inference using a LoRA adapter.

**Endpoint:** `POST /api/v1/llm/lora/query`

**Request:**
```json
{
  "model_id": "llama-2-7b",
  "adapter_id": "themis_help_lora",
  "prompt": "How do I enable sharding in ThemisDB?",
  "max_tokens": 500,
  "temperature": 0.7,
  "user_id": "user_42"
}
```

**Response:** `200 OK`
```json
{
  "response": "To enable sharding in ThemisDB...",
  "model_id": "llama-2-7b",
  "adapter_id": "themis_help_lora",
  "tokens_used": 145,
  "inference_time_ms": 850,
  "audit_id": "audit_789"
}
```

**cURL Example:**
```bash
curl -X POST http://localhost:8080/api/v1/llm/lora/query \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "llama-2-7b",
    "adapter_id": "themis_help_lora",
    "prompt": "How do I enable sharding in ThemisDB?",
    "max_tokens": 500,
    "temperature": 0.7
  }'
```

## Monitoring

### Get Framework Statistics

Get statistics about the LoRA framework.

**Endpoint:** `GET /api/v1/llm/lora/stats`

**Response:** `200 OK`
```json
{
  "total_adapters": 15,
  "loaded_adapters": 3,
  "cache_hit_rate": 0.842,
  "total_inferences": 1234567,
  "avg_load_time_ms": 450,
  "uptime_seconds": 864000
}
```

**cURL Example:**
```bash
curl -X GET http://localhost:8080/api/v1/llm/lora/stats \
  -H "Authorization: Bearer $TOKEN"
```

### Health Check

Check the health of the LoRA framework.

**Endpoint:** `GET /api/v1/llm/lora/health`

**Response:** `200 OK`
```json
{
  "status": "healthy",
  "storage": "ok",
  "manager": "ok",
  "training": "ok",
  "checks_passed": 3,
  "checks_failed": 0
}
```

**cURL Example:**
```bash
curl -X GET http://localhost:8080/api/v1/llm/lora/health \
  -H "Authorization: Bearer $TOKEN"
```

## Error Handling

All errors follow RFC 7807 Problem Details for HTTP APIs.

### Error Response Format

```json
{
  "error": "Error message",
  "details": "Detailed error information",
  "status": 400
}
```

### Common Status Codes

| Code | Description |
|------|-------------|
| 200 | Success |
| 201 | Created |
| 204 | No Content |
| 400 | Bad Request - Invalid parameters |
| 401 | Unauthorized - Invalid or missing token |
| 404 | Not Found - Resource doesn't exist |
| 500 | Internal Server Error |
| 503 | Service Unavailable - Health check failed |

### Example Error Responses

**401 Unauthorized:**
```json
{
  "error": "Unauthorized",
  "details": "Valid Bearer Token required. Include 'Authorization: Bearer <token>' header.",
  "status": 401
}
```

**404 Not Found:**
```json
{
  "error": "Adapter not found",
  "details": "Unknown adapter_id: invalid_adapter",
  "status": 404
}
```

**400 Bad Request:**
```json
{
  "error": "Invalid JSON body",
  "status": 400
}
```

## Rate Limiting

Rate limiting is applied per API key/JWT token to ensure fair usage.

### Headers

Rate limit information is included in response headers:

```http
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 999
X-RateLimit-Reset: 1641945600
```

### 429 Too Many Requests

When rate limit is exceeded:

```json
{
  "error": "Rate limit exceeded",
  "details": "Maximum 1000 requests per hour exceeded",
  "status": 429
}
```

**Retry-After Header:**
```http
Retry-After: 3600
```

## Pagination

List endpoints support pagination via query parameters.

### Parameters

- `limit`: Maximum number of results (default: 10, max: 100)
- `offset`: Number of results to skip (default: 0)

### Response Format

```json
{
  "items": [...],
  "total": 100,
  "limit": 10,
  "offset": 0
}
```

### Example

```bash
# Get first page (items 0-9)
curl "http://localhost:8080/api/v1/llm/lora/adapters?limit=10&offset=0"

# Get second page (items 10-19)
curl "http://localhost:8080/api/v1/llm/lora/adapters?limit=10&offset=10"
```

## Versioning

API uses URL path versioning: `/api/v1/...`

Future versions will maintain backward compatibility. Deprecated endpoints will include warnings in response headers:

```http
Deprecated: true
Sunset: Sat, 31 Dec 2027 23:59:59 GMT
```

## Best Practices

1. **Always authenticate**: Include Bearer token in all requests
2. **Handle errors**: Check status codes and handle errors appropriately
3. **Use pagination**: Don't fetch all results at once
4. **Cache responses**: Use adapter status endpoints to avoid unnecessary loads
5. **Monitor rate limits**: Check rate limit headers and implement backoff
6. **Async operations**: Use job IDs for long-running operations like training
7. **Audit logging**: Include `user_id` in inference requests for audit trails

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://github.com/makr-code/ThemisDB/blob/main/README.md
- OpenAPI Spec (Source of Truth): `docs/openapi.yaml`
- Generator Config: `openapitools.json`
