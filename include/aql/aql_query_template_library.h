/**
 * @file aql_query_template_library.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class AQLQueryTemplateLibrary {
public:
    AQLQueryTemplateLibrary();
    ~AQLQueryTemplateLibrary() = default;

    // =========================================================================
    // Registration
    // =========================================================================

    /**
     * @brief Register Template.
     * @param[in] tmpl Input parameter.
     */
    void registerTemplate(const AQLQueryTemplate& tmpl);

    // =========================================================================
    // Lookup
    // =========================================================================

    /**
     * @brief All.
     * @return Return value.
     */
    const std::vector<AQLQueryTemplate>& all() const;

    /**
     * @brief Find By Tag.
     * @param[in] tag Input parameter.
     * @return Return value.
     */
    std::vector<AQLQueryTemplate> findByTag(const std::string& tag) const;

    /**
     * @brief Search.
     * @param[in] keyword Input parameter.
     * @return Return value.
     */
    std::vector<AQLQueryTemplate> search(const std::string& keyword) const;

    /**
     * @brief Find By Id.
     * @param[in] id Input parameter.
     * @return Pointer to the result.
     */
    const AQLQueryTemplate* findById(const std::string& id) const;

    // =========================================================================
    // Instantiation
    // =========================================================================

    std::string instantiate(
        const std::string& id,
        const std::unordered_map<std::string, std::string>& parameters
    ) const;

    static std::string instantiate(
        const AQLQueryTemplate& tmpl,
        const std::unordered_map<std::string, std::string>& parameters
    );

private:
    std::vector<AQLQueryTemplate>                             templates_;
    std::unordered_map<std::string, std::size_t>              index_by_id_;

    /**
     * @brief Register Builtins.
     */
    void registerBuiltins_();
};

} // namespace aql
} // namespace themis
