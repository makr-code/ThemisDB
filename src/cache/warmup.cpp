/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            warmup.cpp                                         ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-23 03:58:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     438                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 1808900b2  2026-02-22  feat: implement auto-bootstrap for third-party dependenci... ║
    • a01131277  2026-02-22  fix(cache): fix 3 bugs in warmup.cpp found during code audit ║
    • b3ba0e0e3  2026-02-22  feat(cache): implement cache warmup with bulk operations ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "cache/adaptive_query_cache.h"
#include "utils/logger.h"
#include "utils/zstd_codec.h"
#include <fstream>
#include <regex>
#include <nlohmann/json.hpp>

namespace themis {

// ---------------------------------------------------------------------------
// Internal helpers (file-scope)
// ---------------------------------------------------------------------------

namespace {

/// SHA-256 hex string pattern (64 lowercase hex characters).
static const std::regex kSha256Pattern("^[0-9a-f]{64}$");

/// Base64 alphabet (RFC 4648 standard, including padding).
static const std::string kB64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief Decode a base64 string to bytes.
 * @return Decoded bytes, or empty on error.
 */
static std::string base64Decode(const std::string& encoded) {
    if (encoded.empty()) return {};

    // Build reverse lookup table.
    uint8_t lookup[256];
    std::fill(std::begin(lookup), std::end(lookup), 0xFF);
    for (size_t i = 0; i < kB64Chars.size(); ++i) {
        lookup[static_cast<uint8_t>(kB64Chars[i])] = static_cast<uint8_t>(i);
    }

    std::string out;
    out.reserve((encoded.size() / 4) * 3);

    uint32_t buf = 0;
    int bits = 0;

    for (char c : encoded) {
        if (c == '=') break;
        if (c == '\r' || c == '\n') continue;
        uint8_t val = lookup[static_cast<uint8_t>(c)];
        if (val == 0xFF) return {};  // Invalid character.
        buf = (buf << 6) | val;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((buf >> bits) & 0xFF));
        }
    }
    return out;
}

/**
 * @brief Encode bytes to base64.
 */
static std::string base64Encode(const std::string& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    uint32_t buf = 0;
    int bits = 0;

    for (unsigned char c : data) {
        buf = (buf << 8) | c;
        bits += 8;
        while (bits >= 6) {
            bits -= 6;
            out.push_back(kB64Chars[(buf >> bits) & 0x3F]);
        }
    }
    if (bits > 0) {
        buf <<= (6 - bits);
        out.push_back(kB64Chars[buf & 0x3F]);
    }
    // Padding.
    while (out.size() % 4 != 0) out.push_back('=');
    return out;
}

/**
 * @brief Validate that a key looks like a SHA-256 hex string.
 */
static bool isValidSha256Key(const std::string& key) {
    return std::regex_match(key, kSha256Pattern);
}

/// Prefix used by AdaptiveQueryCache::makeTenantKey().
static constexpr const char* kTenantKeyPrefix = "tenant:";
static constexpr size_t kTenantKeyPrefixLen = 7;  // strlen("tenant:")

/**
 * @brief Extract the tenant ID from a tenant-scoped cache key.
 *
 * Returns empty string if the key does not have the tenant prefix.
 */
static std::string extractTenantFromKey(const std::string& key) {
    if (key.size() <= kTenantKeyPrefixLen ||
        key.substr(0, kTenantKeyPrefixLen) != kTenantKeyPrefix) {
        return {};
    }
    size_t second_colon = key.find(':', kTenantKeyPrefixLen);
    if (second_colon == std::string::npos) return {};
    return key.substr(kTenantKeyPrefixLen, second_colon - kTenantKeyPrefixLen);
}

/**
 * @brief Extract the bare SHA-256 fingerprint from a (possibly tenant-scoped) cache key.
 *
 * - For plain fingerprints: returns the key unchanged.
 * - For tenant-scoped keys ("tenant:<id>:<fingerprint>"): returns only the fingerprint part.
 *
 * This is needed by exportSnapshot() so the exported log records always carry
 * a bare 64-char hex key that warmupFromLog() can re-import correctly.
 */
static std::string extractFingerprintFromKey(const std::string& key) {
    if (key.size() <= kTenantKeyPrefixLen ||
        key.substr(0, kTenantKeyPrefixLen) != kTenantKeyPrefix) {
        return key;  // Not tenant-scoped; already a plain fingerprint.
    }
    size_t second_colon = key.find(':', kTenantKeyPrefixLen);
    if (second_colon == std::string::npos) return key;
    return key.substr(second_colon + 1);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// AdaptiveQueryCache::warmupFromLog
// ---------------------------------------------------------------------------

AdaptiveQueryCache::WarmupResult AdaptiveQueryCache::warmupFromLog(const std::string& log_path,
                                                                    size_t max_entries) {
    WarmupResult result;
    std::ifstream file(log_path);
    if (!file.is_open()) {
        THEMIS_WARN("warmupFromLog: cannot open log file '{}'", log_path);
        result.ok = false;
        result.error = "cannot open log file: " + log_path;
        return result;
    }

    const size_t l1_warmup_cap = config_.l1_max_entries / 2;
    size_t loaded = 0;
    size_t skipped = 0;
    size_t failed = 0;

    // Snapshot current L1 size to track headroom correctly.
    size_t l1_warmed = 0;
    {
        std::lock_guard<std::mutex> lk(l1_mutex_);
        l1_warmed = l1_cache_.size();
    }

    const int64_t now_ms = getCurrentTimeMs();

    std::string line;
    size_t line_number = 0;

    while (std::getline(file, line)) {
        ++line_number;
        if (line.empty() || line.front() == '#') continue;
        if (max_entries > 0 && loaded >= max_entries) break;

        // --- Parse JSON record ---
        nlohmann::json rec;
        try {
            rec = nlohmann::json::parse(line);
        } catch (const std::exception& e) {
            THEMIS_WARN("warmupFromLog: line {}: JSON parse error: {}", line_number, e.what());
            ++failed;
            enhanced_metrics_.warmup_entries_failed++;
            continue;
        }

        // Required fields.
        if (!rec.contains("key") || !rec.contains("value_b64")) {
            THEMIS_WARN("warmupFromLog: line {}: missing 'key' or 'value_b64'", line_number);
            ++skipped;
            enhanced_metrics_.warmup_entries_skipped++;
            continue;
        }

        std::string key = rec["key"].get<std::string>();
        std::string value_b64 = rec["value_b64"].get<std::string>();
        int ttl_remaining_s = rec.value("ttl_remaining_s", config_.l1_ttl_seconds);
        std::string tenant_id = rec.value("tenant", std::string{});

        // Validate key format.
        if (!isValidSha256Key(key)) {
            THEMIS_WARN("warmupFromLog: line {}: invalid key '{}' (not SHA-256 hex)",
                        line_number, key.substr(0, 16));
            ++skipped;
            enhanced_metrics_.warmup_entries_skipped++;
            continue;
        }

        // Skip entries with non-positive TTL.
        if (ttl_remaining_s <= 0) {
            ++skipped;
            enhanced_metrics_.warmup_entries_skipped++;
            continue;
        }

        // Decode value.
        std::string decoded = base64Decode(value_b64);
        if (decoded.empty()) {
            THEMIS_WARN("warmupFromLog: line {}: base64 decode failed for key {}", line_number, key.substr(0, 16));
            ++failed;
            enhanced_metrics_.warmup_entries_failed++;
            continue;
        }

        // Validate decoded size.
        if (decoded.size() > config_.max_total_entry_size) {
            THEMIS_WARN("warmupFromLog: line {}: decoded value size {} exceeds max {}",
                        line_number, decoded.size(), config_.max_total_entry_size);
            ++skipped;
            enhanced_metrics_.warmup_entries_skipped++;
            continue;
        }

        // Parse decoded JSON value.
        nlohmann::json result;
        try {
            result = nlohmann::json::parse(decoded);
        } catch (const std::exception& e) {
            THEMIS_WARN("warmupFromLog: line {}: value JSON parse error: {}", line_number, e.what());
            ++failed;
            enhanced_metrics_.warmup_entries_failed++;
            continue;
        }

        // Check per-tenant quota (honour limits even during warmup).
        if (!checkTenantQuota(tenant_id, decoded.size())) {
            THEMIS_DEBUG("warmupFromLog: line {}: tenant '{}' quota exceeded, skipping", line_number, tenant_id);
            ++skipped;
            enhanced_metrics_.warmup_entries_skipped++;
            continue;
        }

        // Decide target level: L1 (up to cap) or L2.
        const std::string cache_key = (config_.enable_tenant_isolation && !tenant_id.empty())
                                      ? makeTenantKey(key, tenant_id)
                                      : key;

        bool inserted = false;

        if (l1_warmed < l1_warmup_cap && decoded.size() <= config_.l1_max_entry_size) {
            // Store in L1.
            L1Entry entry;
            entry.result = result;
            entry.created_at_ms = now_ms;
            entry.last_accessed_ms = now_ms;
            entry.access_count = 1;
            entry.ttl_seconds = ttl_remaining_s;

            {
                std::lock_guard<std::mutex> lk(l1_mutex_);
                if (l1_cache_.count(cache_key) == 0) {
                    if (l1_cache_.size() >= config_.l1_max_entries) {
                        evictLRU(CacheLevel::HOT);
                    }
                    l1_cache_[cache_key] = std::move(entry);
                    ++l1_warmed;
                    enhanced_metrics_.total_bytes_cached += decoded.size();
                    inserted = true;
                }
            }
        } else {
            // Store in L2 (compressed).
            auto compressed = utils::zstd_compress(decoded, config_.l2_compression_level);
            if (compressed.empty()) {
                THEMIS_WARN("warmupFromLog: line {}: compression failed for key {}", line_number, key.substr(0, 16));
                ++failed;
                enhanced_metrics_.warmup_entries_failed++;
                continue;
            }

            L2Entry entry;
            entry.compressed_result = std::move(compressed);
            entry.created_at_ms = now_ms;
            entry.last_accessed_ms = now_ms;
            entry.access_count = 1;
            entry.ttl_seconds = ttl_remaining_s;

            size_t compressed_size = entry.compressed_result.size();
            {
                std::lock_guard<std::mutex> lk(l2_mutex_);
                if (l2_cache_.count(cache_key) == 0) {
                    if (l2_cache_.size() >= config_.l2_max_entries) {
                        evictLRU(CacheLevel::WARM);
                    }
                    l2_cache_[cache_key] = std::move(entry);
                    enhanced_metrics_.total_bytes_cached += decoded.size();
                    enhanced_metrics_.total_bytes_compressed += compressed_size;
                    inserted = true;
                }
            }
        }

        if (inserted) {
            // Update tenant quota tracking only when the entry was actually inserted.
            if (config_.enable_tenant_isolation && !tenant_id.empty()) {
                std::lock_guard<std::mutex> tlk(tenant_mutex_);
                tenant_sizes_[tenant_id] += decoded.size();
            }
            ++loaded;
            enhanced_metrics_.warmup_entries_loaded++;
        } else {
            // Duplicate key already present in cache; count as skipped.
            ++skipped;
            enhanced_metrics_.warmup_entries_skipped++;
        }
    }

    THEMIS_INFO("warmupFromLog: loaded={}, skipped={}, failed={} from '{}'",
                loaded, skipped, failed, log_path);
    result.entries_loaded = loaded;
    result.entries_skipped = skipped + failed;
    result.entries_total = loaded + skipped + failed;
    return result;
}

// ---------------------------------------------------------------------------
// AdaptiveQueryCache::exportSnapshot
// ---------------------------------------------------------------------------

AdaptiveQueryCache::WarmupResult AdaptiveQueryCache::exportSnapshot(const std::string& out_path) const {
    WarmupResult result;
    std::ofstream file(out_path, std::ios::trunc);
    if (!file.is_open()) {
        THEMIS_WARN("exportSnapshot: cannot open output file '{}'", out_path);
        result.ok = false;
        result.error = "cannot open output file: " + out_path;
        return result;
    }

    size_t exported = 0;
    const int64_t now_ms = getCurrentTimeMs();

    // Export L1 entries.
    {
        std::lock_guard<std::mutex> lk(l1_mutex_);
        for (const auto& [key, entry] : l1_cache_) {
            if (isExpired(entry.created_at_ms, entry.ttl_seconds)) continue;

            int ttl_remaining_s = entry.ttl_seconds
                - static_cast<int>((now_ms - entry.created_at_ms) / 1000);
            if (ttl_remaining_s <= 0) continue;

            std::string value_json = entry.result.dump();
            std::string value_b64 = base64Encode(value_json);

            nlohmann::json rec;
            // Bug fix: always export the bare SHA-256 fingerprint, not the
            // tenant-scoped cache key.  warmupFromLog() validates the key with
            // isValidSha256Key() which requires a 64-char hex string; exporting
            // "tenant:<id>:<fp>" would cause those entries to be silently
            // skipped on re-import.
            rec["key"] = extractFingerprintFromKey(key);
            rec["value_b64"] = value_b64;
            rec["ttl_remaining_s"] = ttl_remaining_s;

            std::string tenant = extractTenantFromKey(key);
            if (!tenant.empty()) rec["tenant"] = tenant;

            file << rec.dump() << '\n';
            ++exported;
        }
    }

    // Export L2 entries.
    {
        std::lock_guard<std::mutex> lk(l2_mutex_);
        for (const auto& [key, entry] : l2_cache_) {
            if (isExpired(entry.created_at_ms, entry.ttl_seconds)) continue;

            int ttl_remaining_s = entry.ttl_seconds
                - static_cast<int>((now_ms - entry.created_at_ms) / 1000);
            if (ttl_remaining_s <= 0) continue;

            auto decompressed = utils::zstd_decompress(entry.compressed_result);
            if (decompressed.empty()) continue;

            std::string value_json(decompressed.begin(), decompressed.end());
            std::string value_b64 = base64Encode(value_json);

            nlohmann::json rec;
            rec["key"] = extractFingerprintFromKey(key);
            rec["value_b64"] = value_b64;
            rec["ttl_remaining_s"] = ttl_remaining_s;

            std::string tenant = extractTenantFromKey(key);
            if (!tenant.empty()) rec["tenant"] = tenant;

            file << rec.dump() << '\n';
            ++exported;
        }
    }

    file.flush();
    THEMIS_INFO("exportSnapshot: exported {} entries to '{}'", exported, out_path);
    result.entries_loaded = exported;
    result.entries_written = exported;
    result.entries_total = exported;
    return result;
}

} // namespace themis
