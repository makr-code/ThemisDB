---
name: LoRA Framework - Real LLM Integration
about: Replace placeholder responses with real LLM inference using llama.cpp
title: '[FEATURE] Implement Real LLM Integration for LoRA Framework'
labels: ['enhancement', 'llm', 'lora-framework', 'integration']
assignees: ''
---

## Description

Replace placeholder responses in `themis_help_lora.cpp` with real LLM inference using llama.cpp integration, enabling actual adapter loading and text generation.

## Motivation

Real LLM integration would:
- Enable actual text generation with LoRA adapters
- Validate llama.cpp integration architecture
- Provide working documentation assistant
- Demonstrate framework capabilities end-to-end
- Enable real-world testing and evaluation
- Support production deployments

## Current State

**Placeholder implementation** in `themis_help_lora.cpp`:
```cpp
std::string ThemisHelpLoRA::query(const std::string& question, const std::string& user_id) {
    // TODO: Replace with actual LLM inference
    return "This is a placeholder response. Real LLM integration pending.";
}
```

## Proposed Solution

Integrate with existing ThemisDB LLM infrastructure:
1. Connect to `EmbeddedLLM` or `LlamaWrapper`
2. Load base models via `llama_model_load()`
3. Load LoRA adapters via `llama_lora_adapter_init()`
4. Apply adapters via `llama_lora_adapter_set()`
5. Generate text via `llama_decode()` and `llama_sample_token()`

## Implementation Details

### Architecture

```
ThemisHelpLoRA
      ↓
LlamaWrapper (new or existing)
      ↓
llama.cpp C API
      ↓
GPU/CPU execution
```

### Required Changes

#### 1. LlamaWrapper Integration

**File**: `src/llm/llama_wrapper.h` (extend existing or create)

```cpp
class LlamaWrapper {
public:
    // Model management
    bool loadModel(const std::string& model_path,
                   const ModelParams& params);
    bool unloadModel();
    
    // LoRA management
    bool loadLoRAAdapter(const std::string& adapter_path,
                        float scale = 1.0f);
    bool unloadLoRAAdapter(const std::string& adapter_id);
    bool switchLoRAAdapter(const std::string& from_id,
                          const std::string& to_id);
    
    // Inference
    std::string generate(const std::string& prompt,
                        const GenerationParams& params);
    std::vector<float> getEmbedding(const std::string& text);
    
    // Multi-LoRA support
    bool loadMultipleAdapters(
        const std::vector<std::string>& adapter_paths);
    bool setActiveAdapter(const std::string& adapter_id);
    
private:
    llama_model* model_;
    llama_context* context_;
    std::map<std::string, llama_lora_adapter*> adapters_;
    std::string active_adapter_id_;
};
```

#### 2. Update themis_help_lora.cpp

**File**: `src/llm/applications/themis_help_lora.cpp`

```cpp
class ThemisHelpLoRA::Impl {
private:
    std::shared_ptr<LlamaWrapper> llama_wrapper_;
    std::string base_model_id_;
    std::string adapter_id_;
    
public:
    Impl() {
        // Initialize llama.cpp wrapper
        llama_wrapper_ = std::make_shared<LlamaWrapper>();
        
        // Load base model
        auto model_info = model_storage_->getModel("llama-2-7b");
        llama_wrapper_->loadModel(model_info.gguf_path, ModelParams{});
        
        // Load LoRA adapter
        auto adapter_info = lora_storage_->getAdapterInfo("themis_help_lora");
        if (adapter_info.weights_path.empty()) {
            // Train initial adapter
            trainInitial();
        }
        llama_wrapper_->loadLoRAAdapter(adapter_info.weights_path);
    }
    
    std::string query(const std::string& question, 
                     const std::string& user_id) {
        // Build prompt
        std::string prompt = buildPrompt(question);
        
        // Generate response with LoRA
        GenerationParams params;
        params.max_tokens = 500;
        params.temperature = 0.7;
        params.top_p = 0.9;
        
        std::string response = llama_wrapper_->generate(prompt, params);
        
        // Log inference audit
        logInferenceAudit(question, response, user_id);
        
        return response;
    }
    
    bool trainFromFeedback() {
        // Collect feedback samples
        auto training_data = collectFeedbackSamples();
        
        // Train LoRA adapter
        auto result = lora_training_->trainOnTheFly(training_data);
        
        if (result.success) {
            // Reload updated adapter
            llama_wrapper_->unloadLoRAAdapter(adapter_id_);
            auto updated_info = lora_storage_->getAdapterInfo(adapter_id_);
            llama_wrapper_->loadLoRAAdapter(updated_info.weights_path);
            
            version_ = incrementVersion();
        }
        
        return result.success;
    }
};
```

#### 3. llama.cpp API Wrapper

**File**: `src/llm/llama_cpp_wrapper.cpp` (new)

```cpp
#include <llama.h>

bool LlamaWrapper::loadModel(const std::string& model_path,
                             const ModelParams& params) {
    // Initialize llama.cpp
    llama_backend_init(params.numa);
    
    // Load model
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = params.n_gpu_layers;
    model_params.use_mmap = params.use_mmap;
    model_params.use_mlock = params.use_mlock;
    
    model_ = llama_load_model_from_file(model_path.c_str(), model_params);
    if (!model_) {
        return false;
    }
    
    // Create context
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = params.context_length;
    ctx_params.n_batch = params.batch_size;
    ctx_params.n_threads = params.n_threads;
    
    context_ = llama_new_context_with_model(model_, ctx_params);
    if (!context_) {
        llama_free_model(model_);
        return false;
    }
    
    return true;
}

bool LlamaWrapper::loadLoRAAdapter(const std::string& adapter_path,
                                   float scale) {
    // Load adapter
    llama_lora_adapter* adapter = llama_lora_adapter_init(
        model_,
        adapter_path.c_str()
    );
    
    if (!adapter) {
        return false;
    }
    
    // Apply adapter to context
    int result = llama_lora_adapter_set(context_, adapter, scale);
    if (result != 0) {
        llama_lora_adapter_free(adapter);
        return false;
    }
    
    // Store adapter
    std::string adapter_id = extractAdapterId(adapter_path);
    adapters_[adapter_id] = adapter;
    active_adapter_id_ = adapter_id;
    
    return true;
}

std::string LlamaWrapper::generate(const std::string& prompt,
                                   const GenerationParams& params) {
    // Tokenize prompt
    std::vector<llama_token> tokens = tokenize(prompt);
    
    // Process prompt batch
    llama_batch batch = llama_batch_init(tokens.size(), 0, 1);
    for (size_t i = 0; i < tokens.size(); i++) {
        llama_batch_add(batch, tokens[i], i, {0}, false);
    }
    batch.logits[batch.n_tokens - 1] = true;
    
    int decode_result = llama_decode(context_, batch);
    if (decode_result != 0) {
        llama_batch_free(batch);
        return "";
    }
    
    // Generate tokens
    std::string generated_text;
    for (int i = 0; i < params.max_tokens; i++) {
        // Get logits
        float* logits = llama_get_logits(context_);
        
        // Sample next token
        llama_token next_token = sampleToken(
            logits,
            params.temperature,
            params.top_p,
            params.top_k
        );
        
        // Check for EOS
        if (next_token == llama_token_eos(model_)) {
            break;
        }
        
        // Decode token
        std::string token_text = decodeToken(next_token);
        generated_text += token_text;
        
        // Prepare next batch
        llama_batch_clear(batch);
        llama_batch_add(batch, next_token, tokens.size() + i, {0}, true);
        decode_result = llama_decode(context_, batch);
        if (decode_result != 0) {
            break;
        }
    }
    
    llama_batch_free(batch);
    return generated_text;
}
```

### Prompt Engineering

**File**: `src/llm/applications/themis_help_lora_prompts.h` (new)

```cpp
class PromptBuilder {
public:
    static std::string buildDocumentationPrompt(const std::string& question) {
        return R"(
### System:
You are a helpful ThemisDB documentation assistant. Provide accurate,
concise answers based on ThemisDB documentation. Include code examples
when relevant. If you don't know the answer, say so.

### User:
)" + question + R"(

### Assistant:
)";
    }
    
    static std::string buildFeedbackPrompt(
        const std::string& question,
        const std::string& previous_answer,
        const std::string& correction
    ) {
        return R"(
### System:
Learn from this correction to improve future responses.

### Previous Question:
)" + question + R"(

### Previous Answer:
)" + previous_answer + R"(

### Correction:
)" + correction + R"(

### Updated Answer:
)";
    }
};
```

## Tasks

### Core Integration
- [ ] Extend or create `LlamaWrapper` class
- [ ] Implement model loading via `llama_model_load()`
- [ ] Implement LoRA loading via `llama_lora_adapter_init()`
- [ ] Implement LoRA application via `llama_lora_adapter_set()`
- [ ] Implement text generation via `llama_decode()`
- [ ] Implement token sampling (temperature, top_p, top_k)
- [ ] Implement multi-LoRA support
- [ ] Implement hot-swapping between adapters

### themis_help_lora Updates
- [ ] Replace placeholder `query()` with real inference
- [ ] Implement prompt templates for documentation Q&A
- [ ] Update `trainFromFeedback()` to reload adapters
- [ ] Update `trainFromDocumentation()` integration
- [ ] Add generation parameter configuration
- [ ] Implement response post-processing
- [ ] Add context management (conversation history)

### Performance Optimization
- [ ] Implement KV cache management
- [ ] Add batch processing for multiple queries
- [ ] Optimize memory usage (model quantization)
- [ ] Add GPU acceleration (CUDA/ROCm)
- [ ] Implement request queuing
- [ ] Add inference timeout handling

### Testing
- [ ] Unit tests for LlamaWrapper
- [ ] Integration tests with real models
- [ ] End-to-end tests with themis_help_lora
- [ ] Performance benchmarks (tokens/sec)
- [ ] Quality evaluation (response accuracy)
- [ ] Load testing (concurrent requests)

### Documentation
- [ ] Update `LLM_LORA_LLAMACPP_INTEGRATION.md` with implementation details
- [ ] Add model setup guide (downloading, quantizing GGUF models)
- [ ] Add LoRA training guide (creating adapters)
- [ ] Add prompt engineering guide
- [ ] Update `LORA_USAGE_EXAMPLES.md` with real examples
- [ ] Add troubleshooting section for common issues

## Acceptance Criteria

- [ ] Real LLM inference working with llama.cpp
- [ ] LoRA adapters loading and applying correctly
- [ ] themis_help_lora generating real responses
- [ ] Hot-swapping between adapters functional
- [ ] Training updates reload adapters automatically
- [ ] Performance meets targets (> 20 tokens/sec)
- [ ] Complete audit logging of inferences
- [ ] Prometheus metrics capturing inference stats
- [ ] Unit and integration tests passing
- [ ] Documentation complete with examples
- [ ] GGUF model setup guide provided

## Model Requirements

### Recommended Models
- **llama-2-7b-chat.Q4_K_M.gguf** - Base model for documentation Q&A
- **mistral-7b-instruct-v0.2.Q4_K_M.gguf** - Alternative base model
- **themis_help_lora.bin** - Fine-tuned adapter (created via training)

### Download Instructions
```bash
# Download base model (example)
wget https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF/resolve/main/llama-2-7b-chat.Q4_K_M.gguf

# Place in models directory
mv llama-2-7b-chat.Q4_K_M.gguf /var/lib/themisdb/models/
```

## Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| Model load time | < 3s | 7B Q4 model |
| LoRA load time | < 100ms | Rank-8 adapter |
| First token latency | < 500ms | Cold start |
| Generation speed | > 20 tokens/sec | With LoRA |
| Hot-swap time | < 10ms | Between adapters |
| Memory usage | < 8GB | Model + adapters |

## Related Files

- `src/llm/applications/themis_help_lora.cpp` - Application implementation
- `src/llm/llama_wrapper.h` - LLM wrapper (existing or new)
- `LLM_LORA_LLAMACPP_INTEGRATION.md` - Integration documentation
- `LORA_USAGE_EXAMPLES.md` - Usage examples

## References

- llama.cpp repository: https://github.com/ggerganov/llama.cpp
- llama.cpp examples: https://github.com/ggerganov/llama.cpp/tree/master/examples
- GGUF model format: https://github.com/ggerganov/ggml/blob/master/docs/gguf.md

## Priority

**Medium** - Enhances functionality but placeholder responses allow framework testing.

## Estimated Effort

**Large** (24-32 hours)
- LlamaWrapper implementation: 8-10 hours
- themis_help_lora integration: 4-6 hours
- Prompt engineering: 2-3 hours
- Performance optimization: 4-5 hours
- Testing: 4-6 hours
- Documentation: 2-4 hours
