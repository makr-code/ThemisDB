/**
 * @file grammar.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <memory>

// Forward declaration for llama.cpp types
struct llama_grammar;
struct llama_model;
struct llama_vocab;

namespace themis {
namespace llm {

class Grammar {
public:
    Grammar(const std::string& ebnf_text, const std::string& start_symbol);
    
    Grammar(const std::string& ebnf_text,
            const std::string& start_symbol,
            const struct llama_model* model);
    
    ~Grammar();
    
    // Prevent copying
    Grammar(const Grammar&) = delete;
    Grammar& operator=(const Grammar&) = delete;
    
    // Allow moving
    Grammar(Grammar&& other) noexcept;
    Grammar& operator=(Grammar&& other) noexcept;
    
    /**
     * @brief Is Valid.
     * @return True when the operation succeeds.
     */
    bool isValid() const;
    
    /**
     * @brief Get Error.
     * @return Return value.
     */
    std::string getError() const;
    
    /**
     * @brief Get EBNFText.
     * @return Return value.
     */
    std::string getEBNFText() const;
    
    /**
     * @brief Get Start Symbol.
     * @return Return value.
     */
    std::string getStartSymbol() const;
    
    /**
     * @brief Get Handle.
     * @return Pointer to the result.
     */
    llama_grammar* getHandle() const;
    
private:
    llama_grammar* grammar_ = nullptr;
    std::string ebnf_text_;
    std::string start_symbol_;
    std::string error_;
    
    /**
     * @brief Helper to compile EBNF using a specific vocab pointer (may be nullptr only when the caller has verified that structural-only rules are used).
     * @return True when the operation succeeds.
     */
    bool compile();
    /**
     * @brief Compile With Vocab.
     * @param[in] vocab Input parameter.
     * @return True when the operation succeeds.
     */
    bool compileWithVocab(const ::llama_vocab* vocab);
};

} // namespace llm
} // namespace themis
