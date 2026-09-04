/**
 * @file temporal_cold_store.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Cold Store Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_cold_store.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace themisdb {
namespace temporal {

// ============================================================================
// InMemoryBackend
// ============================================================================

bool InMemoryBackend::put(const std::string& key, const std::string& value) {
    std::unique_lock lk(mutex_);
    data_[key] = value;
    return true;
}

std::string InMemoryBackend::get(const std::string& key) const {
    std::shared_lock lk(mutex_);
    auto it = data_.find(key);
    return (it != data_.end()) ? it->second : std::string{};
}

bool InMemoryBackend::del(const std::string& key) {
    std::unique_lock lk(mutex_);
    return data_.erase(key) > 0;
}

std::vector<std::string>
InMemoryBackend::listKeysWithPrefix(const std::string& prefix) const {
    std::shared_lock lk(mutex_);
    std::vector<std::string> result = {};

    for (auto it = data_.lower_bound(prefix); it != data_.end(); ++it) {
        if (it->first.substr(0,static_cast<int>(prefix.size())) != prefix) {
          break;
        }
        result.push_back(it->first);
    }
    return result;
}

size_t InMemoryBackend::deletePrefix(const std::string& prefix) {
    std::unique_lock lk(mutex_);
    size_t count = 0;
    auto it = data_.lower_bound(prefix);
    while (it != data_.end() && it->first.substr(0,static_cast<int>(prefix.size())) == prefix) {
        it = data_.erase(it);
        ++count;
    }
    return count;
}

void InMemoryBackend::clearAll() {
    std::unique_lock lk(mutex_);
    data_.clear();
}

// ============================================================================
// FileSystemBackend — helpers
// ============================================================================

// Percent-encode a string so every byte is safe as a filesystem path component.
// Encodes everything except unreserved URI characters (A-Z a-z 0-9 - _ . ~).
std::string FileSystemBackend::percentEncode(const std::string& s) {
    static const char hex[] = "0123456789abcdef";
    std::string out = {};
    out.reserve(s.size() * 3);
    for (unsigned char c : s) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            out += static_cast<char>(c);
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 0xF];
        }
    }
    return out;
}

std::string FileSystemBackend::percentDecode(const std::string& s) {
    std::string out = {};
    out.reserve(s.size());
    for (size_t i = 0; i <static_cast<int>(s.size()); ) {
        if (s[i] == '%' && i + 2 <static_cast<int>(s.size())) {
            auto hexVal = [](char c) -> int {
                if (c >= '0' && c <= '9') {
                  return c - '0';
                }
                if (c >= 'a' && c <= 'f') {
                  return c - 'a' + 10;
                }
                if (c >= 'A' && c <= 'F') {
                  return c - 'A' + 10;
                }
                return -1;
            };
            int hi = hexVal(s[i+1]);
            int lo = hexVal(s[i+2]);
            if (hi >= 0 && lo >= 0) {
                out += static_cast<char>((hi << 4) | lo);
                i += 3;
                continue;
            }
        }
        out += s[i++];
    }
    return out;
}

// Composite key format:  table '\x01' doc_key '\x01' 016x_ts
// File layout:  base_dir / pct(table) / pct(doc_key) / 016x_ts.json
fs::path FileSystemBackend::keyToPath(const std::string& composite_key) const {
    // Split on '\x01' separators
    const auto sep1 = composite_key.find('\x01');
    if (sep1 == std::string::npos)
        throw std::invalid_argument("malformed composite key (no sep1)");
    const auto sep2 = composite_key.find('\x01', sep1 + 1);
    if (sep2 == std::string::npos)
        throw std::invalid_argument("malformed composite key (no sep2)");

    const std::string table   = composite_key.substr(0, sep1);
    const std::string doc_key = composite_key.substr(sep1 + 1, sep2 - sep1 - 1);
    const std::string ts_hex  = composite_key.substr(sep2 + 1); // 016x

    return base_dir_ / percentEncode(table) / percentEncode(doc_key)
                     / (ts_hex + ".json");
}

std::string
FileSystemBackend::pathToKey(const fs::path& full_path) const {
    // Reconstruct: base_dir / pct(table) / pct(doc_key) / ts_hex.json
    const fs::path rel = fs::relative(full_path, base_dir_);
    auto it = rel.begin();
    if (it == rel.end()) {
      throw std::runtime_error("empty relative path");
    }
    const std::string table = percentDecode(it->string()); ++it;
    if (it == rel.end()) {
      throw std::runtime_error("missing doc_key in path");
    }
    const std::string doc_key = percentDecode(it->string()); ++it;
    if (it == rel.end()) {
      throw std::runtime_error("missing ts file in path");
    }
    std::string ts_hex = it->stem().string(); // strip .json

    return table + '\x01' + doc_key + '\x01' + ts_hex;
}

// ============================================================================
// FileSystemBackend — public
// ============================================================================

FileSystemBackend::FileSystemBackend(fs::path base_dir)
    : base_dir_(std::move(base_dir)) {
    fs::create_directories(base_dir_);
}

bool FileSystemBackend::put(const std::string& key, const std::string& value) {
    std::unique_lock lk(mutex_);
    try {
        const fs::path target = keyToPath(key);
        fs::create_directories(target.parent_path());

        // Atomic write: write to .tmp then rename
        const fs::path tmp = fs::path(target.string() + ".tmp");
        {
            std::ofstream ofs(tmp, std::ios::out | std::ios::trunc);
            if (!ofs) {
              return false;
            }
            ofs << value;
            if (!ofs) {
                fs::remove(tmp);
                return false;
            }
        }
        fs::rename(tmp, target);
        return true;
    } catch (...) {
        return false;
    }
}

std::string FileSystemBackend::get(const std::string& key) const {
    std::shared_lock lk(mutex_);
    try {
        const fs::path target = keyToPath(key);
        std::ifstream ifs(target);
        if (!ifs) return {};
        std::ostringstream ss = {};
        ss << ifs.rdbuf();
        return ss.str();
    } catch (...) {
        return {};
    }
}

bool FileSystemBackend::del(const std::string& key) {
    std::unique_lock lk(mutex_);
    try {
        const fs::path target = keyToPath(key);
        return fs::remove(target);
    } catch (...) {
        return false;
    }
}

std::vector<std::string>
FileSystemBackend::listKeysWithPrefix(const std::string& prefix) const {
    // Reconstruct the directory path the prefix maps to.
    // prefix format: table'\x01'doc_key'\x01'  (with trailing sep → per-key list)
    //                table'\x01'               (with one sep → per-table list)
    std::shared_lock lk(mutex_);
    std::vector<std::string> result;
    try {
        for (auto& entry : fs::recursive_directory_iterator(base_dir_)) {
            if (!entry.is_regular_file()) {
              continue;
            }
            const std::string ext = entry.path().extension().string();
            if (ext != ".json") {
              continue;
            }
            const std::string ck = pathToKey(entry.path());
            if (ck.substr(0,static_cast<int>(prefix.size())) == prefix)
                result.push_back(ck);
        }
    } catch (const fs::filesystem_error&) {
        // base_dir_ does not exist yet or is not accessible
    }
    // Sort to match InMemoryBackend ordering
    std::sort(result.begin(), result.end());
    return result;
}

size_t FileSystemBackend::deletePrefix(const std::string& prefix) {
    std::unique_lock lk(mutex_);
    size_t count = 0;
    try {
        std::vector<fs::path> to_delete = {};

        for (auto& entry : fs::recursive_directory_iterator(base_dir_)) {
            if (!entry.is_regular_file()) {
              continue;
            }
            if (entry.path().extension() != ".json") {
              continue;
            }
            const std::string ck = pathToKey(entry.path());
            if (ck.substr(0,static_cast<int>(prefix.size())) == prefix)
                to_delete.push_back(entry.path());
        }
        for (const auto& p : to_delete) {
            if (fs::remove(p)) {
              ++count;
            }
        }
    } catch (const fs::filesystem_error&) {}
    return count;
}

void FileSystemBackend::clearAll() {
    std::unique_lock lk(mutex_);
    try {
        for (auto& entry : fs::directory_iterator(base_dir_)) {
            fs::remove_all(entry.path());
        }
    } catch (const fs::filesystem_error&) {}
}

// ============================================================================
// TemporalColdStore — key encoding
// ============================================================================

uint64_t TemporalColdStore::biasedTimestamp(Timestamp t) noexcept {
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
    std::string key = {};
    key.reserve(static_cast<int>(table_name.size()) + 1 + static_cast<int>(doc_key.size()) + 1 + 16);
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
    std::string p = {};
    p.reserve(static_cast<int>(table_name.size()) + 1 + static_cast<int>(doc_key.size()) + 1);
    p += table_name;
    p += '\x01';
    p += doc_key;
    p += '\x01';
    return p;
}

// static
std::string TemporalColdStore::tablePrefix(const std::string& table_name) {
    return table_name + '\x01';
}

// ============================================================================
// TemporalColdStore — document parsing
// ============================================================================

// static
std::optional<VersionedDocument>
TemporalColdStore::parseDocument(const std::string& json_str) {
    if (json_str.empty()) {
      return std::nullopt;
    }
    try {
        auto j = nlohmann::json::parse(json_str);
        VersionedDocument doc;
        doc.key         = j.at("key").get<std::string>();
        doc.data        = j.at("data");
        doc.sys_time    = TimeRange::fromJson(j.at("sys_time"));
        doc.valid_time  = TimeRange::fromJson(j.at("valid_time"));
        doc.modified_by = j.value("modified_by", std::string{});
        return doc;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

// ============================================================================
// TemporalColdStore — construction
// ============================================================================

TemporalColdStore::TemporalColdStore(std::unique_ptr<IColdStoreBackend> backend)
    : backend_(backend ? std::move(backend)
                       : std::make_unique<InMemoryBackend>()) {
    // Rebuild RAM index from backend (no-op for freshly created InMemoryBackend;
    // required for FileSystemBackend that may have pre-existing data on disk).
    rebuildIndexFromBackend();
}

void TemporalColdStore::rebuildIndexFromBackend() {
    auto all_keys = backend_->listKeysWithPrefix("");
    std::unique_lock lk(mutex_);
    key_index_.clear();
    for (auto& k : all_keys) {
        key_index_.insert(std::move(k));
    }
    total_count_ = key_index_.size();
    stats_.total_versions = total_count_.load();
}

// ============================================================================
// TemporalColdStore — mutation
// ============================================================================

bool TemporalColdStore::store(const std::string& table_name,
                               const VersionedDocument& doc) {
    if (doc.isCurrent()) {
      return false;
    }

    const std::string ck    = encodeKey(table_name, doc.key, doc.sys_time.start);
    const std::string value = doc.toJson().dump();

    if (!backend_->put(ck, value)) {
      return false;
    }

    std::unique_lock lk(mutex_);
    const bool is_new = key_index_.insert(ck).second;
    if (is_new) {
        ++total_count_;
        ++stats_.total_versions;
    }
    ++stats_.store_calls;
    ++stats_.backend_writes;
    return true;
}

size_t TemporalColdStore::remove(const std::string& table_name,
                                  const std::string& doc_key) {
    const std::string prefix = keyPrefix(table_name, doc_key);
    std::unique_lock lk(mutex_);

    std::vector<std::string> to_remove;
    auto it = key_index_.lower_bound(prefix);
    while (it != key_index_.end() &&
           it->substr(0,static_cast<int>(prefix.size())) == prefix) {
        to_remove.push_back(*it);
        ++it;
    }
    for (const auto& k : to_remove) {
        backend_->del(k);
        key_index_.erase(k);
    }
    const size_t removed = to_remove.size();
    total_count_ -= removed;
    stats_.total_versions -= removed;
    return removed;
}

size_t TemporalColdStore::removeTable(const std::string& table_name) {
    const std::string prefix = tablePrefix(table_name);
    std::unique_lock lk(mutex_);

    std::vector<std::string> to_remove;
    auto it = key_index_.lower_bound(prefix);
    while (it != key_index_.end() &&
           it->substr(0,static_cast<int>(prefix.size())) == prefix) {
        to_remove.push_back(*it);
        ++it;
    }
    for (const auto& k : to_remove) {
        backend_->del(k);
        key_index_.erase(k);
    }
    const size_t removed = to_remove.size();
    total_count_ -= removed;
    stats_.total_versions -= removed;
    return removed;
}

void TemporalColdStore::clear() {
    std::unique_lock lk(mutex_);
    backend_->clearAll();
    key_index_.clear();
    total_count_ = 0;
    stats_ = ColdStoreStats{};
}

// ============================================================================
// TemporalColdStore — queries
// ============================================================================

std::optional<VersionedDocument>
TemporalColdStore::getAsOf(const std::string& table_name,
                            const std::string& doc_key,
                            Timestamp as_of) const {
    const std::string search_key = encodeKey(table_name, doc_key, as_of);
    const std::string prefix     = keyPrefix(table_name, doc_key);

    std::shared_lock lk(mutex_);

    // upper_bound gives first key > search_key; step back to find candidates.
    auto it = key_index_.upper_bound(search_key);
    while (it != key_index_.begin()) {
        --it;
        if (it->substr(0,static_cast<int>(prefix.size())) != prefix) {
          break;
        }

        ++stats_.backend_reads;
        const std::string json_str = backend_->get(*it);
        auto doc = parseDocument(json_str);
        if (doc && doc->sys_time.contains(as_of)) {
            ++stats_.hit_getAsOf;
            return doc;
        }
        // sys_start <= as_of but version was already closed before as_of;
        // continue stepping back.
    }

    ++stats_.miss_getAsOf;
    return std::nullopt;
}

std::vector<VersionedDocument>
TemporalColdStore::getAll(const std::string& table_name,
                           const std::string& doc_key) const {
    const std::string prefix = keyPrefix(table_name, doc_key);
    std::shared_lock lk(mutex_);

    std::vector<VersionedDocument> result = {};

    for (auto it = key_index_.lower_bound(prefix);
         it != key_index_.end() && it->substr(0,static_cast<int>(prefix.size())) == prefix;
         ++it) {
        ++stats_.backend_reads;
        auto doc = parseDocument(backend_->get(*it));
        if (doc) {
          result.push_back(std::move(*doc));
        }
    }
    stats_.total_getAll_results += result.size();
    return result;
}

std::vector<VersionedDocument>
TemporalColdStore::getRange(const std::string& table_name,
                             const std::string& doc_key,
                             const TimeRange& range) const {
    const std::string prefix = keyPrefix(table_name, doc_key);
    std::shared_lock lk(mutex_);

    std::vector<VersionedDocument> result = {};

    for (auto it = key_index_.lower_bound(prefix);
         it != key_index_.end() && it->substr(0,static_cast<int>(prefix.size())) == prefix;
         ++it) {
        ++stats_.backend_reads;
        auto doc = parseDocument(backend_->get(*it));
        if (!doc) {
          continue;
        }
        // Prune: if sys_start >= range.end, no later entry can overlap.
        if (doc->sys_time.start >= range.end) {
          break;
        }
        if (doc->sys_time.overlaps(range)) {
          result.push_back(std::move(*doc));
        }
    }
    return result;
}

// ============================================================================
// TemporalColdStore — metadata
// ============================================================================

size_t TemporalColdStore::versionCount(const std::string& table_name,
                                        const std::string& doc_key) const {
    const std::string prefix = keyPrefix(table_name, doc_key);
    std::shared_lock lk(mutex_);
    size_t count = 0;
    for (auto it = key_index_.lower_bound(prefix);
         it != key_index_.end() && it->substr(0,static_cast<int>(prefix.size())) == prefix;
         ++it) ++count;
    return count;
}

size_t TemporalColdStore::totalVersionCount() const noexcept {
    return total_count_.load(std::memory_order_relaxed);
}

ColdStoreStats TemporalColdStore::stats() const {
    std::shared_lock lk(mutex_);
    stats_.total_versions = total_count_.load();
    return stats_;
}

} // namespace temporal
} // namespace themisdb

