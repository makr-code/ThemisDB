/**
 * @file aql_migration_assistant.h
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

namespace themis {
namespace aql {

struct MigrationIssue {
    enum class Severity {
        ERROR,   ///< Construct is unsupported and cannot be automatically migrated
        WARNING, ///< Construct was rewritten but manual review is recommended
        INFO     ///< Informational note about compatibility or style differences
    };

    Severity    severity;
    std::string message;    ///< Human-readable description of the issue
    std::string suggestion; ///< Suggested alternative or manual action
};

struct MigrationResult {
    std::string migrated_query;

    bool is_fully_automatable = true;

    std::vector<MigrationIssue> issues;

    /**
     * @brief Summary.
     * @return Return value.
     */
    std::string summary() const;
};

class AQLMigrationAssistant {
public:
    AQLMigrationAssistant()  = default;
    ~AQLMigrationAssistant() = default;

    /**
     * @brief Migrate.
     * @param[in] arango_aql Input parameter.
     * @return Return value.
     */
    MigrationResult migrate(const std::string& arango_aql) const;

private:
    /**
     * @brief Rewrite Near.
     * @param[in] query Input parameter.
     * @param[in,out] issues Input/output parameter.
     * @return Return value.
     */
    std::string rewriteNear(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /**
     * @brief Rewrite Within.
     * @param[in] query Input parameter.
     * @param[in,out] issues Input/output parameter.
     * @return Return value.
     */
    std::string rewriteWithin(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /**
     * @brief Rewrite Fulltext.
     * @param[in] query Input parameter.
     * @param[in,out] issues Input/output parameter.
     * @return Return value.
     */
    std::string rewriteFulltext(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /**
     * @brief Rewrite Document.
     * @param[in] query Input parameter.
     * @param[in,out] issues Input/output parameter.
     * @return Return value.
     */
    std::string rewriteDocument(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /**
     * @brief Rewrite Double At Bind.
     * @param[in] query Input parameter.
     * @param[in,out] issues Input/output parameter.
     * @return Return value.
     */
    std::string rewriteDoubleAtBind(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /**
     * @brief Detect V8.
     * @param[in] query Input parameter.
     * @param[in,out] issues Input/output parameter.
     */
    void detectV8(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /**
     * @brief Detect Type Check Functions.
     * @param[in] query Input parameter.
     * @param[in,out] issues Input/output parameter.
     */
    void detectTypeCheckFunctions(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /**
     * @brief Detect Hash Function.
     * @param[in] query Input parameter.
     * @param[in,out] issues Input/output parameter.
     */
    void detectHashFunction(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /**
     * @brief Detect Attributes Function.
     * @param[in] query Input parameter.
     * @param[in,out] issues Input/output parameter.
     */
    void detectAttributesFunction(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /**
     * @brief Detect Translate Function.
     * @param[in] query Input parameter.
     * @param[in,out] issues Input/output parameter.
     */
    void detectTranslateFunction(
        const std::string& query,
        std::vector<MigrationIssue>& issues
    ) const;

    /**
     * @brief Extract Args.
     * @param[in] src Input parameter.
     * @param[in] open_paren Input parameter.
     * @return Return value.
     */
    static std::string extractArgs(const std::string& src, std::size_t open_paren);
};

} // namespace aql
} // namespace themis
