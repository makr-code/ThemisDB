---
name: "🔤 llama.cpp Tokenizer Integration"
about: Replace SimpleTokenizer with llama.cpp native tokenizer (High Priority - P1)
title: "[LoRa] Integrate llama.cpp Native Tokenizer"
labels: priority:P1, type:feature, area:llm, effort:small, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Training verwendet aktuell SimpleTokenizer (character-level) anstatt llama.cpp's nativen Tokenizer. Dies kann zu Tokenization-Mismatch zwischen Training und Inference führen.

**EN**: Training currently uses SimpleTokenizer (character-level) instead of llama.cpp's native tokenizer. This may cause tokenization mismatch between training and inference.

**Related Analysis**: `REMAINING_GAPS_SUMMARY.md` §5 (Priority 1)  
**Current Status**: `src/llm/lora_framework/lora_training_service.cpp:176` - TODO comment  
**Impact**: ⚠️ **Training/Inference Mismatch** - Vocabulary alignment issues

## 🎯 Ziele / Goals

- [ ] llama.cpp Tokenizer integrieren
- [ ] SimpleTokenizer ersetzen
- [ ] Training/Inference Tokenization synchronisieren
- [ ] Tests für Tokenization Correctness
- [ ] Support für verschiedene Model Types (Llama, Mistral, etc.)

## 📝 Aufgaben / Tasks

### 1. llama.cpp Tokenizer Wrapper
**Priorität**: P1 - High

**Current Code** (Line 176):
```cpp
// TODO: Replace with llama.cpp tokenizer in future PR
// For now, use simple character-level tokenizer for testing
class SimpleTokenizer : public ITokenizer {
    std::vector<int> encode(const std::string& text) override {
        std::vector<int> tokens;
        for (char c : text) {
            tokens.push_back(static_cast<int>(c));
        }
        return tokens;
    }
};
```

**Implementation**:
```cpp
// File: include/llm/lora_framework/llama_tokenizer.h

class LlamaTokenizer : public ITokenizer {
public:
    explicit LlamaTokenizer(const std::string& model_path);
    ~LlamaTokenizer() override;
    
    // ITokenizer interface
    std::vector<int> encode(const std::string& text) override;
    std::string decode(const std::vector<int>& tokens) override;
    
    // Additional llama.cpp features
    std::vector<int> encodeWithBOS(const std::string& text);
    std::vector<int> encodeWithEOS(const std::string& text);
    int getBOSToken() const;
    int getEOSToken() const;
    size_t getVocabSize() const;
    
private:
    llama_model* model_ = nullptr;
    llama_context* context_ = nullptr;
};
```

**Implementation** (`src/llm/lora_framework/llama_tokenizer.cpp`):
```cpp
#include "llm/lora_framework/llama_tokenizer.h"
#include <llama.h>
#include <spdlog/spdlog.h>

LlamaTokenizer::LlamaTokenizer(const std::string& model_path) {
    spdlog::info("Initializing llama.cpp tokenizer from: {}", model_path);
    
    // Initialize llama.cpp
    llama_backend_init(false);
    
    // Load model (for tokenizer only)
    llama_model_params model_params = llama_model_default_params();
    model_params.vocab_only = true;  // Only load tokenizer, not weights
    
    model_ = llama_load_model_from_file(model_path.c_str(), model_params);
    if (!model_) {
        throw std::runtime_error("Failed to load model for tokenizer: " + model_path);
    }
    
    // Create context (needed for tokenization)
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 512;  // Small context for tokenization
    
    context_ = llama_new_context_with_model(model_, ctx_params);
    if (!context_) {
        llama_free_model(model_);
        throw std::runtime_error("Failed to create context for tokenizer");
    }
    
    spdlog::info("✓ llama.cpp tokenizer initialized (vocab_size={})", 
                 llama_n_vocab(model_));
}

LlamaTokenizer::~LlamaTokenizer() {
    if (context_) llama_free(context_);
    if (model_) llama_free_model(model_);
    llama_backend_free();
}

std::vector<int> LlamaTokenizer::encode(const std::string& text) {
    if (!model_ || !context_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    
    // Allocate buffer for tokens
    std::vector<llama_token> tokens(text.size() + 16);  // Extra space for special tokens
    
    // Tokenize using llama.cpp
    int n_tokens = llama_tokenize(
        model_,
        text.c_str(),
        text.size(),
        tokens.data(),
        tokens.size(),
        false,  // add_bos
        false   // special
    );
    
    if (n_tokens < 0) {
        throw std::runtime_error("Tokenization failed (buffer too small)");
    }
    
    tokens.resize(n_tokens);
    
    // Convert to std::vector<int>
    std::vector<int> result(tokens.begin(), tokens.end());
    return result;
}

std::string LlamaTokenizer::decode(const std::vector<int>& tokens) {
    if (!model_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    
    std::string result;
    result.reserve(tokens.size() * 4);  // Estimate 4 chars per token
    
    for (int token : tokens) {
        const char* piece = llama_token_to_piece(model_, token);
        if (piece) {
            result += piece;
        }
    }
    
    return result;
}

std::vector<int> LlamaTokenizer::encodeWithBOS(const std::string& text) {
    auto tokens = encode(text);
    tokens.insert(tokens.begin(), getBOSToken());
    return tokens;
}

int LlamaTokenizer::getBOSToken() const {
    return llama_token_bos(model_);
}

int LlamaTokenizer::getEOSToken() const {
    return llama_token_eos(model_);
}

size_t LlamaTokenizer::getVocabSize() const {
    return llama_n_vocab(model_);
}
```

**Tasks**:
- [ ] Create LlamaTokenizer class implementing ITokenizer
- [ ] Use llama.cpp tokenization APIs
- [ ] Load model in vocab-only mode (fast)
- [ ] Add encode/decode methods
- [ ] Handle BOS/EOS tokens correctly
- [ ] Add error handling

---

### 2. DataLoader Integration
**Priorität**: P1 - High

**Update DataLoader** (`src/llm/lora_framework/data_loader.cpp`):
```cpp
// OLD: Create SimpleTokenizer
auto tokenizer = std::make_shared<SimpleTokenizer>();

// NEW: Create LlamaTokenizer from model path
std::string model_path = config.base_model_path;
if (model_path.empty()) {
    spdlog::warn("No base model path, falling back to SimpleTokenizer");
    tokenizer = std::make_shared<SimpleTokenizer>();
} else {
    try {
        tokenizer = std::make_shared<LlamaTokenizer>(model_path);
        spdlog::info("Using llama.cpp tokenizer");
    } catch (const std::exception& e) {
        spdlog::error("Failed to load llama tokenizer: {}", e.what());
        spdlog::warn("Falling back to SimpleTokenizer");
        tokenizer = std::make_shared<SimpleTokenizer>();
    }
}
```

**Configuration**:
```yaml
# lora_training_config.yaml
data:
  tokenizer: "llama.cpp"  # or "simple" for fallback
  base_model_path: "models/llama-2-7b.gguf"  # Required for llama.cpp tokenizer
```

**Tasks**:
- [ ] Update DataLoader to use LlamaTokenizer
- [ ] Add configuration option for tokenizer type
- [ ] Keep SimpleTokenizer as fallback
- [ ] Add model path validation
- [ ] Update logging

---

### 3. Training Service Integration
**Priorität**: P1 - High

**Update Training Service** (`src/llm/lora_framework/lora_training_service.cpp`):
```cpp
void LoRATrainingService::Impl::initializeDataLoader() {
    DataLoader::Config dl_config;
    dl_config.format = config_.data_format;
    dl_config.batch_size = config_.default_hyperparameters.batch_size;
    
    // Use llama.cpp tokenizer if base model available
    if (config_.use_base_model && !config_.base_model_path.empty()) {
        spdlog::info("Initializing llama.cpp tokenizer from: {}", 
                     config_.base_model_path);
        
        try {
            auto tokenizer = std::make_shared<LlamaTokenizer>(config_.base_model_path);
            dl_config.tokenizer = tokenizer;
            
            // Verify vocabulary size matches
            size_t vocab_size = tokenizer->getVocabSize();
            spdlog::info("✓ llama.cpp tokenizer loaded (vocab_size={})", vocab_size);
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to load llama tokenizer: {}", e.what());
            spdlog::warn("Falling back to SimpleTokenizer");
            // Will use default SimpleTokenizer
        }
    } else {
        spdlog::info("Using SimpleTokenizer (standalone mode)");
    }
    
    data_loader_ = std::make_unique<DataLoader>(dl_config);
}
```

**Tasks**:
- [ ] Update training service initialization
- [ ] Add tokenizer configuration
- [ ] Verify vocab size consistency
- [ ] Add graceful fallback
- [ ] Update documentation

---

### 4. Testing and Validation
**Priorität**: P1 - High

**Test Cases**:
```cpp
// Test file: tests/test_llama_tokenizer.cpp

TEST(LlamaTokenizerTest, BasicTokenization) {
    LlamaTokenizer tokenizer("models/llama-2-7b.gguf");
    
    std::string text = "Hello, world!";
    auto tokens = tokenizer.encode(text);
    
    // Should produce multiple tokens (not character-level)
    EXPECT_GT(tokens.size(), 0);
    EXPECT_LT(tokens.size(), text.size());  // Subword tokenization
}

TEST(LlamaTokenizerTest, RoundTrip) {
    LlamaTokenizer tokenizer("models/llama-2-7b.gguf");
    
    std::string original = "The quick brown fox jumps over the lazy dog.";
    auto tokens = tokenizer.encode(original);
    std::string decoded = tokenizer.decode(tokens);
    
    EXPECT_EQ(original, decoded);
}

TEST(LlamaTokenizerTest, SpecialTokens) {
    LlamaTokenizer tokenizer("models/llama-2-7b.gguf");
    
    int bos = tokenizer.getBOSToken();
    int eos = tokenizer.getEOSToken();
    
    EXPECT_GT(bos, 0);
    EXPECT_GT(eos, 0);
    EXPECT_NE(bos, eos);
}

TEST(LlamaTokenizerTest, VocabSize) {
    LlamaTokenizer tokenizer("models/llama-2-7b.gguf");
    
    size_t vocab_size = tokenizer.getVocabSize();
    EXPECT_EQ(vocab_size, 32000);  // Llama-2 vocab size
}

TEST(LlamaTokenizerTest, CompareWithSimple) {
    LlamaTokenizer llama_tok("models/llama-2-7b.gguf");
    SimpleTokenizer simple_tok;
    
    std::string text = "Hello";
    
    auto llama_tokens = llama_tok.encode(text);
    auto simple_tokens = simple_tok.encode(text);
    
    // llama.cpp should produce fewer tokens (subword)
    EXPECT_LT(llama_tokens.size(), simple_tokens.size());
}

TEST(LlamaTokenizerTest, TrainingConsistency) {
    // Train with llama.cpp tokenizer
    // Inference with llama.cpp tokenizer
    // Verify outputs are consistent
}
```

**Validation Strategy**:
```cpp
void validateTokenizerConsistency() {
    std::string test_text = "Machine learning is transforming technology.";
    
    // Tokenize with llama.cpp
    LlamaTokenizer tokenizer("models/llama-2-7b.gguf");
    auto tokens = tokenizer.encode(test_text);
    
    spdlog::info("Text: {}", test_text);
    spdlog::info("Tokens ({}): {}", tokens.size(), fmt::join(tokens, ", "));
    
    // Decode and verify
    auto decoded = tokenizer.decode(tokens);
    spdlog::info("Decoded: {}", decoded);
    
    assert(test_text == decoded);
}
```

**Tasks**:
- [ ] Create comprehensive test suite
- [ ] Test with multiple model types (Llama, Mistral, CodeLlama)
- [ ] Validate round-trip (encode → decode)
- [ ] Compare with SimpleTokenizer
- [ ] Test training/inference consistency
- [ ] Add benchmark for tokenization speed

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

- [ ] LlamaTokenizer class implemented and working
- [ ] Integrated with DataLoader and TrainingService
- [ ] SimpleTokenizer kept as fallback
- [ ] Round-trip tokenization works correctly
- [ ] Training/inference use same tokenizer
- [ ] Vocab size matches between train/inference
- [ ] Comprehensive tests pass (>90% coverage)
- [ ] Documentation updated with usage examples
- [ ] No regressions in existing functionality

## 📊 Effort Estimation

- **Aufwand / Effort**: 3-5 days (Small)
- **Komplexität / Complexity**: Low-Medium
- **Risiko / Risk**: Low (well-defined llama.cpp API)

## 🔗 Related Issues

- Issue #07: LoRa llama.cpp Integration
- Issue #31: Real Embeddings Extraction
- Original analysis: `REMAINING_GAPS_SUMMARY.md` §5

## 📚 References

- Code location: `src/llm/lora_framework/lora_training_service.cpp:176`
- llama.cpp tokenization: https://github.com/ggerganov/llama.cpp/blob/master/llama.h
- Current tokenizer: `src/llm/lora_framework/data_loader.cpp`
- ITokenizer interface: `include/llm/lora_framework/data_loader.h`

---

**Priority**: P1 - High priority for production quality  
**Impact**: Training/inference consistency, vocabulary alignment  
**Status**: Ready to implement
