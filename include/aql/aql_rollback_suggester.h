/**
 * @file aql_rollback_suggester.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace aql {

// ============================================================================
// IAQLRollbackSuggester
// ============================================================================

enum class MutationType {
    INSERT,  ///< FOR … INSERT document INTO collection
    UPDATE,  ///< FOR … UPDATE key WITH changes IN collection
    REPLACE, ///< FOR … REPLACE key WITH document IN collection
    REMOVE,  ///< FOR … REMOVE key IN collection
    UPSERT,  ///< UPSERT searchExpr INSERT insertExpr UPDATE updateExpr IN collection
    NONE,    ///< Query is read-only or empty
};

struct RollbackSuggestion {
    bool is_automatic = false;

    std::string rollback_query;

    MutationType mutation_type = MutationType::NONE;

    std::string collection;

    std::string caveat;

    std::vector<std::string> manual_steps;
};

class IAQLRollbackSuggester {
public:
    /**
     * @brief IAQLRollback Suggester.
     * @return Return value.
     */
    virtual ~IAQLRollbackSuggester() = default;

    /**
     * @brief Suggest.
     * @param[in] aql_query Input parameter.
     * @return Return value.
     */
    virtual RollbackSuggestion suggest(const std::string& aql_query) const = 0;
};

class AQLRollbackSuggester : public IAQLRollbackSuggester {
public:
    AQLRollbackSuggester()  = default;
    ~AQLRollbackSuggester() override = default;

    RollbackSuggestion suggest(const std::string& aql_query) const override;
};

} // namespace aql
} // namespace themis
