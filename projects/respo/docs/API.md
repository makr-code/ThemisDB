# RESPO API Reference

## Base URL

```
http://localhost:8080
```

## Endpoints

### Chat

#### POST /chat

Chat with RAG context.

**Request:**
```json
{
  "message": "How do I implement an LRU cache?",
  "language": "python",
  "context_limit": 5
}
```

**Response:**
```json
{
  "answer": "Here's how to implement an LRU cache...",
  "sources": [{"file": "cache.py", "score": 0.95}],
  "task_id": "abc-123"
}
```

#### POST /chat/stream

Streaming chat with SSE.

**Request:** Same as `/chat`

**Response:** Server-Sent Events

### Code Operations

#### POST /complete

Code completion.

**Request:**
```json
{
  "code": "def fibonacci(",
  "language": "python",
  "max_tokens": 100
}
```

#### POST /explain

Explain code.

**Request:**
```json
{
  "code": "def merge_sort(arr): ...",
  "language": "python"
}
```

#### POST /review

Code review.

**Request:**
```json
{
  "code": "def process(data): ...",
  "language": "python"
}
```

### Search & Ingestion

#### POST /search

Semantic code search.

**Request:**
```json
{
  "query": "database connection pooling",
  "limit": 10,
  "language": "python"
}
```

#### POST /ingest

Index code repository.

**Request:**
```json
{
  "source_type": "github",
  "source": "owner/repo",
  "token": "optional-github-token"
}
```

### Agents

#### POST /agents/plan

Create execution plan.

**Request:**
```json
{
  "problem": "Implement OAuth2 with PKCE",
  "language": "python"
}
```

#### POST /agents/plan/stream

Stream plan execution (SSE).

#### POST /agents/research

Deep research.

**Request:**
```json
{
  "query": "Best practices for async Python",
  "depth": 3
}
```

### Tasks

#### GET /tasks

List tasks.

**Query Params:** `status=running|completed|cancelled`

#### POST /tasks

Create task.

#### GET /tasks/{id}

Get task status.

#### POST /tasks/{id}/cancel

Cancel task.

#### POST /tasks/{id}/pause

Pause task.

#### POST /tasks/{id}/resume

Resume task.

#### DELETE /tasks/{id}

Delete task.

### MCP (Model Context Protocol)

#### POST /mcp

JSON-RPC 2.0 endpoint.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "respo_search",
    "arguments": {"query": "async patterns"}
  },
  "id": 1
}
```

### System

#### GET /capabilities

Server capabilities and API schema.

#### GET /health

Health check.

**Response:**
```json
{
  "status": "healthy",
  "components": {
    "api": "healthy",
    "vectorstore": "healthy",
    "llm": "degraded"
  }
}
```

#### GET /metrics

Prometheus metrics.

## Error Responses

```json
{
  "error": {
    "code": 400,
    "message": "Invalid request"
  }
}
```

## Rate Limits

Default: 100 requests/minute per IP.
