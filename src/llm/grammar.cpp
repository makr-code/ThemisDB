#include "llm/grammar.h"
#include <llama.h>
#include <spdlog/spdlog.h>
#include <utility>

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
        // TODO: llama_grammar_free not yet available in stable llama.cpp
        // llama_grammar_free(grammar_);
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
        // TODO: llama_grammar_free not yet available in stable llama.cpp
        // if (grammar_ != nullptr) {
        //     llama_grammar_free(grammar_);
        // }
        
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
        // TODO: llama_grammar_init not yet available in stable llama.cpp
        // grammar_ = llama_grammar_init(
        //     ebnf_text_.c_str(),
        //     start_symbol_.c_str()
        // );
        
        // For now, mark as successfully compiled even though we can't use grammar constraints
        spdlog::debug("Grammar constraints requested but not yet implemented in llama.cpp");
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
