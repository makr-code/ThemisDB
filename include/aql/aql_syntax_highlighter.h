/**
 * @file aql_syntax_highlighter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.36
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <cstddef>

namespace themis {
namespace aql {

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

struct AQLToken {
    AQLTokenType type;
    std::string  value;
    std::size_t  line;    ///< 1-based line number
    std::size_t  column;  ///< 1-based column number
};

struct AQLAnnotation {
    std::size_t line;     ///< 1-based line where the error starts
    std::size_t column;   ///< 1-based column where the error starts
    std::string message;  ///< Human-readable description
};

struct HighlightedResponse {
    std::string              text;         ///< Full response with AQL blocks highlighted
    std::vector<AQLAnnotation> annotations; ///< Any syntax errors found in AQL blocks
};

class AQLSyntaxHighlighter {
public:
    explicit AQLSyntaxHighlighter(bool use_ansi = true);

    /**
     * @brief Tokenize.
     * @param[in] aql_code Input parameter.
     * @return Return value.
     */
    std::vector<AQLToken> tokenize(const std::string& aql_code) const;

    /**
     * @brief Highlight Block.
     * @param[in] aql_code Input parameter.
     * @return Return value.
     */
    std::string highlightBlock(const std::string& aql_code) const;

    /**
     * @brief Annotate Errors.
     * @param[in] aql_code Input parameter.
     * @return Return value.
     */
    std::vector<AQLAnnotation> annotateErrors(const std::string& aql_code) const;

    /**
     * @brief Format LLMResponse.
     * @param[in] llm_response Input parameter.
     * @return Return value.
     */
    HighlightedResponse formatLLMResponse(const std::string& llm_response) const;

private:
    bool use_ansi_;

    /**
     * @brief Colorize.
     * @param[in] tok Input parameter.
     * @return Return value.
     */
    std::string colorize(const AQLToken& tok) const;
    /**
     * @brief Ansi Reset.
     * @return Return value.
     */
    std::string ansiReset() const;
};

} // namespace aql
} // namespace themis
