/**
 * @file aql_syntax_highlighter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.36
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <cstddef>

namespace themis {
namespace aql {

/**
 * @brief Token types recognized by the AQL syntax highlighter.
 */
enum class AQLTokenType {
    KEYWORD,      ///< Core AQL keyword (FOR, FILTER, RETURN, …)
    LLM_KEYWORD,  ///< LLM-extension keyword (LLM, INFER, RAG, …)
    FUNCTION,     ///< Built-in function call (SIMILARITY, SUM, …)
    IDENTIFIER,   ///< User-defined names, collection names
    STRING,       ///< Quoted string literal
    NUMBER,       ///< Integer or floating-point literal
    OPERATOR,     ///< Comparison / arithmetic / logic operators
    PUNCTUATION,  ///< Braces, brackets, commas, dots
    COMMENT,      ///< Single-line (//…) or block (/* … */) comment
    UNKNOWN       ///< Any text that does not match the above categories
};

/**
 * @brief A single token produced by the AQL tokenizer.
 */
struct AQLToken {
    AQLTokenType type;
    std::string  value;
    std::size_t  line;    ///< 1-based line number
    std::size_t  column;  ///< 1-based column number
};

/**
 * @brief A syntax or semantic error annotation attached to an AQL snippet.
 */
struct AQLAnnotation {
    std::size_t line;     ///< 1-based line where the error starts
    std::size_t column;   ///< 1-based column where the error starts
    std::string message;  ///< Human-readable description
};

/**
 * @brief Result of processing an LLM response that may contain AQL code blocks.
 */
struct HighlightedResponse {
    std::string              text;         ///< Full response with AQL blocks highlighted
    std::vector<AQLAnnotation> annotations; ///< Any syntax errors found in AQL blocks
};

/**
 * @brief AQL syntax highlighter and error annotator for LLM responses.
 *
 * Usage example (terminal/ANSI output):
 * @code
 *   AQLSyntaxHighlighter h;
 *   auto result = h.formatLLMResponse(llm_output);
 *   std::cout << result.text;
 *   for (auto& ann : result.annotations)
 *       std::cout << "Error at " << ann.line << ':' << ann.column
 *                 << " - " << ann.message << '\n';
 * @endcode
 *
 * If ANSI escape codes are not desired (e.g. when writing to a file), pass
 * @c use_ansi = @c false to the constructor.
 */
class AQLSyntaxHighlighter {
public:
    /**
     * @param use_ansi  When @c true (default) ANSI escape sequences are
     *                  embedded so that terminals render colours.  When
     *                  @c false the output is plain text.
     */
    explicit AQLSyntaxHighlighter(bool use_ansi = true);

    /**
     * @brief Tokenize a raw AQL snippet.
     * @param aql_code  The AQL source code (may span multiple lines).
     * @return Ordered sequence of tokens.
     */
    std::vector<AQLToken> tokenize(const std::string& aql_code) const;

    /**
     * @brief Apply syntax highlighting to a raw AQL snippet.
     * @param aql_code  Source AQL code.
     * @return The same code with ANSI colour escapes (or plain if !use_ansi).
     */
    std::string highlightBlock(const std::string& aql_code) const;

    /**
     * @brief Validate an AQL snippet and return error annotations.
     *
     * Performs a lightweight structural check:
     * - balanced braces / brackets / parentheses
     * - unterminated string literals
     * - unrecognised token sequences that indicate a likely syntax error
     *
     * @param aql_code  Source AQL code.
     * @return List of annotations (empty if no errors detected).
     */
    std::vector<AQLAnnotation> annotateErrors(const std::string& aql_code) const;

    /**
     * @brief Process a full LLM response, highlighting every AQL code block.
     *
     * Finds every @c ```aql … ``` (or @c ``` … ```) block, highlights the AQL
     * inside, and collects any error annotations from those blocks.  Surrounding
     * prose is left unchanged.
     *
     * @param llm_response  Raw text from the LLM.
     * @return Struct with the modified text and a merged list of annotations.
     */
    HighlightedResponse formatLLMResponse(const std::string& llm_response) const;

private:
    bool use_ansi_;

    std::string colorize(const AQLToken& tok) const;
    std::string ansiReset() const;
};

} // namespace aql
} // namespace themis
