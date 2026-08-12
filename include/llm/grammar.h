/**
 * @file grammar.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Grammar class for constrained generation using EBNF grammars
 * 
 * This class wraps llama.cpp's grammar functionality to enable grammar-constrained
 * text generation, guaranteeing valid structured outputs (JSON, XML, etc.).
 * 
 * Based on GRAMMAR_CONSTRAINED_GENERATION.md documentation.
 */
class Grammar {
public:
    /**
     * @brief Construct a grammar from EBNF text
     * @param ebnf_text EBNF grammar definition
     * @param start_symbol Starting symbol for the grammar (e.g., "root")
     *
     * @note This constructor cannot bind the llama vocab because no model is
     *       provided.  If the llama grammar API is available at runtime the
     *       grammar is compiled with a null vocab pointer; purely structural
     *       grammars (balanced brackets, numeric patterns) work, but
     *       token-filtering rules that require vocab knowledge may not.  To
     *       guarantee correct token filtering, use the model-aware constructor:
     *       Grammar(ebnf_text, start_symbol, model).
     */
    Grammar(const std::string& ebnf_text, const std::string& start_symbol);
    
    /**
     * @brief Construct a grammar from EBNF text with explicit model binding
     *
     * Uses llama_model_get_vocab() to obtain the vocabulary from @p model and
     * passes it to llama_grammar_init().  This is the preferred constructor
     * when a loaded model is available because it enables correct
     * token-level filtering for all grammar rule types.
     *
     * If the llama grammar API is unavailable at runtime (i.e.
     * themis_llama_grammar_available() returns false), the constructor sets
     * an error and isValid() returns false — it does NOT fall back to
     * unconstrained generation silently.
     *
     * @param ebnf_text    EBNF grammar definition
     * @param start_symbol Starting symbol for the grammar (e.g., "root")
     * @param model        Loaded llama_model whose vocabulary should be used
     */
    Grammar(const std::string& ebnf_text,
            const std::string& start_symbol,
            const struct llama_model* model);
    
    /**
     * @brief Destructor - frees llama_grammar resources
     */
    ~Grammar();
    
    // Prevent copying
    Grammar(const Grammar&) = delete;
    Grammar& operator=(const Grammar&) = delete;
    
    // Allow moving
    Grammar(Grammar&& other) noexcept;
    Grammar& operator=(Grammar&& other) noexcept;
    
    /**
     * @brief Check if grammar was compiled successfully
     * @return true if grammar is valid and ready to use
     */
    bool isValid() const;
    
    /**
     * @brief Get error message if compilation failed
     * @return Error message or empty string if valid
     */
    std::string getError() const;
    
    /**
     * @brief Get the EBNF text used to create this grammar
     * @return Original EBNF text
     */
    std::string getEBNFText() const;
    
    /**
     * @brief Get the start symbol
     * @return Start symbol name
     */
    std::string getStartSymbol() const;
    
    /**
     * @brief Get internal llama_grammar handle
     * 
     * This is used internally by LlamaWrapper for token sampling.
     * Users should not need to access this directly.
     * 
     * @return Pointer to llama_grammar or nullptr if invalid
     */
    llama_grammar* getHandle() const;
    
private:
    llama_grammar* grammar_ = nullptr;
    std::string ebnf_text_;
    std::string start_symbol_;
    std::string error_;
    
    // Helper to compile EBNF using a specific vocab pointer (may be nullptr
    // only when the caller has verified that structural-only rules are used).
    bool compile();
    bool compileWithVocab(const ::llama_vocab* vocab);
};

} // namespace llm
} // namespace themis
