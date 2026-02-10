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
    
    // Compile the grammar
    compile();
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
        spdlog::warn("Grammar compilation skipped: {}", error_);
        grammar_ = nullptr;
        return false;
    }
    
    try {
        // Note: llama_grammar_init requires a vocab pointer from a loaded model
        // For now, we'll pass nullptr and handle this at usage time
        // The grammar will be fully initialized when used with a specific model
        // This is a limitation of the current API design
        
        // Attempt to compile grammar with null vocab (will work for basic grammars)
        grammar_ = llama_grammar_init(nullptr, ebnf_text_.c_str(), start_symbol_.c_str());
        
        if (grammar_ == nullptr) {
            error_ = "Failed to compile grammar: Invalid EBNF syntax or start symbol";
            spdlog::error("Grammar compilation failed for start symbol: {}", start_symbol_);
            return false;
        }
        
        spdlog::info("✓ Grammar compiled successfully: start_symbol={}", start_symbol_);
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
