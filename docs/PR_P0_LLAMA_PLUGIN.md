# P0: LLaMA.cpp Plugin Real Implementation

**Priorität:** P0 - KRITISCH  
**Zeitrahmen:** 5 Tage (1 Woche)  
**Deadline:** Sprint 1 (Woche 1-2)  
**Status:** 🔴 Blockiert LLM-Features

---

## 🎯 Zielsetzung

Ersetzen der Placeholder-Implementation des LLaMA.cpp Plugins durch eine vollständige Integration mit der llama.cpp API, um alle LLM-Features funktional zu machen.

---

## 📊 Aktuelle Situation

### Betroffene Dateien
- `src/llm/llama_wrapper.cpp` (167 Zeilen)
- `src/llm/llamacpp_inference_engine.cpp` (170 Zeilen)
- `include/llm/llama_wrapper.h`
- `include/llm/llamacpp_inference_engine.h`

### Aktueller Code (Stub)

```cpp
// src/llm/llama_wrapper.cpp (Zeile 173-176)
// LLM inference stubbed out - llama.cpp API needs refactoring
// For now, return a stub response with plausible timing & token counts
// This works with both stub (nullptr) and real (valid) handles
std::string output = "[Generated response placeholder for: " + request.prompt + "]";

LLMResponse response;
response.text = output;
response.prompt_tokens = request.prompt.length() / 4;  // Rough estimate
response.completion_tokens = output.length() / 4;
response.latency_ms = 150.0;
```

### Problem

**Kritischer Impact:**
- ❌ Alle LLM-Features nicht funktional
- ❌ Text-Generation gibt nur Placeholder zurück
- ❌ Embeddings nicht korrekt berechnet
- ❌ Chat-Completion funktioniert nicht
- ❌ Model Loading simuliert nur
- ❌ Token Streaming nicht verfügbar

**Betroffene Features:**
1. `/llm/generate` API-Endpoint
2. `/llm/chat` API-Endpoint
3. `/llm/embeddings` API-Endpoint
4. AQL `LLM_GENERATE()` Funktion
5. Voice Assistant LLM-Integration
6. Content Analysis mit LLM

---

## 🔧 Zu implementierende Funktionen

### 1. Model Loading & Context Management

**Datei:** `src/llm/llama_wrapper.cpp`

**Aktuelle Stub-Implementation:**
```cpp
bool LlamaWrapper::loadModel(const std::string& model_path) {
    // For testing with stub models, allow nullptr handles
    if (!handle_) {
        THEMIS_WARN("LlamaWrapper: Model handle is null, using stub response");
    }
    return true;  // Stub always succeeds
}
```

**Zu implementieren:**
```cpp
bool LlamaWrapper::loadModel(const std::string& model_path) {
    // 1. llama_backend_init()
    llama_backend_init(false);
    
    // 2. Load model params
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = gpu_layers_;
    model_params.main_gpu = 0;
    model_params.use_mmap = true;
    model_params.use_mlock = false;
    
    // 3. Load model
    model_ = llama_load_model_from_file(model_path.c_str(), model_params);
    if (!model_) {
        THEMIS_ERROR("Failed to load model: {}", model_path);
        return false;
    }
    
    // 4. Create context
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.seed = seed_;
    ctx_params.n_ctx = context_size_;
    ctx_params.n_batch = batch_size_;
    ctx_params.n_threads = num_threads_;
    ctx_params.n_threads_batch = num_threads_;
    
    ctx_ = llama_new_context_with_model(model_, ctx_params);
    if (!ctx_) {
        THEMIS_ERROR("Failed to create context");
        llama_free_model(model_);
        model_ = nullptr;
        return false;
    }
    
    // 5. Load tokenizer
    tokenizer_ = llama_get_model(ctx_);
    
    THEMIS_INFO("Model loaded: {} (ctx_size={}, gpu_layers={})", 
                model_path, context_size_, gpu_layers_);
    return true;
}
```

**Aufwand:** 1 Tag

---

### 2. Text Generation (Inference)

**Datei:** `src/llm/llama_wrapper.cpp`

**Zu implementieren:**
```cpp
LLMResponse LlamaWrapper::generate(const LLMRequest& request) {
    if (!ctx_ || !model_) {
        throw std::runtime_error("Model not loaded");
    }
    
    LLMResponse response;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 1. Tokenize prompt
    std::vector<llama_token> tokens = tokenize(request.prompt, true);
    response.prompt_tokens = tokens.size();
    
    // 2. Evaluate prompt
    if (llama_decode(ctx_, llama_batch_get_one(tokens.data(), tokens.size(), 0, 0))) {
        throw std::runtime_error("Failed to decode prompt");
    }
    
    // 3. Generate tokens
    std::vector<llama_token> generated_tokens;
    int n_generated = 0;
    int max_tokens = request.max_tokens > 0 ? request.max_tokens : 512;
    
    while (n_generated < max_tokens) {
        // Sample next token
        llama_token next_token = sample_token(ctx_, request.temperature, request.top_p);
        
        // Check for EOS
        if (next_token == llama_token_eos(model_)) {
            break;
        }
        
        generated_tokens.push_back(next_token);
        n_generated++;
        
        // Evaluate next token
        if (llama_decode(ctx_, llama_batch_get_one(&next_token, 1, tokens.size() + n_generated - 1, 0))) {
            throw std::runtime_error("Failed to decode token");
        }
        
        // Optional: Streaming callback
        if (request.stream && stream_callback_) {
            std::string token_text = detokenize({next_token});
            stream_callback_(token_text);
        }
    }
    
    // 4. Detokenize
    response.text = detokenize(generated_tokens);
    response.completion_tokens = generated_tokens.size();
    
    // 5. Calculate timing
    auto end_time = std::chrono::high_resolution_clock::now();
    response.latency_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    return response;
}
```

**Hilfs-Funktionen:**
```cpp
std::vector<llama_token> LlamaWrapper::tokenize(const std::string& text, bool add_bos) {
    int n_tokens = text.length() + (add_bos ? 1 : 0);
    std::vector<llama_token> tokens(n_tokens);
    n_tokens = llama_tokenize(model_, text.c_str(), text.length(), 
                               tokens.data(), tokens.size(), add_bos, false);
    tokens.resize(n_tokens);
    return tokens;
}

std::string LlamaWrapper::detokenize(const std::vector<llama_token>& tokens) {
    std::string text;
    for (llama_token token : tokens) {
        text += llama_token_to_piece(ctx_, token);
    }
    return text;
}

llama_token LlamaWrapper::sample_token(llama_context* ctx, float temperature, float top_p) {
    auto logits = llama_get_logits(ctx);
    auto n_vocab = llama_n_vocab(model_);
    
    std::vector<llama_token_data> candidates;
    candidates.reserve(n_vocab);
    for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
        candidates.push_back({token_id, logits[token_id], 0.0f});
    }
    
    llama_token_data_array candidates_p = {
        candidates.data(), candidates.size(), false
    };
    
    // Apply temperature
    llama_sample_temp(ctx, &candidates_p, temperature);
    
    // Apply top_p
    llama_sample_top_p(ctx, &candidates_p, top_p, 1);
    
    // Sample token
    return llama_sample_token(ctx, &candidates_p);
}
```

**Aufwand:** 2 Tage

---

### 3. Embeddings Generation

**Datei:** `src/llm/llama_wrapper.cpp`

**Zu implementieren:**
```cpp
std::vector<float> LlamaWrapper::generateEmbedding(const std::string& text) {
    if (!ctx_ || !model_) {
        throw std::runtime_error("Model not loaded");
    }
    
    // 1. Tokenize
    std::vector<llama_token> tokens = tokenize(text, true);
    
    // 2. Evaluate with embedding mode
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size(), 0, 0);
    if (llama_decode(ctx_, batch)) {
        throw std::runtime_error("Failed to decode for embedding");
    }
    
    // 3. Get embeddings
    int n_embd = llama_n_embd(model_);
    float* embd = llama_get_embeddings(ctx_);
    if (!embd) {
        throw std::runtime_error("Failed to get embeddings");
    }
    
    // 4. Normalize (optional)
    std::vector<float> embedding(embd, embd + n_embd);
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    if (norm > 0.0f) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
    
    return embedding;
}
```

**Aufwand:** 0.5 Tage

---

### 4. Chat Completion (mit Chat Template)

**Datei:** `src/llm/llama_wrapper.cpp`

**Zu implementieren:**
```cpp
LLMResponse LlamaWrapper::chat(const std::vector<ChatMessage>& messages, 
                                  const LLMRequest& request) {
    // 1. Format messages with chat template
    std::string formatted_prompt = formatChatTemplate(messages);
    
    // 2. Generate response
    LLMRequest modified_request = request;
    modified_request.prompt = formatted_prompt;
    
    return generate(modified_request);
}

std::string LlamaWrapper::formatChatTemplate(const std::vector<ChatMessage>& messages) {
    // ChatML format (for Mistral, Llama-3, etc.)
    std::ostringstream oss;
    for (const auto& msg : messages) {
        oss << "<|im_start|>" << msg.role << "\n";
        oss << msg.content << "\n";
        oss << "<|im_end|>\n";
    }
    oss << "<|im_start|>assistant\n";
    return oss.str();
}
```

**Aufwand:** 0.5 Tage

---

### 5. Resource Management & Cleanup

**Datei:** `src/llm/llama_wrapper.cpp`

**Zu implementieren:**
```cpp
LlamaWrapper::~LlamaWrapper() {
    unloadModel();
}

void LlamaWrapper::unloadModel() {
    if (ctx_) {
        llama_free(ctx_);
        ctx_ = nullptr;
    }
    if (model_) {
        llama_free_model(model_);
        model_ = nullptr;
    }
    llama_backend_free();
    
    THEMIS_INFO("Model unloaded and resources freed");
}
```

**Aufwand:** 0.5 Tage

---

### 6. Inference Engine Integration

**Datei:** `src/llm/llamacpp_inference_engine.cpp`

**Aktuelle Stub:**
```cpp
InferenceResult LlamaCppInferenceEngine::generateCompletion(const InferenceRequest& request) {
    InferenceResult result;
    result.text = "[Model output placeholder]";
    result.tokens_generated = 50;
    result.latency_ms = 100.0;
    return result;
}
```

**Zu implementieren:**
```cpp
InferenceResult LlamaCppInferenceEngine::generateCompletion(const InferenceRequest& request) {
    // Get plugin instance
    auto plugin = plugin_manager_->getPlugin(request.model_id);
    if (!plugin) {
        throw std::runtime_error("Model not loaded: " + request.model_id);
    }
    
    // Convert request format
    LLMRequest llm_request;
    llm_request.prompt = request.prompt;
    llm_request.max_tokens = request.max_tokens;
    llm_request.temperature = request.temperature;
    llm_request.top_p = request.top_p;
    llm_request.stream = false;
    
    // Generate
    auto llm_response = plugin->generate(llm_request);
    
    // Convert response format
    InferenceResult result;
    result.text = llm_response.text;
    result.tokens_generated = llm_response.completion_tokens;
    result.latency_ms = llm_response.latency_ms;
    result.finish_reason = "stop";
    
    return result;
}
```

**Aufwand:** 0.5 Tage

---

## 🧪 Testing

### Unit Tests

**Neue Datei:** `tests/test_llama_wrapper_real.cpp`

```cpp
#include <gtest/gtest.h>
#include "llm/llama_wrapper.h"

class LlamaWrapperRealTest : public ::testing::Test {
protected:
    void SetUp() override {
        plugin_ = std::make_unique<LlamaWrapper>();
        // Use small test model (e.g., TinyLlama-1.1B)
        model_path_ = "/models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf";
    }
    
    std::unique_ptr<LlamaWrapper> plugin_;
    std::string model_path_;
};

TEST_F(LlamaWrapperRealTest, LoadModel) {
    ASSERT_TRUE(plugin_->loadModel(model_path_));
}

TEST_F(LlamaWrapperRealTest, GenerateSimple) {
    ASSERT_TRUE(plugin_->loadModel(model_path_));
    
    LLMRequest request;
    request.prompt = "What is 2+2?";
    request.max_tokens = 50;
    request.temperature = 0.7;
    
    auto response = plugin_->generate(request);
    
    EXPECT_GT(response.text.length(), 0);
    EXPECT_GT(response.completion_tokens, 0);
    EXPECT_GT(response.prompt_tokens, 0);
    EXPECT_GT(response.latency_ms, 0);
    
    // Should not be placeholder
    EXPECT_EQ(response.text.find("[Generated response placeholder"), std::string::npos);
}

TEST_F(LlamaWrapperRealTest, GenerateEmbedding) {
    ASSERT_TRUE(plugin_->loadModel(model_path_));
    
    auto embedding = plugin_->generateEmbedding("Hello world");
    
    EXPECT_GT(embedding.size(), 0);
    // Check normalization
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    EXPECT_NEAR(std::sqrt(norm), 1.0f, 0.01f);
}

TEST_F(LlamaWrapperRealTest, ChatCompletion) {
    ASSERT_TRUE(plugin_->loadModel(model_path_));
    
    std::vector<ChatMessage> messages = {
        {"system", "You are a helpful assistant."},
        {"user", "What is the capital of France?"}
    };
    
    LLMRequest request;
    request.max_tokens = 50;
    
    auto response = plugin_->chat(messages, request);
    
    EXPECT_GT(response.text.length(), 0);
    EXPECT_EQ(response.text.find("Paris"), std::string::npos) == false 
        || response.text.find("paris"), std::string::npos) == false;
}
```

**Aufwand:** 0.5 Tage

---

## 📦 Dependencies

### llama.cpp Integration

**CMakeLists.txt Änderungen:**

```cmake
# Option to enable real llama.cpp
option(THEMIS_ENABLE_LLAMACPP "Enable real llama.cpp integration" ON)

if(THEMIS_ENABLE_LLAMACPP)
    # Find or build llama.cpp
    find_package(llama QUIET)
    if(NOT llama_FOUND)
        # Build llama.cpp from submodule
        add_subdirectory(${CMAKE_SOURCE_DIR}/llama.cpp EXCLUDE_FROM_ALL)
    endif()
    
    # Link to LLM targets
    target_link_libraries(themis_llm PRIVATE llama)
    target_compile_definitions(themis_llm PRIVATE THEMIS_ENABLE_LLAMACPP)
endif()
```

**Submodule hinzufügen:**
```bash
git submodule add https://github.com/ggerganov/llama.cpp.git llama.cpp
git submodule update --init --recursive
```

---

## 🔍 Verification

### Akzeptanzkriterien

1. ✅ Model Loading funktioniert
   - Test: TinyLlama-1.1B lädt ohne Fehler
   - Kontext-Größe konfigurierbar

2. ✅ Text Generation funktioniert
   - Test: Prompt "What is 2+2?" gibt sinnvolle Antwort
   - Keine Placeholder-Strings im Output

3. ✅ Embeddings funktionieren
   - Test: "Hello world" gibt normalisierten Vektor
   - Dimensionen korrekt (z.B. 4096 für Llama-2)

4. ✅ Chat Completion funktioniert
   - Test: Multi-Turn Conversation
   - Chat Template korrekt formatiert

5. ✅ Performance akzeptabel
   - Latenz < 1s für 50 Tokens (CPU)
   - Latenz < 100ms für 50 Tokens (GPU)

6. ✅ API-Endpoints funktional
   - `/llm/generate` gibt echte Antworten
   - `/llm/chat` funktioniert mit Messages
   - `/llm/embeddings` gibt Vektoren zurück

---

## 📋 Implementation Checklist

### Tag 1: Model Loading & Context
- [ ] llama.cpp Submodule hinzufügen
- [ ] CMake-Integration
- [ ] `loadModel()` implementieren
- [ ] `unloadModel()` implementieren
- [ ] Tokenizer-Integration
- [ ] Unit Tests für Model Loading

### Tag 2: Text Generation
- [ ] `generate()` implementieren
- [ ] `tokenize()` / `detokenize()` implementieren
- [ ] `sample_token()` implementieren
- [ ] Temperature/Top-P Sampling
- [ ] Unit Tests für Generation

### Tag 3: Embeddings & Chat
- [ ] `generateEmbedding()` implementieren
- [ ] Embedding Normalization
- [ ] `chat()` implementieren
- [ ] Chat Template Formatting
- [ ] Unit Tests für Embeddings/Chat

### Tag 4: Integration & Testing
- [ ] Inference Engine Integration
- [ ] API-Handler Updates
- [ ] Integration Tests
- [ ] Performance Tests

### Tag 5: Bug Fixes & Documentation
- [ ] Bug Fixes aus Testing
- [ ] Code Review
- [ ] Dokumentation aktualisieren
- [ ] Performance-Optimierung

---

## 🚀 Deployment

### Build-Anleitung

```bash
# Mit llama.cpp
cmake -B build -DTHEMIS_ENABLE_LLAMACPP=ON

# Ohne llama.cpp (Stub bleibt)
cmake -B build -DTHEMIS_ENABLE_LLAMACPP=OFF

# Build
cmake --build build -j$(nproc)
```

### Model Download

```bash
# TinyLlama für Tests (1.1B, ~637MB)
huggingface-cli download TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF \
    tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
    --local-dir /models/

# Llama-2-7B für Produktion (~3.8GB)
huggingface-cli download TheBloke/Llama-2-7B-Chat-GGUF \
    llama-2-7b-chat.Q4_K_M.gguf \
    --local-dir /models/
```

---

## 📊 Success Metrics

| Metric | Stub | Target | Status |
|--------|------|--------|--------|
| Text Generation Working | ❌ Placeholder | ✅ Real Output | 🔴 TODO |
| Embeddings Valid | ❌ Mock | ✅ Normalized | 🔴 TODO |
| Latency (50 tokens, CPU) | N/A | < 1s | 🔴 TODO |
| Latency (50 tokens, GPU) | N/A | < 100ms | 🔴 TODO |
| API Success Rate | 100% (Fake) | > 95% (Real) | 🔴 TODO |

---

## 🔗 Related Issues

- Issue #XXX: LLM Features nicht funktional
- Issue #XXX: `/llm/generate` gibt Placeholder zurück
- Issue #XXX: Embeddings nicht korrekt

---

## 👥 Assignee

- **Developer:** TBD
- **Reviewer:** @makr-code
- **QA:** TBD

---

**Erstellt:** 4. Januar 2026  
**Deadline:** 11. Januar 2026 (Sprint 1 Ende)  
**Status:** 🔴 Not Started
