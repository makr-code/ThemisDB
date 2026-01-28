# RoPE REST API

This document describes the REST API endpoints for Rotary Position Embeddings (RoPE) in ThemisDB.

## Overview

RoPE (Rotary Position Embeddings) enables position-aware and relation-aware vector embeddings in ThemisDB. This REST API exposes RoPE functionality through standard HTTP endpoints, allowing you to:

- Configure RoPE for vector indices
- Add entities with positional or relational rotation
- Perform rotation-aware similarity search
- Batch process entities efficiently
- Monitor RoPE usage and statistics

## Base URL

```
http://localhost:8529/api/v1/vector-index/{index_name}/rope
```

Replace `{index_name}` with your vector index name (e.g., `documents`, `entities`).

## Authentication

All endpoints support ThemisDB's standard authentication mechanisms:

```bash
# Bearer token
curl -H "Authorization: Bearer YOUR_TOKEN" ...

# API key
curl -H "X-API-Key: YOUR_API_KEY" ...

# Custom headers
curl -H "X-User-ID: user123" -H "X-Tenant-ID: tenant456" ...
```

## Quick Start

### 1. Configure RoPE

First, enable RoPE for your vector index:

```bash
curl -X POST http://localhost:8529/api/v1/vector-index/documents/rope/config \
  -H "Content-Type: application/json" \
  -d '{
    "hidden_dim": 768,
    "num_rotation_pairs": 384,
    "base_theta": 10000.0,
    "normalize_after": false
  }'
```

Response:
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

### 2. Add Entity with Rotation

Add a document with positional rotation:

```bash
curl -X POST http://localhost:8529/api/v1/vector-index/documents/rope/add \
  -H "Content-Type: application/json" \
  -d '{
    "entity": {
      "id": "doc123",
      "embedding": [0.1, 0.2, ..., 0.768],
      "content": "Medical document about diabetes",
      "category": "medical"
    },
    "vector_field": "embedding",
    "position": 42
  }'
```

Response:
```json
{
  "status": "success",
  "message": "Entity added with rotation",
  "entity_id": "doc123",
  "position": 42
}
```

### 3. Search with Rotation

Search for similar entities at a specific position:

```bash
curl -X POST http://localhost:8529/api/v1/vector-index/documents/rope/search \
  -H "Content-Type: application/json" \
  -d '{
    "query": [0.1, 0.2, ..., 0.768],
    "k": 10,
    "position": 100
  }'
```

Response:
```json
{
  "status": "success",
  "results": [
    {
      "id": "doc123",
      "distance": 0.123,
      "position": 42
    },
    ...
  ],
  "query_time_ms": 5.2,
  "rotation_enabled": true,
  "k": 10,
  "count": 10
}
```

## API Endpoints

### Configuration Management

#### POST /config - Enable RoPE

Configure RoPE for a vector index.

**Request:**
```json
{
  "hidden_dim": 768,
  "num_rotation_pairs": 384,
  "base_theta": 10000.0,
  "normalize_after": false
}
```

**Parameters:**
- `hidden_dim` (integer, required): Embedding dimension (must be even)
- `num_rotation_pairs` (integer, required): Number of rotation pairs (≤ hidden_dim/2)
- `base_theta` (number, optional): Base frequency for rotation (default: 10000.0)
- `normalize_after` (boolean, optional): Apply L2 normalization after rotation (default: false)

**Validation:**
- `hidden_dim` must be a positive even number
- `num_rotation_pairs` must be > 0 and ≤ hidden_dim/2
- `base_theta` must be positive

#### GET /config - Get Configuration

Retrieve current RoPE configuration.

**Response:**
```json
{
  "enabled": true,
  "config": {
    "hidden_dim": 768,
    "num_rotation_pairs": 384,
    "base_theta": 10000.0,
    "normalize_after": false
  }
}
```

#### DELETE /config - Disable RoPE

Disable RoPE for the vector index.

**Response:**
```json
{
  "status": "success",
  "message": "RoPE disabled for index 'documents'"
}
```

### Entity Operations

#### POST /add - Add Entity with Position

Add an entity with rotation based on position.

**Request:**
```json
{
  "entity": {
    "id": "doc123",
    "embedding": [0.1, 0.2, ...],
    "content": "Document text",
    "metadata": {...}
  },
  "vector_field": "embedding",
  "position": 42
}
```

**Parameters:**
- `entity` (object, required): Entity data with at least `id` and embedding
- `vector_field` (string, optional): Name of embedding field (default: "embedding")
- `position` (integer, required): Position for rotation (≥ 0)

#### POST /add-relational - Add Entity with Relation

Add an entity with rotation based on relation type.

**Request:**
```json
{
  "entity": {
    "id": "entity_a",
    "embedding": [0.1, 0.2, ...],
    "name": "Entity A"
  },
  "vector_field": "embedding",
  "relation_type": "parent_of"
}
```

**Parameters:**
- `entity` (object, required): Entity data
- `vector_field` (string, optional): Name of embedding field
- `relation_type` (string, required): Relation type for rotation

**Common relation types:**
- `parent_of`, `child_of`
- `member_of`, `has_member`
- `located_in`, `contains`
- `caused_by`, `causes`
- Custom relation types supported

#### POST /batch-add - Batch Add Entities

Add multiple entities efficiently.

**Request:**
```json
{
  "entities": [
    {
      "entity": {"id": "doc1", "embedding": [...]},
      "position": 0
    },
    {
      "entity": {"id": "doc2", "embedding": [...]},
      "position": 1
    }
  ],
  "vector_field": "embedding"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Batch add completed",
  "inserted": 100,
  "errors": 0,
  "total": 100
}
```

### Search Operations

#### POST /search - Search with Rotation

Perform k-NN search with query rotation.

**Request:**
```json
{
  "query": [0.1, 0.2, ...],
  "k": 10,
  "position": 100,
  "filters": {
    "field": "category",
    "value": "medical"
  }
}
```

**Parameters:**
- `query` (array, required): Query embedding vector
- `k` (integer, optional): Number of results (default: 10)
- `position` (integer, required): Position for query rotation
- `filters` (object, optional): Metadata filters

**Response:**
```json
{
  "status": "success",
  "results": [
    {
      "id": "doc123",
      "distance": 0.123,
      "position": 42,
      "metadata": {...}
    }
  ],
  "query_time_ms": 5.2,
  "rotation_enabled": true
}
```

### Monitoring

#### GET /stats - Get Statistics

Retrieve RoPE usage statistics.

**Response:**
```json
{
  "enabled": true,
  "config": {...},
  "statistics": {
    "total_rotated_entities": 10000,
    "avg_rotation_time_us": 1.5,
    "relational_rotations": 500,
    "adapters_loaded": ["medical", "legal"]
  }
}
```

## Error Handling

All endpoints return errors in this format:

```json
{
  "error": true,
  "message": "Error description",
  "status_code": 400
}
```

**Common error codes:**
- `400`: Bad request (invalid configuration, missing fields)
- `404`: Resource not found (index not found, RoPE not enabled)
- `500`: Internal server error

**Common error scenarios:**

1. **RoPE not configured:**
```json
{
  "error": true,
  "message": "RoPE is not enabled. Please configure RoPE first.",
  "status_code": 400
}
```

2. **Invalid configuration:**
```json
{
  "error": true,
  "message": "Invalid RoPE configuration: hidden_dim must be a positive even number",
  "status_code": 400
}
```

3. **Dimension mismatch:**
```json
{
  "error": true,
  "message": "Vector dimension mismatch: expected 768, got 512",
  "status_code": 400
}
```

## Use Cases

### 1. Sequential Document Processing

Process documents in order with position-aware embeddings:

```python
import requests

# Configure RoPE
requests.post('http://localhost:8529/api/v1/vector-index/documents/rope/config',
    json={'hidden_dim': 768, 'num_rotation_pairs': 384})

# Add documents sequentially
for i, doc in enumerate(documents):
    requests.post('http://localhost:8529/api/v1/vector-index/documents/rope/add',
        json={
            'entity': {'id': f'doc{i}', 'embedding': doc.embedding, 'text': doc.text},
            'position': i
        })

# Search at specific position
results = requests.post('http://localhost:8529/api/v1/vector-index/documents/rope/search',
    json={'query': query_embedding, 'k': 10, 'position': 50}).json()
```

### 2. Knowledge Graph Embeddings

Model entity relationships with relational rotation:

```python
# Add entities with different relations
for entity, relation in knowledge_graph:
    requests.post('http://localhost:8529/api/v1/vector-index/entities/rope/add-relational',
        json={
            'entity': {'id': entity.id, 'embedding': entity.embedding},
            'relation_type': relation.type
        })
```

### 3. Temporal Data

Handle time-series data with position-based rotation:

```python
# Add time-series vectors
for timestamp, vector in time_series:
    requests.post('http://localhost:8529/api/v1/vector-index/timeseries/rope/add',
        json={
            'entity': {'id': f'ts_{timestamp}', 'embedding': vector},
            'position': timestamp
        })
```

### 4. Batch Processing

Efficiently process large datasets:

```python
# Batch add 1000 entities
batch = {
    'entities': [
        {'entity': {'id': f'doc{i}', 'embedding': emb}, 'position': i}
        for i, emb in enumerate(embeddings[:1000])
    ],
    'vector_field': 'embedding'
}
response = requests.post('http://localhost:8529/api/v1/vector-index/docs/rope/batch-add',
    json=batch)
```

## Performance Considerations

### Batch Operations

- Use `/batch-add` for inserting > 100 entities
- Batch size sweet spot: 500-1000 entities
- Reduces network overhead by ~90%

### Caching

- Theta values are cached after configuration
- First rotation: ~10μs
- Subsequent rotations: ~1-2μs
- Cache persists until configuration changes

### Scalability

- Rotation is O(d) where d = embedding dimension
- Search complexity unchanged (still O(log N) with HNSW)
- Memory overhead: ~8 bytes per rotation pair
- Example: 768-dim with 384 pairs = ~3KB cache

## Rate Limits

Default rate limits (configurable per deployment):

- **Configuration**: 10 requests/minute
- **Add/Search**: 1000 requests/minute
- **Batch operations**: 100 requests/minute

## Security

### Authentication

All endpoints respect ThemisDB's authentication layer:

```bash
# JWT token
curl -H "Authorization: Bearer eyJhbGc..." \
  http://localhost:8529/api/v1/vector-index/docs/rope/config

# API key
curl -H "X-API-Key: sk_live_..." \
  http://localhost:8529/api/v1/vector-index/docs/rope/config
```

### Permissions

Required permissions:
- `vector:write` - Configuration, add operations
- `vector:read` - Search, stats operations

### Encryption

- All API traffic supports TLS/HTTPS
- Embeddings can be encrypted at rest (see ThemisDB docs)
- Metadata fields support field-level encryption

## Examples

See the [examples directory](../examples/) for complete working examples:

- `rope_rest_example.py` - Python client examples
- `rope_rest_example.js` - JavaScript/Node.js examples
- `rope_rest_example.sh` - Bash/curl examples

## OpenAPI Specification

Full OpenAPI 3.0 specification available at:
```
openapi/rope_api.yaml
```

Generate client libraries:
```bash
openapi-generator-cli generate -i openapi/rope_api.yaml -g python
openapi-generator-cli generate -i openapi/rope_api.yaml -g javascript
```

## Troubleshooting

### RoPE not working after configuration

Check that:
1. Vector index is initialized
2. Configuration is valid (even dimension, valid pairs)
3. Entities have embeddings in the specified field

### Performance issues

- Ensure theta cache is computed (check theta_cache_size in config response)
- Use batch operations for bulk inserts
- Consider dimension reduction if rotation is slow

### Dimension mismatches

- Verify all embeddings match configured hidden_dim
- Check vector_field parameter matches your data
- Use consistent embedding models

## Support

- Documentation: https://docs.themisdb.io/rope
- Examples: https://github.com/themisdb/examples
- Issues: https://github.com/themisdb/themisdb/issues
- Community: https://discord.gg/themisdb
