/**
 * @file canonical_resolver.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/entity_linker.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

enum class ResolutionPolicy {
    NEWEST_FIRST,        ///< Use the record with the most-recent timestamp
    MOST_COMPLETE,       ///< Use the record with the fewest null/empty fields
    EXISTING_PREFERRED,  ///< Prefer the record already in ThemisDB
    INCOMING_PREFERRED,  ///< Prefer the newly imported record
    RICHEST_MERGE,       ///< Field-level: pick the non-null / longer value from each record
    CUSTOM_RULES         ///< Caller-supplied field rules (via field_rules map)
};

enum class FieldRule {
    KEEP_EXISTING,  ///< Never overwrite with incoming value
    TAKE_INCOMING,  ///< Always take incoming value
    TAKE_MAX,       ///< Numeric/string: take the larger value
    TAKE_MIN,       ///< Numeric/string: take the smaller value
    TAKE_SUM,       ///< Numeric: sum both values
    CONCATENATE,    ///< String: append incoming to existing with a separator
    TAKE_LONGEST,   ///< String: keep the longer, non-empty value
    TAKE_NEWEST     ///< ISO timestamp string: keep the later timestamp
};

struct GoldenRecord {
    std::string              canonical_id;          ///< UUID identifying the golden record
    json                     merged_data;            ///< Best-of-breed merged entity data
    std::vector<std::string> contributing_ids;       ///< Source entity IDs merged in
    double                   completeness_score = 0.0; ///< 0–1.0: fraction of fields that are non-null
    json                     field_provenance;       ///< {"field_name": "source_entity_id", …}
    std::string              last_reconciliation;   ///< RFC 3339 timestamp

    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
};

struct FieldQualityPolicy {
    size_t min_length         = 0;     ///< Minimum acceptable string length
    bool   prefer_upper_case  = false; ///< Prefer capitalised values
    bool   prefer_digits_only = false; ///< For phone / ID fields: prefer digit-only strings
};

class CanonicalEntityResolver {
public:
    CanonicalEntityResolver() = default;

    GoldenRecord createGoldenRecord(
        const std::vector<std::pair<std::string, json>>& linked_entities,
        const std::string&                               collection_name,
        ResolutionPolicy                                 policy,
        const std::map<std::string, FieldRule>&          field_rules       = {},
        const std::vector<std::string>&                  protected_fields  = {}
    ) const;

    // -----------------------------------------------------------------------
    // Field-level reconciliation helpers (also usable standalone)
    // -----------------------------------------------------------------------

    static std::string reconcileStringField(
        const std::string& value1,
        const std::string& value2,
        FieldRule          rule,
        const std::string& separator = " | "
    );

    /**
     * @brief Reconcile Numeric Field.
     * @param[in] value1 Input parameter.
     * @param[in] value2 Input parameter.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    static int64_t reconcileNumericField(
        int64_t   value1,
        int64_t   value2,
        FieldRule rule
    );

    static json reconcileObjectField(
        const json&      obj1,
        const json&      obj2,
        ResolutionPolicy policy,
        int              depth = -1
    );

    static double scoreFieldQuality(
        const std::string&    field_name,
        const std::string&    value,
        const FieldQualityPolicy& policy = {}
    );

private:
    /**
     * @brief Compute Completeness.
     * @param[in] entity Input parameter.
     * @return Return value.
     */
    static double computeCompleteness(const json& entity);
    /**
     * @brief Best String Value.
     * @param[in] v1 Input parameter.
     * @param[in] v2 Input parameter.
     * @param[in] policy Input parameter.
     * @return Return value.
     */
    static std::string bestStringValue(
        const std::string& v1,
        const std::string& v2,
        ResolutionPolicy   policy
    );
};

} // namespace importers
} // namespace themis
