# LLM Response Cache - Usage Guide

## Overview

Production-ready LLM response cache with semantic similarity using HNSW indexing from ThemisDB's VectorIndexManager.

## Key Features

✅ **No Code Duplication**: Reuses existing ThemisDB infrastructure
- VectorIndexManager with HNSW for ANN search
- EmbeddedLLM::embed() for real LLM embeddings
- RocksDB for persistent storage via pointer exchange

✅ **Flexible Embedding Strategy**:
1. Custom embedding function (highest priority)
2. LLM instance (EmbeddedLLM) for real semantic embeddings
3. Simple feature-based fallback

✅ **Production Ready**:
- Thread-safe operations
- LRU eviction policy
- TTL-based expiration
- Comprehensive metrics

## Usage Examples

### Basic Usage (with fallback embeddings)

```cpp
#include "llm/llm_response_cache.h"

using namespace themis::llm;

// Basic configuration
LLMResponseCache::Config config;
config.similarity_threshold = 0.90f;
config.ttl_seconds = 3600;
config.max_entries = 10000;

LLMResponseCache cache("my_cache", config);

// Store response
InferenceResponse response;
response.text = "Paris is the capital of France.";
response.tokens_generated = 50;
response.inference_time_ms = 150.0f;
cache.put("What is the capital of France?", response);

// Retrieve (exact or semantic match)
auto cached = cache.get("What is the capital of France?");
if (cached) {
    std::cout << "Cache hit: " << cached->text << std::endl;
}
```

### Advanced Usage (with real LLM embeddings)

```cpp
#include "llm/llm_response_cache.h"
#include "llm/embedded_llm.h"

using namespace themis::llm;

// Initialize LLM for embeddings
EmbeddedLLM::Config llm_config;
llm_config.model_path = "models/embeddings.gguf";
auto llm = std::make_unique<EmbeddedLLM>(llm_config);

// Configure cache with LLM pointer exchange
LLMResponseCache::Config config;
config.llm_ptr = llm.get();  // Use real LLM embeddings
config.embedding_dim = 768;  // Match LLM embedding size
config.similarity_threshold = 0.95f;  // Higher threshold with real embeddings

LLMResponseCache cache("semantic_cache", config);

// Now cache uses real semantic embeddings!
```

### Custom Embedding Function

```cpp
LLMResponseCache::Config config;

// Use custom embedding service (e.g., OpenAI, sentence-transformers)
config.embedding_fn = [](const std::string& text) -> std::vector<float> {
    // Call your embedding service
    return myEmbeddingService.embed(text);
};

LLMResponseCache cache("custom_cache", config);
```

### Pointer Exchange with Shared RocksDB

```cpp
// Share RocksDB instance between cache and main DB
RocksDBWrapper::Config db_config;
db_config.db_path = "/data/themis";
auto db = std::make_unique<RocksDBWrapper>(db_config);

LLMResponseCache::Config cache_config;
cache_config.db_ptr = db.get();  // Zero-copy integration

LLMResponseCache cache("shared_db_cache", cache_config);
```

## Performance Benchmarks

Run benchmarks:
```bash
./build/benchmarks/bench_llm_response_cache
```

Expected results:
- **Cache Put**: ~0.5-2ms per operation
- **Cache Get (Hit)**: ~0.1-0.5ms per operation
- **Cache Get (Semantic)**: ~1-5ms per operation (HNSW search)
- **Cache Hit Rate**: 70-90% with semantic matching
- **Speedup**: 75x faster than full LLM inference (2ms vs 150ms)

## Testing

Run unit tests:
```bash
./build/tests/test_llm_response_cache
```

Tests cover:
- Basic put/get operations
- Semantic similarity matching
- TTL expiration
- LRU eviction
- Concurrent access
- Cache statistics

## Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `similarity_threshold` | 0.90 | Cosine similarity threshold for cache hits |
| `ttl_seconds` | 3600 | Time-to-live for cache entries (1 hour) |
| `max_entries` | 10000 | Maximum cache size (LRU eviction) |
| `cache_dir` | "./llm_cache" | Directory for RocksDB storage |
| `embedding_dim` | 384 | Embedding vector dimension |
| `use_vector_index` | true | Enable HNSW index |
| `db_ptr` | nullptr | External RocksDB instance (pointer exchange) |
| `llm_ptr` | nullptr | LLM instance for embeddings (pointer exchange) |
| `embedding_fn` | nullptr | Custom embedding function |

## Integration with ThemisDB Core

The cache uses **pointer exchange pattern** to avoid duplication:

```cpp
// In your ThemisDB initialization
auto db = initializeRocksDB();
auto llm = initializeEmbeddedLLM();

// Configure cache with shared resources
LLMResponseCache::Config config;
config.db_ptr = db.get();     // Shared storage
config.llm_ptr = llm.get();   // Shared embeddings
config.cache_dir = db->getPath();

auto cache = std::make_unique<LLMResponseCache>("main_cache", config);
```

## Metrics

Get cache statistics:
```cpp
auto stats = cache.getStatistics();
std::cout << "Hit rate: " << stats.getHitRate() << std::endl;
std::cout << "Total entries: " << stats.total_entries << std::endl;
std::cout << "Avg lookup time: " << stats.avg_lookup_time_ms << "ms" << std::endl;
```

Integrate with Grafana metrics:
```cpp
#include "llm/grafana_metrics.h"

auto metrics = std::make_shared<LLMMetricsCollector>();
cache.setMetricsCollector(metrics.get());

// Metrics are now automatically recorded
```

## Production Deployment

### Recommended Settings

```cpp
LLMResponseCache::Config config;

// Use real LLM embeddings for best semantic matching
config.llm_ptr = &llm_instance;
config.embedding_dim = 768;  // or 1536 for OpenAI ada-002

// Tune for your workload
config.similarity_threshold = 0.95f;  // Higher with real embeddings
config.ttl_seconds = 7200;           // 2 hours
config.max_entries = 50000;          // Scale based on memory

// Share resources
config.db_ptr = &main_db;

// Optional: Custom embeddings for best quality
config.embedding_fn = openai_embed;
```

### Monitoring

Monitor these metrics:
- `cache_hit_rate`: Should be >70% for good caching
- `avg_lookup_time_ms`: Should be <5ms
- `total_entries`: Should stay below max_entries
- `cache_size_mb`: Monitor memory usage

## Troubleshooting

### Low Hit Rate

- Check `similarity_threshold` (try lowering to 0.85-0.90)
- Verify embeddings are working (check logs for fallback warnings)
- Ensure prompts are normalized

### High Memory Usage

- Reduce `max_entries`
- Lower `ttl_seconds` for faster cleanup
- Check for memory leaks in custom embedding_fn

### Slow Lookups

- Verify HNSW index is enabled (`use_vector_index = true`)
- Check if RocksDB storage is on fast SSD
- Monitor vector index statistics

## Architecture

```
LLMResponseCache
    ├── VectorIndexManager (HNSW ANN search)
    │   └── RocksDB (persistent storage)
    ├── Embedding Generation
    │   ├── Custom Function (priority 1)
    │   ├── EmbeddedLLM::embed() (priority 2)
    │   └── Simple Features (fallback)
    └── Response Store (in-memory map)
```

## See Also

- [VectorIndexManager Documentation](../index/vector_index.h)
- [EmbeddedLLM Documentation](../llm/embedded_llm.h)
- [Performance Benchmarks](../../benchmarks/bench_llm_response_cache.cpp)
- [Unit Tests](../../tests/test_llm_response_cache.cpp)
