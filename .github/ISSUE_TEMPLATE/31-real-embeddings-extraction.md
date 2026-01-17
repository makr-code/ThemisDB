---
name: "🧮 Real Embeddings from Base Model"
about: Extract real embeddings from base model instead of hash-based (High Priority - P1)
title: "[LoRa] Implement Real Embeddings Extraction from Base Model"
labels: priority:P1, type:feature, area:llm, effort:medium, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Training verwendet aktuell hash-basierte Embeddings anstatt echte Embeddings aus dem Base Model. Dies kann die Training-Qualität beeinträchtigen und zu suboptimalen Gradienten führen.

**EN**: Training currently uses hash-based embeddings instead of real embeddings from the base model. This may impact training quality and lead to suboptimal gradients.

**Related Analysis**: `REMAINING_GAPS_SUMMARY.md` §4 (Priority 1)  
**Current Status**: `src/llm/lora_framework/lora_training_service.cpp:421` - TODO comment  
**Impact**: ⚠️ **Training Quality** - Embeddings nicht aligned mit Base Model

## 🎯 Ziele / Goals

- [ ] GGUF Base Model Embedding Extraction implementieren
- [ ] llama.cpp Embedding APIs nutzen
- [ ] Hash-basierte Embeddings ersetzen
- [ ] Tests für Embedding Correctness
- [ ] Performance-Optimierung (Caching)

## 📝 Aufgaben / Tasks

### 1. GGUF Embedding Extraction API
**Priorität**: P1 - High

**Current Code** (Line 421):
```cpp
// TODO: Extract embeddings from base model: enhanced_model->getBaseModel()->getEmbeddings(tokens)
// For now, use simple hash-based embeddings
std::vector<float> embeddings(tokens.size() * hidden_dim, 0.0f);
for (size_t i = 0; i < tokens.size(); ++i) {
    size_t hash = std::hash<int>{}(tokens[i]);
    float value = static_cast<float>(hash % 1000) / 1000.0f;
    for (size_t j = 0; j < hidden_dim; ++j) {
        embeddings[i * hidden_dim + j] = value;
    }
}
```

**Implementation**:
```cpp
std::vector<float> LoRATrainingService::Impl::extractEmbeddings(
    const std::vector<int>& tokens
) {
    if (!enhanced_model || !enhanced_model->getBaseModel()) {
        // Fallback to hash-based for standalone mode
        spdlog::warn("No base model available, using hash-based embeddings");
        return generateHashEmbeddings(tokens);
    }
    
    auto base_model = enhanced_model->getBaseModel();
    
    // Extract embeddings from GGUF model
    std::vector<float> embeddings;
    embeddings.reserve(tokens.size() * hidden_dim);
    
    for (int token_id : tokens) {
        // Use llama.cpp embedding extraction
        auto token_embedding = base_model->getTokenEmbedding(token_id);
        
        if (token_embedding.empty()) {
            spdlog::error("Failed to get embedding for token {}", token_id);
            // Fallback to hash for this token
            auto hash_emb = generateHashEmbedding(token_id);
            embeddings.insert(embeddings.end(), hash_emb.begin(), hash_emb.end());
        } else {
            embeddings.insert(embeddings.end(), token_embedding.begin(), token_embedding.end());
        }
    }
    
    return embeddings;
}
```

**Tasks**:
- [ ] Research llama.cpp embedding extraction APIs
- [ ] Implement `getTokenEmbedding()` in BaseModelAdapter
- [ ] Add error handling for missing embeddings
- [ ] Keep hash-based fallback for standalone mode
- [ ] Add logging for debugging

**File**: `src/llm/lora_framework/lora_training_service.cpp`

---

### 2. BaseModelAdapter Embedding Support
**Priorität**: P1 - High

**Implementation**:
```cpp
// File: include/llm/lora_framework/base_model_adapter.h

class BaseModelAdapter {
public:
    /**
     * @brief Extract embedding vector for a token
     * @param token_id Token ID
     * @return Embedding vector (size = model's hidden_dim)
     */
    std::vector<float> getTokenEmbedding(int token_id) const;
    
    /**
     * @brief Extract embeddings for multiple tokens (batched)
     * @param token_ids Vector of token IDs
     * @return Flattened embedding matrix [num_tokens * hidden_dim]
     */
    std::vector<float> getTokenEmbeddings(const std::vector<int>& token_ids) const;
    
    /**
     * @brief Get embedding matrix for all vocabulary
     * @return Full embedding matrix [vocab_size * hidden_dim]
     */
    const float* getEmbeddingMatrix() const;
};
```

**Implementation** (`src/llm/lora_framework/base_model_adapter.cpp`):
```cpp
std::vector<float> BaseModelAdapter::getTokenEmbedding(int token_id) const {
    if (!gguf_loader_) {
        throw std::runtime_error("GGUF loader not initialized");
    }
    
    // 1. Find embedding tensor in GGUF
    const auto& metadata = gguf_loader_->getMetadata();
    std::string embed_tensor_name = "token_embd.weight";  // or model-specific name
    
    // 2. Get tensor pointer
    void* tensor_ptr = gguf_loader_->mmapTensor(embed_tensor_name);
    if (!tensor_ptr) {
        throw std::runtime_error("Embedding tensor not found: " + embed_tensor_name);
    }
    
    // 3. Extract single row (token embedding)
    size_t hidden_dim = getHiddenDim();
    const float* embed_matrix = static_cast<const float*>(tensor_ptr);
    
    // 4. Copy token embedding
    std::vector<float> embedding(hidden_dim);
    const float* token_embed = embed_matrix + (token_id * hidden_dim);
    std::copy(token_embed, token_embed + hidden_dim, embedding.begin());
    
    return embedding;
}

std::vector<float> BaseModelAdapter::getTokenEmbeddings(
    const std::vector<int>& token_ids
) const {
    std::vector<float> embeddings;
    embeddings.reserve(token_ids.size() * getHiddenDim());
    
    for (int token_id : token_ids) {
        auto emb = getTokenEmbedding(token_id);
        embeddings.insert(embeddings.end(), emb.begin(), emb.end());
    }
    
    return embeddings;
}
```

**Tasks**:
- [ ] Add embedding extraction methods to BaseModelAdapter
- [ ] Handle different tensor names (Llama vs Mistral vs GPT-NeoX)
- [ ] Add caching for frequently used embeddings
- [ ] Handle quantized embeddings (dequantization)
- [ ] Add unit tests

---

### 3. Embedding Caching
**Priorität**: P1 - High

**Implementation**:
```cpp
class BaseModelAdapter {
private:
    // LRU cache for embeddings
    mutable std::unordered_map<int, std::vector<float>> embedding_cache_;
    mutable size_t cache_hits_ = 0;
    mutable size_t cache_misses_ = 0;
    static constexpr size_t MAX_CACHE_SIZE = 10000;  // Cache top 10k tokens
    
public:
    std::vector<float> getTokenEmbedding(int token_id) const {
        // Check cache first
        auto it = embedding_cache_.find(token_id);
        if (it != embedding_cache_.end()) {
            ++cache_hits_;
            return it->second;
        }
        
        ++cache_misses_;
        
        // Extract from GGUF
        auto embedding = extractEmbeddingFromGGUF(token_id);
        
        // Add to cache if not full
        if (embedding_cache_.size() < MAX_CACHE_SIZE) {
            embedding_cache_[token_id] = embedding;
        }
        
        return embedding;
    }
    
    void logCacheStats() const {
        float hit_rate = 0.0f;
        if (cache_hits_ + cache_misses_ > 0) {
            hit_rate = static_cast<float>(cache_hits_) / 
                      (cache_hits_ + cache_misses_) * 100.0f;
        }
        spdlog::info("Embedding cache: {} hits, {} misses, {:.1f}% hit rate",
                     cache_hits_, cache_misses_, hit_rate);
    }
};
```

**Tasks**:
- [ ] Implement LRU cache for embeddings
- [ ] Add cache statistics logging
- [ ] Make cache size configurable
- [ ] Add cache warm-up for common tokens
- [ ] Benchmark cache performance

---

### 4. Testing and Validation
**Priorität**: P1 - High

**Test Cases**:
```cpp
// Test file: tests/test_base_model_embeddings.cpp

TEST(BaseModelEmbeddingsTest, SingleTokenEmbedding) {
    // Load Llama-2-7B model
    // Extract embedding for token 0 (BOS)
    // Verify shape [1, 4096]
    // Verify values are reasonable (not all zeros, not all same)
}

TEST(BaseModelEmbeddingsTest, MultipleTokenEmbeddings) {
    // Extract embeddings for [0, 1, 2, 3, 4]
    // Verify shape [5, 4096]
    // Verify embeddings are different
}

TEST(BaseModelEmbeddingsTest, EmbeddingConsistency) {
    // Extract same token embedding twice
    // Should return identical values
}

TEST(BaseModelEmbeddingsTest, CachePerformance) {
    // Extract 1000 random tokens
    // Extract same 1000 tokens again
    // Second pass should be faster (>90% cache hit)
}

TEST(BaseModelEmbeddingsTest, TrainingWithRealEmbeddings) {
    // Train with real embeddings vs hash-based
    // Real embeddings should converge faster
    // Final loss should be lower
}

TEST(BaseModelEmbeddingsTest, GradientAlignment) {
    // Compute gradients with real embeddings
    // Verify gradients align with base model space
}
```

**Validation Strategy**:
```cpp
// Compare hash-based vs real embeddings
void validateEmbeddingQuality() {
    std::vector<int> test_tokens = {0, 1, 2, 100, 1000};
    
    // Hash-based
    auto hash_emb = generateHashEmbeddings(test_tokens);
    
    // Real embeddings
    auto real_emb = extractEmbeddings(test_tokens);
    
    // Compute cosine similarity between consecutive tokens
    // Hash-based: random similarity
    // Real embeddings: structured similarity (semantic relationships)
    
    spdlog::info("Hash-based embedding quality: random");
    spdlog::info("Real embedding quality: structured semantic space");
}
```

**Tasks**:
- [ ] Create comprehensive test suite
- [ ] Test with multiple model architectures
- [ ] Validate embedding correctness
- [ ] Compare training quality (hash vs real)
- [ ] Add performance benchmarks

---

### 5. Update Training Service Integration
**Priorität**: P1 - High

**Replace Hash-Based Code**:
```cpp
// OLD (Line 421):
// Hash-based embeddings
std::vector<float> embeddings(tokens.size() * hidden_dim, 0.0f);
for (size_t i = 0; i < tokens.size(); ++i) {
    size_t hash = std::hash<int>{}(tokens[i]);
    // ... hash logic
}

// NEW:
// Real embeddings from base model
std::vector<float> embeddings;
if (enhanced_model && enhanced_model->getBaseModel()) {
    embeddings = enhanced_model->getBaseModel()->getTokenEmbeddings(tokens);
    spdlog::debug("Using real embeddings from base model");
} else {
    // Fallback for standalone mode
    embeddings = generateHashEmbeddings(tokens);
    spdlog::debug("Using hash-based embeddings (standalone mode)");
}
```

**Tasks**:
- [ ] Replace all hash-based embedding calls
- [ ] Update forward pass to use real embeddings
- [ ] Update backward pass if needed
- [ ] Add configuration flag: `use_real_embeddings` (default: true)
- [ ] Keep hash-based as fallback for standalone

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

- [ ] Real embeddings extracted from GGUF base models
- [ ] Embedding extraction works for Llama, Mistral, GPT-NeoX
- [ ] Hash-based embeddings replaced in training service
- [ ] Embedding cache implemented with >90% hit rate
- [ ] Training quality improved (lower final loss)
- [ ] Gradients properly aligned with base model space
- [ ] Comprehensive tests pass (>90% coverage)
- [ ] Performance acceptable (<1ms per token embedding)
- [ ] Fallback to hash-based works in standalone mode

## 📊 Effort Estimation

- **Aufwand / Effort**: 1 week (Medium)
- **Komplexität / Complexity**: Medium
- **Risiko / Risk**: Low-Medium (well-defined task)

## 🔗 Related Issues

- Issue #07: LoRa llama.cpp Integration
- Issue #30: LoRa Adapter Application
- Original analysis: `REMAINING_GAPS_SUMMARY.md` §4

## 📚 References

- Code location: `src/llm/lora_framework/lora_training_service.cpp:421`
- Base model adapter: `src/llm/lora_framework/base_model_adapter.cpp`
- GGUF loader: llama.cpp gguf.h/cpp
- Embedding theory: https://arxiv.org/abs/2103.00020 (LoRA paper)

---

**Priority**: P1 - High priority for production quality  
**Impact**: Training quality, gradient alignment  
**Status**: Ready to implement
