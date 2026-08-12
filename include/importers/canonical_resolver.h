/**
 * @file canonical_resolver.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Policy governing how a canonical (golden) record is selected or built.
 */
enum class ResolutionPolicy {
    NEWEST_FIRST,        ///< Use the record with the most-recent timestamp
    MOST_COMPLETE,       ///< Use the record with the fewest null/empty fields
    EXISTING_PREFERRED,  ///< Prefer the record already in ThemisDB
    INCOMING_PREFERRED,  ///< Prefer the newly imported record
    RICHEST_MERGE,       ///< Field-level: pick the non-null / longer value from each record
    CUSTOM_RULES         ///< Caller-supplied field rules (via field_rules map)
};

/**
 * @brief Per-field resolution rule used by CUSTOM_RULES policy.
 */
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

/**
 * @brief A canonical (golden) record produced from one or more linked entities.
 */
struct GoldenRecord {
    std::string              canonical_id;          ///< UUID identifying the golden record
    json                     merged_data;            ///< Best-of-breed merged entity data
    std::vector<std::string> contributing_ids;       ///< Source entity IDs merged in
    double                   completeness_score = 0.0; ///< 0–1.0: fraction of fields that are non-null
    json                     field_provenance;       ///< {"field_name": "source_entity_id", …}
    std::string              last_reconciliation;   ///< RFC 3339 timestamp

    json toJson() const;
};

/**
 * @brief Quality policy controlling field-quality scoring.
 */
struct FieldQualityPolicy {
    size_t min_length         = 0;     ///< Minimum acceptable string length
    bool   prefer_upper_case  = false; ///< Prefer capitalised values
    bool   prefer_digits_only = false; ///< For phone / ID fields: prefer digit-only strings
};

/**
 * @brief Resolves conflicting linked entities into a single canonical record.
 *
 * All methods are stateless.  The caller provides the set of linked entity
 * JSON objects and the resolver produces a GoldenRecord without persisting
 * anything itself.
 *
 * Thread-safety: all public methods are stateless and safe to call
 * concurrently from multiple worker threads.
 */
class CanonicalEntityResolver {
public:
    CanonicalEntityResolver() = default;

    /**
     * @brief Create a golden record from a set of linked entities.
     *
     * @param linked_entities   JSON objects of all entities contributing to
     *                          the golden record, paired with their IDs.
     * @param collection_name   Collection the golden record belongs to.
     * @param policy            Merge policy to apply.
     * @param field_rules       Per-field rule overrides (used when policy is
     *                          CUSTOM_RULES or as tie-breakers for other policies).
     * @param protected_fields  Fields that must NEVER be overwritten (e.g., "id",
     *                          "created_at").
     * @return                  A new GoldenRecord.
     */
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

    /**
     * @brief Reconcile two string field values.
     *
     * @param value1    Existing value.
     * @param value2    Incoming value.
     * @param rule      Field-level rule to apply.
     * @param separator Separator for CONCATENATE rule (default: " | ").
     * @return          Resolved value.
     */
    static std::string reconcileStringField(
        const std::string& value1,
        const std::string& value2,
        FieldRule          rule,
        const std::string& separator = " | "
    );

    /**
     * @brief Reconcile two 64-bit integer field values.
     *
     * @param value1  Existing value.
     * @param value2  Incoming value.
     * @param rule    Must be one of: TAKE_MAX, TAKE_MIN, TAKE_SUM, KEEP_EXISTING, TAKE_INCOMING.
     * @return        Resolved value.
     */
    static int64_t reconcileNumericField(
        int64_t   value1,
        int64_t   value2,
        FieldRule rule
    );

    /**
     * @brief Recursively reconcile two JSON objects.
     *
     * @param obj1    Existing object.
     * @param obj2    Incoming object.
     * @param policy  Resolution policy.
     * @param depth   Remaining merge depth (-1 = unlimited, 0 = replace entirely).
     * @return        Merged JSON object.
     */
    static json reconcileObjectField(
        const json&      obj1,
        const json&      obj2,
        ResolutionPolicy policy,
        int              depth = -1
    );

    /**
     * @brief Score the quality of a single field value.
     *
     * @param field_name  Name of the field being scored.
     * @param value       String representation of the field value.
     * @param policy      Quality scoring policy.
     * @return            Quality score in [0.0, 1.0].
     */
    static double scoreFieldQuality(
        const std::string&    field_name,
        const std::string&    value,
        const FieldQualityPolicy& policy = {}
    );

private:
    static double computeCompleteness(const json& entity);
    static std::string bestStringValue(
        const std::string& v1,
        const std::string& v2,
        ResolutionPolicy   policy
    );
};

} // namespace importers
} // namespace themis
