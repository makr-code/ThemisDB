# Rotary Position Embeddings (RoPE) in ThemisDB

## Overview

ThemisDB integrates **Rotary Position Embeddings (RoPE)** from transformer inference models into its vector storage layer, enabling advanced positional encoding capabilities for sequential entities, knowledge graph operations, and temporal data processing.

## Scientific Foundation

RoPE was introduced in the paper "RoFormer: Enhanced Transformer with Rotary Position Embedding" by Su, J., et al. (2021), arXiv:2104.09864. The core innovation is applying rotation matrices to embedding coordinates to encode positional information:

### Mathematical Formulation

For a hidden dimension `d`, RoPE computes rotation frequencies:

```
θᵢ = base^(-2i/d)  where i ∈ [0, d/2)
```

For each coordinate pair `(x₀, x₁)` at position `m`, a 2D rotation is applied:

```
[x'₀]   [cos(mθ)  -sin(mθ)] [x₀]
[x'₁] = [sin(mθ)   cos(mθ)] [x₁]
```

The full transformation for position `m` is:

```
f(xₘ) = R(xₘ, mθ₀) ⊕ R(xₘ, mθ₁) ⊕ ... ⊕ R(xₘ, mθ_{d/2-1})
```

This approach preserves:
- **Relative position information** between tokens/entities
- **Vector magnitude** (rotation is orthogonal)
- **Computational efficiency** through precomputed theta cache

## Architecture in ThemisDB

### Core Components

```
include/index/rotary_embeddings.h          # RoPE implementation header
src/index/rotary_embeddings.cpp            # Implementation
tests/test_rotary_embeddings.cpp           # Unit tests (gtest)
benchmarks/bench_rotary_embeddings.cpp     # Performance benchmarks
```

### Integration Points

1. **BaseEntity**: Extended to store rotation metadata
   - `embedding_rotation_pos`: Position used for rotation
   - `embedding_rotation_type`: Relation type (for relational rotation)

2. **VectorIndexManager**: Core integration point
   - Rotation-aware entity addition
   - Rotation-aware search operations
   - Relational rotation for knowledge graphs

## Use Cases

### 1. Sequential Document Processing

Perfect for documents with inherent ordering (e.g., chapters, pages, time-series):

```cpp
// Initialize rotation configuration
RotationConfig config;
config.hidden_dim = 768;
config.num_rotation_pairs = 384;
config.base_theta = 10000.0;
config.computeThetaCache();

vector_mgr->setRotaryEmbeddingConfig(config);

// Add documents with positional encoding
for (size_t i = 0; i < documents.size(); ++i) {
    BaseEntity doc(documents[i].id);
    doc.setField("embedding", documents[i].embedding);
    doc.setField("content", documents[i].text);
    
    // Position i encodes sequential order
    vector_mgr->addEntityWithRotation(doc, "embedding", i);
}

// Search with positional awareness
std::vector<float> query = getQueryEmbedding("Find document near position 100");
auto [status, results] = vector_mgr->searchWithRotation(query, 10, 100);
```

### 2. Knowledge Graph Relations

Enable TransE-like relational embeddings in vector space:

```cpp
// Entity with "parent_of" relation
BaseEntity parent("entity_A");
parent.setField("embedding", embedding_A);

vector_mgr->addEntityWithRelationalRotation(
    parent, "embedding", "parent_of"
);

// Entity with "child_of" relation
BaseEntity child("entity_B");
child.setField("embedding", embedding_B);

vector_mgr->addEntityWithRelationalRotation(
    child, "embedding", "child_of"
);

// Different relation types get unique rotations
// Enables queries like: "Find entities with 'sibling_of' relation"
```

### 3. Temporal Data Encoding

Embed timestamps as rotational positions:

```cpp
// Convert timestamp to position
size_t position = timestamp / time_quantum;

BaseEntity event("event_" + std::to_string(event_id));
event.setField("embedding", event_embedding);
event.setField("timestamp", timestamp);

vector_mgr->addEntityWithRotation(event, "embedding", position);

// Search for events near a specific time
size_t query_position = query_timestamp / time_quantum;
auto [status, results] = vector_mgr->searchWithRotation(
    query_embedding, 20, query_position
);
```

### 4. Multi-Relational Vector Search

Combine positional and relational encoding:

```cpp
// Knowledge graph with temporal ordering
BaseEntity kg_node("node_X");
kg_node.setField("embedding", node_embedding);
kg_node.setField("created_at", creation_time);

// First apply temporal rotation
size_t temporal_pos = creation_time / 3600; // hourly buckets
RotaryEmbedding rope(config);
auto temp_rotated = rope.rotate(node_embedding, temporal_pos);

// Then apply relational rotation
auto final_embedding = rope.rotateRelational(temp_rotated, "belongs_to");

kg_node.setField("embedding", final_embedding);
vector_mgr->addEntity(kg_node, "embedding");
```

## API Reference

### RotationConfig

Configuration for rotary embeddings:

```cpp
struct RotationConfig {
    size_t hidden_dim;              // Embedding dimension (must be even)
    size_t num_rotation_pairs;      // Number of 2D rotation pairs
    double base_theta = 10000.0;    // Base frequency (from RoPE paper)
    bool normalize_after = false;   // L2 normalize after rotation
    
    std::vector<double> theta_cache; // Precomputed theta values
    
    void computeThetaCache();        // Compute theta cache
    bool isValid() const;            // Validate configuration
};
```

**Example:**

```cpp
RotationConfig config;
config.hidden_dim = 512;
config.num_rotation_pairs = 256;  // Must be <= hidden_dim / 2
config.base_theta = 10000.0;      // Standard value from RoPE paper
config.normalize_after = false;    // Optional L2 normalization
config.computeThetaCache();        // Required before use

if (!config.isValid()) {
    throw std::invalid_argument("Invalid configuration");
}
```

### RotaryEmbedding Class

Core rotation operations:

```cpp
class RotaryEmbedding {
public:
    explicit RotaryEmbedding(const RotationConfig& config);
    
    // Core operations
    std::vector<float> rotate(const std::vector<float>& embedding, size_t position) const;
    std::vector<float> rotateInverse(const std::vector<float>& embedding, size_t position) const;
    
    // Batch operations
    std::vector<std::vector<float>> rotateBatch(
        const std::vector<std::vector<float>>& embeddings,
        const std::vector<size_t>& positions
    ) const;
    
    // Relational rotation (for KG)
    std::vector<float> rotateRelational(
        const std::vector<float>& embedding,
        const std::string& relation_type
    ) const;
    
    // Configuration access
    const RotationConfig& getConfig() const;
};
```

### VectorIndexManager Extensions

```cpp
// Enable/disable rotary embeddings
Status setRotaryEmbeddingConfig(const RotationConfig& config);
bool isRotaryEmbeddingEnabled() const;
std::optional<RotationConfig> getRotaryEmbeddingConfig() const;

// Add with rotation
Status addEntityWithRotation(
    const BaseEntity& e,
    std::string_view vectorField,
    size_t position
);

// Add with relational rotation
Status addEntityWithRelationalRotation(
    const BaseEntity& e,
    std::string_view vectorField,
    const std::string& relation_type
);

// Search with rotation
std::pair<Status, std::vector<Result>> searchWithRotation(
    const std::vector<float>& query,
    int k,
    size_t query_position,
    const std::vector<std::string>* whitelistPks = nullptr
) const;
```

### BaseEntity Extensions

```cpp
// Check if entity has rotated embedding
bool hasRotatedEmbedding(std::string_view field_name) const;

// Get rotation position
std::optional<size_t> getRotationPosition(std::string_view field_name) const;

// Get rotation type (relation)
std::optional<std::string> getRotationType(std::string_view field_name) const;
```

## Performance Characteristics

### Computational Complexity

- **Single rotation**: O(d) where d is the embedding dimension
- **Batch rotation**: O(n×d) where n is batch size
- **Theta cache computation**: O(d/2), done once at initialization
- **Relational rotation**: O(d) + O(1) hash lookup (cached)

### Benchmark Results

Based on `bench_rotary_embeddings.cpp`:

| Operation | Dimension | Throughput | Notes |
|-----------|-----------|------------|-------|
| Single Rotation | 128 | ~1-2 µs | Fast for typical use |
| Single Rotation | 1024 | ~8-15 µs | Scales linearly |
| Batch Rotation (100) | 128 | ~150-200 µs | ~1.5-2 µs per vector |
| Relational Rotation | 128 | ~1-2 µs | Similar to single rotation |
| VectorIndex Integration | 128 | <10% overhead | Minimal performance impact |

**Key Findings:**

1. **Negligible Overhead**: Adding rotary embeddings incurs <10% performance overhead compared to standard vector operations
2. **Linear Scaling**: Performance scales linearly with embedding dimension
3. **Batch Efficiency**: Batch operations achieve near-linear scaling
4. **Cache Effectiveness**: Precomputed theta cache eliminates repeated calculations

## Configuration Best Practices

### Dimension Selection

```cpp
// Standard dimensions (typical embeddings)
config.hidden_dim = 768;        // BERT-base, RoBERTa
config.hidden_dim = 1024;       // BERT-large
config.hidden_dim = 4096;       // GPT-3, LLaMA-7B

// Always use even dimensions
config.num_rotation_pairs = config.hidden_dim / 2;
```

### Base Theta Selection

```cpp
// Standard value (from RoPE paper)
config.base_theta = 10000.0;    // Good for most use cases

// Longer sequences (>2048 tokens)
config.base_theta = 100000.0;   // Better long-range encoding

// Shorter sequences (<512 tokens)
config.base_theta = 1000.0;     // More fine-grained positions
```

### Normalization

```cpp
// Generally not needed (rotation preserves magnitude)
config.normalize_after = false;

// Use if combining with other operations that may affect magnitude
config.normalize_after = true;
```

## Integration with Existing Features

### HNSW Index Compatibility

RoPE works seamlessly with HNSW vector search:

```cpp
// Initialize HNSW index
vector_mgr->init("vectors", 768, VectorIndexManager::Metric::COSINE,
                16,    // M parameter
                200,   // efConstruction
                64);   // efSearch

// Enable rotary embeddings
RotationConfig config;
config.hidden_dim = 768;
config.num_rotation_pairs = 384;
config.computeThetaCache();
vector_mgr->setRotaryEmbeddingConfig(config);

// HNSW automatically uses rotated vectors
```

### Encryption Support

Rotated embeddings work with ThemisDB's field-level encryption:

```cpp
// Enable vector encryption
vector_mgr->setVectorEncryptionEnabled(true);
vector_mgr->setVectorKeyId("my_vector_key");

// Rotation happens before encryption
vector_mgr->addEntityWithRotation(entity, "embedding", position);
// → Rotate → Encrypt → Store
```

### Transaction Support

Rotary embeddings participate in ACID transactions:

```cpp
auto txn = db->beginTransaction();

for (const auto& doc : batch) {
    vector_mgr->addEntityWithRotation(doc, "embedding", doc.position, txn);
}

txn.commit();  // All rotations persisted atomically
```

## Testing

Comprehensive test suite in `tests/test_rotary_embeddings.cpp`:

```bash
# Build tests
cmake --build . --target themis_tests

# Run all RoPE tests
./tests/themis_tests --gtest_filter="*RotaryEmbedding*"

# Run specific test
./tests/themis_tests --gtest_filter="RotaryEmbeddingTest.InverseRotation"
```

**Test Coverage:**

- ✅ Configuration validation
- ✅ Theta cache computation
- ✅ Basic rotation operations
- ✅ Inverse rotation (rotation + inverse = identity)
- ✅ Positional orthogonality
- ✅ Batch operations
- ✅ Relational rotation
- ✅ VectorIndexManager integration
- ✅ BaseEntity metadata support
- ✅ Error handling

## Benchmarking

Performance benchmarks in `benchmarks/bench_rotary_embeddings.cpp`:

```bash
# Build benchmarks
cmake --build . --target bench_rotary_embeddings

# Run all benchmarks
./benchmarks/bench_rotary_embeddings

# Run specific benchmark
./benchmarks/bench_rotary_embeddings --benchmark_filter="SingleRotation"

# Output JSON results
./benchmarks/bench_rotary_embeddings --benchmark_format=json > results.json
```

## Troubleshooting

### Common Issues

**Issue**: `Invalid RotationConfig` error

```cpp
// Problem: Odd dimension
config.hidden_dim = 127;  // ❌ Must be even
config.num_rotation_pairs = 64;

// Solution:
config.hidden_dim = 128;  // ✅ Even dimension
config.num_rotation_pairs = 64;
```

**Issue**: `theta_cache is empty` error

```cpp
// Problem: Forgot to compute cache
RotationConfig config;
config.hidden_dim = 128;
// ❌ Missing: config.computeThetaCache();
RotaryEmbedding rope(config);  // Throws exception

// Solution:
config.computeThetaCache();  // ✅ Compute before use
RotaryEmbedding rope(config);
```

**Issue**: Dimension mismatch in rotation

```cpp
// Problem: Vector size doesn't match config
std::vector<float> vec(256);  // 256-dim vector
RotaryEmbedding rope(config);  // config.hidden_dim = 128
rope.rotate(vec, 0);  // ❌ Throws exception

// Solution: Match dimensions
std::vector<float> vec(128);  // ✅ Matches config
```

## Future Extensions

Planned enhancements (not yet implemented):

- [ ] **CUDA/HIP Kernels**: GPU-accelerated rotation for large batches
- [ ] **Learned Rotation Parameters**: Trainable θ values for domain-specific optimization
- [ ] **LoRA Integration**: Combine with LoRA adapters for dynamic rotation patterns
- [ ] **REST API Endpoints**: HTTP endpoints for rotation configuration
- [ ] **Visualization Tools**: Tools to visualize rotated embedding spaces

## References

1. **Su, J., Lu, Y., Pan, S., Wen, B., & Liu, Y.** (2021). "RoFormer: Enhanced Transformer with Rotary Position Embedding." *arXiv preprint arXiv:2104.09864*. https://arxiv.org/abs/2104.09864

2. **Vaswani, A., et al.** (2017). "Attention Is All You Need." *Advances in Neural Information Processing Systems*.

3. **Bordes, A., et al.** (2013). "Translating Embeddings for Modeling Multi-relational Data" (TransE). *Advances in Neural Information Processing Systems*.

## Support

For questions, issues, or feature requests related to RoPE in ThemisDB:

- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://themisdb.com/docs/features/rotary-embeddings
- Community Forum: https://community.themisdb.com

---

**Last Updated**: 2026-04-06  
**Version**: 1.5.0+  
**Status**: Production Ready
