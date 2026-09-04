/**
 * @file conflict_resolver.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/conflict_resolver.h"

#include <algorithm>
#include <set>
#include <sstream>

namespace themis {
namespace importers {

// ============================================================================
// ImportConflictResolver
// ============================================================================

void ImportConflictResolver::reset() {
    registry_.clear();
}

std::string ImportConflictResolver::computeKey(const json &entity, const std::vector<std::string> &key_columns) {
    if (key_columns.empty()) {
        return {};
    }

    // Separator unlikely to appear in field values
    static constexpr char kSep = '\x1F'; // ASCII unit separator

    std::string key = {};
    for (const auto &col : key_columns) {
        if (!key.empty()) {
            key += kSep;
        }
        if (entity.contains(col)) {
            const auto &v = entity[col];
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

json ImportConflictResolver::resolve(const json &entity, const std::string &table_name, const std::string &conflict_key,
                                     ConflictStrategy strategy, int merge_depth,
                                     const std::vector<std::string> &protected_fields, bool &conflict_detected) {
    conflict_detected = false;

    // Lookup the per-table registry entry
    auto &table_registry = registry_[table_name];
    auto it              = table_registry.find(conflict_key);

    if (it == table_registry.end()) {
        // First time we see this key – no conflict
        table_registry.emplace(conflict_key, entity);
        return entity;
    }

    // ---- Conflict detected ----
    conflict_detected = true;
    json &existing    = it->second;

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
            existing    = merged;
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
json ImportConflictResolver::mergeEntities(const json &existing, const json &incoming, int depth,
                                           const std::vector<std::string> &protected_fields) {
    // If either value is not an object, incoming wins (unless protected)
    if (!existing.is_object() || !incoming.is_object()) {
        return incoming;
    }

    json result = existing;

    for (auto it = incoming.begin(); it != incoming.end(); ++it) {
        const std::string &key = it.key();
        const json &value      = it.value();

        // Skip protected fields
        if (std::find(protected_fields.begin(), protected_fields.end(), key) != protected_fields.end()) {
            continue;
        }

        bool can_recurse
            = (depth == -1 || depth > 1) && result.contains(key) && result[key].is_object() && value.is_object();

        if (can_recurse) {
            int next_depth = (depth == -1) ? -1 : (depth - 1);
            result[key]    = mergeEntities(result[key], value, next_depth, {});
        } else {
            // depth == 1 or non-object: incoming wins
            result[key] = value;
        }
    }

    return result;
}

// ============================================================================
// Phase 2 T2.3.1 – Conflict Determinism & Reason Tracking
// ============================================================================

ConflictReasonType ImportConflictResolver::determineConflictReason(
    const json& existing,
    const json& incoming,
    std::vector<std::string>& affected_fields) {
    // PHASE-2-HARDENING: Conflict Reason Determination
    // Determinism: yes (enum-based classification)
    // Audit: reason type for audit trail
    // Bounded: classification ≤ 1ms

    affected_fields.clear();

    // Collect all keys from both entities
    std::set<std::string> all_keys = {};

    if (existing.is_object()) {
        for (const auto& item : existing.items()) {
            all_keys.insert(item.key());
        }
    }
    if (incoming.is_object()) {
        for (const auto& item : incoming.items()) {
            all_keys.insert(item.key());
        }
    }

    // Find differing fields
    for (const auto& key : all_keys) {
        bool existing_has = existing.contains(key);
        bool incoming_has = incoming.contains(key);

        if (existing_has && incoming_has) {
            if (existing[key] != incoming[key]) {
                affected_fields.push_back(key);
            }
        } else if (existing_has != incoming_has) {
            affected_fields.push_back(key);
        }
    }

    // Classification logic (deterministic)
    if (affected_fields.empty()) {
        // Identical entities → Primary key collision (duplicate)
        return ConflictReasonType::PRIMARY_KEY_COLLISION;
    }

    // Check if timestamp field is the only difference → timestamp conflict
    if (affected_fields.size() == 1 && affected_fields[0] == "timestamp") {
        return ConflictReasonType::TIMESTAMP_CONFLICT;
    }

    // Check for constraint indicators in the entities
    // (This is a simplified heuristic; real implementation would have more detail)
    if (existing.contains("_id") && incoming.contains("_id")) {
        const auto& ex_id = existing["_id"];
        const auto& inc_id = incoming["_id"];
        if (ex_id != inc_id && !affected_fields.empty()) {
            return ConflictReasonType::CONSTRAINT_VIOLATION;
        }
    }

    // Multiple field differences → merge conflict
    if (static_cast<int>(affected_fields.size()) > 1) {
        return ConflictReasonType::MERGE_CONFLICT;
    }

    return ConflictReasonType::UNKNOWN;
}

json ImportConflictResolver::resolveWithMetadata(
    const json& entity,
    const std::string& table_name,
    const std::string& conflict_key,
    ConflictStrategy strategy,
    int merge_depth,
    const std::vector<std::string>& protected_fields,
    bool& conflict_detected,
    ConflictMetadata& metadata) {
    // PHASE-2-HARDENING: Conflict Determinism & Reason Tracking
    // Determinism: yes (CRDT LWW with row_id tiebreaker)
    // Audit: returns full ConflictMetadata for audit trail
    // Bounded: resolution ≤ 100ms

    conflict_detected = false;
    metadata.timestamp_used = 0;

    // Lookup the per-table registry entry
    auto& table_registry = registry_[table_name];
    auto it = table_registry.find(conflict_key);

    if (it == table_registry.end()) {
        // First time we see this key – no conflict
        table_registry.emplace(conflict_key, entity);
        metadata.reason = ConflictReasonType::UNKNOWN;
        metadata.resolution_strategy = "NONE";
        metadata.affected_fields.clear();
        return entity;
    }

    // ---- Conflict detected ----
    conflict_detected = true;
    json& existing = it->second;

    // Determine reason and affected fields
    metadata.reason = determineConflictReason(existing, entity, metadata.affected_fields);

    json resolved_entity;

    switch (strategy) {
        case ConflictStrategy::SKIP:
            metadata.resolution_strategy = "SKIP";
            resolved_entity = existing;
            break;

        case ConflictStrategy::OVERWRITE:
            metadata.resolution_strategy = "OVERWRITE";
            resolved_entity = entity;
            existing = entity;
            break;

        case ConflictStrategy::MERGE: {
            metadata.resolution_strategy = "MERGE";
            json merged = mergeEntities(existing, entity, merge_depth, protected_fields);
            existing = merged;
            resolved_entity = merged;
            break;
        }

        case ConflictStrategy::ERROR:
            metadata.resolution_strategy = "ERROR";
            resolved_entity = existing;
            break;

        default:
            // CRDT Last-Write-Wins fallback (deterministic with row_id tiebreaker)
            metadata.resolution_strategy = "CRDT_LWW";
            {
                // Extract timestamp if available (for audit trail)
                uint64_t existing_ts = 0;
                uint64_t incoming_ts = 0;

                if (existing.contains("timestamp") && existing["timestamp"].is_number()) {
                    existing_ts = existing["timestamp"].get<uint64_t>();
                }
                if (entity.contains("timestamp") && entity["timestamp"].is_number()) {
                    incoming_ts = entity["timestamp"].get<uint64_t>();
                }

                // Deterministic tiebreaker: use row_id if timestamps are equal
                bool use_incoming = incoming_ts > existing_ts;
                if (incoming_ts == existing_ts && entity.contains("row_id") && existing.contains("row_id")) {
                    std::string incoming_rid = entity["row_id"].dump();
                    std::string existing_rid = existing["row_id"].dump();
                    use_incoming = incoming_rid > existing_rid;
                }

                metadata.timestamp_used = std::max(existing_ts, incoming_ts);
                resolved_entity = use_incoming ? entity : existing;
                if (use_incoming) {
                    existing = entity;
                }
            }
            break;
    }

    return resolved_entity;
}

} // namespace importers
} // namespace themis
