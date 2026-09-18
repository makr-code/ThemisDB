/**
 * @file aql_autocomplete.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
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

// ============================================================================
// LSP-compatible completion item kinds
// (mirrors CompletionItemKind from the Language Server Protocol spec)
// ============================================================================

enum class CompletionItemKind {
    Text        = 1,
    Method      = 2,
    Function    = 3,
    Constructor = 4,
    Field       = 5,
    Variable    = 6,
    Class       = 7,
    Interface   = 8,
    Module      = 9,
    Property    = 10,
    Unit        = 11,
    Value       = 12,
    Enum        = 13,
    Keyword     = 14,
    Snippet     = 15,
    Color       = 16,
    File        = 17,
    Reference   = 18,
    Folder      = 19,
    EnumMember  = 20,
    Constant    = 21,
    Struct      = 22,
    Event       = 23,
    Operator    = 24,
    TypeParameter = 25,
};

// ============================================================================
// CompletionItem
// ============================================================================

struct CompletionItem {
    std::string label = {};

    CompletionItemKind kind = CompletionItemKind::Keyword;

    std::string detail;

    std::string documentation;

    std::string insert_text = {};

    std::size_t prefix_start = 0;

    int sort_order = 0;
};

// ============================================================================
// CompletionContext — cursor position and surrounding text
// ============================================================================

struct CompletionContext {
    std::string query_text = {};

    std::size_t cursor_offset = std::string::npos;

    std::string schema_context;

    std::string trigger_character;
};

// ============================================================================
// AQLAutoComplete
// ============================================================================

class AQLAutoComplete {
public:
    AQLAutoComplete()  = default;
    ~AQLAutoComplete() = default;

    /**
     * @brief Complete.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    std::vector<CompletionItem> complete(const CompletionContext& ctx) const;

    /**
     * @brief All Keywords.
     * @return Return value.
     */
    std::vector<std::string> allKeywords() const;

    /**
     * @brief All Functions.
     * @return Return value.
     */
    std::vector<std::string> allFunctions() const;

private:
    /**
     * @brief ----- Internal helpers -------------------------------------------------
     * @param[in] text Input parameter.
     * @param[in] cursor Input parameter.
     * @return Return value.
     */

    std::string extractPrefix(const std::string& text, std::size_t cursor) const;

    /**
     * @brief Prefix Start.
     * @param[in] text Input parameter.
     * @param[in] cursor Input parameter.
     * @return Return value.
     */
    std::size_t prefixStart(const std::string& text, std::size_t cursor) const;

    /**
     * @brief Is After Dot.
     * @param[in] text Input parameter.
     * @param[in] cursor Input parameter.
     * @return True when the operation succeeds.
     */
    bool isAfterDot(const std::string& text, std::size_t cursor) const;

    /**
     * @brief Variable Before Dot.
     * @param[in] text Input parameter.
     * @param[in] cursor Input parameter.
     * @return Return value.
     */
    std::string variableBeforeDot(const std::string& text, std::size_t cursor) const;

    /**
     * @brief Declared Variables.
     * @param[in] text Input parameter.
     * @param[in] cursor Input parameter.
     * @return Return value.
     */
    std::vector<std::string> declaredVariables(
        const std::string& text, std::size_t cursor) const;

    struct SchemaInfo {
        std::string collection_name;
        std::vector<std::string> fields;
    };
    /**
     * @brief Parse Schema.
     * @param[in] schema_context Input parameter.
     * @return Return value.
     */
    std::vector<SchemaInfo> parseSchema(const std::string& schema_context) const;

    /**
     * @brief Keyword Candidates.
     * @param[in] text Input parameter.
     * @param[in] cursor Input parameter.
     * @return Return value.
     */
    std::vector<CompletionItem> keywordCandidates(
        const std::string& text, std::size_t cursor) const;

    /**
     * @brief Function Candidates.
     * @return Return value.
     */
    std::vector<CompletionItem> functionCandidates() const;

    /**
     * @brief Variable Candidates.
     * @param[in] text Input parameter.
     * @param[in] cursor Input parameter.
     * @return Return value.
     */
    std::vector<CompletionItem> variableCandidates(
        const std::string& text, std::size_t cursor) const;

    /**
     * @brief Attribute Candidates.
     * @param[in] variable Input parameter.
     * @param[in] schema Input parameter.
     * @param[in] text Input parameter.
     * @param[in] cursor Input parameter.
     * @return Return value.
     */
    std::vector<CompletionItem> attributeCandidates(
        const std::string& variable,
        const std::vector<SchemaInfo>& schema,
        const std::string& text,
        std::size_t cursor) const;

    /**
     * @brief Filter And Sort.
     * @param[in] candidates Input parameter.
     * @param[in] prefix Input parameter.
     * @param[in] prefix_start_col Input parameter.
     * @return Return value.
     */
    std::vector<CompletionItem> filterAndSort(
        std::vector<CompletionItem> candidates,
        const std::string& prefix,
        std::size_t prefix_start_col) const;
};

} // namespace aql
} // namespace themis
