# Client SDK Guide for LLM Integration

## Overview

ThemisDB provides official client SDKs for Python, JavaScript/TypeScript, Go, and Rust, offering type-safe, idiomatic interfaces for LLM operations. All SDKs support both HTTP REST and gRPC binary protocols.

**Authentication**: All requests require Bearer Token authentication.

## Installation

### Python

```bash
pip install themis-client
```

**Requirements**: Python 3.8+

### JavaScript/TypeScript

```bash
npm install @themis/client
# or
yarn add @themis/client
```

**Requirements**: Node.js 16+

### Go

```bash
go get github.com/themis/go-client
```

**Requirements**: Go 1.19+

### Rust

```toml
[dependencies]
themis-client = "0.1.0"
```

**Requirements**: Rust 1.70+

## Python SDK

### Installation & Setup

```python
from themis import ThemisClient

# Initialize client with Bearer Token
client = ThemisClient(
    url="http://localhost:8080",
    token="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
)

# Or use gRPC for better performance
client = ThemisClient(
    url="localhost:9090",
    protocol="grpc",
    token="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
    use_tls=True
)
```

### Authentication

```python
# Obtain token from login
from themis import authenticate

token = authenticate(
    url="http://localhost:8080",
    username="user",
    password="password"
)

client = ThemisClient(url="http://localhost:8080", token=token)
```

### Basic Inference

```python
# Simple inference
response = client.llm.infer(
    prompt="What is ThemisDB?",
    model="mistral-7b",
    lora="general-qa",
    max_tokens=100,
    temperature=0.7
)

print(response.text)
print(f"Tokens: {response.tokens_generated}")
print(f"Time: {response.inference_time_ms}ms")
print(f"Cache hit: {response.cache_hit}")
```

### RAG Inference

```python
# RAG with vector search
response = client.llm.rag(
    query="What are the penalties for breach of contract?",
    collection="legal_documents",
    top_k=5,
    similarity_threshold=0.8,
    model="mistral-7b",
    lora="legal-qa",
    max_tokens=512
)

print(response.text)
print(f"Documents used: {response.documents_used}")
print(f"Retrieval time: {response.retrieval_time_ms}ms")
```

### Streaming Inference

```python
# Stream tokens as generated
for token in client.llm.stream_infer(
    prompt="Write a story about databases...",
    model="mistral-7b",
    max_tokens=500
):
    print(token, end="", flush=True)

print()  # Newline at end
```

### Async/Await Support

```python
import asyncio

async def main():
    # Async client
    client = ThemisClient(
        url="http://localhost:8080",
        token="your-token",
        async_mode=True
    )
    
    # Concurrent requests
    tasks = [
        client.llm.infer(f"Query {i}", model="mistral-7b")
        for i in range(10)
    ]
    
    results = await asyncio.gather(*tasks)
    
    for i, result in enumerate(results):
        print(f"Result {i}: {result.text[:50]}...")

asyncio.run(main())
```

### Model Management

```python
# List models
models = client.llm.list_models()
for model in models:
    print(f"{model.model_id}: {model.status} ({model.size_bytes / 1e9:.1f} GB)")

# Load model
client.llm.load_model(
    model_id="mistral-7b",
    path="/models/mistral-7b.gguf",
    options={
        "n_gpu_layers": 32,
        "n_ctx": 4096
    },
    pin=False
)

# Unload model
client.llm.unload_model("mistral-7b")

# Get model info
info = client.llm.get_model_info("mistral-7b")
print(f"Status: {info.status}")
print(f"Memory: {info.memory_usage_mb} MB")
print(f"Usage: {info.usage_count} requests")
```

### Model Ingestion

```python
# Upload model to blob storage
response = client.llm.ingest_model(
    model_id="llama-3-8b",
    source="/local/path/llama-3-8b.gguf",
    version="v1.0",
    replicate=True,
    progress_callback=lambda pct: print(f"Upload: {pct}%")
)

print(f"URN: {response.urn}")
print(f"Checksum: {response.checksum}")
print(f"Replication: {response.shards_replicated}/{response.total_shards}")
```

### LoRA Management

```python
# List LoRAs
loras = client.llm.list_loras(base_model="mistral-7b")
for lora in loras:
    print(f"{lora.lora_id}: {lora.status}")

# Load LoRA
client.llm.load_lora(
    lora_id="legal-qa",
    base_model="mistral-7b",
    path="/loras/legal-qa.bin",
    scale=1.0
)

# Unload LoRA
client.llm.unload_lora("legal-qa", base_model="mistral-7b")
```

### Statistics

```python
# Get LLM statistics
stats = client.llm.get_stats()
print(f"Throughput: {stats.throughput.requests_per_second:.1f} req/s")
print(f"Avg latency: {stats.latency.avg_ms:.1f}ms")
print(f"Active requests: {stats.active_requests}")

# Cache statistics
cache_stats = client.llm.get_cache_stats()
print(f"Response cache hit rate: {cache_stats.response_cache.hit_rate:.1%}")
print(f"Prefix cache hit rate: {cache_stats.prefix_cache.hit_rate:.1%}")

# Clear cache
client.llm.clear_cache("response")  # or "prefix", "all"
```

### Embedding Generation

```python
# Generate embedding
embedding = client.llm.embed(
    text="Sample text for embedding",
    model="mistral-7b",
    normalize=True
)

print(f"Dimension: {len(embedding)}")
print(f"First values: {embedding[:5]}")
```

### Error Handling

```python
from themis.exceptions import (
    ModelNotFoundError,
    InferenceError,
    QueueFullError,
    AuthenticationError
)

try:
    response = client.llm.infer(
        prompt="Test",
        model="non-existent-model"
    )
except ModelNotFoundError as e:
    print(f"Model not found: {e}")
except InferenceError as e:
    print(f"Inference failed: {e}")
except QueueFullError as e:
    print(f"Queue full, retry after {e.retry_after_seconds}s")
except AuthenticationError as e:
    print(f"Auth failed: {e}")
```

## JavaScript/TypeScript SDK

### Installation & Setup

```typescript
import { ThemisClient } from '@themis/client';

// Initialize with Bearer Token
const client = new ThemisClient({
  url: 'http://localhost:8080',
  token: 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...'
});

// Or use gRPC
const client = new ThemisClient({
  url: 'localhost:9090',
  protocol: 'grpc',
  token: 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...',
  useTls: true
});
```

### Authentication

```typescript
import { authenticate } from '@themis/client';

// Obtain token
const token = await authenticate({
  url: 'http://localhost:8080',
  username: 'user',
  password: 'password'
});

const client = new ThemisClient({ url: 'http://localhost:8080', token });
```

### Basic Inference

```typescript
// Simple inference
const response = await client.llm.infer({
  prompt: 'What is ThemisDB?',
  model: 'mistral-7b',
  loraAdapter: 'general-qa',
  maxTokens: 100,
  temperature: 0.7
});

console.log(response.text);
console.log(`Tokens: ${response.tokensGenerated}`);
console.log(`Cache hit: ${response.cacheHit}`);
```

### RAG Inference

```typescript
const response = await client.llm.rag({
  query: 'What are the contract provisions?',
  collection: 'legal_documents',
  topK: 5,
  similarityThreshold: 0.8,
  model: 'mistral-7b',
  loraAdapter: 'legal-qa'
});

console.log(response.text);
console.log(`Documents: ${response.documentsUsed}`);
```

### Streaming Inference

```typescript
// Stream tokens
const stream = client.llm.streamInfer({
  prompt: 'Write a story...',
  model: 'mistral-7b',
  maxTokens: 500
});

for await (const token of stream) {
  process.stdout.write(token);
}
console.log();
```

### Concurrent Requests

```typescript
// Process multiple requests in parallel
const prompts = ['Query 1', 'Query 2', 'Query 3'];

const results = await Promise.all(
  prompts.map(prompt =>
    client.llm.infer({
      prompt,
      model: 'mistral-7b'
    })
  )
);

results.forEach((result, i) => {
  console.log(`Result ${i}: ${result.text}`);
});
```

### Model Management

```typescript
// List models
const models = await client.llm.listModels();
models.forEach(model => {
  console.log(`${model.modelId}: ${model.status}`);
});

// Load model
await client.llm.loadModel({
  modelId: 'mistral-7b',
  path: '/models/mistral-7b.gguf',
  options: {
    nGpuLayers: 32,
    nCtx: 4096
  }
});

// Ingest model
const response = await client.llm.ingestModel({
  modelId: 'llama-3-8b',
  source: '/local/llama-3-8b.gguf',
  version: 'v1.0',
  replicate: true,
  onProgress: (percent) => console.log(`Upload: ${percent}%`)
});
```

### TypeScript Types

```typescript
import type {
  InferenceRequest,
  InferenceResponse,
  RAGRequest,
  ModelInfo,
  LoRAInfo,
  Statistics,
  CacheStatistics
} from '@themis/client';

// Fully typed
const request: InferenceRequest = {
  prompt: 'Test',
  model: 'mistral-7b',
  maxTokens: 100
};

const response: InferenceResponse = await client.llm.infer(request);
```

## Go SDK

### Installation & Setup

```go
package main

import (
    "context"
    "fmt"
    "log"
    
    themis "github.com/themis/go-client"
)

func main() {
    // Initialize with Bearer Token
    client, err := themis.NewClient(&themis.Config{
        URL:   "http://localhost:8080",
        Token: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
    })
    if err != nil {
        log.Fatal(err)
    }
    defer client.Close()
    
    // Or use gRPC
    client, err := themis.NewClient(&themis.Config{
        URL:      "localhost:9090",
        Protocol: themis.ProtocolGRPC,
        Token:    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
        UseTLS:   true,
    })
}
```

### Authentication

```go
// Obtain token
token, err := themis.Authenticate(context.Background(), &themis.AuthRequest{
    URL:      "http://localhost:8080",
    Username: "user",
    Password: "password",
})
if err != nil {
    log.Fatal(err)
}

client, err := themis.NewClient(&themis.Config{
    URL:   "http://localhost:8080",
    Token: token,
})
```

### Basic Inference

```go
ctx := context.Background()

response, err := client.LLM.Infer(ctx, &themis.InferenceRequest{
    Prompt:       "What is ThemisDB?",
    Model:        "mistral-7b",
    LoraAdapter:  "general-qa",
    MaxTokens:    100,
    Temperature:  0.7,
})
if err != nil {
    log.Fatal(err)
}

fmt.Printf("Response: %s\n", response.Text)
fmt.Printf("Tokens: %d\n", response.TokensGenerated)
fmt.Printf("Time: %dms\n", response.InferenceTimeMs)
```

### RAG Inference

```go
response, err := client.LLM.RAG(ctx, &themis.RAGRequest{
    Query:               "Contract provisions?",
    Collection:          "legal_documents",
    TopK:                5,
    SimilarityThreshold: 0.8,
    Model:               "mistral-7b",
    LoraAdapter:         "legal-qa",
})
if err != nil {
    log.Fatal(err)
}

fmt.Printf("Answer: %s\n", response.Text)
fmt.Printf("Documents used: %d\n", response.DocumentsUsed)
```

### Streaming Inference

```go
stream, err := client.LLM.StreamInfer(ctx, &themis.InferenceRequest{
    Prompt:    "Write a story...",
    Model:     "mistral-7b",
    MaxTokens: 500,
})
if err != nil {
    log.Fatal(err)
}

for {
    token, err := stream.Recv()
    if err == io.EOF {
        break
    }
    if err != nil {
        log.Fatal(err)
    }
    fmt.Print(token.Text)
}
fmt.Println()
```

### Concurrent Requests

```go
import "golang.org/x/sync/errgroup"

g, ctx := errgroup.WithContext(context.Background())

prompts := []string{"Query 1", "Query 2", "Query 3"}
results := make([]*themis.InferenceResponse, len(prompts))

for i, prompt := range prompts {
    i, prompt := i, prompt  // Capture loop vars
    g.Go(func() error {
        resp, err := client.LLM.Infer(ctx, &themis.InferenceRequest{
            Prompt: prompt,
            Model:  "mistral-7b",
        })
        if err != nil {
            return err
        }
        results[i] = resp
        return nil
    })
}

if err := g.Wait(); err != nil {
    log.Fatal(err)
}

for i, result := range results {
    fmt.Printf("Result %d: %s\n", i, result.Text)
}
```

## Rust SDK

### Installation & Setup

```rust
use themis_client::{ThemisClient, Config, Protocol};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Initialize with Bearer Token
    let client = ThemisClient::new(Config {
        url: "http://localhost:8080".to_string(),
        token: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...".to_string(),
        ..Default::default()
    })?;
    
    // Or use gRPC
    let client = ThemisClient::new(Config {
        url: "localhost:9090".to_string(),
        protocol: Protocol::GRPC,
        token: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...".to_string(),
        use_tls: true,
        ..Default::default()
    })?;
    
    Ok(())
}
```

### Basic Inference

```rust
use themis_client::InferenceRequest;

let response = client.llm().infer(InferenceRequest {
    prompt: "What is ThemisDB?".to_string(),
    model: "mistral-7b".to_string(),
    lora_adapter: Some("general-qa".to_string()),
    max_tokens: Some(100),
    temperature: Some(0.7),
    ..Default::default()
}).await?;

println!("Response: {}", response.text);
println!("Tokens: {}", response.tokens_generated);
println!("Cache hit: {}", response.cache_hit);
```

### RAG Inference

```rust
use themis_client::RAGRequest;

let response = client.llm().rag(RAGRequest {
    query: "Contract provisions?".to_string(),
    collection: "legal_documents".to_string(),
    top_k: 5,
    similarity_threshold: Some(0.8),
    model: "mistral-7b".to_string(),
    lora_adapter: Some("legal-qa".to_string()),
    ..Default::default()
}).await?;

println!("Answer: {}", response.text);
println!("Documents: {}", response.documents_used);
```

### Streaming Inference

```rust
use futures::StreamExt;

let mut stream = client.llm().stream_infer(InferenceRequest {
    prompt: "Write a story...".to_string(),
    model: "mistral-7b".to_string(),
    max_tokens: Some(500),
    ..Default::default()
}).await?;

while let Some(token) = stream.next().await {
    let token = token?;
    print!("{}", token.text);
}
println!();
```

### Concurrent Requests

```rust
use futures::future::join_all;

let prompts = vec!["Query 1", "Query 2", "Query 3"];

let futures: Vec<_> = prompts.iter().map(|&prompt| {
    client.llm().infer(InferenceRequest {
        prompt: prompt.to_string(),
        model: "mistral-7b".to_string(),
        ..Default::default()
    })
}).collect();

let results = join_all(futures).await;

for (i, result) in results.iter().enumerate() {
    match result {
        Ok(response) => println!("Result {}: {}", i, response.text),
        Err(e) => eprintln!("Error {}: {}", i, e),
    }
}
```

## Best Practices

### 1. Token Management

```python
# Refresh tokens automatically
from themis import ThemisClient, TokenRefreshError

client = ThemisClient(
    url="http://localhost:8080",
    token="initial-token",
    auto_refresh=True,
    refresh_callback=lambda: authenticate(url, user, pass)
)
```

### 2. Connection Pooling

```python
# Reuse client across requests
client = ThemisClient(url="...", token="...")

# Don't create new client per request
for i in range(1000):
    response = client.llm.infer(...)  # Good
    # ThemisClient(...).llm.infer(...)  # Bad
```

### 3. Error Handling with Retries

```python
from themis import ThemisClient
from tenacity import retry, stop_after_attempt, wait_exponential

@retry(
    stop=stop_after_attempt(3),
    wait=wait_exponential(multiplier=1, min=2, max=10)
)
def infer_with_retry(client, prompt):
    return client.llm.infer(prompt=prompt, model="mistral-7b")

response = infer_with_retry(client, "What is ThemisDB?")
```

### 4. Batch Processing

```python
import asyncio

async def process_batch(client, prompts):
    tasks = [
        client.llm.infer(prompt=p, model="mistral-7b")
        for p in prompts
    ]
    return await asyncio.gather(*tasks)

# Process 100 prompts concurrently
results = await process_batch(client, prompts)
```

### 5. Monitoring

```python
import time

def infer_with_metrics(client, prompt):
    start = time.time()
    try:
        response = client.llm.infer(prompt=prompt, model="mistral-7b")
        duration = time.time() - start
        
        # Log metrics
        print(f"Latency: {duration*1000:.1f}ms")
        print(f"Tokens: {response.tokens_generated}")
        print(f"Cache: {response.cache_hit}")
        
        return response
    except Exception as e:
        duration = time.time() - start
        print(f"Error after {duration*1000:.1f}ms: {e}")
        raise
```

## Performance Comparison

| SDK | Protocol | Latency (p50) | Throughput | Notes |
|-----|----------|---------------|------------|-------|
| Python | HTTP | 25ms | 95 req/s | Good for scripts |
| Python | gRPC | 12ms | 180 req/s | 2x faster |
| JavaScript | HTTP | 28ms | 85 req/s | Node.js |
| JavaScript | gRPC | 14ms | 165 req/s | 2x faster |
| Go | HTTP | 18ms | 125 req/s | Compiled |
| Go | gRPC | 8ms | 245 req/s | 3x faster |
| Rust | HTTP | 15ms | 145 req/s | Compiled |
| Rust | gRPC | 7ms | 280 req/s | 4x faster |

**Recommendation**: Use gRPC for production, HTTP for development/scripts.

## Troubleshooting

### Authentication Errors

```python
# Check token expiration
import jwt

token = "your-token"
payload = jwt.decode(token, options={"verify_signature": False})
print(f"Expires: {payload['exp']}")

# Refresh if expired
if time.time() > payload['exp']:
    token = authenticate(...)
    client = ThemisClient(url="...", token=token)
```

### Connection Issues

```python
# Test connection
try:
    health = client.llm.get_health()
    print(f"Status: {health.status}")
except ConnectionError as e:
    print(f"Cannot connect: {e}")
```

### Rate Limiting

```python
from themis.exceptions import RateLimitError
import time

try:
    response = client.llm.infer(...)
except RateLimitError as e:
    print(f"Rate limited, retry after {e.retry_after_seconds}s")
    time.sleep(e.retry_after_seconds)
    response = client.llm.infer(...)  # Retry
```

## Migration Guide

### From REST API to SDK

**Before** (raw HTTP):
```python
import requests

response = requests.post(
    "http://localhost:8080/api/v1/llm/inference",
    headers={"Authorization": "Bearer token"},
    json={"prompt": "test", "model": "mistral-7b"}
).json()
```

**After** (SDK):
```python
from themis import ThemisClient

client = ThemisClient(url="http://localhost:8080", token="token")
response = client.llm.infer(prompt="test", model="mistral-7b")
```

### From Synchronous to Async

**Before**:
```python
for prompt in prompts:
    result = client.llm.infer(prompt=prompt, model="mistral-7b")
    results.append(result)
```

**After**:
```python
async def main():
    tasks = [client.llm.infer(prompt=p, model="mistral-7b") for p in prompts]
    results = await asyncio.gather(*tasks)
```

## Support

- **Documentation**: https://docs.themisdb.io/llm/sdk
- **GitHub**: https://github.com/themis/themis-client
- **Issues**: https://github.com/themis/themis-client/issues
- **Discord**: https://discord.gg/themisdb
