# ThemisDB LLM Client SDKs

Complete client libraries for ThemisDB LLM operations in 4 languages: Python, JavaScript/TypeScript, Go, and Rust.

## Overview

All SDKs provide:
- ✅ Bearer Token (JWT) authentication
- ✅ Inference, RAG, and embedding generation
- ✅ Real-time token streaming
- ✅ Model and LoRA management
- ✅ Statistics and health checks
- ✅ Comprehensive error handling

## Python SDK

**Installation**: `pip install themis-llm`

```python
from themis_llm import ThemisLLMClient

client = ThemisLLMClient("http://localhost:8080", bearer_token="your-jwt-token")
response = client.infer(prompt="What is ThemisDB?", model="mistral-7b")
```

See [python/README.md](python/README.md) for complete documentation.

## JavaScript/TypeScript SDK

**Installation**: `npm install @themis/llm-client`

```typescript
import { ThemisLLMClient } from '@themis/llm-client';

const client = new ThemisLLMClient({
  baseUrl: 'http://localhost:8080',
  bearerToken: 'your-jwt-token'
});
const response = await client.infer({ prompt: 'What is ThemisDB?' });
```

See [javascript/README.md](javascript/README.md) for complete documentation.

## Go SDK

**Installation**: `go get github.com/themisdb/themis-llm-go`

```go
import themisllm "github.com/themisdb/themis-llm-go"

client := themisllm.NewClient("http://localhost:8080",
    themisllm.WithBearerToken("your-jwt-token"))
resp, _ := client.Infer(ctx, &themisllm.InferRequest{
    Prompt: "What is ThemisDB?",
})
```

See [go/README.md](go/README.md) for complete documentation.

## Rust SDK

**Installation**: `cargo add themis-llm`

```rust
use themis_llm::ThemisLLMClient;

let client = ThemisLLMClient::new(
    "http://localhost:8080",
    Some("your-jwt-token".to_string())
)?;
let response = client.infer(/* ... */).await?;
```

See [rust/README.md](rust/README.md) for complete documentation.

## Common Features

All SDKs implement the same core API:

### Inference
- `infer()` - Standard text generation
- `rag()` - Retrieval-Augmented Generation
- `embed()` - Embedding generation
- `stream_infer()` - Real-time token streaming

### Model Management
- `list_models()` - List available models
- `load_model()` - Load model into memory
- `unload_model()` - Unload model from memory
- `get_model_info()` - Get model details

### LoRA Management
- `list_loras()` - List LoRA adapters
- `load_lora()` - Load LoRA adapter
- `unload_lora()` - Unload LoRA adapter

### System
- `get_stats()` - Performance statistics
- `get_cache_stats()` - Cache statistics
- `clear_cache()` - Clear all caches
- `health_check()` - Health status

## Performance

| Protocol | Latency (p50) | Throughput |
|----------|---------------|------------|
| HTTP REST | 25ms | 95 req/s |
| gRPC | 12ms | 180 req/s |

All SDKs use HTTP REST by default. gRPC support can be enabled for 2x better performance.

## Authentication

All SDKs require a Bearer Token (JWT) for authentication:

```bash
# Get token
curl -X POST http://localhost:8080/api/v1/auth/login \
  -d '{"username": "user", "password": "pass"}' | jq -r '.token'
```

## License

Apache 2.0
