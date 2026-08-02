/**
 * @file conflict_resolver.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: conflict_resolver.h | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 122
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #2813 [importers] Implement confl... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

/**
 * @brief Conflict reason classification (Phase 2 T2.3.1).
 *
 * PHASE-2-HARDENING: Conflict Determinism & Reason Tracking
 * Determinism: yes (enum-based classification, no randomness)
 * Audit: conflicts classified with reason type
 * Bounded: classification ≤ 1ms
 */
enum class ConflictReasonType {
    PRIMARY_KEY_COLLISION,  ///< Duplicate primary key
    MERGE_CONFLICT,         ///< Field-level merge needed
    TIMESTAMP_CONFLICT,     ///< Competing versions by timestamp
    CONSTRAINT_VIOLATION,   ///< Unique/foreign key constraint violation
    UNKNOWN                 ///< Reason could not be determined
};

/**
 * @brief Metadata tracked during conflict resolution (Phase 2 T2.3.1).
 *
 * PHASE-2-HARDENING: Conflict Resolution Metadata
 * Determinism: yes (all fields deterministic)
 * Audit: full metadata available for audit trail
 * Bounded: memory O(number of affected fields)
 */
struct ConflictMetadata {
    ConflictReasonType reason;              ///< Why conflict occurred
    std::vector<std::string> affected_fields; ///< Fields involved in conflict
    std::string resolution_strategy;        ///< e.g., "CRDT_LWW", "MERGE", "SKIP"
    uint64_t timestamp_used;                ///< Timestamp used for tiebreaker (0 if not used)
};

/**
 * @brief Import conflict resolver for in-session duplicate detection.
 *
 * Tracks entities by a caller-supplied conflict key during a single import
 * session.  When the same key is encountered a second time the configured
 * ConflictStrategy is applied:
 *
 * - SKIP      – discard the incoming duplicate; return the already-seen entity.
 * - OVERWRITE – replace the already-seen entity with the incoming one.
 * - MERGE     – field-level merge: incoming fields win unless listed in
 *               @c protected_fields.  Respects @c merge_depth for nested objects.
 * - ERROR     – signal a conflict error to the caller (returns nullopt-like
 *               empty json and sets @c conflict_detected out-parameter).
 *
 * Thread-safety: instances are not thread-safe.  Create one resolver per
 * import worker thread / per importData() call.
 *
 * Usage example:
 * @code
 *   ImportConflictResolver resolver;
 *   resolver.reset();
 *
 *   // For each entity produced by the parser:
 *   bool conflict = false;
 *   json resolved = resolver.resolve(entity, "users", "42",
 *                                    ConflictStrategy::MERGE,
 *                                    1, {"created_at"}, conflict);
 *   if (conflict && strategy == ConflictStrategy::ERROR) { ... abort ... }
 * @endcode
 */
class ImportConflictResolver {
public:
    ImportConflictResolver() = default;

    /**
     * @brief Reset all state (call once at the beginning of each import session).
     */
    void reset();

    /**
     * @brief Compute a string conflict key from an entity and the configured key columns.
     *
     * @param entity          The JSON entity from which field values are read.
     * @param key_columns     Column names whose values are concatenated to form the key.
     * @return                ASCII unit-separator (0x1F) delimited concatenation of the
     *                        field values, or an empty string if @p key_columns is empty.
     */
    static std::string computeKey(const json& entity,
                                  const std::vector<std::string>& key_columns);

    /**
     * @brief Apply conflict resolution for a single entity.
     *
     * @param entity            Incoming entity (just produced by the parser).
     * @param table_name        Source table name (used to scope the key registry).
     * @param conflict_key      Pre-computed conflict key (see computeKey()).
     * @param strategy          Conflict resolution strategy.
     * @param merge_depth       Merge depth for MERGE strategy (1 = top-level,
     *                          -1 = deep recursive).
     * @param protected_fields  Fields that the MERGE strategy must not overwrite.
     * @param[out] conflict_detected  Set to true if the key was already seen.
     * @return                  The resolved entity to use for further processing.
     *                          When @p strategy is ERROR and a conflict is detected
     *                          the returned value equals the already-seen entity.
     */
    json resolve(const json& entity,
                 const std::string& table_name,
                 const std::string& conflict_key,
                 ConflictStrategy strategy,
                 int merge_depth,
                 const std::vector<std::string>& protected_fields,
                 bool& conflict_detected);

    /**
     * @brief Merge two JSON entities field by field.
     *
     * Incoming fields win over existing ones unless they appear in
     * @p protected_fields.  Nested objects are recursed into when
     * @p depth != 1 (see merge_depth semantics above).
     *
     * @param existing         Entity already stored from a previous row.
     * @param incoming         Entity produced from the current duplicate row.
     * @param depth            Merge depth (1 = flat, -1 = unlimited recursion).
     * @param protected_fields Fields not to overwrite from @p incoming.
     * @return                 Merged entity.
     */
    static json mergeEntities(const json& existing,
                               const json& incoming,
                               int depth,
                               const std::vector<std::string>& protected_fields);

    /**
     * @brief Resolve conflict with full metadata tracking (Phase 2 T2.3.1).
     *
     * PHASE-2-HARDENING: Conflict Determinism & Reason Tracking
     * Determinism: yes (CRDT LWW with row_id tiebreaker)
     * Audit: returns full ConflictMetadata for audit trail
     * Bounded: resolution ≤ 100ms
     *
     * Same signature as resolve() but also returns structured metadata about
     * the conflict reason, affected fields, and strategy used. This is the
     * primary entry point for Phase 2 hardening.
     *
     * @param entity            Incoming entity (just produced by the parser).
     * @param table_name        Source table name (used to scope the key registry).
     * @param conflict_key      Pre-computed conflict key (see computeKey()).
     * @param strategy          Conflict resolution strategy.
     * @param merge_depth       Merge depth for MERGE strategy (1 = top-level,
     *                          -1 = deep recursive).
     * @param protected_fields  Fields that the MERGE strategy must not overwrite.
     * @param[out] conflict_detected  Set to true if the key was already seen.
     * @param[out] metadata    Conflict metadata including reason and strategy used.
     * @return                  The resolved entity to use for further processing.
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
     * @brief Determine the reason for a conflict (Phase 2 T2.3.1).
     *
     * PHASE-2-HARDENING: Conflict Reason Determination
     * Determinism: yes (enum-based classification)
     * Audit: reason type for audit trail
     * Bounded: classification ≤ 1ms
     *
     * Analyzes two conflicting entities to determine the reason. Checks for:
     * 1. Primary key collision (identical key fields)
     * 2. Timestamp conflicts (different timestamps)
     * 3. Merge conflicts (field-level differences)
     * 4. Constraint violations (detected from entity structure)
     *
     * @param existing    Previously seen entity with same conflict key
     * @param incoming    Newly encountered entity with same conflict key
     * @param[out] affected_fields  Fields that differ between entities
     * @return            ConflictReasonType enum value
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

