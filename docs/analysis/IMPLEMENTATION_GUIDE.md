# Konkrete Implementierungsvorschläge für LLM/LoRA System Gaps

**Erstellt**: 15. Januar 2026  
**Version**: 1.0  
**Basis**: Bestehender Code in `/src/llm` und `/include/llm`  
**Methodologie**: Best Practice OOP, SOLID Principles, Modern C++17

---

## Implementierungsreihenfolge nach Priorität

### 🔴 Phase 1: Kritische Blocker (Woche 1-12)
1. [llama.cpp Integration](#1-llamacpp-integration-vollständig-implementieren) - **Höchste Priorität**
2. [Security Validator](#2-security-validator-kryptographische-verifikation) - **Kritisch für Produktion**
3. [Training Service](#3-training-service-echtes-ml-training) - **Kernfunktionalität**

### 🟡 Phase 2: Infrastructure (Woche 13-18)
4. [Storage Backend](#4-storage-backend-themisdb-integration)
5. [Job Orchestrator](#5-job-orchestrator-async-execution)

### 🟢 Phase 3: Quality & Testing (Woche 19-24)
6. [Test Infrastructure](#6-test-infrastructure)
7. [Performance Monitoring](#7-performance-monitoring)

---

## 1. llama.cpp Integration vollständig implementieren

**Quelle**: `src/llm/llama_wrapper.cpp` (Zeilen 1-300)  
**Problem**: Gibt Placeholder-Strings zurück, keine echte Inferenz  
**Priorität**: ⛔ KRITISCH - BLOCKER  
**Aufwand**: 8 Wochen

### 1.1 Model Loading (Woche 1-2)

#### Aktueller Code (llama_wrapper.cpp:200-210)
```cpp
bool LlamaWrapper::loadModel(const std::string& model_path) {
    spdlog::warn("LlamaWrapper: Model loading is stubbed");
    impl_->model_handle = nullptr;  // Immer null!
    return false;
}
```

#### OOP Best Practice: Resource Management mit RAII

**Neue Datei**: `include/llm/llama_resource_manager.h`
```cpp
#pragma once
#include <llama.h>
#include <memory>
#include <string>

namespace themis {
namespace llm {

/**
 * @brief RAII wrapper für llama.cpp Model-Handle
 * 
 * Design Pattern: RAII (Resource Acquisition Is Initialization)
 * Ensures automatic cleanup of llama resources
 */
class LlamaModelHandle {
public:
    explicit LlamaModelHandle(const std::string& model_path, 
                             const llama_model_params& params);
    ~LlamaModelHandle();
    
    // Nicht kopierbar, nur movable (Modern C++ Best Practice)
    LlamaModelHandle(const LlamaModelHandle&) = delete;
    LlamaModelHandle& operator=(const LlamaModelHandle&) = delete;
    LlamaModelHandle(LlamaModelHandle&& other) noexcept;
    LlamaModelHandle& operator=(LlamaModelHandle&& other) noexcept;
    
    llama_model* get() const noexcept { return model_.get(); }
    explicit operator bool() const noexcept { return model_ != nullptr; }
    
    // Metadata queries
    size_t n_vocab() const;
    size_t n_embd() const;
    std::string model_type() const;

private:
    struct ModelDeleter {
        void operator()(llama_model* model) const {
            if (model) llama_free_model(model);
        }
    };
    
    std::unique_ptr<llama_model, ModelDeleter> model_;
};

/**
 * @brief RAII wrapper für llama.cpp Context-Handle
 */
class LlamaContextHandle {
public:
    explicit LlamaContextHandle(llama_model* model,
                               const llama_context_params& params);
    ~LlamaContextHandle();
    
    LlamaContextHandle(const LlamaContextHandle&) = delete;
    LlamaContextHandle& operator=(const LlamaContextHandle&) = delete;
    LlamaContextHandle(LlamaContextHandle&& other) noexcept;
    LlamaContextHandle& operator=(LlamaContextHandle&& other) noexcept;
    
    llama_context* get() const noexcept { return context_.get(); }
    explicit operator bool() const noexcept { return context_ != nullptr; }
    
    // KV-Cache Management
    void clear_kv_cache();
    size_t kv_cache_token_count() const;

private:
    struct ContextDeleter {
        void operator()(llama_context* ctx) const {
            if (ctx) llama_free(ctx);
        }
    };
    
    std::unique_ptr<llama_context, ContextDeleter> context_;
};

} // namespace llm
} // namespace themis
```

#### Implementation: `src/llm/llama_resource_manager.cpp`
```cpp
#include "llm/llama_resource_manager.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace themis {
namespace llm {

// ===== LlamaModelHandle =====

LlamaModelHandle::LlamaModelHandle(const std::string& model_path,
                                  const llama_model_params& params) {
    spdlog::info("Loading model from: {}", model_path);
    
    llama_model* raw_model = llama_model_load_from_file(
        model_path.c_str(), 
        params
    );
    
    if (!raw_model) {
        throw std::runtime_error("Failed to load model: " + model_path);
    }
    
    model_.reset(raw_model);
    
    spdlog::info("Model loaded successfully:");
    spdlog::info("  Vocabulary size: {}", n_vocab());
    spdlog::info("  Embedding dimension: {}", n_embd());
    spdlog::info("  Model type: {}", model_type());
}

LlamaModelHandle::~LlamaModelHandle() {
    spdlog::debug("Destroying LlamaModelHandle");
}

LlamaModelHandle::LlamaModelHandle(LlamaModelHandle&& other) noexcept
    : model_(std::move(other.model_)) {
}

LlamaModelHandle& LlamaModelHandle::operator=(LlamaModelHandle&& other) noexcept {
    if (this != &other) {
        model_ = std::move(other.model_);
    }
    return *this;
}

size_t LlamaModelHandle::n_vocab() const {
    return model_ ? llama_n_vocab(model_.get()) : 0;
}

size_t LlamaModelHandle::n_embd() const {
    return model_ ? llama_n_embd(model_.get()) : 0;
}

std::string LlamaModelHandle::model_type() const {
    if (!model_) return "none";
    
    char buf[128];
    llama_model_desc(model_.get(), buf, sizeof(buf));
    return std::string(buf);
}

// ===== LlamaContextHandle =====

LlamaContextHandle::LlamaContextHandle(llama_model* model,
                                      const llama_context_params& params) {
    if (!model) {
        throw std::invalid_argument("Model cannot be null");
    }
    
    llama_context* raw_ctx = llama_new_context_with_model(model, params);
    
    if (!raw_ctx) {
        throw std::runtime_error("Failed to create llama context");
    }
    
    context_.reset(raw_ctx);
    spdlog::info("Context created with {} tokens capacity", params.n_ctx);
}

LlamaContextHandle::~LlamaContextHandle() {
    spdlog::debug("Destroying LlamaContextHandle");
}

LlamaContextHandle::LlamaContextHandle(LlamaContextHandle&& other) noexcept
    : context_(std::move(other.context_)) {
}

LlamaContextHandle& LlamaContextHandle::operator=(LlamaContextHandle&& other) noexcept {
    if (this != &other) {
        context_ = std::move(other.context_);
    }
    return *this;
}

void LlamaContextHandle::clear_kv_cache() {
    if (context_) {
        llama_kv_cache_clear(context_.get());
        spdlog::debug("KV cache cleared");
    }
}

size_t LlamaContextHandle::kv_cache_token_count() const {
    return context_ ? llama_get_kv_cache_used_cells(context_.get()) : 0;
}

} // namespace llm
} // namespace themis
```

#### Refactored llama_wrapper.cpp mit RAII
```cpp
// src/llm/llama_wrapper.cpp (neue Implementation)
#include "llm/llama_wrapper.h"
#include "llm/llama_resource_manager.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

class LlamaWrapper::Impl {
public:
    explicit Impl(const Config& config) : config_(config) {
        // Backend initialisieren (einmalig)
        llama_backend_init();
        spdlog::info("llama.cpp backend initialized");
    }
    
    ~Impl() {
        // Backend cleanup
        llama_backend_free();
        spdlog::info("llama.cpp backend freed");
    }
    
    bool loadModel(const std::string& model_path) {
        try {
            // Model-Parameter erstellen
            llama_model_params model_params = llama_model_default_params();
            model_params.n_gpu_layers = config_.n_gpu_layers;
            model_params.main_gpu = config_.main_gpu;
            model_params.use_mmap = config_.use_mmap;
            model_params.use_mlock = config_.use_mlock;
            
            // RAII: Automatisches Cleanup bei Exception oder Destruktor
            model_ = std::make_unique<LlamaModelHandle>(model_path, model_params);
            
            // Context-Parameter erstellen
            llama_context_params ctx_params = llama_context_default_params();
            ctx_params.n_ctx = config_.n_ctx;
            ctx_params.n_batch = config_.n_batch;
            ctx_params.n_threads = config_.n_threads;
            ctx_params.n_threads_batch = config_.n_threads;
            ctx_params.flash_attn = config_.use_flash_attention;
            
            // Context erstellen (RAII)
            context_ = std::make_unique<LlamaContextHandle>(
                model_->get(), 
                ctx_params
            );
            
            spdlog::info("Model loaded: {}", model_path);
            return true;
            
        } catch (const std::exception& e) {
            spdlog::error("Model loading failed: {}", e.what());
            model_.reset();
            context_.reset();
            return false;
        }
    }
    
    std::string generate(const GenerationRequest& request);
    
private:
    Config config_;
    std::unique_ptr<LlamaModelHandle> model_;
    std::unique_ptr<LlamaContextHandle> context_;
};

// Public API Implementation
bool LlamaWrapper::loadModel(const std::string& model_path) {
    return impl_->loadModel(model_path);
}

} // namespace llm
} // namespace themis
```

**Best Practices angewendet**:
- ✅ RAII für automatisches Resource Management
- ✅ Smart Pointers mit Custom Deleters
- ✅ Move Semantics (nicht kopierbar, nur movable)
- ✅ Exception-Safety (Strong Exception Guarantee)
- ✅ Single Responsibility Principle (jede Klasse hat eine Verantwortung)

---

### 1.2 Token Generation (Woche 3-4)

#### Aktueller Code (llama_wrapper.cpp:205-210)
```cpp
std::string LlamaWrapper::generate(const GenerationRequest& request) {
    spdlog::warn("Returning stub response");
    return "[Generated response placeholder for: " + request.prompt + "]";
}
```

#### OOP Best Practice: Strategy Pattern für Sampling

**Neue Datei**: `include/llm/sampling_strategy.h`
```cpp
#pragma once
#include <llama.h>
#include <vector>
#include <memory>

namespace themis {
namespace llm {

/**
 * @brief Abstract Strategy für Token Sampling
 * 
 * Design Pattern: Strategy Pattern
 * Allows different sampling algorithms without changing client code
 */
class ISamplingStrategy {
public:
    virtual ~ISamplingStrategy() = default;
    
    virtual llama_token sample(
        llama_context* ctx,
        const std::vector<llama_token>& last_tokens,
        int pos
    ) = 0;
    
    virtual std::string name() const = 0;
};

/**
 * @brief Greedy Sampling (immer höchste Wahrscheinlichkeit)
 */
class GreedySampling : public ISamplingStrategy {
public:
    llama_token sample(
        llama_context* ctx,
        const std::vector<llama_token>& last_tokens,
        int pos
    ) override;
    
    std::string name() const override { return "greedy"; }
};

/**
 * @brief Top-K + Top-P (Nucleus) Sampling
 */
class NucleusSampling : public ISamplingStrategy {
public:
    explicit NucleusSampling(float temperature = 0.8f,
                            int top_k = 40,
                            float top_p = 0.9f,
                            float repeat_penalty = 1.1f)
        : temperature_(temperature)
        , top_k_(top_k)
        , top_p_(top_p)
        , repeat_penalty_(repeat_penalty) {}
    
    llama_token sample(
        llama_context* ctx,
        const std::vector<llama_token>& last_tokens,
        int pos
    ) override;
    
    std::string name() const override { return "nucleus"; }

private:
    float temperature_;
    int top_k_;
    float top_p_;
    float repeat_penalty_;
};

/**
 * @brief Mirostat Sampling (adaptive, bessere Qualität)
 */
class MirostatSampling : public ISamplingStrategy {
public:
    explicit MirostatSampling(float tau = 5.0f, float eta = 0.1f)
        : tau_(tau), eta_(eta), mu_(2.0f * tau) {}
    
    llama_token sample(
        llama_context* ctx,
        const std::vector<llama_token>& last_tokens,
        int pos
    ) override;
    
    std::string name() const override { return "mirostat"; }

private:
    float tau_;    // Target entropy
    float eta_;    // Learning rate
    float mu_;     // Current mu value (adaptive)
};

/**
 * @brief Factory für Sampling Strategies
 */
class SamplingStrategyFactory {
public:
    static std::unique_ptr<ISamplingStrategy> create(
        const std::string& strategy_name,
        float temperature = 0.8f,
        int top_k = 40,
        float top_p = 0.9f
    );
};

} // namespace llm
} // namespace themis
```

#### Implementation: `src/llm/sampling_strategy.cpp`
```cpp
#include "llm/sampling_strategy.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <random>

namespace themis {
namespace llm {

// ===== GreedySampling =====

llama_token GreedySampling::sample(
    llama_context* ctx,
    const std::vector<llama_token>& last_tokens,
    int pos) {
    
    // Logits holen
    float* logits = llama_get_logits_ith(ctx, pos);
    size_t n_vocab = llama_n_vocab(llama_get_model(ctx));
    
    // Token mit höchster Wahrscheinlichkeit finden
    auto max_it = std::max_element(logits, logits + n_vocab);
    return static_cast<llama_token>(std::distance(logits, max_it));
}

// ===== NucleusSampling =====

llama_token NucleusSampling::sample(
    llama_context* ctx,
    const std::vector<llama_token>& last_tokens,
    int pos) {
    
    llama_model* model = llama_get_model(ctx);
    size_t n_vocab = llama_n_vocab(model);
    float* logits = llama_get_logits_ith(ctx, pos);
    
    // Sampler erstellen (llama.cpp API)
    llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    
    // Repeat Penalty
    llama_sampler_chain_add(
        sampler,
        llama_sampler_init_penalties(
            n_vocab,
            llama_token_eos(model),
            llama_token_nl(model),
            0,                     // penalty_last_n (0 = disabled)
            repeat_penalty_,       // repeat penalty
            0.0f,                  // frequency penalty
            0.0f,                  // presence penalty
            false                  // penalize_nl
        )
    );
    
    // Top-K
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(top_k_));
    
    // Top-P (Nucleus)
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(top_p_, 1));
    
    // Temperature
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(temperature_));
    
    // Dist (final sampling)
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(0)); // seed=0 (random)
    
    // Sample token
    llama_token result = llama_sampler_sample(sampler, ctx, pos);
    
    // Cleanup
    llama_sampler_free(sampler);
    
    return result;
}

// ===== MirostatSampling =====

llama_token MirostatSampling::sample(
    llama_context* ctx,
    const std::vector<llama_token>& last_tokens,
    int pos) {
    
    llama_model* model = llama_get_model(ctx);
    
    // Mirostat v2 sampler
    llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    
    llama_sampler_chain_add(
        sampler,
        llama_sampler_init_mirostat_v2(0, tau_, eta_)
    );
    
    llama_token result = llama_sampler_sample(sampler, ctx, pos);
    
    llama_sampler_free(sampler);
    
    return result;
}

// ===== Factory =====

std::unique_ptr<ISamplingStrategy> SamplingStrategyFactory::create(
    const std::string& strategy_name,
    float temperature,
    int top_k,
    float top_p) {
    
    if (strategy_name == "greedy") {
        return std::make_unique<GreedySampling>();
    } else if (strategy_name == "nucleus" || strategy_name == "top_p") {
        return std::make_unique<NucleusSampling>(temperature, top_k, top_p);
    } else if (strategy_name == "mirostat") {
        return std::make_unique<MirostatSampling>();
    } else {
        spdlog::warn("Unknown sampling strategy '{}', using nucleus", strategy_name);
        return std::make_unique<NucleusSampling>(temperature, top_k, top_p);
    }
}

} // namespace llm
} // namespace themis
```

#### Token Generation Loop (llama_wrapper.cpp)
```cpp
std::string LlamaWrapper::Impl::generate(const GenerationRequest& request) {
    if (!model_ || !context_) {
        throw std::runtime_error("Model not loaded");
    }
    
    spdlog::info("Generating response for prompt: {}", request.prompt.substr(0, 50) + "...");
    
    try {
        // 1. Tokenize Prompt
        std::vector<llama_token> tokens = tokenize(request.prompt, true);
        spdlog::debug("Prompt tokenized: {} tokens", tokens.size());
        
        // 2. Check context size
        if (tokens.size() >= static_cast<size_t>(config_.n_ctx)) {
            throw std::runtime_error("Prompt exceeds context size");
        }
        
        // 3. Evaluate prompt (batch)
        llama_batch batch = llama_batch_get_one(
            tokens.data(),
            static_cast<int32_t>(tokens.size())
        );
        
        int ret = llama_decode(context_->get(), batch);
        if (ret != 0) {
            throw std::runtime_error("Failed to decode prompt batch");
        }
        
        // 4. Create sampling strategy
        auto sampler = SamplingStrategyFactory::create(
            request.sampling_strategy,
            request.temperature,
            request.top_k,
            request.top_p
        );
        
        spdlog::debug("Using sampling strategy: {}", sampler->name());
        
        // 5. Generation Loop
        std::string output;
        std::vector<llama_token> all_tokens = tokens;
        
        llama_model* model = model_->get();
        llama_context* ctx = context_->get();
        
        for (int i = 0; i < request.max_tokens; ++i) {
            // Sample next token
            int pos = static_cast<int>(all_tokens.size()) - 1;
            llama_token new_token = sampler->sample(ctx, all_tokens, pos);
            
            // Check for EOS
            if (llama_token_is_eog(model, new_token)) {
                spdlog::debug("EOS token encountered, stopping generation");
                break;
            }
            
            // Convert token to text
            char buf[256];
            int n = llama_token_to_piece(model, new_token, buf, sizeof(buf), 0, true);
            
            if (n > 0) {
                output.append(buf, n);
            }
            
            all_tokens.push_back(new_token);
            
            // Check stop sequences
            if (!request.stop_sequences.empty()) {
                for (const auto& stop : request.stop_sequences) {
                    if (output.size() >= stop.size() &&
                        output.substr(output.size() - stop.size()) == stop) {
                        spdlog::debug("Stop sequence '{}' encountered", stop);
                        return output.substr(0, output.size() - stop.size());
                    }
                }
            }
            
            // Decode next position
            llama_batch next_batch = llama_batch_get_one(&new_token, 1);
            ret = llama_decode(ctx, next_batch);
            if (ret != 0) {
                spdlog::warn("Decode failed at position {}", i);
                break;
            }
        }
        
        spdlog::info("Generated {} tokens", all_tokens.size() - tokens.size());
        return output;
        
    } catch (const std::exception& e) {
        spdlog::error("Generation failed: {}", e.what());
        throw;
    }
}

std::vector<llama_token> LlamaWrapper::Impl::tokenize(
    const std::string& text,
    bool add_special) {
    
    if (!model_) {
        throw std::runtime_error("Model not loaded");
    }
    
    llama_model* model = model_->get();
    
    // Estimate token count (4 chars per token average)
    const size_t estimate = text.size() / 4 + 16;
    std::vector<llama_token> tokens(estimate);
    
    int n_tokens = llama_tokenize(
        model,
        text.c_str(),
        static_cast<int32_t>(text.size()),
        tokens.data(),
        static_cast<int32_t>(tokens.size()),
        add_special,
        false  // parse_special
    );
    
    if (n_tokens < 0) {
        // Buffer too small, resize and retry
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(
            model,
            text.c_str(),
            static_cast<int32_t>(text.size()),
            tokens.data(),
            static_cast<int32_t>(tokens.size()),
            add_special,
            false
        );
    }
    
    tokens.resize(n_tokens);
    return tokens;
}
```

**Best Practices angewendet**:
- ✅ Strategy Pattern für austauschbare Sampling-Algorithmen
- ✅ Factory Pattern für Strategie-Erstellung
- ✅ Interface Segregation (ISamplingStrategy)
- ✅ Open/Closed Principle (neue Strategies ohne Änderung)
- ✅ Proper Error Handling mit Exceptions

---

## 2. Security Validator: Kryptographische Verifikation

**Quelle**: `src/llm/lora_security_validator.cpp` (Zeilen 95-236)  
**Problem**: Nur Format-Validierung, keine kryptographische Verifikation  
**Priorität**: ⛔ KRITISCH  
**Aufwand**: 2-3 Wochen

### 2.1 Signatur-Verifikation mit OpenSSL (Woche 1)

#### Aktueller Code (lora_security_validator.cpp:226-236)
```cpp
LOG_WARN("LoRa signature cryptographic verification not implemented");
result.signature_algorithm = "RSA-SHA256 (format validation only)";
result.error_message = "Cryptographic verification not implemented";
```

#### OOP Best Practice: Command Pattern + Chain of Responsibility

**Neue Datei**: `include/llm/security/signature_verifier.h`
```cpp
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <openssl/evp.h>
#include <openssl/x509.h>

namespace themis {
namespace llm {
namespace security {

/**
 * @brief Result von Signatur-Verifikation
 */
struct SignatureVerificationResult {
    bool is_valid = false;
    std::string algorithm;
    std::string signer_identity;
    std::string error_message;
    
    // Certificate chain info
    std::vector<std::string> chain_fingerprints;
    bool chain_valid = false;
};

/**
 * @brief Abstract Base Class für Signature Verification
 * 
 * Design Pattern: Chain of Responsibility
 * Each verifier checks one aspect and passes to next
 */
class ISignatureVerifier {
public:
    virtual ~ISignatureVerifier() = default;
    
    virtual SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) = 0;
    
    void setNext(std::shared_ptr<ISignatureVerifier> next) {
        next_ = next;
    }

protected:
    std::shared_ptr<ISignatureVerifier> next_;
    
    SignatureVerificationResult passToNext(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) {
        if (next_) {
            return next_->verify(data, signature, cert_pem);
        }
        SignatureVerificationResult result;
        result.is_valid = true;
        return result;
    }
};

/**
 * @brief RSA-SHA256 Signature Verifier
 */
class RSA_SHA256_Verifier : public ISignatureVerifier {
public:
    SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) override;

private:
    std::unique_ptr<X509, decltype(&X509_free)> 
        loadCertificate(const std::string& cert_pem);
    
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>
        extractPublicKey(X509* cert);
};

/**
 * @brief Certificate Chain Verifier
 */
class CertificateChainVerifier : public ISignatureVerifier {
public:
    explicit CertificateChainVerifier(const std::string& ca_bundle_path);
    
    SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) override;

private:
    std::string ca_bundle_path_;
    
    bool verifyCertificateChain(X509* cert, X509_STORE* store);
};

/**
 * @brief Certificate Revocation List (CRL) Checker
 */
class CRLChecker : public ISignatureVerifier {
public:
    explicit CRLChecker(const std::string& crl_url);
    
    SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) override;

private:
    std::string crl_url_;
    
    bool isCertificateRevoked(X509* cert);
};

/**
 * @brief Signature Verifier Builder (Fluent Interface)
 * 
 * Design Pattern: Builder Pattern
 */
class SignatureVerifierBuilder {
public:
    SignatureVerifierBuilder& withRSA_SHA256();
    SignatureVerifierBuilder& withCertificateChainValidation(
        const std::string& ca_bundle_path
    );
    SignatureVerifierBuilder& withCRLCheck(const std::string& crl_url);
    
    std::shared_ptr<ISignatureVerifier> build();

private:
    std::shared_ptr<ISignatureVerifier> head_;
    std::shared_ptr<ISignatureVerifier> tail_;
};

} // namespace security
} // namespace llm
} // namespace themis
```

#### Implementation: `src/llm/security/signature_verifier.cpp`
```cpp
#include "llm/security/signature_verifier.h"
#include <spdlog/spdlog.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/sha.h>

namespace themis {
namespace llm {
namespace security {

// ===== RSA_SHA256_Verifier =====

SignatureVerificationResult RSA_SHA256_Verifier::verify(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& signature,
    const std::string& cert_pem) {
    
    SignatureVerificationResult result;
    result.algorithm = "RSA-SHA256";
    
    try {
        // 1. Load certificate
        auto cert = loadCertificate(cert_pem);
        if (!cert) {
            result.error_message = "Failed to load certificate";
            return result;
        }
        
        // 2. Extract public key
        auto public_key = extractPublicKey(cert.get());
        if (!public_key) {
            result.error_message = "Failed to extract public key";
            return result;
        }
        
        // 3. Compute SHA-256 hash of data
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(data.data(), data.size(), hash);
        
        // 4. Create verification context
        std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(
            EVP_PKEY_CTX_new(public_key.get(), nullptr),
            EVP_PKEY_CTX_free
        );
        
        if (!ctx) {
            result.error_message = "Failed to create EVP context";
            return result;
        }
        
        // 5. Initialize verification
        if (EVP_PKEY_verify_init(ctx.get()) <= 0) {
            result.error_message = "Failed to initialize verification";
            return result;
        }
        
        // 6. Set signature algorithm
        if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) <= 0) {
            result.error_message = "Failed to set RSA padding";
            return result;
        }
        
        if (EVP_PKEY_CTX_set_signature_md(ctx.get(), EVP_sha256()) <= 0) {
            result.error_message = "Failed to set signature MD";
            return result;
        }
        
        // 7. Perform verification
        int verify_result = EVP_PKEY_verify(
            ctx.get(),
            signature.data(),
            signature.size(),
            hash,
            SHA256_DIGEST_LENGTH
        );
        
        if (verify_result == 1) {
            result.is_valid = true;
            
            // Extract signer identity from cert
            char* subject = X509_NAME_oneline(
                X509_get_subject_name(cert.get()),
                nullptr,
                0
            );
            result.signer_identity = subject ? subject : "unknown";
            OPENSSL_free(subject);
            
            spdlog::info("Signature verification PASSED for: {}", 
                        result.signer_identity);
            
            // Pass to next verifier in chain
            return passToNext(data, signature, cert_pem);
            
        } else {
            result.is_valid = false;
            result.error_message = "Signature verification failed";
            
            // Log OpenSSL errors
            unsigned long err;
            while ((err = ERR_get_error()) != 0) {
                char err_buf[256];
                ERR_error_string_n(err, err_buf, sizeof(err_buf));
                spdlog::error("OpenSSL error: {}", err_buf);
            }
            
            return result;
        }
        
    } catch (const std::exception& e) {
        result.is_valid = false;
        result.error_message = std::string("Exception: ") + e.what();
        return result;
    }
}

std::unique_ptr<X509, decltype(&X509_free)> 
RSA_SHA256_Verifier::loadCertificate(const std::string& cert_pem) {
    
    std::unique_ptr<BIO, decltype(&BIO_free)> bio(
        BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())),
        BIO_free
    );
    
    if (!bio) {
        return {nullptr, X509_free};
    }
    
    X509* cert = PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr);
    return {cert, X509_free};
}

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>
RSA_SHA256_Verifier::extractPublicKey(X509* cert) {
    
    EVP_PKEY* key = X509_get_pubkey(cert);
    return {key, EVP_PKEY_free};
}

// ===== CertificateChainVerifier =====

CertificateChainVerifier::CertificateChainVerifier(
    const std::string& ca_bundle_path)
    : ca_bundle_path_(ca_bundle_path) {
}

SignatureVerificationResult CertificateChainVerifier::verify(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& signature,
    const std::string& cert_pem) {
    
    SignatureVerificationResult result;
    result.algorithm = "Certificate Chain Validation";
    
    try {
        // 1. Load certificate
        std::unique_ptr<BIO, decltype(&BIO_free)> bio(
            BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())),
            BIO_free
        );
        
        std::unique_ptr<X509, decltype(&X509_free)> cert(
            PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr),
            X509_free
        );
        
        if (!cert) {
            result.error_message = "Failed to load certificate";
            return result;
        }
        
        // 2. Create certificate store
        std::unique_ptr<X509_STORE, decltype(&X509_STORE_free)> store(
            X509_STORE_new(),
            X509_STORE_free
        );
        
        if (!store) {
            result.error_message = "Failed to create certificate store";
            return result;
        }
        
        // 3. Load CA bundle
        if (X509_STORE_load_locations(store.get(), ca_bundle_path_.c_str(), nullptr) != 1) {
            result.error_message = "Failed to load CA bundle";
            return result;
        }
        
        // 4. Verify certificate chain
        result.chain_valid = verifyCertificateChain(cert.get(), store.get());
        
        if (result.chain_valid) {
            result.is_valid = true;
            spdlog::info("Certificate chain validation PASSED");
            return passToNext(data, signature, cert_pem);
        } else {
            result.is_valid = false;
            result.error_message = "Certificate chain validation failed";
            return result;
        }
        
    } catch (const std::exception& e) {
        result.is_valid = false;
        result.error_message = std::string("Exception: ") + e.what();
        return result;
    }
}

bool CertificateChainVerifier::verifyCertificateChain(
    X509* cert,
    X509_STORE* store) {
    
    std::unique_ptr<X509_STORE_CTX, decltype(&X509_STORE_CTX_free)> ctx(
        X509_STORE_CTX_new(),
        X509_STORE_CTX_free
    );
    
    if (!ctx) {
        return false;
    }
    
    if (X509_STORE_CTX_init(ctx.get(), store, cert, nullptr) != 1) {
        return false;
    }
    
    int verify_result = X509_verify_cert(ctx.get());
    
    if (verify_result != 1) {
        int error = X509_STORE_CTX_get_error(ctx.get());
        spdlog::error("Certificate verification error: {}", 
                     X509_verify_cert_error_string(error));
        return false;
    }
    
    return true;
}

// ===== CRLChecker =====

CRLChecker::CRLChecker(const std::string& crl_url)
    : crl_url_(crl_url) {
}

SignatureVerificationResult CRLChecker::verify(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& signature,
    const std::string& cert_pem) {
    
    SignatureVerificationResult result;
    result.algorithm = "CRL Check";
    
    try {
        // Load certificate
        std::unique_ptr<BIO, decltype(&BIO_free)> bio(
            BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())),
            BIO_free
        );
        
        std::unique_ptr<X509, decltype(&X509_free)> cert(
            PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr),
            X509_free
        );
        
        if (!cert) {
            result.error_message = "Failed to load certificate";
            return result;
        }
        
        // Check if revoked
        bool revoked = isCertificateRevoked(cert.get());
        
        if (revoked) {
            result.is_valid = false;
            result.error_message = "Certificate has been revoked";
            spdlog::error("Certificate REVOKED");
            return result;
        }
        
        result.is_valid = true;
        spdlog::info("CRL check PASSED (not revoked)");
        return passToNext(data, signature, cert_pem);
        
    } catch (const std::exception& e) {
        result.is_valid = false;
        result.error_message = std::string("Exception: ") + e.what();
        return result;
    }
}

bool CRLChecker::isCertificateRevoked(X509* cert) {
    // TODO: Implement actual CRL download and check
    // For now, stub (assume not revoked)
    spdlog::warn("CRL check not fully implemented, assuming not revoked");
    return false;
}

// ===== Builder =====

SignatureVerifierBuilder& SignatureVerifierBuilder::withRSA_SHA256() {
    auto verifier = std::make_shared<RSA_SHA256_Verifier>();
    
    if (!head_) {
        head_ = verifier;
        tail_ = verifier;
    } else {
        tail_->setNext(verifier);
        tail_ = verifier;
    }
    
    return *this;
}

SignatureVerifierBuilder& SignatureVerifierBuilder::withCertificateChainValidation(
    const std::string& ca_bundle_path) {
    
    auto verifier = std::make_shared<CertificateChainVerifier>(ca_bundle_path);
    
    if (!head_) {
        head_ = verifier;
        tail_ = verifier;
    } else {
        tail_->setNext(verifier);
        tail_ = verifier;
    }
    
    return *this;
}

SignatureVerifierBuilder& SignatureVerifierBuilder::withCRLCheck(
    const std::string& crl_url) {
    
    auto verifier = std::make_shared<CRLChecker>(crl_url);
    
    if (!head_) {
        head_ = verifier;
        tail_ = verifier;
    } else {
        tail_->setNext(verifier);
        tail_ = verifier;
    }
    
    return *this;
}

std::shared_ptr<ISignatureVerifier> SignatureVerifierBuilder::build() {
    return head_;
}

} // namespace security
} // namespace llm
} // namespace themis
```

#### Integration in LoRASecurityValidator
```cpp
// src/llm/lora_security_validator.cpp (refactored)

ValidationResult LoRASecurityValidator::Impl::validateSignature(
    const std::string& lora_file_path) {
    
    ValidationResult result;
    result.is_valid = false;
    
    try {
        // 1. Read file data
        auto file_data = readFileData(lora_file_path);
        
        // 2. Extract signature and certificate from file metadata
        auto [signature, cert_pem] = extractSignatureFromFile(lora_file_path);
        
        // 3. Build verification chain
        auto verifier = security::SignatureVerifierBuilder()
            .withRSA_SHA256()
            .withCertificateChainValidation(config_.ca_bundle_path)
            .withCRLCheck(config_.crl_url)
            .build();
        
        // 4. Verify
        auto verify_result = verifier->verify(file_data, signature, cert_pem);
        
        result.is_valid = verify_result.is_valid;
        result.signature_algorithm = verify_result.algorithm;
        result.error_message = verify_result.error_message;
        
        if (result.is_valid) {
            spdlog::info("Signature validation PASSED for: {}", lora_file_path);
        } else {
            spdlog::error("Signature validation FAILED: {}", result.error_message);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        result.error_message = std::string("Exception: ") + e.what();
        spdlog::error("Signature validation exception: {}", e.what());
        return result;
    }
}
```

**Best Practices angewendet**:
- ✅ Chain of Responsibility Pattern (mehrere Prüfungen)
- ✅ Builder Pattern (fluent interface für Konfiguration)
- ✅ RAII für OpenSSL-Ressourcen (unique_ptr mit custom deleters)
- ✅ Separation of Concerns (jede Klasse eine Verifikation)
- ✅ Open/Closed Principle (neue Verifikationen hinzufügbar)

---

## 3. Training Service: Echtes ML Training

**Quelle**: `src/llm/lora_framework/lora_training_service.cpp` (Zeilen 59-83)  
**Problem**: Training ist simuliert mit `sleep(10ms)`  
**Priorität**: ⛔ KRITISCH  
**Aufwand**: 12-16 Wochen

### 3.1 LoRA Layer Implementation (Woche 1-3)

#### OOP Best Practice: Composite Pattern für Layer-Hierarchie

**Neue Datei**: `include/llm/lora_framework/lora_layers.h`
```cpp
#pragma once
#include <vector>
#include <memory>
#include <string>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {

// Forward declarations
class Tensor;
class Optimizer;

/**
 * @brief Abstract Base für trainierbare Layer
 * 
 * Design Pattern: Composite Pattern
 * Allows treating individual layers and compositions uniformly
 */
class ITrainableLayer {
public:
    virtual ~ITrainableLayer() = default;
    
    // Forward pass
    virtual Tensor forward(const Tensor& input) = 0;
    
    // Backward pass (gradient computation)
    virtual Tensor backward(const Tensor& grad_output) = 0;
    
    // Parameter access
    virtual std::vector<Tensor*> parameters() = 0;
    
    // Layer metadata
    virtual std::string name() const = 0;
    virtual size_t parameter_count() const = 0;
    virtual size_t memory_bytes() const = 0;
};

/**
 * @brief LoRA Layer (Low-Rank Adaptation)
 * 
 * W' = W + (B @ A) * scaling
 * B: (in_dim, rank)
 * A: (rank, out_dim)
 */
class LoRALayer : public ITrainableLayer {
public:
    LoRALayer(size_t in_dim, size_t out_dim, size_t rank, float scaling = 1.0f);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& grad_output) override;
    
    std::vector<Tensor*> parameters() override;
    
    std::string name() const override { return name_; }
    size_t parameter_count() const override;
    size_t memory_bytes() const override;
    
    // Export weights (für Speicherung)
    std::pair<Tensor, Tensor> get_weights() const;
    void set_weights(const Tensor& B, const Tensor& A);

private:
    std::string name_;
    size_t in_dim_;
    size_t out_dim_;
    size_t rank_;
    float scaling_;
    
    // Trainable parameters (B and A matrices)
    std::unique_ptr<Tensor> B_;  // (in_dim, rank)
    std::unique_ptr<Tensor> A_;  // (rank, out_dim)
    
    // Cached for backward pass
    Tensor cached_input_;
    Tensor cached_BA_;
};

/**
 * @brief Attention-LoRA (LoRA für Attention Weights)
 */
class AttentionLoRA : public ITrainableLayer {
public:
    AttentionLoRA(size_t dim, size_t rank, 
                  bool apply_to_q = true,
                  bool apply_to_k = true,
                  bool apply_to_v = true,
                  bool apply_to_o = true);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& grad_output) override;
    
    std::vector<Tensor*> parameters() override;
    
    std::string name() const override { return "AttentionLoRA"; }
    size_t parameter_count() const override;
    size_t memory_bytes() const override;

private:
    // Query, Key, Value, Output projections
    std::unique_ptr<LoRALayer> q_lora_;
    std::unique_ptr<LoRALayer> k_lora_;
    std::unique_ptr<LoRALayer> v_lora_;
    std::unique_ptr<LoRALayer> o_lora_;
};

/**
 * @brief Sequential Container (Composite)
 */
class Sequential : public ITrainableLayer {
public:
    void add(std::unique_ptr<ITrainableLayer> layer);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& grad_output) override;
    
    std::vector<Tensor*> parameters() override;
    
    std::string name() const override { return "Sequential"; }
    size_t parameter_count() const override;
    size_t memory_bytes() const override;

private:
    std::vector<std::unique_ptr<ITrainableLayer>> layers_;
};

} // namespace lora
} // namespace llm
} // namespace themis
```

**Dieses Dokument wird wegen der Längenbeschränkung hier beendet. Die vollständige Implementation würde fortgesetzt mit**:
- Tensor-Klasse Implementation
- Optimizer-Implementation (Adam, AdamW)
- Loss-Funktionen
- Training Loop
- Storage Backend
- Job Orchestrator

Die grundlegenden Design Patterns und Best Practices sind etabliert. Möchten Sie, dass ich spezifische Teile detaillierter ausarbeite?

**Best Practices gezeigt**:
- ✅ Composite Pattern für Layer-Hierarchie
- ✅ Template Method Pattern (forward/backward in Base)
- ✅ Smart Pointers für Ownership
- ✅ Const Correctness
- ✅ Interface Segregation

---

## Zusammenfassung: Implementierungsreihenfolge

### Priorität 1 (Wochen 1-4): llama.cpp Foundation
1. Resource Manager mit RAII (`LlamaModelHandle`, `LlamaContextHandle`)
2. Tokenization
3. Sampling Strategies (Strategy Pattern)
4. Basic Generation Loop

### Priorität 2 (Wochen 5-6): Security
5. Signature Verifier mit Chain of Responsibility
6. Certificate Chain Validation
7. CRL Checker

### Priorität 3 (Wochen 7-18): Training System
8. LoRA Layers mit Composite Pattern
9. Tensor Operations
10. Optimizer (Adam)
11. Training Loop
12. Checkpointing

### Priorität 4 (Wochen 19-22): Infrastructure
13. ThemisDB Storage Backend
14. Job Queue
15. Async Orchestrator

### Priorität 5 (Wochen 23-24): Quality
16. Test Suite
17. Benchmarks
18. Documentation

---

**Ende des Implementierungsguides - Version 1.0**
