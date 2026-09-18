/**
 * @file conflict_resolver.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

enum class ConflictReasonType {
    PRIMARY_KEY_COLLISION,  ///< Duplicate primary key
    MERGE_CONFLICT,         ///< Field-level merge needed
    TIMESTAMP_CONFLICT,     ///< Competing versions by timestamp
    CONSTRAINT_VIOLATION,   ///< Unique/foreign key constraint violation
    UNKNOWN                 ///< Reason could not be determined
};

struct ConflictMetadata {
    ConflictReasonType reason;              ///< Why conflict occurred
    std::vector<std::string> affected_fields; ///< Fields involved in conflict
    std::string resolution_strategy;        ///< e.g., "CRDT_LWW", "MERGE", "SKIP"
    uint64_t timestamp_used;                ///< Timestamp used for tiebreaker (0 if not used)
};

class ImportConflictResolver {
public:
    ImportConflictResolver() = default;

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

    /**
     * @brief Compute Key.
     * @param[in] entity Input parameter.
     * @param[in] key_columns Input parameter.
     * @return Return value.
     */
    static std::string computeKey(const json& entity,
                                  const std::vector<std::string>& key_columns);

    /**
     * @brief Resolve.
     * @param[in] entity Input parameter.
     * @param[in] table_name Name of the table.
     * @param[in] conflict_key Input parameter.
     * @param[in] strategy Input parameter.
     * @param[in] merge_depth Input parameter.
     * @param[in] protected_fields Input parameter.
     * @param[in,out] conflict_detected Input/output parameter.
     * @return Return value.
     */
    json resolve(const json& entity,
                 const std::string& table_name,
                 const std::string& conflict_key,
                 ConflictStrategy strategy,
                 int merge_depth,
                 const std::vector<std::string>& protected_fields,
                 bool& conflict_detected);

    /**
     * @brief Merge Entities.
     * @param[in] existing Input parameter.
     * @param[in] incoming Input parameter.
     * @param[in] depth Input parameter.
     * @param[in] protected_fields Input parameter.
     * @return Return value.
     */
    static json mergeEntities(const json& existing,
                               const json& incoming,
                               int depth,
                               const std::vector<std::string>& protected_fields);

    /**
     * @brief Resolve With Metadata.
     * @param[in] entity Input parameter.
     * @param[in] table_name Name of the table.
     * @param[in] conflict_key Input parameter.
     * @param[in] strategy Input parameter.
     * @param[in] merge_depth Input parameter.
     * @param[in] protected_fields Input parameter.
     * @param[in,out] conflict_detected Input/output parameter.
     * @param[in,out] metadata Input/output parameter.
     * @return Return value.
     */
    json resolveWithMetadata(const json& entity,
                            const std::string& table_name,
                            const std::string& conflict_key,
                            ConflictStrategy strategy,
                            int merge_depth,
                            const std::vector<std::string>& protected_fields,
                            bool& conflict_detected,
                            ConflictMetadata& metadata);

    /**
     * @brief Determine Conflict Reason.
     * @param[in] existing Input parameter.
     * @param[in] incoming Input parameter.
     * @param[in,out] affected_fields Input/output parameter.
     * @return Return value.
     */
    static ConflictReasonType determineConflictReason(
        const json& existing,
        const json& incoming,
        std::vector<std::string>& affected_fields);

private:
    // table_name -> (conflict_key -> stored entity)
    std::unordered_map<std::string,
                       std::unordered_map<std::string, json>> registry_;
};

} // namespace importers
} // namespace themis

