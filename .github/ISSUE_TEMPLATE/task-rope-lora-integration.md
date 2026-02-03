---
name: "RoPE Enhancement: LoRA Integration"
about: Combine RoPE with LoRA adapters for dynamic rotation patterns
title: '[RoPE] LoRA Integration for Dynamic Rotation Patterns'
labels: 'enhancement, priority:P2, area:vector-index, component:rotary-embeddings, component:lora, effort:medium'
assignees: ''
---

## Feature Description

Integrate Rotary Position Embeddings (RoPE) with ThemisDB's LoRA (Low-Rank Adaptation) framework to enable dynamic, context-aware rotation patterns. This allows different LoRA adapters to modify rotation behavior for specific domains or tasks without retraining the entire embedding model.

## Problem Statement

Current RoPE implementation uses static rotation parameters for all use cases. However, different domains (medical, legal, technical) or tasks (search, classification, clustering) may benefit from different rotation patterns. Retraining embeddings is expensive; LoRA adapters provide a lightweight alternative.

## Proposed Solution

### Core Components

1. **LoRA-Enhanced RoPE Layer** (`include/index/lora_rope.h`)
   ```cpp
   class LoRARotaryEmbedding : public RotaryEmbedding {
   public:
       // Initialize with LoRA adapter registry
       LoRARotaryEmbedding(
           const RotationConfig& config,
           std::shared_ptr<LoRAAdapterRegistry> adapter_registry
       );
       
       // Rotation with LoRA adapter
       std::vector<float> rotateWithAdapter(
           const std::vector<float>& embedding,
           size_t position,
           const std::string& adapter_name
       ) const;
       
       // Register LoRA adapter for RoPE
       void registerAdapter(
           const std::string& name,
           const LoRARopeAdapter& adapter
       );
       
   private:
       std::shared_ptr<LoRAAdapterRegistry> adapter_registry_;
       std::unordered_map<std::string, LoRARopeAdapter> adapters_;
   };
   ```

2. **LoRA RoPE Adapter** (`include/lora/lora_rope_adapter.h`)
   ```cpp
   struct LoRARopeAdapter {
       // Low-rank matrices for theta adjustment
       std::vector<float> A;  // Down-projection (rank × hidden_dim/2)
       std::vector<float> B;  // Up-projection (hidden_dim/2 × rank)
       int rank;              // LoRA rank (typically 8-64)
       float alpha;           // Scaling factor
       
       // Compute LoRA-adjusted theta
       std::vector<double> adjustTheta(
           const std::vector<double>& base_theta,
           size_t position
       ) const {
           // ΔΘ = α * B * A
           // Θ_adapted = Θ_base + ΔΘ
       }
   };
   ```

3. **VectorIndexManager Integration**
   ```cpp
   // Add entity with LoRA-adapted rotation
   Status addEntityWithLoRARotation(
       const BaseEntity& e,
       std::string_view vectorField,
       size_t position,
       const std::string& lora_adapter_name
   );
   
   // Search with LoRA-adapted query rotation
   std::pair<Status, std::vector<Result>> searchWithLoRARotation(
       const std::vector<float>& query,
       int k,
       size_t query_position,
       const std::string& lora_adapter_name
   ) const;
   ```

## Technical Design

### LoRA Adaptation Formula

Base RoPE: `θᵢ = base^(-2i/d)`

LoRA-Adapted RoPE:
```
θᵢ' = θᵢ + α * (B * A)ᵢ
where:
  A ∈ ℝ^(r×d/2)  (down-projection)
  B ∈ ℝ^(d/2×r)  (up-projection)
  r << d/2       (rank, typically 8-64)
```

**Memory Efficiency:**
- Standard theta cache: d/2 values (512 bytes for 768-dim)
- LoRA adapter: 2*r*(d/2) values (e.g., 24KB for r=32, d=768)
- Multiple adapters: 24KB × num_adapters (can be lazy-loaded)

### Adapter Training

**Training Data:**
```cpp
struct LoRATrainingSample {
    std::vector<float> embedding;
    size_t position;
    std::string domain;  // e.g., "medical", "legal"
    float target_similarity;
};
```

**Training Process:**
1. Freeze base RoPE parameters
2. Train only LoRA matrices (A, B) for each domain
3. Use contrastive loss or task-specific objective
4. Save adapters to disk

**Storage Format (HDF5):**
```
/adapters/
  /medical/
    rank: 32
    alpha: 1.0
    A: [32 × 384 matrix]
    B: [384 × 32 matrix]
  /legal/
    rank: 32
    alpha: 1.0
    A: [32 × 384 matrix]
    B: [384 × 32 matrix]
```

## Implementation Considerations

### Dependencies
- ThemisDB LoRA Framework (`include/lora/lora_manager.h`)
- HDF5 or similar for adapter storage
- Integration with existing LoRA adapter registry

### CMakeLists.txt Integration
```cmake
# No new dependencies - reuses existing LoRA infrastructure
target_link_libraries(rotary_embeddings
    lora_manager
    adapter_registry
)
```

### Adapter Selection Strategy

1. **Explicit**: User specifies adapter name in query
2. **Auto-Detection**: Infer domain from query content
3. **Multi-Adapter**: Blend multiple adapters with weights
4. **Fallback**: Use base RoPE if adapter not found

### Performance Impact

| Operation | Base RoPE | LoRA RoPE | Overhead |
|-----------|-----------|-----------|----------|
| Single Rotation | 1-2 µs | 1.5-3 µs | +50% |
| Batch (100) | 150-200 µs | 200-300 µs | +33% |
| Adapter Loading | N/A | ~1 ms | One-time |

**Mitigation:**
- Cache adapted theta values per adapter
- Lazy-load adapters on first use
- Async adapter loading for large adapter sets

## Use Cases

1. **Multi-Domain Search**
   - Medical adapter for clinical documents
   - Legal adapter for case law
   - Technical adapter for patents
   - Switch adapters based on query domain

2. **Task-Specific Rotation**
   - Classification adapter (emphasize local context)
   - Retrieval adapter (emphasize global patterns)
   - Clustering adapter (balance local/global)

3. **Temporal Adaptation**
   - Recent adapter (fine-grained time resolution)
   - Historical adapter (coarse-grained time buckets)
   - Switch based on query time range

## Example Usage

```cpp
// Initialize with LoRA support
auto adapter_registry = std::make_shared<LoRAAdapterRegistry>();
LoRARotaryEmbedding lora_rope(config, adapter_registry);

// Register domain-specific adapters
LoRARopeAdapter medical_adapter = loadAdapter("medical_rope.h5");
lora_rope.registerAdapter("medical", medical_adapter);

// Add documents with domain-specific rotation
BaseEntity medical_doc("doc1");
medical_doc.setField("embedding", embedding);
vector_mgr->addEntityWithLoRARotation(medical_doc, "embedding", 0, "medical");

// Search with adapter
auto [status, results] = vector_mgr->searchWithLoRARotation(
    query, 10, query_pos, "medical"
);

// Dynamic adapter switching
lora_rope.registerAdapter("legal", loadAdapter("legal_rope.h5"));
auto legal_results = vector_mgr->searchWithLoRARotation(
    query, 10, query_pos, "legal"
);
```

## Alternative Solutions

1. **Multiple RoPE Instances**: Create separate RotaryEmbedding instances per domain (higher memory)
2. **Mixture of Experts**: Route to different RoPE experts based on input (more complex)
3. **Conditional RoPE**: Condition rotation on embedding content (requires forward pass)

## Related Features

- LoRA Training Framework ([#existing_issue])
- Multi-LoRA Fusion ([#existing_issue])
- LoRA Adapter Registry ([#existing_issue])
- GPU LoRA Kernels ([#existing_issue])

## Additional Context

**References:**
- LoRA Paper: https://arxiv.org/abs/2106.09685
- ThemisDB LoRA Implementation: `LORA_ADAPTER_IMPLEMENTATION_COMPLETE.md`
- Adapter-Based Transfer Learning: https://arxiv.org/abs/1902.00751

**Integration Benefits:**
- Reuses existing LoRA infrastructure
- Compatible with LoRA adapter management UI
- Supports multi-adapter scenarios
- Minimal memory overhead per adapter

**Priority:** P2 (Medium) - Builds on existing LoRA framework  
**Effort:** 2-3 weeks  
**Complexity:** Medium (requires understanding of both RoPE and LoRA)

---

**Checklist:**
- [ ] I have searched existing issues to ensure this is not a duplicate
- [ ] I have clearly described the problem this feature solves
- [ ] I have provided a detailed description of the proposed solution
- [ ] I have considered the impact on existing functionality
- [ ] I have identified integration points with existing LoRA framework
- [ ] I have specified adapter storage and loading mechanisms
