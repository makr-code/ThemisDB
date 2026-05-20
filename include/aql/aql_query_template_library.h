/*
 * ThemisDB | File: aql_query_template_library.h | Version: 0.0.39 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 157
 * Open Issues: TODOs=1, Stubs=6, Gaps=8, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=8 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace themis {
namespace aql {

// ============================================================================
// Data types
// ============================================================================

/**
 * @brief A single AQL query template with metadata.
 *
 * Templates use `{{placeholder}}` syntax for parameters.
 * Example:
 * @code
 * // template_body: "FOR {{var}} IN {{collection}} RETURN {{var}}"
 * // Apply with: {{"var","u"},{"collection","users"}}
 * // Result:     "FOR u IN users RETURN u"
 * @endcode
 */
struct AQLQueryTemplate {
    std::string              id;          ///< Unique identifier (e.g., "simple_scan")
    std::string              name;        ///< Human-readable name
    std::string              description; ///< What the template does
    std::string              template_body; ///< AQL with {{placeholders}}
    std::vector<std::string> tags;        ///< Search tags (e.g., "graph", "vector", "filter")
    std::vector<std::string> parameters; ///< List of placeholder names (without braces)
};

// ============================================================================
// Library
// ============================================================================

/**
 * @brief Static registry of common AQL query patterns.
 *
 * All built-in templates are registered at construction time and are
 * immediately available without any LLM connection.
 *
 * Typical usage:
 * @code
 * AQLQueryTemplateLibrary lib;
 *
 * // List all templates that deal with graphs
 * auto graph_templates = lib.findByTag("graph");
 *
 * // Retrieve a specific template by id
 * auto tmpl = lib.findById("graph_traversal");
 *
 * // Instantiate with concrete parameter values
 * std::string aql = lib.instantiate("graph_traversal", {
 *     {"var",        "v"},
 *     {"start_node", "users/42"},
 *     {"edge_coll",  "friends"},
 *     {"depth_min",  "1"},
 *     {"depth_max",  "3"},
 *     {"direction",  "OUTBOUND"},
 * });
 * @endcode
 */
class AQLQueryTemplateLibrary {
public:
    /**
     * @brief Construct the library and register all built-in templates.
     */
    AQLQueryTemplateLibrary();
    ~AQLQueryTemplateLibrary() = default;

    // =========================================================================
    // Registration
    // =========================================================================

    /**
     * @brief Register a custom template.
     * @throws std::invalid_argument if the id is empty or already registered
     */
    void registerTemplate(const AQLQueryTemplate& tmpl);

    // =========================================================================
    // Lookup
    // =========================================================================

    /**
     * @brief Return all registered templates.
     */
    const std::vector<AQLQueryTemplate>& all() const;

    /**
     * @brief Find templates whose tags contain @p tag (case-insensitive).
     * @return Matching templates (may be empty)
     */
    std::vector<AQLQueryTemplate> findByTag(const std::string& tag) const;

    /**
     * @brief Find templates whose name or description contains @p keyword
     *        (case-insensitive).
     * @return Matching templates (may be empty)
     */
    std::vector<AQLQueryTemplate> search(const std::string& keyword) const;

    /**
     * @brief Look up a template by its unique id.
     * @return Pointer to the template, or nullptr if not found
     */
    const AQLQueryTemplate* findById(const std::string& id) const;

    // =========================================================================
    // Instantiation
    // =========================================================================

    /**
     * @brief Instantiate a template, substituting all {{placeholder}} tokens.
     *
     * @param id         Template id
     * @param parameters Map of placeholder name → value
     * @return Instantiated AQL string
     * @throws std::invalid_argument if the template id is unknown
     * @throws std::invalid_argument if a required parameter is missing
     */
    std::string instantiate(
        const std::string& id,
        const std::unordered_map<std::string, std::string>& parameters
    ) const;

    /**
     * @brief Instantiate a template object directly (without lookup by id).
     * @param tmpl       Template to instantiate
     * @param parameters Map of placeholder name → value
     * @return Instantiated AQL string
     * @throws std::invalid_argument if a required parameter is missing
     */
    static std::string instantiate(
        const AQLQueryTemplate& tmpl,
        const std::unordered_map<std::string, std::string>& parameters
    );

private:
    std::vector<AQLQueryTemplate>                             templates_;
    std::unordered_map<std::string, std::size_t>              index_by_id_;

    void registerBuiltins_();
};

} // namespace aql
} // namespace themis
