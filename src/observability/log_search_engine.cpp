/*
 * ThemisDB - Hybrid Database System
 * File:    log_search_engine.cpp
 * Version: 0.0.1
 * Status:  Production Ready
 */

#include "observability/log_search_engine.h"

#include <algorithm>
#include <set>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// LogFieldFilter::matches
// ---------------------------------------------------------------------------

bool LogFieldFilter::matches(const LogEntry& entry) const {
    auto it = entry.fields.find(key);
    if (it == entry.fields.end()) {
        // Key not present -- NOT_EQUALS is still "true" (absent != anything).
        return op == FieldMatchOp::NOT_EQUALS;
    }
    const std::string& actual = it->second;
    switch (op) {
        case FieldMatchOp::EQUALS:
            return actual == value;
        case FieldMatchOp::NOT_EQUALS:
            return actual != value;
        case FieldMatchOp::CONTAINS:
            return actual.find(value) != std::string::npos;
        case FieldMatchOp::STARTS_WITH:
            return actual.size() >= value.size() &&
                   actual.compare(0, value.size(), value) == 0;
    }
    return false;
}

// ---------------------------------------------------------------------------
// LogSearchEngine -- internal helpers
// ---------------------------------------------------------------------------

bool LogSearchEngine::matchesQuery(const LogEntry& entry,
                                    const LogSearchQuery& query) const
{
    // Level filter
    if (static_cast<int>(entry.level) < static_cast<int>(query.min_level)) {
        return false;
    }

    // Time range filters
    if (query.has_from_time && entry.timestamp < query.from_time) {
        return false;
    }
    if (query.has_to_time && entry.timestamp >= query.to_time) {
        return false;
    }

    // Message substring search
    if (!query.message_contains.empty()) {
        if (entry.message.find(query.message_contains) == std::string::npos) {
            return false;
        }
    }

    // Field filters (AND semantics)
    for (const auto& filter : query.field_filters) {
        if (!filter.matches(entry)) {
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// LogSearchEngine::search
// ---------------------------------------------------------------------------

LogSearchResult LogSearchEngine::search(const std::vector<LogEntry>& entries,
                                         const LogSearchQuery& query) const
{
    // Collect all matching entries into a working vector.
    std::vector<const LogEntry*> matched;
    matched.reserve(entries.size());

    for (const auto& entry : entries) {
        if (matchesQuery(entry, query)) {
            matched.push_back(&entry);
        }
    }

    const size_t total = matched.size();

    // Sort by timestamp.
    auto cmp_asc  = [](const LogEntry* a, const LogEntry* b) {
        return a->timestamp < b->timestamp;
    };
    auto cmp_desc = [](const LogEntry* a, const LogEntry* b) {
        return a->timestamp > b->timestamp;
    };

    if (query.ascending) {
        std::stable_sort(matched.begin(), matched.end(), cmp_asc);
    } else {
        std::stable_sort(matched.begin(), matched.end(), cmp_desc);
    }

    // Apply offset.
    size_t start = std::min(query.offset, total);

    // Apply limit.
    size_t end = total;
    if (query.limit > 0) {
        end = std::min(start + query.limit, total);
    }

    // Build result.
    LogSearchResult result;
    result.total_matched = total;
    result.offset = query.offset;
    result.limit  = query.limit;
    result.entries.reserve(end - start);

    for (size_t i = start; i < end; ++i) {
        result.entries.push_back(*matched[i]);
    }

    return result;
}

// ---------------------------------------------------------------------------
// LogSearchEngine::count
// ---------------------------------------------------------------------------

size_t LogSearchEngine::count(const std::vector<LogEntry>& entries,
                               const LogSearchQuery& query) const
{
    size_t n = 0;
    for (const auto& entry : entries) {
        if (matchesQuery(entry, query)) {
            ++n;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// LogSearchEngine::distinctFieldValues
// ---------------------------------------------------------------------------

std::vector<std::string> LogSearchEngine::distinctFieldValues(
    const std::vector<LogEntry>& entries,
    const std::string& field_key) const
{
    std::set<std::string> seen;
    for (const auto& entry : entries) {
        auto it = entry.fields.find(field_key);
        if (it != entry.fields.end()) {
            seen.insert(it->second);
        }
    }
    return std::vector<std::string>(seen.begin(), seen.end());
}

} // namespace observability
} // namespace themis
