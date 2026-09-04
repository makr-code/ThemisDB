/**
 * @file aql_autocomplete.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Completion item kind — mirrors LSP CompletionItemKind values.
 *
 * The numeric values intentionally match the LSP specification so that
 * editor integrations can forward them directly without a mapping step.
 */
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

/**
 * @brief A single completion suggestion, compatible with the LSP CompletionItem.
 *
 * Fields match the LSP specification wherever relevant.  Unused LSP fields
 * are omitted for simplicity; they can be added without breaking existing users.
 */
struct CompletionItem {
    /// The label shown in the editor completion list (e.g., "FILTER")
    std::string label = {};

    /// Kind hint for the editor (maps to an icon / colour)
    CompletionItemKind kind = CompletionItemKind::Keyword;

    /// Short one-line description shown alongside the label
    std::string detail;

    /// Longer Markdown documentation string (optional)
    std::string documentation;

    /// Text to insert when the item is accepted.
    /// May contain snippet placeholders using ${N:placeholder} syntax.
    /// Defaults to `label` when empty.
    std::string insert_text = {};

    /// 0-based column offset of the start of the prefix to replace.
    /// The range [prefix_start, cursor_column) will be replaced by insert_text.
    std::size_t prefix_start = 0;

    /// Sort key: lower values appear higher in the list (default 0)
    int sort_order = 0;
};

// ============================================================================
// CompletionContext — cursor position and surrounding text
// ============================================================================

/**
 * @brief Describes the position within a query where auto-complete is triggered.
 *
 * Mirrors LSP's TextDocumentPositionParams + CompletionContext.
 */
struct CompletionContext {
    /// The full AQL query text as typed so far
    std::string query_text = {};

    /// 0-based offset of the cursor within query_text.
    /// Defaults to end-of-text when set to std::string::npos.
    std::size_t cursor_offset = std::string::npos;

    /// Optional: schema context string describing available collections/fields.
    /// Format: "collection: users(id, name, age), orders(id, user_id, total)"
    std::string schema_context;

    /// Optional: trigger character that opened the completion (e.g. "." or " ")
    std::string trigger_character;
};

// ============================================================================
// AQLAutoComplete
// ============================================================================

/**
 * @brief Rule-based AQL auto-complete engine with LSP-compatible output.
 *
 * Provides keyword, function, operator and schema-aware attribute completions
 * based purely on lexical analysis of the partial query — no LLM required.
 *
 * ### LSP integration
 * The returned `CompletionItem` list can be serialised to JSON and wrapped in
 * an LSP `CompletionList` response with minimal effort:
 * @code
 *   {
 *     "jsonrpc": "2.0",
 *     "id": <request-id>,
 *     "result": {
 *       "isIncomplete": false,
 *       "items": [ (CompletionItem objects) ]
 *     }
 *   }
 * @endcode
 *
 * ### Usage example
 * @code
 *   AQLAutoComplete ac;
 *   CompletionContext ctx;
 *   ctx.query_text     = "FOR u IN users FILT";
 *   ctx.cursor_offset  = ctx.query_text.size();  // end of text
 *   ctx.schema_context = "collection: users(id, name, age)";
 *
 *   auto items = ac.complete(ctx);
 *   for (const auto& item : items)
 *       std::cout << item.label << " - " << item.detail << '\n';
 * @endcode
 */
class AQLAutoComplete {
public:
    AQLAutoComplete()  = default;
    ~AQLAutoComplete() = default;

    /**
     * @brief Compute completion items for the given cursor context.
     *
     * The method performs the following steps:
     *  1. Resolve the effective cursor offset (defaults to end-of-text).
     *  2. Extract the token prefix at the cursor position.
     *  3. Determine the completion context (clause position, after-dot, etc.).
     *  4. Build and filter the candidate list by matching against the prefix.
     *  5. Sort candidates: exact-prefix matches first, then alphabetically.
     *
     * @param ctx  Completion context (see CompletionContext)
     * @return     Ordered list of completion items (may be empty)
     */
    std::vector<CompletionItem> complete(const CompletionContext& ctx) const;

    /**
     * @brief Return all AQL keywords known to this engine.
     *
     * Useful for pre-populating editor grammars or syntax highlighting configs.
     */
    std::vector<std::string> allKeywords() const;

    /**
     * @brief Return all built-in AQL function names known to this engine.
     */
    std::vector<std::string> allFunctions() const;

private:
    // ----- Internal helpers -------------------------------------------------

    /// Extract the word/prefix that immediately precedes cursor_offset in text.
    std::string extractPrefix(const std::string& text, std::size_t cursor) const;

    /// Compute the 0-based column where the current prefix starts.
    std::size_t prefixStart(const std::string& text, std::size_t cursor) const;

    /// True when the cursor is immediately after a dot (for attribute completion)
    bool isAfterDot(const std::string& text, std::size_t cursor) const;

    /// Extract variable name before the dot, or empty string
    std::string variableBeforeDot(const std::string& text, std::size_t cursor) const;

    /// Collect FOR/LET variable names declared before cursor_offset
    std::vector<std::string> declaredVariables(
        const std::string& text, std::size_t cursor) const;

    /// Parse schema_context into a map: collection -> list of fields
    /// Format accepted: "collection: col1(f1, f2), col2(f3)"
    /// or plain "col1, col2"
    struct SchemaInfo {
        std::string collection_name;
        std::vector<std::string> fields;
    };
    std::vector<SchemaInfo> parseSchema(const std::string& schema_context) const;

    /// Build keyword completion candidates
    std::vector<CompletionItem> keywordCandidates(
        const std::string& text, std::size_t cursor) const;

    /// Build function completion candidates
    std::vector<CompletionItem> functionCandidates() const;

    /// Build variable completion candidates
    std::vector<CompletionItem> variableCandidates(
        const std::string& text, std::size_t cursor) const;

    /// Build attribute completion candidates after a dot (schema-aware)
    std::vector<CompletionItem> attributeCandidates(
        const std::string& variable,
        const std::vector<SchemaInfo>& schema,
        const std::string& text,
        std::size_t cursor) const;

    /// Filter candidates by case-insensitive prefix match, then sort
    std::vector<CompletionItem> filterAndSort(
        std::vector<CompletionItem> candidates,
        const std::string& prefix,
        std::size_t prefix_start_col) const;
};

} // namespace aql
} // namespace themis
