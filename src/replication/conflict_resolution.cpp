/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            conflict_resolution.cpp                            ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:35:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     388                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 79f0815052  2026-03-28  Add test statistics documentation and collection script ║
    • f7a7b43c19  2026-03-09  fix(replication): fix inverted HLC comparison and remove ... ║
    • 3ed3b012d6  2026-03-09  feat(replication): implement new module features - observ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Advanced Conflict Resolution Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "replication/conflict_resolution.h"

#include <algorithm>
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
 */
const MMWriteEntry& pickLatestHlc(const std::vector<MMWriteEntry>& writes)
{
    return *std::max_element(
        writes.begin(), writes.end(),
        [](const MMWriteEntry& a, const MMWriteEntry& b) {
            return a.hlc < b.hlc;
        });
}

/**
 * Select the write with the lowest HLC timestamp (earliest).
 */
const MMWriteEntry& pickEarliestHlc(const std::vector<MMWriteEntry>& writes)
{
    return *std::min_element(
        writes.begin(), writes.end(),
        [](const MMWriteEntry& a, const MMWriteEntry& b) {
            return a.hlc < b.hlc;
        });
}

/**
 * Parse top-level fields from a JSON object string into a key→value map.
 */
std::map<std::string, std::string> parseTopLevelFields(const std::string& json)
{
    std::map<std::string, std::string> fields;
    if (json.empty()) return fields;

    const char* p   = json.c_str();
    const char* end = p + json.size();

    // Skip leading whitespace and opening '{'
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p;
    if (p >= end || *p != '{') return fields;
    ++p;

    while (p < end) {
        // Skip whitespace
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',')) ++p;
        if (p >= end || *p == '}') break;

        // Parse key (expect '"')
        if (*p != '"') break;
        ++p;
        std::string key;
        while (p < end && *p != '"') {
            if (*p == '\\' && (p + 1) < end) { ++p; } // skip escape
            key += *p++;
        }
        if (p >= end) break;
        ++p; // closing '"'

        // Skip ':'
        while (p < end && (*p == ' ' || *p == '\t')) ++p;
        if (p >= end || *p != ':') break;
        ++p;
        while (p < end && (*p == ' ' || *p == '\t')) ++p;

        // Parse value — capture raw token/object/array
        std::string value;
        if (p < end && *p == '"') {
            // String value
            value += '"';
            ++p;
            while (p < end && *p != '"') {
                if (*p == '\\' && (p + 1) < end) { value += *p++; }
                value += *p++;
            }
            value += '"';
            if (p < end) ++p; // closing '"'
        } else if (p < end && (*p == '{' || *p == '[')) {
            // Nested object/array — balance braces
            char open  = *p;
            char close = (open == '{') ? '}' : ']';
            int depth  = 0;
            while (p < end) {
                if (*p == open)  ++depth;
                else if (*p == close) { --depth; if (depth == 0) { value += *p++; break; } }
                else if (*p == '"') {
                    // skip string inside nested
                    value += *p++;
                    while (p < end && *p != '"') {
                        if (*p == '\\' && (p + 1) < end) { value += *p++; }
                        value += *p++;
                    }
                    value += '"';
                    if (p < end) ++p;
                    continue;
                }
                value += *p++;
            }
        } else {
            // Primitive (number / bool / null)
            while (p < end && *p != ',' && *p != '}' && *p != ' ' && *p != '\t' && *p != '\n') {
                value += *p++;
            }
        }
        fields[key] = value;
    }
    return fields;
}

// Build a JSON object from a key→value map where values are raw JSON tokens.
std::string buildJson(const std::map<std::string, std::string>& fields)
{
    std::ostringstream oss;
    oss << '{';
    bool first = true;
    for (const auto& kv : fields) {
        if (!first) oss << ',';
        first = false;
        oss << '"' << kv.first << "\":" << kv.second;
    }
    oss << '}';
    return oss.str();
}

} // anonymous namespace

// ============================================================================
// ThreeWayMergeResolver
// ============================================================================

MMWriteEntry ThreeWayMergeResolver::selectBase(
    const std::vector<MMWriteEntry>& writes) const
{
    // The "base" is the write whose vector clock happens-before all others,
    // i.e. it is dominated by every other write in the set.
    // We identify this by comparing all pairs and keeping the one with the
    // most "dominated by" relationships.
    if (writes.size() == 1) return writes[0];

    int   best_score = -1;
    size_t best_idx  = 0;

    for (size_t i = 0; i < writes.size(); ++i) {
        int dominated_by = 0;
        for (size_t j = 0; j < writes.size(); ++j) {
            if (i == j) continue;
            // writes[i] dominated by writes[j] iff writes[i].vc < writes[j].vc
            if (writes[i].vector_clock.happensBefore(writes[j].vector_clock))
                ++dominated_by;
        }
        if (dominated_by > best_score) {
            best_score = dominated_by;
            best_idx   = i;
        }
    }
    return writes[best_idx];
}

std::string ThreeWayMergeResolver::mergeJson(
    const std::string& base,
    const std::string& left,
    const std::string& right) const
{
    const auto base_f  = parseTopLevelFields(base);
    const auto left_f  = parseTopLevelFields(left);
    const auto right_f = parseTopLevelFields(right);

    std::map<std::string, std::string> merged;

    // Collect all keys
    for (const auto& kv : base_f)  merged[kv.first] = kv.second;
    for (const auto& kv : left_f)  merged[kv.first] = kv.second;
    for (const auto& kv : right_f) merged[kv.first] = kv.second;

    for (auto& kv : merged) {
        const std::string& key = kv.first;
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
}

MMWriteEntry ThreeWayMergeResolver::resolve(
    const std::string&                /*document_id*/,
    const std::vector<MMWriteEntry>&  conflicting_writes,
    const ResolutionContext&          /*context*/)
{
    if (conflicting_writes.size() == 1) return conflicting_writes[0];

    const MMWriteEntry base = selectBase(conflicting_writes);

    // Find the index of the base to exclude it from left/right selection
    size_t base_idx = 0;
    {
        int best_score = -1;
        for (size_t i = 0; i < conflicting_writes.size(); ++i) {
            int dominated = 0;
            for (size_t j = 0; j < conflicting_writes.size(); ++j) {
                if (i == j) continue;
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
        if (i == base_idx) continue;
        if (first) { left_idx = right_idx = i; first = false; continue; }
        if (conflicting_writes[i].hlc < conflicting_writes[left_idx].hlc)  left_idx  = i;
        if (conflicting_writes[right_idx].hlc < conflicting_writes[i].hlc) right_idx = i;
    }

    if (first) return conflicting_writes[base_idx]; // no non-base entries

    MMWriteEntry winner = conflicting_writes[right_idx];
    winner.data = mergeJson(base.data,
                            conflicting_writes[left_idx].data,
                            conflicting_writes[right_idx].data);
    return winner;
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
    if (writes.empty()) return "{}";

    // Parse all writes into field maps
    std::vector<std::map<std::string, std::string>> field_maps;
    field_maps.reserve(writes.size());
    for (const auto& w : writes) {
        field_maps.push_back(parseTopLevelFields(w.data));
    }

    // Build union of all keys
    std::map<std::string, bool> all_keys;
    for (const auto& fm : field_maps) {
        for (const auto& kv : fm) all_keys[kv.first] = true;
    }

    std::map<std::string, std::string> merged;

    for (const auto& key_entry : all_keys) {
        const std::string& key = key_entry.first;

        // Count how many writes contain this key
        std::vector<size_t> present_indices;
        for (size_t i = 0; i < field_maps.size(); ++i) {
            if (field_maps[i].count(key)) present_indices.push_back(i);
        }

        if (strategy_ == MergeStrategy::INTERSECT) {
            // Only include if all writes have the key
            if (present_indices.size() != writes.size()) continue;
        }
        if (present_indices.empty()) continue;

        // Select winning value
        switch (strategy_) {
            case MergeStrategy::LEFT_BIAS:
                merged[key] = field_maps[present_indices.front()][key];
                break;
            case MergeStrategy::RIGHT_BIAS:
                merged[key] = field_maps[present_indices.back()][key];
                break;
            case MergeStrategy::UNION:
            case MergeStrategy::INTERSECT:
            default: {
                // Latest HLC wins for conflicting fields
                size_t best = present_indices[0];
                for (size_t idx : present_indices) {
                    if (writes[best].hlc < writes[idx].hlc) best = idx;
                }
                merged[key] = field_maps[best][key];
                break;
            }
        }
    }

    return buildJson(merged);
}

MMWriteEntry FieldLevelMergeResolver::resolve(
    const std::string&                /*document_id*/,
    const std::vector<MMWriteEntry>&  conflicting_writes,
    const ResolutionContext&          /*context*/)
{
    if (conflicting_writes.size() == 1) return conflicting_writes[0];

    MMWriteEntry winner = pickLatestHlc(conflicting_writes);
    winner.data = mergeFields(conflicting_writes);
    return winner;
}

} // namespace replication
} // namespace themisdb
