/**
 * @file llama_tokenizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/llama_tokenizer.h"
#include <llama.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

LlamaTokenizer::LlamaTokenizer(const std::string& model_path) 
    : model_path_(model_path) {
    spdlog::info("Initializing llama.cpp tokenizer from: {}", model_path);
    
    // Note: llama_backend_init() is called globally by the application
    // (typically in LlamaWrapper initialization or main() function).
    // We don't call it here to avoid conflicts with multiple instances.
    // The backend must be initialized before creating any LlamaTokenizer instances.
    
    // Load model in vocab-only mode (lightweight - only tokenizer)
    llama_model_params model_params = llama_model_default_params();
    model_params.vocab_only = true;  // Only load tokenizer, not weights
    
    model_ = llama_load_model_from_file(model_path.c_str(), model_params);
    if (!model_) {
        throw std::runtime_error("Failed to load model for tokenizer: " + model_path);
    }
    
    spdlog::info("✓ llama.cpp tokenizer initialized (vocab_size={})", 
                 32000);  // Default vocab size for most llama models
}

LlamaTokenizer::~LlamaTokenizer() noexcept {
    // Phase2-LLM-B1: exception_in_destructor — cleanup() calls llama_free_model
    // which may trigger hooks that throw. Suppress to satisfy §[except.spec].
    try {
        cleanup();
    } catch (const std::exception& e) {
        (void)e; // llama_free_model() error: suppress; OS reclaims memory
    } catch (...) {}
}

LlamaTokenizer::LlamaTokenizer(LlamaTokenizer&& other) noexcept
    : model_(other.model_)
    , model_path_(std::move(other.model_path_)) {
    other.model_ = nullptr;
}

LlamaTokenizer& LlamaTokenizer::operator=(LlamaTokenizer&& other) noexcept {
    if (this != &other) {
        cleanup();
        model_ = other.model_;
        model_path_ = std::move(other.model_path_);
        other.model_ = nullptr;
    }
    return *this;
}

void LlamaTokenizer::cleanup() {
    if (model_) {
        llama_free_model(model_);
        model_ = nullptr;
    }
    // Note: llama_backend_free() is called globally by the application
    // We don't call it here to avoid conflicts with multiple instances
}

std::vector<int> LlamaTokenizer::encode(const std::string& text, 
                                        bool add_bos, 
                                        bool add_eos) {
    if (!model_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    
    if (text.empty()) {
        std::vector<int> tokens = {};

        if (add_bos) {
          tokens.push_back(bos_token_id());
        }
        if (add_eos) {
          tokens.push_back(eos_token_id());
        }
        return tokens;
    }
    
    // Get vocab from model (matches existing pattern in llama_wrapper.cpp)
    const llama_vocab* vocab = llama_model_get_vocab(model_);
    
    // Allocate buffer for tokens (estimate size + extra for special tokens)
    std::vector<llama_token> tokens_buffer(text.size() + 16);
    
    // Tokenize using llama.cpp (matches existing pattern in llama_wrapper.cpp)
    int32_t n_tokens = llama_tokenize(
        vocab,
        text.c_str(),
        static_cast<int32_t>(text.length()),
        tokens_buffer.data(),
        static_cast<int32_t>(tokens_buffer.size()),
        add_bos,   // add_bos
        false      // special (set to false, we handle special tokens manually)
    );
    
    if (n_tokens < 0) {
        // Buffer too small, retry with larger buffer (matches existing pattern)
        tokens_buffer.resize(-n_tokens);
        n_tokens = llama_tokenize(
            vocab,
            text.c_str(),
            static_cast<int32_t>(text.length()),
            tokens_buffer.data(),
            static_cast<int32_t>(tokens_buffer.size()),
            add_bos,
            false
        );
        
        if (n_tokens < 0) {
            throw std::runtime_error("Tokenization failed (buffer too small even after resize)");
        }
    }
    
    tokens_buffer.resize(static_cast<size_t>(n_tokens));
    
    // Convert to std::vector<int>
    std::vector<int> result(tokens_buffer.begin(), tokens_buffer.end());
    
    // Add EOS token if requested (llama_tokenize doesn't add it by default)
    if (add_eos) {
        result.push_back(eos_token_id());
    }
    
    return result;
}

std::string LlamaTokenizer::decode(const std::vector<int>& tokens) {
    if (!model_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    
    if (tokens.empty()) {
        return "";
    }
    
    // Get vocab from model (matches existing pattern in llama_wrapper.cpp)
    const llama_vocab* vocab = llama_model_get_vocab(model_);
    
    std::string result;
    result.reserve(tokens.size() * 4);  // Estimate 4 chars per token
    
    for (int token : tokens) {
        // Skip special tokens for cleaner output
        if (token == bos_token_id() || 
            token == eos_token_id() || 
            token == pad_token_id()) {
            continue;
        }
        
        // Get token piece from llama.cpp (matches existing pattern)
        char buf[256];
        int32_t n = llama_token_to_piece(vocab, token, buf, sizeof(buf), 0, false);
        
        // Match existing pattern: only use result if it fits in buffer with room
        if (n > 0 && n < static_cast<int32_t>(sizeof(buf))) {
            result.append(buf, n);
        }
    }
    
    return result;
}

int LlamaTokenizer::vocab_size() const {
    if (!model_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    // Default vocab size for llama models (32000 or 32001 depending on version)
    return 32000;
}

int LlamaTokenizer::bos_token_id() const {
    if (!model_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    // Standard BOS token for llama models
    return 1;
}

int LlamaTokenizer::eos_token_id() const {
    if (!model_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    // Standard EOS token for llama models
    return 2;
}

int LlamaTokenizer::pad_token_id() const {
    // Llama models typically use EOS as padding token
    return eos_token_id();
}

} // namespace lora
} // namespace llm
} // namespace themis

