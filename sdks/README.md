# ThemisDB Client SDKs

Complete client libraries for ThemisDB operations in 5 languages: Python, JavaScript/TypeScript, Go, Rust, and Java.

## Overview

ThemisDB provides official SDKs for multiple programming languages, enabling developers to easily integrate ThemisDB into their applications. All SDKs provide consistent interfaces and functionality across languages.

### Core Features

All SDKs provide:
- ✅ Bearer Token (JWT) authentication
- ✅ Full CRUD operations for collections and documents
- ✅ AQL (Advanced Query Language) query execution
- ✅ LLM inference, RAG, and embedding generation
- ✅ Real-time token streaming
- ✅ Model and LoRA management
- ✅ Statistics and health checks
- ✅ Comprehensive error handling
- ✅ Connection pooling and timeout management

## Available SDKs

### Python SDK

**Status**: ✅ Available  
**Installation**: `pip install themis-llm`

```python
from themis_llm import ThemisLLMClient

client = ThemisLLMClient("http://localhost:8080", bearer_token="your-jwt-token")
response = client.infer(prompt="What is ThemisDB?", model="mistral-7b")
```

See [python/README.md](python/README.md) for complete documentation.

### JavaScript/TypeScript SDK

**Status**: 🚧 Under Development  
**Installation** (planned): `npm install @themisdb/client`

```typescript
import { ThemisDBClient } from '@themisdb/client';

const client = new ThemisDBClient({
  baseUrl: 'http://localhost:8080',
  bearerToken: 'your-jwt-token'
});
const response = await client.llm.infer({ 
  prompt: 'What is ThemisDB?',
  model: 'mistral-7b'
});
```

See [javascript/README.md](javascript/README.md) for complete documentation.

### Go SDK

**Status**: 🚧 Under Development  
**Installation** (planned): `go get github.com/makr-code/ThemisDB/sdks/go/themisclient`

```go
import themisclient "github.com/makr-code/ThemisDB/sdks/go/pkg/themisclient"

client, _ := themisclient.NewClient("http://localhost:8080",
    themisclient.WithBearerToken("your-jwt-token"))
resp, _ := client.LLM().Infer(ctx, &themisclient.InferRequest{
    Prompt: "What is ThemisDB?",
    Model: "mistral-7b",
})
```

See [go/README.md](go/README.md) for complete documentation.

### Rust SDK

**Status**: 🚧 Under Development  
**Installation** (planned): `cargo add themisdb-client`

```rust
use themisdb_client::Client;

let client = Client::builder()
    .base_url("http://localhost:8080")
    .bearer_token("your-jwt-token")
    .build()?;
let response = client.llm().infer()
    .prompt("What is ThemisDB?")
    .model("mistral-7b")
    .send()
    .await?;
```

See [rust/README.md](rust/README.md) for complete documentation.

### Java SDK

**Status**: 🚧 Under Development  
**Installation** (planned): Maven/Gradle

```xml
<dependency>
    <groupId>com.themisdb</groupId>
    <artifactId>themisdb-client</artifactId>
    <version>0.1.0</version>
</dependency>
```

```java
import com.themisdb.client.ThemisDBClient;

ThemisDBClient client = ThemisDBClient.builder()
    .baseUrl("http://localhost:8080")
    .bearerToken("your-jwt-token")
    .build();

InferResponse response = client.llm()
    .infer()
    .prompt("What is ThemisDB?")
    .model("mistral-7b")
    .execute();
```

See [java/README.md](java/README.md) for complete documentation.

## Common Features

All SDKs implement the same core API structure:

### Data Operations
- `query()` - Execute AQL queries with bind variables
- Collections: `list()`, `create()`, `drop()`
- Documents: `get()`, `create()`, `update()`, `delete()`

### LLM Operations
- `infer()` - Standard text generation
- `stream()` - Real-time token streaming
- `rag()` - Retrieval-Augmented Generation
- `embed()` - Embedding generation

### Model Management
- `list_models()` - List available models
- `load_model()` - Load model into memory
- `unload_model()` - Unload model from memory
- `get_model_info()` - Get model details

### LoRA Management
- `list_loras()` - List LoRA adapters
- `load_lora()` - Load LoRA adapter
- `unload_lora()` - Unload LoRA adapter

### Administrative Functions
- `health()` - Health status check
- `stats()` - Performance statistics
- `cache_stats()` - Cache statistics
- `clear_cache()` - Clear all caches

## SDK Development Status

| SDK | Status | Version | Installation | Tests | Examples |
|-----|--------|---------|--------------|-------|----------|
| Python | ✅ Available | 1.0+ | pip | ✅ | ✅ |
| JavaScript | 🚧 Development | 0.1.0-dev | - | ✅ | ✅ |
| Go | 🚧 Development | 0.1.0-dev | - | ✅ | ✅ |
| Rust | 🚧 Development | 0.1.0-dev | - | ✅ | ✅ |
| Java | 🚧 Development | 0.1.0-SNAPSHOT | - | ✅ | ✅ |

**Legend:**
- ✅ Available: Production-ready and published
- 🚧 Development: Structure in place, implementation in progress
- ❌ Planned: Not yet started

## Performance

| Protocol | Latency (p50) | Throughput |
|----------|---------------|------------|
| HTTP REST | 25ms | 95 req/s |
| gRPC | 12ms | 180 req/s |

All SDKs use HTTP REST by default. gRPC support can be enabled for 2x better performance.

## Authentication

All SDKs require a Bearer Token (JWT) for authentication:

```bash
# Get authentication token
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username": "user", "password": "pass"}' | jq -r '.token'
```

## Getting Started

1. **Choose Your SDK**: Select the SDK for your preferred programming language
2. **Install**: Follow the installation instructions in the SDK's README
3. **Configure**: Initialize the client with your ThemisDB server URL and authentication token
4. **Explore Examples**: Check the `examples/` directory in each SDK for usage patterns
5. **Run Tests**: Verify your setup by running the SDK's test suite

## Contributing

We welcome contributions to any of the SDKs! Please see:
- [CONTRIBUTING.md](../CONTRIBUTING.md) - General contribution guidelines
- Individual SDK READMEs - Language-specific development instructions

### Adding a New SDK

If you'd like to add support for a new language:
1. Follow the structure of existing SDKs
2. Include comprehensive tests and examples
3. Document all public APIs
4. Ensure consistent behavior across languages
5. Submit a pull request with your implementation

## Support and Documentation

- **API Documentation**: [docs.themisdb.org](https://docs.themisdb.org)
- **GitHub Issues**: [Report bugs or request features](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [Community support](https://github.com/makr-code/ThemisDB/discussions)
- **Examples**: Each SDK includes working examples in its `examples/` directory

## License

Apache 2.0 - See [LICENSE](../LICENSE) for details.
