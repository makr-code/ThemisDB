# ThemisDB LLM-Specific SDKs

This directory contains LLM-specific client SDKs for ThemisDB. For general database client SDKs, see the [`clients/`](../clients/) directory.

## Overview

The `sdks/` directory focuses on LLM-specific operations (inference, RAG, embeddings) while the [`clients/`](../clients/) directory contains comprehensive database client SDKs with full CRUD operations, AQL queries, transactions, and more.

## LLM-Specific SDKs

### Python LLM SDK

**Status**: ✅ Available  
**Location**: [python/](python/)  
**Installation**: `pip install themis-llm`

```python
from themis_llm import ThemisLLMClient

client = ThemisLLMClient("http://localhost:8080", bearer_token="your-jwt-token")
response = client.infer(prompt="What is ThemisDB?", model="mistral-7b")
```

See [python/README.md](python/README.md) for complete documentation.

## General Database Client SDKs

For full-featured database clients with CRUD operations, AQL queries, transactions, and graph operations, see the **[`clients/`](../clients/) directory**:

- **[JavaScript/TypeScript](../clients/javascript/)** - ✅ Production Ready
- **[Go](../clients/go/)** - ✅ Production Ready
- **[Rust](../clients/rust/)** - ✅ Production Ready
- **[Java](../clients/java/)** - ✅ Production Ready
- **[Python](../clients/python/)** - ✅ Production Ready
- **[C# (.NET)](../clients/csharp/)** - ✅ Production Ready
- **[PHP](../clients/php/)** - ✅ Production Ready
- **[Ruby](../clients/ruby/)** - ✅ Production Ready
- **[Swift](../clients/swift/)** - ✅ Production Ready

See the [clients/README.md](../clients/README.md) for complete documentation on all client SDKs.


## Features

### LLM Operations (sdks/python/)
- `infer()` - Standard text generation
- `stream()` - Real-time token streaming
- `rag()` - Retrieval-Augmented Generation
- `embed()` - Embedding generation
- Model and LoRA management

### Full Database Operations (clients/)
The general client SDKs in [`clients/`](../clients/) provide comprehensive database functionality:
- CRUD operations for collections and documents
- AQL (Advanced Query Language) query execution
- Transaction support
- Graph operations (traverse, shortest path, neighbors)
- Vector operations
- LLM operations (inference, RAG, embeddings)
- Bearer Token (JWT) authentication
- Connection pooling and timeout management

## Contributing

We welcome contributions! Please see:
- [CONTRIBUTING.md](../CONTRIBUTING.md) - General contribution guidelines
- [clients/README.md](../clients/README.md) - Full client SDK documentation

## Support and Documentation

- **API Documentation**: [docs.themisdb.org](https://docs.themisdb.org)
- **Client SDKs**: [clients/README.md](../clients/README.md)
- **GitHub Issues**: [Report bugs or request features](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [Community support](https://github.com/makr-code/ThemisDB/discussions)

## License

Apache 2.0 - See [LICENSE](../LICENSE) for details.
