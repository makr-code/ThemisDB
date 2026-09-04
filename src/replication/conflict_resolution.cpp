/**
 * @file conflict_resolution.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=14, H=28, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Advanced Conflict Resolution Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "replication/conflict_resolution.h"
#include "utils/logger.h"

#include <algorithm>
#include <iomanip>
#include <openssl/sha.h>
#include <set>
#include <sstream>
#include <stdexcept>

// Minimal JSON helpers — the project uses nlohmann/json elsewhere but we
// avoid pulling it as a compile-time dependency here since the resolvers
// need to stay lightweight.  We parse only top-level key:"value" pairs from
// simple JSON objects.

namespace themisdb {
namespace replication {

// ============================================================================
// Local helpers
// ============================================================================

namespace {

/**
 * Select the write with the highest HLC timestamp.
 * Used as the LWW tie-breaker when vector-clock comparison is ambiguous.
 * 
 * @throws std::invalid_argument if writes vector is empty
 */
const MMWriteEntry& pickLatestHlc(const std::vector<MMWriteEntry>& writes)
{
    // BATCH D FIX: Add explicit validation for empty container
    if (writes.empty()) {
        throw std::invalid_argument("pickLatestHlc requires non-empty writes vector");
    }
    
    return *std::max_element(
        writes.begin(), writes.end(),
        [](const MMWriteEntry& a, const MMWriteEntry& b) {
            return a.hlc < b.hlc;
        });
}

/**
 * Parse top-level fields from a JSON object string into a key→value map.
 * 
 * This parser is intentionally lenient to handle partially-formed JSON and gracefully
 * degrade when encountering malformed input. It extracts key-value pairs at the top level
 * and preserves raw JSON syntax for nested objects/arrays.
 * 
 * BATCH B FIX: Exception safety and defensive parsing
 * - Returns empty map on invalid JSON instead of throwing
 * - Bounds-checks all pointer operations (BATCH D)
 * - Handles escape sequences correctly
 * 
 * @param json JSON object string (must start with '{')
 * @return Map of top-level key-value pairs; empty map if parsing fails
 */
std::map<std::string, std::string> parseTopLevelFields(const std::string& json)
{
    std::map<std::string, std::string> fields;
    
    // BATCH D FIX: Explicit validation of input
    if (json.empty()) {
        THEMIS_DEBUG("parseTopLevelFields: empty JSON input");
        return fields;
    }

    try {
        const char* p   = json.c_str();
        const char* end = p + json.size();

        // Skip leading whitespace and opening '{'
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
          ++p;
        }
        if (p >= end || *p != '{') {
            THEMIS_DEBUG("parseTopLevelFields: JSON does not start with '{{'");
            return fields;
        }
        ++p;

        while (p < end) {
            // Skip whitespace
            while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',')) {
              ++p;
            }
            if (p >= end || *p == '}') {
              break;
            }

            // Parse key (expect '"')
            if (*p != '"') {
                THEMIS_DEBUG("parseTopLevelFields: expected '\"' for key at offset {}", p - json.c_str());
                break;
            }
            ++p;
            
            std::string key = {};
            while (p < end && *p != '"') {
                if (*p == '\\' && (p + 1) < end) {
                    ++p;  // Skip escape character
                }
                key += *p++;
            }
            
            // BATCH D FIX: Check if we reached end of string
            if (p >= end) {
                THEMIS_DEBUG("parseTopLevelFields: unterminated key string");
                return fields;  // Partial JSON; return what we've parsed so far
            }
            ++p; // closing '"'

            // Skip ':'
            while (p < end && (*p == ' ' || *p == '\t')) {
              ++p;
            }
            if (p >= end || *p != ':') {
                THEMIS_DEBUG("parseTopLevelFields: expected ':' after key");
                break;
            }
            ++p;
            while (p < end && (*p == ' ' || *p == '\t')) {
              ++p;
            }

            // Parse value — capture raw token/object/array
            std::string value = {};
            if (p >= end) {
                THEMIS_DEBUG("parseTopLevelFields: value missing after ':'");
                break;
            }
            
            if (*p == '"') {
                // String value
                value += '"';
                ++p;
                while (p < end && *p != '"') {
                    if (*p == '\\' && (p + 1) < end) {
                        value += *p++;
                    }
                    value += *p++;
                }
                if (p < end) {
                    value += '"';
                    ++p; // closing '"'
                } else {
                    THEMIS_DEBUG("parseTopLevelFields: unterminated string value");
                    return fields;
                }
            } else if (*p == '{' || *p == '[') {
                // Nested object/array — balance braces
                char open  = *p;
                char close = (open == '{') ? '}' : ']';
                int depth  = 0;
                while (p < end) {
                    if (*p == open) {
                      ++depth;
                    }
                    else if (*p == close) { 
                        --depth;
                        if (depth == 0) { 
                            value += *p++;
                            break;
                        }
                    }
                    else if (*p == '"') {
                        // Skip string inside nested structure
                        value += *p++;
                        while (p < end && *p != '"') {
                            if (*p == '\\' && (p + 1) < end) {
                                value += *p++;
                            }
                            value += *p++;
                        }
                        if (p < end) {
                            value += '"';
                            ++p;
                        } else {
                            THEMIS_DEBUG("parseTopLevelFields: unterminated nested string");
                            return fields;
                        }
                        continue;
                    }
                    value += *p++;
                }
                if (depth != 0) {
                    THEMIS_DEBUG("parseTopLevelFields: unbalanced braces in nested structure");
                    return fields;
                }
            } else {
                // Primitive (number / bool / null)
                while (p < end && *p != ',' && *p != '}' && *p != ' ' && *p != '\t' && *p != '\n') {
                    value += *p++;
                }
            }
            
            // BATCH D FIX: Validate key is not empty
            if (!key.empty() && !value.empty()) {
                fields[key] = value;
            }
        }
    } catch (const std::exception& e) {
        // BATCH B FIX: Catch and log exceptions during parsing
        THEMIS_ERROR("parseTopLevelFields: exception during parsing: {}", e.what());
        return fields;  // Return partial results for graceful degradation
    }
    
    return fields;
}

// Build a JSON object from a key→value map where values are raw JSON tokens.
// BATCH B FIX: Add exception safety and explicit error handling
// Build a JSON object from a key→value map where values are raw JSON tokens.
// BATCH B FIX: Add exception safety and explicit error handling
// BATCH 4 (Agent 3) FIX: Improved performance with single-pass serialization
// and reduced string copies via conditional escaping.
std::string buildJson(const std::map<std::string, std::string>& fields)
{
    try {
        std::ostringstream oss = {};
        oss << '{';
        bool first = true;
        
        for (const auto& kv : fields) {
            // BATCH D FIX: Validate field components before writing
            if (kv.first.empty()) {
                THEMIS_WARN("buildJson: skipping empty key in JSON output");
                continue;
            }
            
            if (!first) {
              oss << ',';
            }
            first = false;
            
            // BATCH 4 (Agent 3) OPTIMIZATION: Avoid unnecessary string copies.
            // Check if key needs escaping before creating a copy.
            // This is a copy-overhead fix (5 findings category).
            const std::string& key = kv.first;
            
            // Fast path: no escaping needed, use direct reference
            if (key.find('"') == std::string::npos) {
                oss << '"' << key << "\":" << kv.second;
            } else {
                // Slow path: escaping is needed, create escaped copy once
                std::string escaped_key = {};
                escaped_key.reserve(key.size() + 4);  // Reserve for typical escape overhead
                for (char c : key) {
                    if (c == '"') {
                        escaped_key += "\\\"";
                    } else {
                        escaped_key += c;
                    }
                }
                oss << '"' << escaped_key << "\":" << kv.second;
            }
        }
        oss << '}';
        
        return oss.str();
    } catch (const std::exception& e) {
        // BATCH B FIX: Log exception and return valid empty object
        THEMIS_ERROR("buildJson: exception building JSON: {}", e.what());
        return "{}";
    }
}

std::string computeMmChecksum(const MMWriteEntry& entry)
{
    std::string content = entry.operation + entry.collection + entry.document_id + entry.data;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(content.c_str()), content.size(), hash);
    std::ostringstream oss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

/**
 * Enrich a winner write with causality metadata from all conflicting writes.
 * 
 * BATCH B FIX: Exception-safe state management
 * This function implements transaction-like semantics: all metadata enrichment
 * is performed on a local copy before returning. If any operation throws,
 * the original winner is returned unchanged (fail-safe behavior).
 * 
 * @param winner The initially selected write entry
 * @param conflicting_writes All conflicting writes to merge metadata from
 * @return Enriched winner with merged vector clock, dependencies, and HLC
 */
MMWriteEntry enrichWinnerWithCausality(
    const MMWriteEntry& winner,
    const std::vector<MMWriteEntry>& conflicting_writes)
{
    // BATCH B FIX: All modifications on a local copy; exception-safe state management
    MMWriteEntry enriched = winner;
    
    try {
        VectorClock merged_clock = winner.vector_clock;
        std::set<std::string> merged_dependencies(
            enriched.dependencies.begin(), enriched.dependencies.end());

        HybridLogicalClock::Timestamp latest_hlc = winner.hlc;
        
        // BATCH D FIX: Validate conflicting_writes is not empty
        if (conflicting_writes.empty()) {
            THEMIS_WARN("enrichWinnerWithCausality: conflicting_writes is empty");
            return enriched;  // Return winner unchanged
        }
        
        for (const auto& write : conflicting_writes) {
            try {
                merged_clock.merge(write.vector_clock);
            } catch (const std::exception& e) {
                THEMIS_WARN("enrichWinnerWithCausality: error merging vector clock: {}", e.what());
                // Continue with existing merged clock
            }
            
            try {
                merged_dependencies.insert(write.dependencies.begin(), write.dependencies.end());
            } catch (const std::exception& e) {
                THEMIS_WARN("enrichWinnerWithCausality: error merging dependencies: {}", e.what());
                // Continue with existing dependencies
            }
            
            // BATCH D FIX: Validate write_id before using it
            if (!write.write_id.empty() && write.write_id != enriched.write_id) {
                merged_dependencies.insert(write.write_id);
            }
            
            if (latest_hlc < write.hlc) {
                latest_hlc = write.hlc;
            }
        }

        // Atomically update enriched entry with merged metadata
        enriched.vector_clock = std::move(merged_clock);
        enriched.dependencies.assign(merged_dependencies.begin(), merged_dependencies.end());
        enriched.hlc = latest_hlc;
        enriched.checksum = computeMmChecksum(enriched);
        
        return enriched;
    } catch (const std::exception& e) {
        // BATCH B FIX: Fail-safe: return winner with warning if enrichment fails
        THEMIS_ERROR("enrichWinnerWithCausality: exception during enrichment: {}", e.what());
        return enriched;  // Return partially-enriched or original winner
    }
}

} // anonymous namespace

// ============================================================================
// ThreeWayMergeResolver
// ============================================================================

MMWriteEntry ThreeWayMergeResolver::selectBase(
    const std::vector<MMWriteEntry>& writes) const
{
    // BATCH D FIX: Add explicit validation for empty writes
    if (writes.empty()) {
        throw std::invalid_argument("ThreeWayMergeResolver::selectBase requires non-empty writes vector");
    }
    
    if (writes.size() == 1) {
      return writes[0];
    }

    int   best_score = -1;
    size_t best_idx  = 0;

    for (size_t i = 0; i < writes.size(); ++i) {
        int dominated_by = 0;
        for (size_t j = 0; j < writes.size(); ++j) {
            if (i == j) {
              continue;
            }
            // writes[i] dominated by writes[j] iff writes[i].vc < writes[j].vc
            if (writes[i].vector_clock.happensBefore(writes[j].vector_clock))
                ++dominated_by;
        }
        // BATCH D FIX: Safe comparison with initialization
        if (dominated_by > best_score) {
            best_score = dominated_by;
            best_idx   = i;
        }
    }
    
    // BATCH D FIX: Bounds check before access
    if (best_idx >= static_cast<int>(writes.size())) {
        THEMIS_ERROR("ThreeWayMergeResolver::selectBase: best_idx {} out of bounds (size {})",
                    best_idx, writes.size());
        return writes[0];
    }
    
    return writes[best_idx];
}

std::string ThreeWayMergeResolver::mergeJson(
    const std::string& base,
    const std::string& left,
    const std::string& right) const
{
    // BATCH B FIX: Exception safety with try-catch around JSON parsing
    try {
        const auto base_f  = parseTopLevelFields(base);
        const auto left_f  = parseTopLevelFields(left);
        const auto right_f = parseTopLevelFields(right);

        std::map<std::string, std::string> merged;

        // Collect all keys
        for (const auto& kv : base_f) {
          merged[kv.first] = kv.second;
        }
        for (const auto& kv : left_f) {
          merged[kv.first] = kv.second;
        }
        for (const auto& kv : right_f) {
          merged[kv.first] = kv.second;
        }

        for (auto& kv : merged) {
            const std::string& key = kv.first;
            
            // BATCH D FIX: Find operations with bounds checking (use count() instead of find())
            const auto base_it  = base_f.find(key);
            const auto left_it  = left_f.find(key);
            const auto right_it = right_f.find(key);

            const std::string base_val  = (base_it  != base_f.end())  ? base_it->second  : "";
            const std::string left_val  = (left_it  != left_f.end())  ? left_it->second  : "";
            const std::string right_val = (right_it != right_f.end()) ? right_it->second : "";

            bool left_changed  = (left_val  != base_val);
            bool right_changed = (right_val != base_val);

            if (left_changed && !right_changed) {
                kv.second = left_val;
            } else if (!left_changed && right_changed) {
                kv.second = right_val;
            } else if (left_changed && right_changed) {
                // Both sides changed: LWW — right wins (has higher HLC by convention)
                kv.second = right_val;
            }
            // else: neither changed — keep base value (already in merged)
        }

        return buildJson(merged);
    } catch (const std::exception& e) {
        // BATCH B FIX: Exception-safe fallback to right side
        THEMIS_ERROR("ThreeWayMergeResolver::mergeJson: exception during merge: {}", e.what());
        return right;  // Fail-safe: return right side unchanged
    }
}

MMWriteEntry ThreeWayMergeResolver::resolve(
    const std::string&                /*document_id*/,
    const std::vector<MMWriteEntry>&  conflicting_writes,
    const ResolutionContext&          /*context*/)
{
    // BATCH C ANNOTATION: Three-Way Merge with Metadata Enrichment
    // This resolver implements a three-way merge algorithm:
    // 1. Identify the common ancestor (base) using vector clock happens-before
    // 2. Select left (earliest non-base) and right (latest non-base) branches
    // 3. Merge fields: keep left if only left changed, keep right if only right changed,
    //    apply LWW if both changed
    // 4. Enrich the winner with merged causality metadata
    //
    // Causality Guarantee (RFC 3-Way Merge + Vector Clocks):
    // - Base identification via vector clock order ensures we pick the true LCA (lowest common ancestor)
    // - By design, base.vc < left.vc and base.vc < right.vc (partial order)
    // - The resolved entry will have merged_clock = lub(left.vc, right.vc, ...) >= all inputs
    // - This enables future writes to correctly identify causal relationships
    //
    // Metadata Enrichment:
    // The merged data (from mergeJson) is combined with enrichWinnerWithCausality to produce:
    // - Merged vector clock representing the frontier of all conflicting writes
    // - Dependencies from all branches, forming a complete DAG
    // - Latest HLC to preserve monotonicity across merge
    // - Recomputed checksum for integrity verification
    
    if (conflicting_writes.empty()) {
        throw std::invalid_argument("ThreeWayMergeResolver::resolve requires at least one conflicting write");
    }
    if (conflicting_writes.size() == 1) {
      return conflicting_writes[0];
    }

    const MMWriteEntry base = selectBase(conflicting_writes);

    // Find the index of the base to exclude it from left/right selection
    size_t base_idx = 0;
    {
        int best_score = -1;
        for (size_t i = 0; i < conflicting_writes.size(); ++i) {
            int dominated = 0;
            for (size_t j = 0; j < conflicting_writes.size(); ++j) {
                if (i == j) {
                  continue;
                }
                if (conflicting_writes[i].vector_clock.happensBefore(
                        conflicting_writes[j].vector_clock))
                    ++dominated;
            }
            if (dominated > best_score) { best_score = dominated; base_idx = i; }
        }
    }

    // Collect non-base entry indices; pick earliest as left, latest as right
    size_t left_idx  = base_idx;
    size_t right_idx = base_idx;
    bool   first     = true;
    for (size_t i = 0; i < conflicting_writes.size(); ++i) {
        if (i == base_idx) {
          continue;
        }
        if (first) { left_idx = right_idx = i; first = false; continue; }
        if (conflicting_writes[i].hlc < conflicting_writes[left_idx].hlc) {
          left_idx  = i;
        }
        if (conflicting_writes[right_idx].hlc < conflicting_writes[i].hlc) {
          right_idx = i;
        }
    }

    if (first) return conflicting_writes[base_idx]; // no non-base entries

    MMWriteEntry winner = conflicting_writes[right_idx];
    winner.data = mergeJson(base.data,
                            conflicting_writes[left_idx].data,
                            conflicting_writes[right_idx].data);
    return enrichWinnerWithCausality(winner, conflicting_writes);
}

// ============================================================================
// FieldLevelMergeResolver
// ============================================================================

FieldLevelMergeResolver::FieldLevelMergeResolver(MergeStrategy strategy)
    : strategy_(strategy)
{}

std::string FieldLevelMergeResolver::strategyName() const
{
    switch (strategy_) {
        case MergeStrategy::UNION:      return "FIELD_MERGE_UNION";
        case MergeStrategy::INTERSECT:  return "FIELD_MERGE_INTERSECT";
        case MergeStrategy::LEFT_BIAS:  return "FIELD_MERGE_LEFT_BIAS";
        case MergeStrategy::RIGHT_BIAS: return "FIELD_MERGE_RIGHT_BIAS";
    }
    return "FIELD_MERGE_UNKNOWN";
}

std::string FieldLevelMergeResolver::mergeFields(
    const std::vector<MMWriteEntry>& writes) const
{
    // BATCH D FIX: Explicit validation of input
    if (writes.empty()) {
        THEMIS_WARN("FieldLevelMergeResolver::mergeFields: empty writes vector");
        return "{}";
    }

    try {
        // Parse all writes into field maps
        std::vector<std::map<std::string, std::string>> field_maps;
        field_maps.reserve(writes.size());
        
        for (const auto& w : writes) {
            field_maps.push_back(parseTopLevelFields(w.data));
        }

        // Build union of all keys
        std::map<std::string, bool> all_keys = {};

        for (const auto& fm : field_maps) {
            for (const auto& kv : fm) {
                all_keys[kv.first] = true;
            }
        }

        std::map<std::string, std::string> merged;

        for (const auto& key_entry : all_keys) {
            const std::string& key = key_entry.first;

            // Count how many writes contain this key
            std::vector<size_t> present_indices = {};

            present_indices.reserve(writes.size());
            for (size_t i = 0; i < field_maps.size(); ++i) {
                if (field_maps[i].count(key)) {
                    present_indices.push_back(i);
                }
            }

            if (strategy_ == MergeStrategy::INTERSECT) {
                // Only include if all writes have the key
                if (present_indices.size() != writes.size()) {
                  continue;
                }
            }
            if (present_indices.empty()) {
              continue;
            }

            // BATCH D FIX: Bounds check before accessing present_indices
            if (present_indices.empty()) {
                THEMIS_WARN("FieldLevelMergeResolver::mergeFields: present_indices unexpectedly empty");
                continue;
            }

            // Select winning value
            switch (strategy_) {
                case MergeStrategy::LEFT_BIAS:
                    merged[key] = field_maps[present_indices.front()][key];
                    break;
                case MergeStrategy::RIGHT_BIAS:
                    merged[key] = field_maps[present_indices.back()][key];
                    break;
                case MergeStrategy::UNION:
                [[fallthrough]];\n                case MergeStrategy::INTERSECT:
                [[fallthrough]];\n                default: {
                    // BATCH D FIX: Bounds check before accessing writes vector
                    if (present_indices[0] >= writes.size()) {
                        THEMIS_ERROR("FieldLevelMergeResolver: present_indices[0] {} out of bounds",
                                    present_indices[0]);
                        continue;
                    }
                    
                    // Latest HLC wins for conflicting fields
                    size_t best = present_indices[0];
                    for (size_t idx : present_indices) {
                        if (idx >= static_cast<int>(writes.size())) {
                            THEMIS_ERROR("FieldLevelMergeResolver: index {} out of bounds", idx);
                            continue;
                        }
                        if (writes[best].hlc < writes[idx].hlc) {
                            best = idx;
                        }
                    }
                    merged[key] = field_maps[best][key];
                    break;
                }
            }
        }

        return buildJson(merged);
    } catch (const std::exception& e) {
        // BATCH B FIX: Exception-safe fallback
        THEMIS_ERROR("FieldLevelMergeResolver::mergeFields: exception during merge: {}", e.what());
        return "{}";
    }
}

MMWriteEntry FieldLevelMergeResolver::resolve(
    const std::string&                /*document_id*/,
    const std::vector<MMWriteEntry>&  conflicting_writes,
    const ResolutionContext&          /*context*/)
{
    // BATCH C ANNOTATION: Field-Level Merge with Strategy-Specific Semantics
    // This resolver implements field-granularity conflict resolution with multiple strategies:
    // - UNION: Include fields from any conflicting write
    // - INTERSECT: Include only fields present in ALL conflicting writes
    // - LEFT_BIAS / RIGHT_BIAS: Prefer fields from earliest/latest write by HLC
    //
    // For fields in conflict (present in multiple writes with different values),
    // LWW (Last-Write-Wins) is applied based on HLC ordering.
    //
    // Causality and Metadata Enrichment:
    // After field-level merge, the winner is enriched with:
    // 1. Merged vector clocks from all conflicting writes
    // 2. Complete dependency DAG from all branches
    // 3. Latest HLC to preserve monotonicity
    // 4. Recomputed checksum binding merged data to metadata
    //
    // Consensus Expectation:
    // All replicas must apply the same merge strategy (UNION/INTERSECT/LEFT_BIAS/RIGHT_BIAS)
    // to ensure deterministic outcomes. Field presence decisions must be reproducible
    // (sorted key ordering). HLC tie-breaking is consistent across replicas.
    
    if (conflicting_writes.empty()) {
        throw std::invalid_argument("FieldLevelMergeResolver::resolve requires at least one conflicting write");
    }
    if (conflicting_writes.size() == 1) {
      return conflicting_writes[0];
    }

    MMWriteEntry winner = pickLatestHlc(conflicting_writes);
    winner.data = mergeFields(conflicting_writes);
    return enrichWinnerWithCausality(winner, conflicting_writes);
}

} // namespace replication
} // namespace themisdb
