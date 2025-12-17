# HTTP API Specification for LLM Integration

## Overview

ThemisDB provides a comprehensive RESTful HTTP API for LLM operations, enabling inference, model management, LoRA operations, and statistics retrieval.

**Base URL**: `/api/v1/llm`

**Authentication**: Bearer token or API key (configured in `llm_config.yaml`)

**Content-Type**: `application/json`

## Endpoints

### 1. Inference Operations

#### 1.1 Standard Inference

**POST** `/api/v1/llm/inference`

Execute LLM inference with a prompt.

**Request Body**:
```json
{
  "prompt": "What is ThemisDB?",
  "model": "mistral-7b",
  "lora_adapter": "general-qa",
  "max_tokens": 512,
  "temperature": 0.7,
  "top_p": 0.9,
  "top_k": 40,
  "stop_sequences": ["\n\n", "END"],
  "stream": false
}
```

**Response**:
```json
{
  "text": "ThemisDB is a distributed graph database...",
  "tokens_generated": 45,
  "inference_time_ms": 150,
  "model_used": "mistral-7b",
  "lora_used": "general-qa",
  "cache_hit": false,
  "finish_reason": "stop"
}
```

**Status Codes**:
- `200 OK`: Successful inference
- `400 Bad Request`: Invalid parameters
- `404 Not Found`: Model or LoRA not found
- `429 Too Many Requests`: Queue full (backpressure)
- `500 Internal Server Error`: Inference failure

#### 1.2 RAG Inference

**POST** `/api/v1/llm/rag`

Execute RAG (Retrieval-Augmented Generation) inference with vector search.

**Request Body**:
```json
{
  "query": "What are the main provisions in contract clause 3.4?",
  "collection": "legal_documents",
  "top_k": 5,
  "similarity_threshold": 0.8,
  "model": "mistral-7b",
  "lora_adapter": "legal-qa",
  "max_tokens": 512,
  "temperature": 0.7,
  "context_assembly": "concat"
}
```

**Response**:
```json
{
  "text": "Contract clause 3.4 contains the following provisions...",
  "tokens_generated": 87,
  "inference_time_ms": 210,
  "documents_retrieved": 5,
  "documents_used": 3,
  "retrieval_time_ms": 45,
  "model_used": "mistral-7b",
  "lora_used": "legal-qa",
  "cache_hit": false,
  "finish_reason": "stop"
}
```

#### 1.3 Streaming Inference

**POST** `/api/v1/llm/inference` (with `stream: true`)

Stream tokens as they are generated.

**Request Body**:
```json
{
  "prompt": "Write a story about...",
  "model": "mistral-7b",
  "stream": true,
  "max_tokens": 1024
}
```

**Response** (Server-Sent Events):
```
data: {"token": "Once", "index": 0}

data: {"token": " upon", "index": 1}

data: {"token": " a", "index": 2}

data: {"token": " time", "index": 3}

...

data: {"done": true, "tokens_generated": 245, "inference_time_ms": 2150}
```

**Headers**:
- `Content-Type: text/event-stream`
- `Cache-Control: no-cache`
- `Connection: keep-alive`

#### 1.4 Embedding Generation

**POST** `/api/v1/llm/embed`

Generate embeddings for text.

**Request Body**:
```json
{
  "text": "Sample text for embedding generation",
  "model": "mistral-7b",
  "normalize": true
}
```

**Response**:
```json
{
  "embedding": [0.123, -0.456, 0.789, ...],
  "dimension": 4096,
  "model_used": "mistral-7b",
  "inference_time_ms": 25
}
```

### 2. Model Management

#### 2.1 List Models

**GET** `/api/v1/llm/models`

List all available models.

**Response**:
```json
{
  "models": [
    {
      "model_id": "mistral-7b",
      "path": "/models/mistral-7b.gguf",
      "status": "loaded",
      "size_bytes": 6400000000,
      "format": "GGUF",
      "n_layers": 32,
      "loaded_timestamp": "2024-01-15T10:30:00Z",
      "last_used": "2024-01-15T12:45:30Z",
      "usage_count": 1247
    },
    {
      "model_id": "llama-3-8b",
      "status": "available",
      "size_bytes": 8500000000,
      "format": "GGUF"
    }
  ]
}
```

#### 2.2 Load Model

**POST** `/api/v1/llm/models/load`

Load a model into memory.

**Request Body**:
```json
{
  "model_id": "mistral-7b",
  "path": "/models/mistral-7b.gguf",
  "options": {
    "n_gpu_layers": 32,
    "n_ctx": 4096,
    "n_batch": 512,
    "n_threads": 8,
    "use_mmap": true,
    "use_mlock": false
  },
  "pin": false
}
```

**Response**:
```json
{
  "model_id": "mistral-7b",
  "status": "loaded",
  "load_time_ms": 2850,
  "memory_used_mb": 6200
}
```

#### 2.3 Unload Model

**POST** `/api/v1/llm/models/unload`

Unload a model from memory.

**Request Body**:
```json
{
  "model_id": "mistral-7b"
}
```

**Response**:
```json
{
  "model_id": "mistral-7b",
  "status": "unloaded",
  "memory_freed_mb": 6200
}
```

#### 2.4 Get Model Info

**GET** `/api/v1/llm/models/{model_id}`

Get detailed information about a specific model.

**Response**:
```json
{
  "model_id": "mistral-7b",
  "path": "/models/mistral-7b.gguf",
  "status": "loaded",
  "size_bytes": 6400000000,
  "format": "GGUF",
  "version": "v0.3",
  "architecture": "llama",
  "n_layers": 32,
  "n_heads": 32,
  "n_embd": 4096,
  "n_vocab": 32000,
  "context_length": 8192,
  "loaded_timestamp": "2024-01-15T10:30:00Z",
  "last_used": "2024-01-15T12:45:30Z",
  "usage_count": 1247,
  "memory_usage_mb": 6200,
  "gpu_layers": 32,
  "pinned": false
}
```

#### 2.5 Ingest Model

**POST** `/api/v1/llm/models/ingest`

Upload and ingest a model into ThemisDB blob storage.

**Request** (multipart/form-data):
```
POST /api/v1/llm/models/ingest
Content-Type: multipart/form-data

--boundary
Content-Disposition: form-data; name="model_id"

llama-3-8b
--boundary
Content-Disposition: form-data; name="file"; filename="llama-3-8b.gguf"
Content-Type: application/octet-stream

[binary data]
--boundary
Content-Disposition: form-data; name="metadata"
Content-Type: application/json

{
  "version": "v1.0",
  "description": "Llama 3 8B quantized Q4",
  "shard_affinity": "legal",
  "replicate": true
}
--boundary--
```

**Response**:
```json
{
  "model_id": "llama-3-8b",
  "version": "v1.0",
  "urn": "urn:themis:model:llama-3-8b:v1",
  "size_bytes": 8500000000,
  "checksum": "sha256:abc123...",
  "upload_time_ms": 45000,
  "replication_status": "pending",
  "shards_replicated": 0,
  "total_shards": 4
}
```

**Note**: For large models, use chunked upload with `Content-Range` headers.

### 3. LoRA Management

#### 3.1 List LoRAs

**GET** `/api/v1/llm/loras`

List all available LoRA adapters.

**Query Parameters**:
- `model`: Filter by base model
- `status`: Filter by status (loaded, available)

**Response**:
```json
{
  "loras": [
    {
      "lora_id": "legal-qa",
      "base_model": "mistral-7b",
      "path": "/loras/legal-qa.bin",
      "status": "loaded",
      "size_bytes": 20971520,
      "rank": 8,
      "alpha": 16,
      "loaded_timestamp": "2024-01-15T11:00:00Z",
      "usage_count": 523
    },
    {
      "lora_id": "medical-qa",
      "base_model": "mistral-7b",
      "status": "available",
      "size_bytes": 20971520
    }
  ]
}
```

#### 3.2 Load LoRA

**POST** `/api/v1/llm/loras/load`

Load a LoRA adapter.

**Request Body**:
```json
{
  "lora_id": "legal-qa",
  "base_model": "mistral-7b",
  "path": "/loras/legal-qa.bin",
  "scale": 1.0
}
```

**Response**:
```json
{
  "lora_id": "legal-qa",
  "base_model": "mistral-7b",
  "status": "loaded",
  "load_time_ms": 150,
  "slot": 3
}
```

#### 3.3 Unload LoRA

**POST** `/api/v1/llm/loras/unload`

Unload a LoRA adapter.

**Request Body**:
```json
{
  "lora_id": "legal-qa",
  "base_model": "mistral-7b"
}
```

**Response**:
```json
{
  "lora_id": "legal-qa",
  "status": "unloaded",
  "slot_freed": 3
}
```

#### 3.4 Get LoRA Info

**GET** `/api/v1/llm/loras/{lora_id}`

Get detailed information about a specific LoRA.

**Query Parameters**:
- `base_model`: Base model ID

**Response**:
```json
{
  "lora_id": "legal-qa",
  "base_model": "mistral-7b",
  "path": "/loras/legal-qa.bin",
  "status": "loaded",
  "size_bytes": 20971520,
  "rank": 8,
  "alpha": 16,
  "target_modules": ["q_proj", "v_proj"],
  "loaded_timestamp": "2024-01-15T11:00:00Z",
  "last_used": "2024-01-15T12:40:15Z",
  "usage_count": 523,
  "slot": 3
}
```

### 4. Statistics & Monitoring

#### 4.1 Get LLM Statistics

**GET** `/api/v1/llm/stats`

Get comprehensive LLM system statistics.

**Response**:
```json
{
  "uptime_seconds": 86400,
  "total_requests": 15234,
  "successful_requests": 15102,
  "failed_requests": 132,
  "active_requests": 8,
  "queued_requests": 23,
  "throughput": {
    "requests_per_second": 128.5,
    "tokens_per_second": 3456.2
  },
  "latency": {
    "p50_ms": 24,
    "p95_ms": 65,
    "p99_ms": 180,
    "avg_ms": 28
  },
  "models": {
    "loaded": 2,
    "total_available": 5,
    "memory_used_mb": 12400
  },
  "loras": {
    "loaded": 8,
    "total_available": 24,
    "memory_used_mb": 160
  },
  "workers": {
    "total": 4,
    "busy": 3,
    "idle": 1,
    "utilization": 0.75
  },
  "gpu": {
    "utilization": 0.89,
    "memory_used_mb": 18456,
    "memory_total_mb": 24576
  }
}
```

#### 4.2 Get Cache Statistics

**GET** `/api/v1/llm/cache/stats`

Get cache performance statistics.

**Response**:
```json
{
  "response_cache": {
    "hits": 12456,
    "misses": 2778,
    "hit_rate": 0.818,
    "total_entries": 5432,
    "memory_used_mb": 890,
    "avg_lookup_time_ms": 1.8
  },
  "prefix_cache": {
    "hits": 8934,
    "misses": 4823,
    "hit_rate": 0.649,
    "total_entries": 2145,
    "memory_used_mb": 125,
    "avg_tokens_saved": 45.3
  },
  "model_metadata_cache": {
    "hits": 45678,
    "misses": 123,
    "hit_rate": 0.997,
    "total_entries": 5
  },
  "lora_metadata_cache": {
    "hits": 23456,
    "misses": 245,
    "hit_rate": 0.990,
    "total_entries": 24
  },
  "kv_cache_buffer_pool": {
    "total_buffers": 8,
    "active_buffers": 4,
    "buffer_reuse_count": 12456
  }
}
```

#### 4.3 Get Worker Statistics

**GET** `/api/v1/llm/workers`

Get per-worker statistics.

**Response**:
```json
{
  "workers": [
    {
      "worker_id": 0,
      "status": "busy",
      "current_request_id": "req_abc123",
      "requests_processed": 3821,
      "total_processing_time_ms": 456782,
      "avg_processing_time_ms": 119.5,
      "utilization": 0.92
    },
    {
      "worker_id": 1,
      "status": "idle",
      "requests_processed": 3756,
      "total_processing_time_ms": 441234,
      "avg_processing_time_ms": 117.5,
      "utilization": 0.88
    }
  ]
}
```

### 5. Health & Status

#### 5.1 Health Check

**GET** `/api/v1/llm/health`

Check LLM service health.

**Response**:
```json
{
  "status": "healthy",
  "timestamp": "2024-01-15T12:45:30Z",
  "checks": {
    "models_loaded": true,
    "workers_active": true,
    "gpu_available": true,
    "queue_ok": true
  }
}
```

**Status Codes**:
- `200 OK`: Healthy
- `503 Service Unavailable`: Unhealthy

#### 5.2 Clear Cache

**POST** `/api/v1/llm/cache/clear`

Clear caches (response, prefix, or all).

**Request Body**:
```json
{
  "cache_type": "response"
}
```

**Cache Types**:
- `response`: Response cache only
- `prefix`: Prefix cache only
- `all`: All caches

**Response**:
```json
{
  "cleared": "response",
  "entries_removed": 5432,
  "memory_freed_mb": 890
}
```

## Error Responses

All error responses follow this format:

```json
{
  "error": {
    "code": "MODEL_NOT_FOUND",
    "message": "Model 'invalid-model' not found",
    "details": {
      "available_models": ["mistral-7b", "llama-3-8b"]
    }
  }
}
```

**Error Codes**:
- `INVALID_REQUEST`: Malformed request
- `MODEL_NOT_FOUND`: Requested model not found
- `LORA_NOT_FOUND`: Requested LoRA not found
- `MODEL_LOAD_FAILED`: Failed to load model
- `INFERENCE_FAILED`: Inference error
- `QUEUE_FULL`: Request queue full
- `INSUFFICIENT_MEMORY`: Not enough memory
- `INVALID_PARAMETERS`: Invalid inference parameters

## Rate Limiting

API endpoints are rate-limited per API key:

**Headers**:
- `X-RateLimit-Limit`: Maximum requests per minute
- `X-RateLimit-Remaining`: Remaining requests in current window
- `X-RateLimit-Reset`: Timestamp when limit resets

**Response** (429 Too Many Requests):
```json
{
  "error": {
    "code": "RATE_LIMIT_EXCEEDED",
    "message": "Rate limit exceeded. Retry after 42 seconds.",
    "retry_after_seconds": 42
  }
}
```

## Authentication

**All API requests require Bearer Token authentication.**

**Header**:
```
Authorization: Bearer <token>
```

**Example**:
```bash
curl -X POST http://localhost:8080/api/v1/llm/inference \
  -H "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..." \
  -H "Content-Type: application/json" \
  -d '{"prompt": "What is ThemisDB?", "model": "mistral-7b"}'
```

**Token Format**: JWT (JSON Web Token)

**Token Acquisition**: Obtain from ThemisDB authentication endpoint:
```bash
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username": "user", "password": "pass"}' \
  | jq -r '.token'
```

**Token Expiration**: Configurable (default: 24 hours)

**Unauthorized Response** (401):
```json
{
  "error": {
    "code": "UNAUTHORIZED",
    "message": "Invalid or expired token"
  }
}
```

## Versioning

API version is included in URL: `/api/v1/llm/*`

Future versions will use `/api/v2/llm/*`, etc.

## Examples

### cURL Examples

**Simple Inference**:
```bash
curl -X POST http://localhost:8080/api/v1/llm/inference \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{
    "prompt": "What is ThemisDB?",
    "model": "mistral-7b",
    "max_tokens": 100
  }'
```

**RAG Query**:
```bash
curl -X POST http://localhost:8080/api/v1/llm/rag \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{
    "query": "Contract provisions in clause 3.4",
    "collection": "legal_docs",
    "top_k": 5,
    "lora_adapter": "legal-qa"
  }'
```

**Model Upload**:
```bash
curl -X POST http://localhost:8080/api/v1/llm/models/ingest \
  -H "Authorization: Bearer <token>" \
  -F "model_id=llama-3-8b" \
  -F "file=@/path/to/llama-3-8b.gguf" \
  -F 'metadata={"version":"v1.0","replicate":true}'
```

## Best Practices

1. **Use streaming for long responses** to improve perceived latency
2. **Leverage caching** by structuring similar prompts consistently
3. **Pre-load frequently used models and LoRAs** to avoid cold starts
4. **Monitor cache hit rates** and adjust similarity thresholds
5. **Use batch inference** via multiple concurrent requests for throughput
6. **Set appropriate timeouts** (recommend 30s for inference, 5min for model loading)
7. **Handle 429 errors** with exponential backoff
8. **Use RAG endpoint** instead of manual vector search + inference

## Performance Considerations

- **Response caching**: 75x speedup for cache hits
- **Prefix caching**: 65% hit rate, ~45 tokens saved per hit
- **Concurrent requests**: 128 req/s with 4 workers
- **Model loading**: ~3s cold start, ~0ms cached
- **LoRA switching**: ~5ms per switch
