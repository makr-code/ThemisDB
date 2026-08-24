## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


> **⚠️ STATUS: STALE – Archivierungskandidat**
> Dieser Inhalt enthält veraltete TODO/FIXME/PLACEHOLDER-Marker und wird im nächsten Archiv-Run nach `docs/ARCHIVED/` verschoben.
> Bitte nicht als aktuelle Referenz nutzen. Inventar: [DOCS_INVENTORY_2026-Q3.md](Audit/DOCS_INVENTORY_2026-Q3.md)

---

# Quick Start Guide - Phase 1 Implementation

**Purpose**: Get started with Phase 1 (Critical Blockers) implementation  
**Target Audience**: Engineers assigned to Issues #1-#4  
**Prerequisites**: C++17, CMake, GPU development experience

---

## 🚀 Getting Started

### 1. Environment Setup

#### Hardware Requirements
- **Minimum**: 16GB RAM, 1x GPU with 8GB VRAM
- **Recommended**: 32GB RAM, 1x GPU with 12GB+ VRAM (RTX 3090/4090, or AMD equivalent)
- **Storage**: 50GB free space

#### Software Requirements

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    vulkan-tools \
    libvulkan-dev \
    mesa-vulkan-drivers

# Optional: CUDA (NVIDIA)
# Download from https://developer.nvidia.com/cuda-downloads

# Optional: ROCm (AMD)
# Follow https://rocmdocs.amd.com/en/latest/Installation_Guide/Installation-Guide.html
```

#### Clone and Build

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Initialize submodules (llama.cpp)
git submodule update --init --recursive

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure
```

---

## 📋 Issue #1: llama.cpp Infrastructure (Weeks 1-3)

### Goal
Replace stub implementations with real llama.cpp integration.

### Files to Modify
1. `src/llm/llama_wrapper.cpp` - Main wrapper implementation
2. `src/llm/llama_resource_manager.cpp` - RAII resource management
3. `include/llm/llama_resource_manager.h` - Header definitions

### Test Model Download

```bash
# Download TinyLlama 1.1B (small model for testing)
cd ThemisDB/models

# Option 1: Using wget
wget https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf

# Option 2: Using curl (if wget not available)
curl -L -o tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
  https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf

# Verify download
ls -lh tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf
# Should be ~670MB

# Note: Model URL may change. Check https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF
# for the latest model versions and download links.
```

### Step-by-Step Implementation

#### Step 1.1: Model Loading (Week 1)

**Current Code** (`llama_wrapper.cpp:200-210`):
```cpp
bool LlamaWrapper::loadModel(const std::string& model_path) {
    spdlog::warn("LlamaWrapper: Model loading is stubbed");
    impl_->model_handle = nullptr;  // Always null!
    return false;
}
```

**Target Implementation**:
```cpp
bool LlamaWrapper::loadModel(const std::string& model_path) {
    // 1. Check if file exists
    if (!std::filesystem::exists(model_path)) {
        spdlog::error("Model file not found: {}", model_path);
        return false;
    }
    
    // 2. Setup model parameters
    llama_model_params model_params = llama_model_default_params();
    // TODO: Make n_gpu_layers configurable based on VRAM availability
    // For TinyLlama 1.1B: 32 layers fit in ~4GB VRAM
    // Auto-detect or make this a configuration parameter
    model_params.n_gpu_layers = 32; // Offload layers to GPU
    model_params.use_mmap = true;   // Memory-map for efficiency
    model_params.use_mlock = false; // Don't lock in RAM
    
    // 3. Load model
    spdlog::info("Loading model from: {}", model_path);
    llama_model* model = llama_model_load_from_file(
        model_path.c_str(), 
        model_params
    );
    
    if (!model) {
        spdlog::error("Failed to load model: {}", model_path);
        return false;
    }
    
    // 4. Create context
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 2048;        // Context size
    ctx_params.n_batch = 512;       // Batch size
    // TODO: Use std::thread::hardware_concurrency() or make configurable
    ctx_params.n_threads = 8;       // CPU threads (adjust for your system)
    ctx_params.n_threads_batch = 8; // Batch threads
    
    llama_context* ctx = llama_new_context_with_model(model, ctx_params);
    if (!ctx) {
        spdlog::error("Failed to create context");
        llama_free_model(model);
        return false;
    }
    
    // 5. Store in RAII wrapper
    impl_->model_handle = std::make_unique<LlamaModelHandle>(model);
    impl_->context_handle = std::make_unique<LlamaContextHandle>(ctx);
    
    spdlog::info("Model loaded successfully: vocab_size={}, n_ctx={}", 
                 llama_n_vocab(model), llama_n_ctx(ctx));
    
    return true;
}
```

**Testing**:
```cpp
// Create test file: tests/test_llama_model_loading.cpp
TEST(LlamaWrapper, LoadValidModel) {
    LlamaWrapper wrapper;
    ASSERT_TRUE(wrapper.loadModel("models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"));
    ASSERT_TRUE(wrapper.isLoaded());
}

TEST(LlamaWrapper, LoadInvalidPath) {
    LlamaWrapper wrapper;
    ASSERT_FALSE(wrapper.loadModel("nonexistent.gguf"));
}
```

#### Step 1.2: Token Generation (Week 2)

**Current Code** (`llama_wrapper.cpp:206`):
```cpp
std::string LlamaWrapper::generate(const GenerationRequest& request) {
    spdlog::warn("LlamaWrapper: Using stub response");
    return "[Generated response placeholder for: " + request.prompt + "]";
}
```

**Target Implementation**:
```cpp
std::string LlamaWrapper::generate(const GenerationRequest& request) {
    if (!impl_->context_handle) {
        throw std::runtime_error("Model not loaded");
    }
    
    auto* model = impl_->model_handle->get();
    auto* ctx = impl_->context_handle->get();
    
    // 1. Tokenize prompt
    // Note: Actual llama.cpp API may vary by version. Check llama.h for exact signature.
    // Example: llama_tokenize(model, text, text_len, tokens, n_tokens_max, add_special, parse_special)
    std::vector<llama_token> tokens;
    tokens.resize(request.prompt.length() + 16); // Reserve space
    
    int n_tokens = llama_tokenize(
        model,
        request.prompt.c_str(),
        request.prompt.length(),
        tokens.data(),
        tokens.size(),
        true,  // add_bos
        false  // parse_special
    );
    
    if (n_tokens < 0) {
        throw std::runtime_error("Failed to tokenize prompt");
    }
    tokens.resize(n_tokens);
    
    // 2. Evaluate prompt tokens
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size(), 0, 0);
    if (llama_decode(ctx, batch) != 0) {
        throw std::runtime_error("Failed to decode prompt");
    }
    
    // 3. Generate tokens
    std::string output;
    int n_gen = 0;
    
    while (n_gen < request.max_tokens) {
        // Sample next token
        // Note: Sampling API varies by llama.cpp version. This is a simplified example.
        // Real implementation needs to:
        // 1. Get logits from context: llama_get_logits(ctx)
        // 2. Apply sampling strategy (greedy, top-p, etc.)
        // 3. Select token based on probabilities
        // See llama.cpp examples/main for reference implementation
        
        auto* logits = llama_get_logits(ctx);
        llama_token new_token = impl_->sampling_strategy->sample(logits, llama_n_vocab(model));
        
        // Check for end-of-generation
        if (llama_token_is_eog(model, new_token)) {
            break;
        }
        
        // Convert token to text
        char buf[128];
        int n = llama_token_to_piece(model, new_token, buf, sizeof(buf));
        if (n < 0) {
            throw std::runtime_error("Failed to decode token");
        }
        output.append(buf, n);
        
        // Decode next position
        llama_batch next_batch = llama_batch_get_one(&new_token, 1, tokens.size() + n_gen, 0);
        if (llama_decode(ctx, next_batch) != 0) {
            throw std::runtime_error("Failed to decode token");
        }
        
        n_gen++;
    }
    
    spdlog::debug("Generated {} tokens: {}", n_gen, output);
    return output;
}
```

**Testing**:
```cpp
TEST(LlamaWrapper, GenerateSimplePrompt) {
    LlamaWrapper wrapper;
    ASSERT_TRUE(wrapper.loadModel("models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"));
    
    GenerationRequest req;
    req.prompt = "What is 2+2?";
    req.max_tokens = 50;
    
    std::string output = wrapper.generate(req);
    ASSERT_FALSE(output.empty());
    ASSERT_NE(output.find("4"), std::string::npos); // Should mention "4"
}
```

#### Step 1.3: GPU Backend Integration (Week 3)

**Target**: Auto-detect and use GPU backends (Vulkan → CUDA → CPU).

```cpp
// In llama_resource_manager.cpp
class BackendAwareLlamaModelHandle {
public:
    BackendAwareLlamaModelHandle(const std::string& model_path) {
        // 1. Detect available backends
        std::vector<std::string> available_backends;
        
        if (has_vulkan()) available_backends.push_back("vulkan");
        if (has_cuda()) available_backends.push_back("cuda");
        if (has_hip()) available_backends.push_back("hip");
        available_backends.push_back("cpu");
        
        // 2. Try backends in priority order
        for (const auto& backend : available_backends) {
            try {
                model_params_.backend = backend;
                model_ = llama_model_load_from_file(
                    model_path.c_str(), 
                    model_params_
                );
                
                if (model_) {
                    spdlog::info("Loaded model with backend: {}", backend);
                    break;
                }
            } catch (const std::exception& e) {
                spdlog::warn("Failed to load with {}: {}", backend, e.what());
            }
        }
        
        if (!model_) {
            throw std::runtime_error("Failed to load model with any backend");
        }
    }
    
private:
    bool has_vulkan() {
        // Check for Vulkan device
        return VulkanBackend::isAvailable();
    }
    
    bool has_cuda() {
        // Check for CUDA device
        return CUDABackend::isAvailable();
    }
    
    bool has_hip() {
        // Check for HIP/ROCm device
        return HIPBackend::isAvailable();
    }
};
```

---

## 📋 Issue #2: Sampling Strategies (Weeks 4-5)

### Goal
Implement real sampling strategies that work with llama.cpp.

### Files to Modify
1. `src/llm/sampling_strategy.cpp`
2. `include/llm/sampling_strategy.h`

### Implementation

```cpp
// Greedy sampling (deterministic)
llama_token GreedySampling::sample(
    llama_context* ctx, 
    const std::vector<llama_token_data>& candidates
) {
    // Return token with highest probability
    return std::max_element(
        candidates.begin(), 
        candidates.end(),
        [](const auto& a, const auto& b) { return a.logit < b.logit; }
    )->id;
}

// Nucleus sampling (Top-P)
llama_token NucleusSampling::sample(
    llama_context* ctx, 
    const std::vector<llama_token_data>& candidates
) {
    // 1. Sort by probability (descending)
    auto sorted = candidates;
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.logit > b.logit; });
    
    // 2. Calculate cumulative probabilities
    float cumsum = 0.0f;
    std::vector<llama_token_data> nucleus;
    
    for (const auto& candidate : sorted) {
        nucleus.push_back(candidate);
        cumsum += std::exp(candidate.logit);
        
        if (cumsum >= config_.top_p) break;
    }
    
    // 3. Sample from nucleus
    return sample_from_distribution(nucleus);
}
```

**Testing**:
```cpp
TEST(GreedySampling, IsDeterministic) {
    GreedySampling sampler;
    
    std::vector<llama_token_data> candidates = {
        {0, 0.1f}, {1, 0.7f}, {2, 0.2f}
    };
    
    // Should always return token 1 (highest prob)
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(sampler.sample(nullptr, candidates), 1);
    }
}
```

---

## 📋 Issue #3: Security Validation (Weeks 5-7)

### Goal
Implement cryptographic signature verification using OpenSSL.

### Files to Modify
1. `src/llm/security/signature_verifier.cpp`
2. `include/llm/security/signature_verifier.h`

### Implementation

```cpp
bool RSA_SHA256_Verifier::verify(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& signature,
    const std::string& public_key_pem
) {
    // 1. Load public key
    BIO* bio = BIO_new_mem_buf(public_key_pem.data(), public_key_pem.size());
    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!pkey) {
        return false;
    }
    
    // 2. Create verification context
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey);
    
    // 3. Verify signature
    EVP_DigestVerifyUpdate(mdctx, data.data(), data.size());
    int result = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());
    
    // 4. Cleanup
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);
    
    return result == 1;
}
```

**Testing**:
```bash
# Generate test certificate
cd tests/data/certificates
./generate_test_certs.sh
```

```cpp
TEST(RSA_SHA256_Verifier, ValidSignature) {
    RSA_SHA256_Verifier verifier;
    
    auto data = load_file("tests/data/test_data.bin");
    auto signature = load_file("tests/data/test_signature.sig");
    auto pubkey = load_file("tests/data/public_key.pem");
    
    ASSERT_TRUE(verifier.verify(data, signature, pubkey));
}

TEST(RSA_SHA256_Verifier, TamperedDataDetected) {
    RSA_SHA256_Verifier verifier;
    
    auto data = load_file("tests/data/test_data.bin");
    data[0] ^= 0xFF; // Tamper with data
    
    auto signature = load_file("tests/data/test_signature.sig");
    auto pubkey = load_file("tests/data/public_key.pem");
    
    ASSERT_FALSE(verifier.verify(data, signature, pubkey));
}
```

---

## 📋 Issue #4: LoRA Training (Weeks 7-14)

### Goal
Implement real LoRA training with tensor operations and optimization.

### Files to Create/Modify
1. `src/llm/lora_framework/tensor.cpp` (new)
2. `src/llm/lora_framework/optimizer.cpp` (new)
3. `src/llm/lora_framework/lora_training_service.cpp` (modify)

### Simple Tensor Implementation

```cpp
class Tensor {
public:
    Tensor(const std::vector<size_t>& shape) 
        : shape_(shape) {
        size_t total = 1;
        for (auto dim : shape) total *= dim;
        data_.resize(total, 0.0f);
    }
    
    // Matrix multiplication: C = A * B
    static Tensor matmul(const Tensor& A, const Tensor& B) {
        // A: [M, K], B: [K, N] -> C: [M, N]
        assert(A.shape_[1] == B.shape_[0]);
        
        size_t M = A.shape_[0];
        size_t K = A.shape_[1];
        size_t N = B.shape_[1];
        
        Tensor C({M, N});
        
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                float sum = 0.0f;
                for (size_t k = 0; k < K; k++) {
                    sum += A.at(i, k) * B.at(k, j);
                }
                C.at(i, j) = sum;
            }
        }
        
        return C;
    }
    
    // Element-wise operations
    Tensor operator+(const Tensor& other) const;
    Tensor operator-(const Tensor& other) const;
    Tensor operator*(float scalar) const;
    
private:
    std::vector<float> data_;
    std::vector<size_t> shape_;
};
```

### LoRA Forward Pass

```cpp
Tensor LoRALayer::forward(const Tensor& x) {
    // LoRA: output = W * x + (B * (A * x)) * scaling
    
    Tensor Wx = Tensor::matmul(W_, x);        // Base model output
    Tensor Ax = Tensor::matmul(A_, x);        // Low-rank path (down)
    Tensor BAx = Tensor::matmul(B_, Ax);      // Low-rank path (up)
    Tensor lora_delta = BAx * scaling_;       // Scale
    
    return Wx + lora_delta;                   // Combine
}
```

### LoRA Backward Pass

```cpp
void LoRALayer::backward(const Tensor& grad_output) {
    // Gradients for A and B matrices
    
    // grad_B = grad_output * (A * x)^T
    grad_B_ = Tensor::matmul(grad_output, Ax_.transpose());
    
    // grad_A = B^T * grad_output * x^T
    Tensor grad_intermediate = Tensor::matmul(B_.transpose(), grad_output);
    grad_A_ = Tensor::matmul(grad_intermediate, x_.transpose());
    
    // Scale gradients
    grad_A_ = grad_A_ * scaling_;
    grad_B_ = grad_B_ * scaling_;
}
```

### Adam Optimizer

```cpp
void AdamOptimizer::step(
    Tensor& param, 
    const Tensor& grad
) {
    // Adam update rule with bias correction
    // m_t = β1 * m_{t-1} + (1 - β1) * grad
    // v_t = β2 * v_{t-1} + (1 - β2) * grad^2
    // m_hat = m_t / (1 - β1^t)
    // v_hat = v_t / (1 - β2^t)
    // param = param - lr * m_hat / (sqrt(v_hat) + ε)
    
    m_ = m_ * beta1_ + grad * (1 - beta1_);
    v_ = v_ * beta2_ + grad.square() * (1 - beta2_);
    
    // Pre-compute bias correction factors for efficiency
    // Cache these if updating multiple parameters in the same step
    float bias_correction1 = 1.0f - std::pow(beta1_, t_);
    float bias_correction2 = 1.0f - std::pow(beta2_, t_);
    
    Tensor m_hat = m_ / bias_correction1;
    Tensor v_hat = v_ / bias_correction2;
    
    param = param - m_hat / (v_hat.sqrt() + epsilon_) * learning_rate_;
    
    t_++;
}
```

### Training Loop

```cpp
void LoRATrainingService::train() {
    for (size_t epoch = 0; epoch < config_.num_epochs; epoch++) {
        for (auto& batch : data_loader_) {
            // Forward pass
            Tensor output = model_.forward(batch.input);
            
            // Compute loss
            float loss = criterion_.compute(output, batch.target);
            
            // Backward pass
            Tensor grad_output = criterion_.gradient();
            model_.backward(grad_output);
            
            // Optimizer step
            optimizer_.step();
            
            // Logging
            metrics_.current_loss = loss;
            metrics_.current_step++;
            
            spdlog::info("Epoch {}/{}, Step {}, Loss: {:.4f}", 
                         epoch+1, config_.num_epochs, 
                         metrics_.current_step, loss);
        }
    }
}
```

**Testing**:
```cpp
TEST(LoRALayer, ForwardPass) {
    LoRALayer layer(/*in_dim=*/10, /*out_dim=*/10, /*rank=*/2);
    
    Tensor input({1, 10});
    input.fill_random();
    
    Tensor output = layer.forward(input);
    
    ASSERT_EQ(output.shape(), std::vector<size_t>({1, 10}));
}

TEST(LoRATraining, ConvergesOnXOR) {
    // Train on simple XOR problem
    LoRATrainingService trainer;
    
    std::vector<std::pair<Tensor, Tensor>> xor_data = {
        {{0, 0}, {0}},
        {{0, 1}, {1}},
        {{1, 0}, {1}},
        {{1, 1}, {0}}
    };
    
    float initial_loss = trainer.evaluate(xor_data);
    trainer.train(xor_data, /*epochs=*/1000);
    float final_loss = trainer.evaluate(xor_data);
    
    ASSERT_LT(final_loss, initial_loss * 0.1); // Loss should decrease by 10x
}
```

---

## 📚 Additional Resources

### Documentation
- **llama.cpp API**: https://github.com/ggerganov/llama.cpp/blob/master/common/common.h
- **OpenSSL EVP**: https://www.openssl.org/docs/man3.0/man7/evp.html
- **LoRA Paper**: https://arxiv.org/abs/2106.09685

### Example Projects
- **llama.cpp examples**: `llama.cpp/examples/main/main.cpp`
- **Vulkan compute**: `llama.cpp/ggml-vulkan.cpp`

### Testing
```bash
# Run specific test
ctest -R test_llama_model_loading -V

# Run all LLM tests
ctest -R llm -V

# Memory leak check
valgrind --leak-check=full ./build/tests/test_llama_wrapper

# Performance profiling
perf record ./build/tests/benchmark_inference
perf report
```

---

## ❓ FAQ

**Q: Which GPU backend should I use?**
A: Vulkan is prioritized for cross-platform compatibility. Use CUDA if you need maximum NVIDIA performance.

**Q: How do I debug llama.cpp issues?**
A: Enable llama.cpp logging: `export LLAMA_DEBUG=1`. Check `llama.cpp/examples/` for reference implementations.

**Q: The model is too slow!**
A: Increase `n_gpu_layers` to offload more layers to GPU. Check VRAM usage with `nvidia-smi` or `rocm-smi`.

**Q: Training doesn't converge!**
A: Check learning rate (try 1e-3 to 1e-5), batch size, and gradient clipping. Verify gradients with numerical gradient check.

**Q: How do I add a new sampling strategy?**
A: Inherit from `ISamplingStrategy`, implement `sample()` method, register in `SamplingStrategyFactory`.

---

**Last Updated**: 2026-04-06  
**Status**: Ready for Phase 1 implementation  
**Questions**: Create issue or contact team lead
