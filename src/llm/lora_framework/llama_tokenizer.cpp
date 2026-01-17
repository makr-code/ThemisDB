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
    
    // Initialize llama.cpp backend
    llama_backend_init();
    
    // Load model in vocab-only mode (lightweight - only tokenizer)
    llama_model_params model_params = llama_model_default_params();
    model_params.vocab_only = true;  // Only load tokenizer, not weights
    
    model_ = llama_load_model_from_file(model_path.c_str(), model_params);
    if (!model_) {
        llama_backend_free();
        throw std::runtime_error("Failed to load model for tokenizer: " + model_path);
    }
    
    // Create context (needed for tokenization)
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 512;  // Small context for tokenization
    ctx_params.n_batch = 512;
    
    context_ = llama_new_context_with_model(model_, ctx_params);
    if (!context_) {
        llama_free_model(model_);
        llama_backend_free();
        model_ = nullptr;
        throw std::runtime_error("Failed to create context for tokenizer");
    }
    
    spdlog::info("✓ llama.cpp tokenizer initialized (vocab_size={})", 
                 llama_n_vocab(model_));
}

LlamaTokenizer::~LlamaTokenizer() {
    cleanup();
}

LlamaTokenizer::LlamaTokenizer(LlamaTokenizer&& other) noexcept
    : model_(other.model_)
    , context_(other.context_)
    , model_path_(std::move(other.model_path_)) {
    other.model_ = nullptr;
    other.context_ = nullptr;
}

LlamaTokenizer& LlamaTokenizer::operator=(LlamaTokenizer&& other) noexcept {
    if (this != &other) {
        cleanup();
        model_ = other.model_;
        context_ = other.context_;
        model_path_ = std::move(other.model_path_);
        other.model_ = nullptr;
        other.context_ = nullptr;
    }
    return *this;
}

void LlamaTokenizer::cleanup() {
    if (context_) {
        llama_free(context_);
        context_ = nullptr;
    }
    if (model_) {
        llama_free_model(model_);
        model_ = nullptr;
    }
    llama_backend_free();
}

std::vector<int> LlamaTokenizer::encode(const std::string& text, 
                                        bool add_bos, 
                                        bool add_eos) {
    if (!model_ || !context_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    
    if (text.empty()) {
        std::vector<int> tokens;
        if (add_bos) tokens.push_back(bos_token_id());
        if (add_eos) tokens.push_back(eos_token_id());
        return tokens;
    }
    
    // Allocate buffer for tokens (estimate size + extra for special tokens)
    std::vector<llama_token> tokens_buffer(text.size() + 16);
    
    // Tokenize using llama.cpp
    int n_tokens = llama_tokenize(
        model_,
        text.c_str(),
        text.size(),
        tokens_buffer.data(),
        tokens_buffer.size(),
        add_bos,   // add_bos
        false      // special (set to false, we handle special tokens manually)
    );
    
    if (n_tokens < 0) {
        // Buffer too small, retry with larger buffer
        tokens_buffer.resize(text.size() * 2 + 32);
        n_tokens = llama_tokenize(
            model_,
            text.c_str(),
            text.size(),
            tokens_buffer.data(),
            tokens_buffer.size(),
            add_bos,
            false
        );
        
        if (n_tokens < 0) {
            throw std::runtime_error("Tokenization failed (buffer too small even after resize)");
        }
    }
    
    tokens_buffer.resize(n_tokens);
    
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
    
    std::string result;
    result.reserve(tokens.size() * 4);  // Estimate 4 chars per token
    
    for (int token : tokens) {
        // Skip special tokens for cleaner output
        if (token == bos_token_id() || 
            token == eos_token_id() || 
            token == pad_token_id()) {
            continue;
        }
        
        // Get token piece from llama.cpp
        char piece_buf[256];
        int n_chars = llama_token_to_piece(model_, token, piece_buf, sizeof(piece_buf));
        
        if (n_chars < 0) {
            // Buffer too small, allocate larger buffer
            std::vector<char> large_buf(-n_chars + 1);
            n_chars = llama_token_to_piece(model_, token, large_buf.data(), large_buf.size());
            if (n_chars > 0) {
                result.append(large_buf.data(), n_chars);
            }
        } else if (n_chars > 0) {
            result.append(piece_buf, n_chars);
        }
    }
    
    return result;
}

int LlamaTokenizer::vocab_size() const {
    if (!model_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    return llama_n_vocab(model_);
}

int LlamaTokenizer::bos_token_id() const {
    if (!model_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    return llama_token_bos(model_);
}

int LlamaTokenizer::eos_token_id() const {
    if (!model_) {
        throw std::runtime_error("Tokenizer not initialized");
    }
    return llama_token_eos(model_);
}

int LlamaTokenizer::pad_token_id() const {
    // Llama models typically use EOS as padding token
    return eos_token_id();
}

} // namespace lora
} // namespace llm
} // namespace themis
