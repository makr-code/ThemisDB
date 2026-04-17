/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_cold_store.cpp                            ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-17                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal Cold Store Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_cold_store.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace themisdb {
namespace temporal {

// ============================================================================
// Key encoding
// ============================================================================

uint64_t TemporalColdStore::biasedTimestamp(Timestamp t) noexcept {
    // Cast via unsigned to avoid UB on signed overflow.
    // kTimestampBias + int64_min = 1  (kMinTimestamp maps to 1)
    // kTimestampBias + 0         = 2^63
    // kTimestampBias + int64_max = UINT64_MAX
    return static_cast<uint64_t>(t) + kTimestampBias;
}

// static
std::string TemporalColdStore::encodeKey(const std::string& table_name,
                                          const std::string& doc_key,
                                          Timestamp sys_start) {
    char buf[17];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    std::snprintf(buf, sizeof(buf), "%016llx",
                  static_cast<unsigned long long>(biasedTimestamp(sys_start)));

    std::string key;
    key.reserve(table_name.size() + 1 + doc_key.size() + 1 + 16);
    key += table_name;
    key += '\x01';
    key += doc_key;
    key += '\x01';
    key += buf;
    return key;
}

// static
std::string TemporalColdStore::keyPrefix(const std::string& table_name,
                                          const std::string& doc_key) {
    std::string prefix;
    prefix.reserve(table_name.size() + 1 + doc_key.size() + 1);
    prefix += table_name;
    prefix += '\x01';
    prefix += doc_key;
    prefix += '\x01';
    return prefix;
}

// static
std::string TemporalColdStore::tablePrefix(const std::string& table_name) {
    return table_name + '\x01';
}

// static
std::vector<VersionedDocument>
TemporalColdStore::collectPrefix(const std::map<std::string, std::string>& map,
                                  const std::string& prefix) {
    std::vector<VersionedDocument> result;
    auto it = map.lower_bound(prefix);
    while (it != map.end() && it->first.substr(0, prefix.size()) == prefix) {
        try {
            auto j = nlohmann::json::parse(it->second);
            VersionedDocument doc;
            doc.key         = j.at("key").get<std::string>();
            doc.data        = j.at("data");
            doc.sys_time    = TimeRange::fromJson(j.at("sys_time"));
            doc.valid_time  = TimeRange::fromJson(j.at("valid_time"));
            doc.modified_by = j.value("modified_by", std::string{});
            result.push_back(std::move(doc));
        } catch (const nlohmann::json::exception&) {
            // Skip malformed entries — should not occur in normal operation
        }
        ++it;
    }
    return result;
}

// ============================================================================
// Mutation
// ============================================================================

bool TemporalColdStore::store(const std::string& table_name,
                               const VersionedDocument& doc) {
    if (doc.isCurrent()) {
        return false; // Never store open-ended (current) versions
    }

    const std::string ck = encodeKey(table_name, doc.key, doc.sys_time.start);
    const std::string value = doc.toJson().dump();

    std::unique_lock<std::shared_mutex> lk(mutex_);
    const bool is_new = (store_.count(ck) == 0);
    store_[ck] = value;
    if (is_new) {
        ++total_count_;
        ++stats_.total_versions;
    }
    ++stats_.store_calls;
    return true;
}

size_t TemporalColdStore::remove(const std::string& table_name,
                                  const std::string& doc_key) {
    const std::string prefix = keyPrefix(table_name, doc_key);
    std::unique_lock<std::shared_mutex> lk(mutex_);

    size_t removed = 0;
    auto it = store_.lower_bound(prefix);
    while (it != store_.end() && it->first.substr(0, prefix.size()) == prefix) {
        it = store_.erase(it);
        ++removed;
    }
    total_count_ -= removed;
    stats_.total_versions -= removed;
    return removed;
}

size_t TemporalColdStore::removeTable(const std::string& table_name) {
    const std::string prefix = tablePrefix(table_name);
    std::unique_lock<std::shared_mutex> lk(mutex_);

    size_t removed = 0;
    auto it = store_.lower_bound(prefix);
    while (it != store_.end() && it->first.substr(0, prefix.size()) == prefix) {
        it = store_.erase(it);
        ++removed;
    }
    total_count_ -= removed;
    stats_.total_versions -= removed;
    return removed;
}

void TemporalColdStore::clear() {
    std::unique_lock<std::shared_mutex> lk(mutex_);
    store_.clear();
    total_count_ = 0;
    stats_ = ColdStoreStats{};
}

// ============================================================================
// Queries
// ============================================================================

std::optional<VersionedDocument>
TemporalColdStore::getAsOf(const std::string& table_name,
                            const std::string& doc_key,
                            Timestamp as_of) const {
    // Build the search key for as_of + 1 so that upper_bound gives us the
    // first entry with sys_start > as_of; we then step back one to find the
    // candidate (the most-recent version whose sys_start <= as_of).
    const std::string search_key = encodeKey(table_name, doc_key, as_of);
    const std::string prefix     = keyPrefix(table_name, doc_key);

    std::shared_lock<std::shared_mutex> lk(mutex_);

    // upper_bound(search_key) points past all entries with key <= search_key.
    // We need: last entry with key <= search_key whose sys_time contains as_of.
    auto it = store_.upper_bound(search_key);

    // Walk backwards over candidate entries that share the same (table, doc_key)
    // prefix to find the version whose sys_time contains as_of.
    while (it != store_.begin()) {
        --it;
        if (it->first.substr(0, prefix.size()) != prefix) {
            break; // Left the key's version range
        }
        try {
            auto j = nlohmann::json::parse(it->second);
            VersionedDocument doc;
            doc.key         = j.at("key").get<std::string>();
            doc.data        = j.at("data");
            doc.sys_time    = TimeRange::fromJson(j.at("sys_time"));
            doc.valid_time  = TimeRange::fromJson(j.at("valid_time"));
            doc.modified_by = j.value("modified_by", std::string{});

            if (doc.sys_time.contains(as_of)) {
                ++stats_.hit_getAsOf;
                return doc;
            }
            // sys_start <= as_of but sys_end <= as_of: this version was already
            // closed before as_of — step back further.
        } catch (const nlohmann::json::exception&) {
            // Skip malformed — should not occur
        }
    }

    ++stats_.miss_getAsOf;
    return std::nullopt;
}

std::vector<VersionedDocument>
TemporalColdStore::getAll(const std::string& table_name,
                           const std::string& doc_key) const {
    std::shared_lock<std::shared_mutex> lk(mutex_);
    auto result = collectPrefix(store_, keyPrefix(table_name, doc_key));
    stats_.total_getAll_results += result.size();
    return result;
}

std::vector<VersionedDocument>
TemporalColdStore::getRange(const std::string& table_name,
                             const std::string& doc_key,
                             const TimeRange& range) const {
    std::shared_lock<std::shared_mutex> lk(mutex_);
    const std::string prefix = keyPrefix(table_name, doc_key);

    // Start from the first entry whose sys_start could overlap [range.start, range.end).
    // The earliest possible overlap has sys_start < range.end.
    // We start from prefix (sys_start = kMinTimestamp) and stop when sys_start >= range.end.
    std::vector<VersionedDocument> result;
    auto it = store_.lower_bound(prefix);
    while (it != store_.end() && it->first.substr(0, prefix.size()) == prefix) {
        try {
            auto j = nlohmann::json::parse(it->second);
            VersionedDocument doc;
            doc.key         = j.at("key").get<std::string>();
            doc.data        = j.at("data");
            doc.sys_time    = TimeRange::fromJson(j.at("sys_time"));
            doc.valid_time  = TimeRange::fromJson(j.at("valid_time"));
            doc.modified_by = j.value("modified_by", std::string{});

            // Prune: if sys_start >= range.end, no further entry can overlap
            if (doc.sys_time.start >= range.end) break;

            if (doc.sys_time.overlaps(range)) {
                result.push_back(std::move(doc));
            }
        } catch (const nlohmann::json::exception&) {
            // Skip malformed
        }
        ++it;
    }
    return result;
}

// ============================================================================
// Metadata
// ============================================================================

size_t TemporalColdStore::versionCount(const std::string& table_name,
                                        const std::string& doc_key) const {
    std::shared_lock<std::shared_mutex> lk(mutex_);
    const std::string prefix = keyPrefix(table_name, doc_key);
    size_t count = 0;
    auto it = store_.lower_bound(prefix);
    while (it != store_.end() && it->first.substr(0, prefix.size()) == prefix) {
        ++count;
        ++it;
    }
    return count;
}

size_t TemporalColdStore::totalVersionCount() const noexcept {
    return total_count_.load(std::memory_order_relaxed);
}

ColdStoreStats TemporalColdStore::stats() const {
    std::shared_lock<std::shared_mutex> lk(mutex_);
    return stats_;
}

} // namespace temporal
} // namespace themisdb
