---
name: "🧮 Replace Hash-Based Embeddings with Base Model Embeddings"
about: Integrate actual base model embeddings instead of hash-based placeholders (High Priority - P1)
title: "[GPU Training] Implement Real Embedding Lookup from Base Model"
labels: priority:P1, type:feature, area:llm, area:gpu, effort:high, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: GPU-Training verwendet aktuell hash-basierte Embeddings als Platzhalter anstatt echter Embeddings aus dem Base Model. Dies führt zu schlechter Training-Qualität und verhindert sinnvolle Gradienten-Updates.

**EN**: GPU training currently uses hash-based embeddings as a placeholder instead of real embeddings from the base model. This leads to poor training quality and prevents meaningful gradient updates.

**Related Analysis**: `LORA_TRAINING_REVIEW.md` §2.2a (HIGH Priority)  
**Current Status**: `src/llm/lora_framework/gpu_training_loop.cpp:433-451` - TODO comment  
**Impact**: ⚠️ **MEDIUM Training Quality Degradation** - Embeddings nicht aligned mit Base Model

## 🎯 Ziele / Goals

- [ ] Base Model Embedding Layer Integration implementieren
- [ ] GPU-based Embedding Lookup (keine CPU-Transfers)
- [ ] Hash-basierte Embeddings komplett ersetzen
- [ ] Tests für Embedding Correctness auf GPU
- [ ] Performance-Optimierung (GPU Caching)

## 📝 Aufgaben / Tasks

### 1. GPU Embedding Lookup Implementation
**Priorität**: P1 - High

**Current Code** (Lines 433-451):
```cpp
// TODO: Replace with actual embedding lookup from base model
// Current implementation uses hash-based embeddings as a placeholder.
// Hash-based embeddings are only suitable for testing the training loop mechanics,
// but will produce poor training quality.

auto token_data = token_ids.cpu_data();  // ⚠️ CPU transfer!
std::vector<float> embedding_data(batch_size * hidden_dim);

for (size_t i = 0; i < batch_size; ++i) {
    for (size_t j = 0; j < hidden_dim; ++j) {
        size_t token_idx = j % seq_len;
        int token_id = static_cast<int>(token_data[i * seq_len + token_idx]);
        embedding_data[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;
    }
}

embeddings.upload(embedding_data);  // ⚠️ CPU transfer!
```

**Proposed Implementation**:
```cpp
GPUTensor createEmbeddingsOnGPU(
    const GPUTensor& token_ids,
    size_t hidden_dim,
    const Device& device,
    const BaseModel* base_model  // NEW parameter
) {
    size_t batch_size = token_ids.shape()[0];
    size_t seq_len = token_ids.shape()[1];
    
    // Create embedding tensor on GPU
    GPUTensor embeddings({batch_size, hidden_dim}, device);
    
    if (base_model && base_model->isLoaded()) {
        // ✅ Extract embeddings directly on GPU (no CPU transfer)
        embeddings = base_model->embedLayer()->forward(token_ids);
        
        spdlog::debug("Using real GPU embeddings from base model");
    } else {
        // Fallback to hash-based (standalone mode)
        spdlog::warn("No base model available, using hash-based embeddings");
        embeddings = generateHashEmbeddingsGPU(token_ids, hidden_dim);
    }
    
    return embeddings;
}
```

**Tasks**:
- [ ] Add `embedLayer()` method to BaseModelAdapter
- [ ] Implement GPU embedding lookup kernel (CUDA/HIP/Vulkan)
- [ ] Remove CPU↔GPU transfers for token data
- [ ] Add base_model parameter to createEmbeddingsOnGPU
- [ ] Keep hash-based fallback for standalone mode
- [ ] Add logging for embedding source

**File**: `src/llm/lora_framework/gpu_training_loop.cpp`

---

### 2. GPU Embedding Layer Abstraction
**Priorität**: P1 - High

**Implementation**:
```cpp
// File: include/llm/lora_framework/gpu_embedding_layer.h

class GPUEmbeddingLayer {
public:
    /**
     * @brief Construct GPU embedding layer from base model
     * @param embedding_weights GPU tensor [vocab_size, hidden_dim]
     * @param device Target device (CUDA, HIP, Vulkan, DirectX)
     */
    GPUEmbeddingLayer(const GPUTensor& embedding_weights, const Device& device);
    
    /**
     * @brief Forward pass: token IDs → embeddings (GPU kernel)
     * @param token_ids Token ID tensor (batch_size, seq_len) on GPU
     * @return Embedding tensor (batch_size, seq_len, hidden_dim) on GPU
     */
    GPUTensor forward(const GPUTensor& token_ids);
    
    /**
     * @brief Get embedding weight matrix
     */
    const GPUTensor& weights() const { return embedding_weights_; }
    
private:
    GPUTensor embedding_weights_;  // [vocab_size, hidden_dim] on GPU
    Device device_;
};
```

**Implementation** (`src/llm/lora_framework/gpu_embedding_layer.cpp`):
```cpp
GPUTensor GPUEmbeddingLayer::forward(const GPUTensor& token_ids) {
    // Shape: token_ids = [batch_size, seq_len]
    // Output: [batch_size, seq_len, hidden_dim]
    
    auto shape = token_ids.shape();
    size_t batch_size = shape[0];
    size_t seq_len = shape[1];
    size_t hidden_dim = embedding_weights_.shape()[1];
    
    GPUTensor embeddings({batch_size, seq_len, hidden_dim}, device_);
    
    // Launch GPU kernel for embedding lookup
    if (device_.type == DeviceType::CUDA) {
        launchCudaEmbeddingLookup(
            embeddings.gpu_ptr(),
            token_ids.gpu_ptr(),
            embedding_weights_.gpu_ptr(),
            batch_size, seq_len, hidden_dim
        );
    } else if (device_.type == DeviceType::HIP) {
        launchHipEmbeddingLookup(...);
    } else if (device_.type == DeviceType::VULKAN) {
        launchVulkanEmbeddingLookup(...);
    }
    
    return embeddings;
}
```

**Tasks**:
- [ ] Create GPUEmbeddingLayer class
- [ ] Implement CUDA embedding lookup kernel
- [ ] Implement HIP embedding lookup kernel
- [ ] Implement Vulkan compute shader for embedding
- [ ] Add DirectX compute shader support
- [ ] Optimize for memory coalescing
- [ ] Add unit tests

---

### 3. Base Model Integration
**Priorität**: P1 - High

**Integration with BaseModelAdapter**:
```cpp
// File: include/llm/lora_framework/base_model_adapter.h

class BaseModelAdapter {
public:
    /**
     * @brief Get GPU embedding layer
     * @return GPU embedding layer (nullptr if not loaded)
     */
    GPUEmbeddingLayer* getGPUEmbeddingLayer() const;
    
    /**
     * @brief Load embedding weights to GPU
     * @param device Target GPU device
     * @return true on success
     */
    bool loadEmbeddingsToGPU(const Device& device);
    
private:
    std::unique_ptr<GPUEmbeddingLayer> gpu_embedding_layer_;
};
```

**Tasks**:
- [ ] Add GPU embedding layer to BaseModelAdapter
- [ ] Load embedding weights from GGUF to GPU
- [ ] Handle different model architectures (Llama, Mistral, etc.)
- [ ] Add embedding weight caching
- [ ] Handle quantized embeddings (dequantization on GPU)
- [ ] Add memory management for large embedding matrices

---

### 4. GPU Training Loop Integration
**Priorität**: P1 - High

**Update trainStep Method**:
```cpp
// File: src/llm/lora_framework/gpu_training_loop.cpp

float GPUTrainingLoop::trainStep(const GPUBatch& batch) {
    size_t hidden_dim = 768;
    
    // ✅ NEW: Use base model embeddings if available
    GPUTensor input_embeddings;
    GPUTensor target_embeddings;
    
    if (base_model_ && base_model_->getGPUEmbeddingLayer()) {
        // Real embeddings from base model (on GPU)
        auto embed_layer = base_model_->getGPUEmbeddingLayer();
        input_embeddings = embed_layer->forward(batch.input_ids);
        target_embeddings = embed_layer->forward(batch.labels);
        
        // Average over sequence dimension for LoRA layer
        input_embeddings = input_embeddings.mean(/*dim=*/1);
        target_embeddings = target_embeddings.mean(/*dim=*/1);
    } else {
        // Fallback to hash-based (standalone mode)
        input_embeddings = createEmbeddingsOnGPU(
            batch.input_ids, hidden_dim, config_.device
        );
        target_embeddings = createEmbeddingsOnGPU(
            batch.labels, hidden_dim, config_.device
        );
    }
    
    // Rest of training step...
}
```

**Tasks**:
- [ ] Add base_model_ member to GPUTrainingLoop
- [ ] Update trainStep to use real embeddings
- [ ] Add embedding source logging
- [ ] Handle sequence dimension reduction (mean/max pooling)
- [ ] Update tests to validate embedding correctness

---

### 5. GPU Kernel Implementation
**Priorität**: P1 - High

**CUDA Kernel Example**:
```cuda
// File: src/llm/lora_framework/cuda_embedding_kernels.cu

__global__ void embeddingLookupKernel(
    float* output,              // [batch_size, seq_len, hidden_dim]
    const float* token_ids,     // [batch_size, seq_len]
    const float* embedding_weights,  // [vocab_size, hidden_dim]
    int batch_size,
    int seq_len,
    int hidden_dim,
    int vocab_size
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total_tokens = batch_size * seq_len;
    
    if (idx < total_tokens) {
        int batch_idx = idx / seq_len;
        int seq_idx = idx % seq_len;
        
        int token_id = static_cast<int>(token_ids[idx]);
        
        // Bounds check
        if (token_id >= 0 && token_id < vocab_size) {
            // Copy embedding vector
            const float* src = embedding_weights + token_id * hidden_dim;
            float* dst = output + idx * hidden_dim;
            
            for (int i = 0; i < hidden_dim; ++i) {
                dst[i] = src[i];
            }
        }
    }
}

void launchCudaEmbeddingLookup(...) {
    int threads = 256;
    int blocks = (batch_size * seq_len + threads - 1) / threads;
    
    embeddingLookupKernel<<<blocks, threads>>>(
        output, token_ids, embedding_weights,
        batch_size, seq_len, hidden_dim, vocab_size
    );
    
    cudaDeviceSynchronize();
}
```

**Tasks**:
- [ ] Implement CUDA embedding kernel
- [ ] Implement HIP embedding kernel (rocm)
- [ ] Implement Vulkan compute shader
- [ ] Optimize for memory coalescing
- [ ] Add bounds checking
- [ ] Benchmark kernel performance
- [ ] Add kernel fusion opportunities

---

### 6. Testing and Validation
**Priorität**: P1 - High

**Test Cases**:
```cpp
// Test file: tests/test_gpu_embedding_layer.cpp

TEST(GPUEmbeddingLayerTest, EmbeddingLookupCUDA) {
    // Load base model
    // Create GPU embedding layer
    // Lookup tokens [0, 1, 2, 3, 4]
    // Verify embeddings match CPU reference
}

TEST(GPUEmbeddingLayerTest, CompareHashVsReal) {
    // Train with hash-based embeddings
    // Train with real embeddings
    // Real should converge faster and lower final loss
}

TEST(GPUEmbeddingLayerTest, GradientAlignment) {
    // Compute gradients with real embeddings
    // Verify gradients point in meaningful directions
}

TEST(GPUEmbeddingLayerTest, PerformanceNoTransfers) {
    // Verify no CPU↔GPU transfers during embedding lookup
    // Profile with nsys/rocprof
}
```

**Tasks**:
- [ ] Create GPU embedding layer tests
- [ ] Compare training quality (hash vs real)
- [ ] Validate gradient correctness
- [ ] Performance profiling (no CPU transfers)
- [ ] Multi-backend testing (CUDA, HIP, Vulkan)

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

- [ ] Real embeddings from base model used in GPU training
- [ ] No CPU↔GPU transfers for embedding lookup (all on GPU)
- [ ] Hash-based embeddings replaced in production code
- [ ] Works with Llama, Mistral, GPT-NeoX models
- [ ] Training quality significantly improved (lower loss)
- [ ] Gradients properly aligned with base model space
- [ ] Performance acceptable (<0.5ms per batch embedding)
- [ ] Comprehensive GPU tests pass (>90% coverage)
- [ ] Fallback to hash-based works in standalone mode
- [ ] All GPU backends supported (CUDA, HIP, Vulkan, DirectX)

## 📊 Effort Estimation

- **Aufwand / Effort**: 2-3 weeks (High)
- **Komplexität / Complexity**: High (GPU kernel development)
- **Risiko / Risk**: Medium (requires GPU expertise)

## 🔗 Related Issues

- Issue #31: Real Embeddings Extraction
- Issue #32: llama.cpp Tokenizer Integration
- Code Review: `LORA_TRAINING_REVIEW.md` §2.2a

## 📚 References

- Code location: `src/llm/lora_framework/gpu_training_loop.cpp:433-451`
- Review analysis: `LORA_TRAINING_REVIEW.md` Section 2.2
- Base model adapter: `src/llm/lora_framework/base_model_adapter.cpp`
- GPU LoRA layers: `src/llm/lora_framework/gpu_lora_layers.cpp`

---

**Priority**: P1 - High priority for production quality  
**Impact**: Training quality, gradient alignment, performance  
**Status**: Ready to implement
