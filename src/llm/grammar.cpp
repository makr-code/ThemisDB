/**
 * @file grammar.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/grammar.h"
#include <spdlog/spdlog.h>
#include <utility>

// Forward declarations for llama.cpp grammar API (from llama_grammar_adapter.cpp)
extern "C" {
    struct llama_vocab;
    struct llama_grammar* llama_grammar_init(const struct llama_vocab* vocab, const char* grammar_str, const char* start_rule);
    void llama_grammar_free(struct llama_grammar* grammar);
    bool themis_llama_grammar_available();
    
    // Helper to get vocab from model (defined in llama.h)
    const struct llama_vocab* llama_model_get_vocab(const struct llama_model* model);
}

namespace themis {
namespace llm {

Grammar::Grammar(const std::string& ebnf_text, const std::string& start_symbol)
    : ebnf_text_(ebnf_text)
    , start_symbol_(start_symbol)
    , grammar_(nullptr)
    , error_() {
    
    if (ebnf_text_.empty()) {
        error_ = "EBNF text cannot be empty";
        return;
    }
    
    if (start_symbol_.empty()) {
        error_ = "Start symbol cannot be empty";
        return;
    }
    
    // Compile without a vocab pointer.  This works for structural grammars but
    // may not filter correctly for token-level rules.  Prefer the model-aware
    // constructor Grammar(ebnf, start, model) when a loaded model is available.
    compile();
}

Grammar::Grammar(const std::string& ebnf_text,
                 const std::string& start_symbol,
                 const struct llama_model* model)
    : ebnf_text_(ebnf_text)
    , start_symbol_(start_symbol)
    , grammar_(nullptr)
    , error_() {
    
    if (ebnf_text_.empty()) {
        error_ = "EBNF text cannot be empty";
        return;
    }
    
    if (start_symbol_.empty()) {
        error_ = "Start symbol cannot be empty";
        return;
    }

    if (model == nullptr) {
        error_ = "Grammar compilation failed: model pointer is null; cannot bind vocabulary";
        spdlog::error("Grammar compilation failed: null model passed to model-aware constructor");
        return;
    }
    
    if (!themis_llama_grammar_available()) {
        // Hard error: the API is required for model-aware compilation.
        error_ = "Grammar support is unavailable (llama.cpp grammar API not present)";
        spdlog::error("Grammar compilation failed (model-aware constructor): {}", error_);
        return;
    }
    
    const ::llama_vocab* vocab = llama_model_get_vocab(model);
    if (vocab == nullptr) {
        error_ = "Grammar compilation failed: llama_model_get_vocab returned null for the provided model";
        spdlog::error("Grammar compilation failed: llama_model_get_vocab returned null");
        return;
    }
    
    compileWithVocab(vocab);
}

Grammar::~Grammar() {
    // Free llama_grammar resources if available
    if (grammar_ != nullptr) {
        llama_grammar_free(grammar_);
        grammar_ = nullptr;
    }
}

Grammar::Grammar(Grammar&& other) noexcept
    : grammar_(other.grammar_)
    , ebnf_text_(std::move(other.ebnf_text_))
    , start_symbol_(std::move(other.start_symbol_))
    , error_(std::move(other.error_)) {
    
    other.grammar_ = nullptr;
}

Grammar& Grammar::operator=(Grammar&& other) noexcept {
    if (this != &other) {
        // Free existing grammar
        if (grammar_ != nullptr) {
            llama_grammar_free(grammar_);
        }
        
        // Move from other
        grammar_ = other.grammar_;
        ebnf_text_ = std::move(other.ebnf_text_);
        start_symbol_ = std::move(other.start_symbol_);
        error_ = std::move(other.error_);
        
        other.grammar_ = nullptr;
    }
    return *this;
}

bool Grammar::isValid() const {
    return grammar_ != nullptr && error_.empty();
}

std::string Grammar::getError() const {
    return error_;
}

std::string Grammar::getEBNFText() const {
    return ebnf_text_;
}

std::string Grammar::getStartSymbol() const {
    return start_symbol_;
}

llama_grammar* Grammar::getHandle() const {
    return grammar_;
}

bool Grammar::compile() {
    // Check if Grammar API is available at runtime
    if (!themis_llama_grammar_available()) {
        error_ = "Grammar support is unavailable (llama.cpp grammar API not present)";
        spdlog::error("Grammar compilation failed: {}", error_);
        grammar_ = nullptr;
        return false;
    }
    
    // Compile without a vocab.  The null vocab path is a known limitation of
    // this (no-model) constructor: purely structural EBNF rules compile
    // successfully, but rules requiring token-to-text vocab knowledge may
    // produce incorrect results.  Use Grammar(ebnf, start, model) for
    // full correctness.
    return compileWithVocab(nullptr);
}

bool Grammar::compileWithVocab(const ::llama_vocab* vocab) {
    try {
        grammar_ = llama_grammar_init(vocab, ebnf_text_.c_str(), start_symbol_.c_str());
        
        if (grammar_ == nullptr) {
            error_ = "Failed to compile grammar: Invalid EBNF syntax or start symbol";
            spdlog::error("Grammar compilation failed for start symbol: {}", start_symbol_);
            return false;
        }
        
        if (vocab != nullptr) {
            spdlog::info("Grammar compiled successfully with vocab binding: start_symbol={}", start_symbol_);
        } else {
            spdlog::info("Grammar compiled successfully (no vocab binding): start_symbol={}", start_symbol_);
        }
        return true;
        
    } catch (const std::exception& e) {
        error_ = std::string("Exception during grammar compilation: ") + e.what();
        spdlog::error("Grammar compilation exception: {}", error_);
        grammar_ = nullptr;
        return false;
    }
}

} // namespace llm
} // namespace themis

