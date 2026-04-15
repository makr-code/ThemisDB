/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            conflict_resolver.cpp                              ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 05:41:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     150                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a3d6da5ace  2026-02-24  feat(importers): implement conflict resolution strategies... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "importers/conflict_resolver.h"
#include <algorithm>
#include <sstream>

namespace themis {
namespace importers {

// ============================================================================
// ImportConflictResolver
// ============================================================================

void ImportConflictResolver::reset() {
    registry_.clear();
}

std::string ImportConflictResolver::computeKey(const json& entity,
                                                const std::vector<std::string>& key_columns) {
    if (key_columns.empty()) return {};

    // Separator unlikely to appear in field values
    static constexpr char kSep = '\x1F';  // ASCII unit separator

    std::string key;
    for (const auto& col : key_columns) {
        if (!key.empty()) key += kSep;
        if (entity.contains(col)) {
            const auto& v = entity[col];
            if (v.is_string()) {
                key += v.get<std::string>();
            } else {
                key += v.dump();
            }
        }
        // If the column is absent the contribution is empty (separator still added)
    }
    return key;
}

json ImportConflictResolver::resolve(const json& entity,
                                      const std::string& table_name,
                                      const std::string& conflict_key,
                                      ConflictStrategy strategy,
                                      int merge_depth,
                                      const std::vector<std::string>& protected_fields,
                                      bool& conflict_detected) {
    conflict_detected = false;

    // Lookup the per-table registry entry
    auto& table_registry = registry_[table_name];
    auto it = table_registry.find(conflict_key);

    if (it == table_registry.end()) {
        // First time we see this key – no conflict
        table_registry.emplace(conflict_key, entity);
        return entity;
    }

    // ---- Conflict detected ----
    conflict_detected = true;
    json& existing = it->second;

    switch (strategy) {
        case ConflictStrategy::SKIP:
            // Keep the entity we already have; discard the incoming one
            return existing;

        case ConflictStrategy::OVERWRITE:
            // Replace stored entity with the incoming one
            existing = entity;
            return entity;

        case ConflictStrategy::MERGE: {
            json merged = mergeEntities(existing, entity, merge_depth, protected_fields);
            existing = merged;
            return merged;
        }

        case ConflictStrategy::ERROR:
            // Signal error to caller; return existing entity unchanged
            return existing;
    }

    // Should be unreachable, but keep the compiler happy
    return entity;
}

// static
json ImportConflictResolver::mergeEntities(const json& existing,
                                            const json& incoming,
                                            int depth,
                                            const std::vector<std::string>& protected_fields) {
    // If either value is not an object, incoming wins (unless protected)
    if (!existing.is_object() || !incoming.is_object()) {
        return incoming;
    }

    json result = existing;

    for (auto it = incoming.begin(); it != incoming.end(); ++it) {
        const std::string& key = it.key();
        const json& value      = it.value();

        // Skip protected fields
        if (std::find(protected_fields.begin(), protected_fields.end(), key)
                != protected_fields.end()) {
            continue;
        }

        bool can_recurse = (depth == -1 || depth > 1)
                           && result.contains(key)
                           && result[key].is_object()
                           && value.is_object();

        if (can_recurse) {
            int next_depth = (depth == -1) ? -1 : (depth - 1);
            result[key] = mergeEntities(result[key], value, next_depth, {});
        } else {
            // depth == 1 or non-object: incoming wins
            result[key] = value;
        }
    }

    return result;
}

} // namespace importers
} // namespace themis
