/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            conflict_resolver.h                                ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:10:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     136                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a3d6da5ace  2026-02-24  feat(importers): implement conflict resolution strategies... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

private:
    // table_name -> (conflict_key -> stored entity)
    std::unordered_map<std::string,
                       std::unordered_map<std::string, json>> registry_;
};

} // namespace importers
} // namespace themis
