# LoRA Integration for Dynamic Rotation Patterns

## Overview

The LoRA-RoPE integration enables dynamic, context-aware rotation patterns in Rotary Position Embeddings (RoPE) through Low-Rank Adaptation (LoRA). This allows different LoRA adapters to modify rotation behavior for specific domains or tasks without retraining the entire embedding model.

## Key Concepts

### Rotary Position Embeddings (RoPE)

RoPE applies rotation-based positional encoding to transformer embeddings, enabling positional awareness through 2D coordinate rotations. For details, see the [RoFormer paper](https://arxiv.org/abs/2104.09864).

### Low-Rank Adaptation (LoRA)

LoRA introduces trainable low-rank matrices that modify model behavior with minimal parameter overhead. Instead of fine-tuning all parameters, LoRA adds small adaptation matrices to specific layers.

### LoRA-RoPE Integration

By combining RoPE with LoRA, we can:
- Create domain-specific rotation patterns (medical, legal, technical, etc.)
- Switch rotation behavior dynamically without model retraining
- Blend multiple adapters for hybrid scenarios
- Maintain low memory overhead through low-rank factorization

## Architecture

### Core Components

#### 1. LoRARopeAdapter

Stores low-rank adaptation parameters for RoPE:

```cpp
struct LoRARopeAdapter {
    std::string name;                           // Adapter identifier
    std::string domain;                         // Target domain
    size_t rank;                                // LoRA rank (r)
    float alpha;                                // LoRA scaling factor (α)
    
    std::vector<std::vector<double>> matrix_B;  // Down-projection (num_pairs × rank)
    std::vector<std::vector<double>> matrix_A;  // Up-projection (rank × num_pairs)
    std::vector<double> theta_delta;            // Theta adjustments
    
    bool enabled = true;
    float scaling = 1.0f;
};
```

The adapter modifies rotation through: `rotation' = rotation + α * scaling * (B @ A @ features)`

#### 2. LoRARopeAdapterRegistry

Thread-safe registry for managing multiple adapters:

```cpp
class LoRARopeAdapterRegistry {
public:
    bool registerAdapter(const LoRARopeAdapter& adapter);
    bool unregisterAdapter(const std::string& name);
    std::optional<LoRARopeAdapter> getAdapter(const std::string& name) const;
    std::vector<std::string> listAdapters() const;
    bool setAdapterEnabled(const std::string& name, bool enabled);
};
```

#### 3. LoRARotaryEmbedding

Extended RoPE class with LoRA adapter support:

```cpp
class LoRARotaryEmbedding : public RotaryEmbedding {
public:
    // Core rotation with adapter
    std::vector<float> rotateWithAdapter(
        const std::vector<float>& embedding,
        size_t position,
        const std::string& adapter_name
    ) const;
    
    // Batch processing
    std::vector<std::vector<float>> rotateBatchWithAdapter(
        const std::vector<std::vector<float>>& embeddings,
        const std::vector<size_t>& positions,
        const std::string& adapter_name
    ) const;
    
    // Adapter blending
    std::vector<float> rotateWithAdapterBlend(
        const std::vector<float>& embedding,
        size_t position,
        const std::vector<std::string>& adapter_names,
        const std::vector<float>& weights
    ) const;
};
```

## Usage Examples

### Basic Usage

```cpp
#include "index/lora_rope.h"

// Configure RoPE
RotationConfig config;
config.hidden_dim = 128;
config.num_rotation_pairs = 64;
config.base_theta = 10000.0;
config.computeThetaCache();

// Create LoRA-enhanced RoPE
LoRARotaryEmbedding lora_rope(config);

// Create and register a medical domain adapter
auto medical_adapter = LoRARopeAdapter::createRandom(
    "medical", "medical", 64, 8, 0.2f
);
lora_rope.registerAdapter("medical", medical_adapter);

// Apply rotation with adapter
std::vector<float> embedding = /* your embedding */;
auto rotated = lora_rope.rotateWithAdapter(embedding, 10, "medical");
```

### Multiple Domain Adapters

```cpp
// Create domain-specific adapters
auto medical = LoRARopeAdapter::createRandom("medical", "medical", 64, 8, 0.2f);
auto legal = LoRARopeAdapter::createRandom("legal", "legal", 64, 8, 0.2f);
auto technical = LoRARopeAdapter::createRandom("technical", "technical", 64, 8, 0.2f);

lora_rope.registerAdapter("medical", medical);
lora_rope.registerAdapter("legal", legal);
lora_rope.registerAdapter("technical", technical);

// Apply different adapters for different documents
auto medical_rotated = lora_rope.rotateWithAdapter(embedding, pos, "medical");
auto legal_rotated = lora_rope.rotateWithAdapter(embedding, pos, "legal");
auto technical_rotated = lora_rope.rotateWithAdapter(embedding, pos, "technical");
```

### Adapter Blending

```cpp
// Blend medical and legal adapters for medico-legal documents
auto blended = lora_rope.rotateWithAdapterBlend(
    embedding, position,
    {"medical", "legal"},
    {0.6f, 0.4f}  // 60% medical, 40% legal
);
```

### Batch Processing

```cpp
std::vector<std::vector<float>> embeddings = /* batch of embeddings */;
std::vector<size_t> positions = {0, 10, 20, 30, 40};

auto batch_rotated = lora_rope.rotateBatchWithAdapter(
    embeddings, positions, "medical"
);
```

### Dynamic Adapter Management

```cpp
// List all adapters
auto adapter_names = lora_rope.listAdapters();

// Check if adapter exists
bool has_medical = lora_rope.hasAdapter("medical");

// Disable adapter temporarily
lora_rope.setAdapterEnabled("medical", false);

// Re-enable adapter
lora_rope.setAdapterEnabled("medical", true);

// Remove adapter
lora_rope.unregisterAdapter("medical");
```

## Use Cases

### 1. Domain-Specific Search

Different domains may benefit from different rotation patterns:

```cpp
// Medical search
auto medical_query = lora_rope.rotateWithAdapter(query_embedding, 0, "medical");
// Search medical documents with medical-specific rotation

// Legal search
auto legal_query = lora_rope.rotateWithAdapter(query_embedding, 0, "legal");
// Search legal documents with legal-specific rotation
```

### 2. Task-Specific Embeddings

Different tasks may require different rotation behaviors:

```cpp
// For classification tasks
auto classification_rotated = lora_rope.rotateWithAdapter(
    embedding, position, "classification"
);

// For search/retrieval tasks
auto retrieval_rotated = lora_rope.rotateWithAdapter(
    embedding, position, "retrieval"
);

// For clustering tasks
auto clustering_rotated = lora_rope.rotateWithAdapter(
    embedding, position, "clustering"
);
```

### 3. Multi-Domain Documents

Use adapter blending for documents spanning multiple domains:

```cpp
// Scientific-technical document (70% scientific, 30% technical)
auto sci_tech = lora_rope.rotateWithAdapterBlend(
    embedding, position,
    {"scientific", "technical"},
    {0.7f, 0.3f}
);
```

## Performance Considerations

### Memory Overhead

LoRA adapters have minimal memory overhead due to low-rank factorization:

- Adapter size: `2 × num_rotation_pairs × rank × sizeof(double)`
- For `num_rotation_pairs=64`, `rank=8`: ~8 KB per adapter
- Full RoPE modification would require: ~32 KB

**Memory savings: ~75%**

### Computational Overhead

LoRA rotation adds minimal computation:

1. Base rotation: `O(num_rotation_pairs)`
2. LoRA forward pass: `O(num_rotation_pairs × rank)`
3. Additional rotation: `O(num_rotation_pairs)`

For typical values (`rank=8`), overhead is ~10-15% compared to base rotation.

### Batch Processing

Use batch operations for efficiency:

```cpp
// Efficient: Single batch call
auto results = lora_rope.rotateBatchWithAdapter(embeddings, positions, "adapter");

// Inefficient: Multiple individual calls
for (size_t i = 0; i < embeddings.size(); ++i) {
    auto result = lora_rope.rotateWithAdapter(embeddings[i], positions[i], "adapter");
}
```

## Training Adapters

While the current implementation focuses on using pre-trained adapters, future enhancements will include adapter training capabilities. Adapters can be trained using:

1. **Contrastive Learning**: Train adapters to maximize similarity within domain, minimize across domains
2. **Task-Specific Objectives**: Train adapters to optimize for specific downstream tasks
3. **Transfer Learning**: Initialize from base RoPE and fine-tune for domain

## API Reference

### LoRARopeAdapter

#### Static Factory Methods

- `createRandom(name, domain, num_pairs, rank, alpha)`: Create adapter with random initialization
- `createZero(name, domain, num_pairs, rank, alpha)`: Create adapter with zero initialization

#### Methods

- `isValid(num_rotation_pairs)`: Validate adapter dimensions

### LoRARopeAdapterRegistry

#### Methods

- `registerAdapter(adapter)`: Register new adapter
- `unregisterAdapter(name)`: Remove adapter
- `getAdapter(name)`: Retrieve adapter by name
- `hasAdapter(name)`: Check if adapter exists
- `listAdapters()`: Get all adapter names
- `setAdapterEnabled(name, enabled)`: Enable/disable adapter
- `clear()`: Remove all adapters
- `size()`: Get number of registered adapters

### LoRARotaryEmbedding

#### Constructors

- `LoRARotaryEmbedding(config)`: Create with new registry
- `LoRARotaryEmbedding(config, registry)`: Create with shared registry

#### Core Methods

- `rotateWithAdapter(embedding, position, adapter_name)`: Rotate with single adapter
- `rotateBatchWithAdapter(embeddings, positions, adapter_name)`: Batch rotation
- `rotateWithAdapterBlend(embedding, position, names, weights)`: Blend multiple adapters

#### Adapter Management

- `registerAdapter(name, adapter)`: Register adapter
- `unregisterAdapter(name)`: Unregister adapter
- `listAdapters()`: List all adapters
- `hasAdapter(name)`: Check adapter existence
- `setAdapterEnabled(name, enabled)`: Enable/disable adapter
- `getAdapterRegistry()`: Get registry reference

## Best Practices

1. **Choose Appropriate Rank**: Start with `rank=8` for most use cases. Increase for more expressiveness, decrease for memory efficiency.

2. **Scale Alpha Carefully**: Use `alpha ∈ [0.1, 0.5]` for subtle modifications, `alpha ∈ [0.5, 2.0]` for stronger effects.

3. **Normalize Weights for Blending**: The API automatically normalizes weights, but ensure they sum to positive values.

4. **Use Batch Operations**: Process multiple embeddings together for better performance.

5. **Cache Frequently Used Adapters**: Keep active adapters registered to avoid re-registration overhead.

6. **Profile Adapter Impact**: Test rotation similarity between base and adapted versions to ensure desired behavior.

## Future Enhancements

- Adapter training capabilities
- GPU acceleration for adapter operations
- Adapter serialization/deserialization
- Automatic adapter selection based on domain detection
- Hierarchical adapter composition
- Adapter pruning for further memory reduction

## References

1. Su, J., et al. (2021). "RoFormer: Enhanced Transformer with Rotary Position Embedding". arXiv:2104.09864
2. Hu, E. J., et al. (2021). "LoRA: Low-Rank Adaptation of Large Language Models". arXiv:2106.09685
3. ThemisDB Documentation: [Rotary Embeddings](./rotary-embeddings.md)

## Examples

See `examples/lora_rope_example.cpp` for a complete working example demonstrating all features.
