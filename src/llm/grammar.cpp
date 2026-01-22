#include "llm/grammar.h"
#include <llama.h>
#include <spdlog/spdlog.h>
#include <utility>

// Some llama.cpp builds may not expose grammar APIs in the headers; declare minimal prototypes.
extern "C" {
    struct llama_grammar;
    llama_grammar* llama_grammar_init(const char* grammar_str, const char* root_symbol);
    void llama_grammar_free(llama_grammar* grammar);
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
    try {
        // Parse EBNF grammar using llama.cpp
        grammar_ = llama_grammar_init(
            ebnf_text_.c_str(),
            start_symbol_.c_str()
        );
        
        if (!grammar_) {
            error_ = "Failed to compile grammar: invalid EBNF syntax or start symbol";
            spdlog::error("Grammar compilation failed for start symbol: {}", start_symbol_);
            return false;
        }
        
        spdlog::info("Grammar compiled successfully for start symbol: {}", start_symbol_);
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
