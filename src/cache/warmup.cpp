/**
 * @file warmup.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.20
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include <atomic>
#include <chrono>
#include <fstream>
#include <future>
#include <nlohmann/json.hpp>
#include <regex>
#include <thread>

#include "cache/adaptive_query_cache.h"
#include "utils/logger.h"
#include "utils/zstd_codec.h"

namespace themis {

// ---------------------------------------------------------------------------
// Internal helpers (file-scope)
// ---------------------------------------------------------------------------

namespace {

/// SHA-256 hex string pattern (64 lowercase hex characters).
static const std::regex kSha256Pattern("^[0-9a-f]{64}$");

/// Base64 alphabet (RFC 4648 standard, including padding).
static const std::string kB64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief Decode a base64 string to bytes.
 * @return Decoded bytes, or empty on error.
 */
static std::string base64Decode(const std::string &encoded) {
    if (encoded.empty()) {
        return {};
    }

    // Build reverse lookup table.
    uint8_t lookup[256];
    std::fill(std::begin(lookup), std::end(lookup), 0xFF);
    for (size_t i = 0; i < kB64Chars.size(); ++i) {
        lookup[static_cast<uint8_t>(kB64Chars[i])] = static_cast<uint8_t>(i);
    }

    std::string out = {};
    out.reserve((encoded.size() / 4) * 3);

    uint32_t buf = 0;
    int bits     = 0;

    for (char c : encoded) {
        if (c == '=') {
            break;
        }
        if (c == '\r' || c == '\n') {
            continue;
        }
        uint8_t val = lookup[static_cast<uint8_t>(c)];
        if (val == 0xFF) {
            return {}; // Invalid character.
        }
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
static std::string base64Encode(const std::string &data) {
    std::string out = {};
    out.reserve(((static_cast<int>(data.size()) + 2) / 3) * 4);

    uint32_t buf = 0;
    int bits     = 0;

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
    while (out.size() % 4 != 0) {
        out.push_back('=');
    }
    return out;
}

/**
 * @brief Validate that a key looks like a SHA-256 hex string.
 */
static bool isValidSha256Key(const std::string &key) {
    return std::regex_match(key, kSha256Pattern);
}

/// Prefix used by AdaptiveQueryCache::makeTenantKey().
static constexpr const char *kTenantKeyPrefix = "tenant:";
static constexpr size_t kTenantKeyPrefixLen   = 7; // strlen("tenant:")

/**
 * @brief Extract the tenant ID from a tenant-scoped cache key.
 *
 * Returns empty string if the key does not have the tenant prefix.
 */
static std::string extractTenantFromKey(const std::string &key) {
    if (static_cast<int>(key.size()) <= kTenantKeyPrefixLen || key.substr(0, kTenantKeyPrefixLen) != kTenantKeyPrefix) {
        return {};
    }
    size_t second_colon = key.find(':', kTenantKeyPrefixLen);
    if (second_colon == std::string::npos) {
        return {};
    }
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
static std::string extractFingerprintFromKey(const std::string &key) {
    if (static_cast<int>(key.size()) <= kTenantKeyPrefixLen || key.substr(0, kTenantKeyPrefixLen) != kTenantKeyPrefix) {
        return key; // Not tenant-scoped; already a plain fingerprint.
    }
    size_t second_colon = key.find(':', kTenantKeyPrefixLen);
    if (second_colon == std::string::npos) {
        return key;
    }
    return key.substr(second_colon + 1);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// AdaptiveQueryCache::warmupFromLog
// ---------------------------------------------------------------------------

AdaptiveQueryCache::WarmupResult AdaptiveQueryCache::warmupFromLog(const std::string &log_path, size_t max_entries) {
    const auto t0 = std::chrono::steady_clock::now();

    WarmupResult result;
    std::ifstream file(log_path);
    if (!file.is_open()) {
        THEMIS_WARN("warmupFromLog: cannot open log file '{}'", log_path);
        result.ok    = false;
        result.error = "cannot open log file: " + log_path;
        return result;
    }

    // Read all lines upfront so they can be partitioned across workers.
    std::vector<std::string> lines;
    {
        std::string line = {};
        while (std::getline(file, line)) {
            lines.push_back(std::move(line));
        }
    }
    file.close();

    // Keep warmup headroom in L1 so live traffic can still promote hot entries.
    const size_t l1_warmup_cap = std::max<size_t>(1, config_.l1_max_entries / 2);
    const int64_t now_ms       = getCurrentTimeMs();

    // Snapshot current L1 size. Shared across workers as an atomic so each
    // worker can check and update headroom without holding the mutex.
    std::atomic<size_t> l1_warmed{0};
    {
        std::shared_lock<std::shared_mutex> lk(l1_mutex_);
        l1_warmed.store(l1_cache_.size(), std::memory_order_relaxed);
    }

    // Aggregate counters: updated atomically by all workers.
    std::atomic<size_t> total_loaded{0};
    std::atomic<size_t> total_skipped{0};
    std::atomic<size_t> total_failed{0};

    // Determine number of workers (at least 1). For small logs, force
    // single-worker mode to keep duplicate handling deterministic.
    uint32_t num_workers = (config_.max_parallel_workers > 0) ? config_.max_parallel_workers : 1u;

    const size_t total_lines = lines.size();
    if (total_lines < 64) {
        num_workers = 1u;
    }
    const size_t chunk_size = (total_lines + num_workers - 1) / num_workers;

    // Worker lambda: processes lines[start..end) independently.
    // Thread-safety notes:
    //  - l1_mutex_ / l2_mutex_ / tenant_mutex_ serialise per-shard insertions.
    //  - total_loaded / total_skipped / total_failed / l1_warmed are
    //    std::atomic<size_t>, updated without holding any other lock.
    //  - enhanced_metrics_.warmup_entries_* / total_bytes_* are
    //    std::atomic<uint64_t> (see cache_metrics.h), safe for concurrent ++.
    auto processChunk = [&](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            const std::string &line  = lines[i];
            const size_t line_number = i + 1;

            if (line.empty() || line.front() == '#') {
                continue;
            }

            // Hard cap is enforced via slot reservation just before insertion.

            // --- Parse JSON record ---
            nlohmann::json rec;
            try {
                rec = nlohmann::json::parse(line);
            } catch (const std::exception &e) {
                THEMIS_WARN("warmupFromLog: line {}: JSON parse error: {}", line_number, e.what());
                total_failed.fetch_add(1, std::memory_order_relaxed);
                enhanced_metrics_.warmup_entries_failed++;
                continue;
            }

            // Required fields.
            if (!rec.contains("key") || !rec.contains("value_b64")) {
                THEMIS_WARN("warmupFromLog: line {}: missing 'key' or 'value_b64'", line_number);
                total_skipped.fetch_add(1, std::memory_order_relaxed);
                enhanced_metrics_.warmup_entries_skipped++;
                continue;
            }

            std::string key       = rec["key"].get<std::string>();
            std::string value_b64 = rec["value_b64"].get<std::string>();
            int ttl_remaining_s   = rec.value("ttl_remaining_s", config_.l1_ttl_seconds);
            std::string tenant_id = rec.value("tenant", std::string{});

            // Validate key format.
            if (!isValidSha256Key(key)) {
                THEMIS_WARN("warmupFromLog: line {}: invalid key '{}' (not SHA-256 hex)", line_number,
                            key.substr(0, 16));
                total_skipped.fetch_add(1, std::memory_order_relaxed);
                enhanced_metrics_.warmup_entries_skipped++;
                continue;
            }

            // Skip entries with non-positive TTL.
            if (ttl_remaining_s <= 0) {
                total_skipped.fetch_add(1, std::memory_order_relaxed);
                enhanced_metrics_.warmup_entries_skipped++;
                continue;
            }

            // Decode value.
            std::string decoded = base64Decode(value_b64);
            if (decoded.empty()) {
                THEMIS_WARN("warmupFromLog: line {}: base64 decode failed for key {}", line_number, key.substr(0, 16));
                total_failed.fetch_add(1, std::memory_order_relaxed);
                enhanced_metrics_.warmup_entries_failed++;
                continue;
            }

            // Do not apply a global decoded-size gate here. Warmup insertion
            // already enforces per-level size limits (L1/L2), which are the
            // authoritative constraints for cache admission.

            // Parse decoded JSON value.
            // Named `value_json` (not `result`) to avoid shadowing the outer
            // WarmupResult variable `result`.
            nlohmann::json value_json;
            try {
                value_json = nlohmann::json::parse(decoded);
            } catch (const std::exception &e) {
                THEMIS_WARN("warmupFromLog: line {}: value JSON parse error: {}", line_number, e.what());
                total_failed.fetch_add(1, std::memory_order_relaxed);
                enhanced_metrics_.warmup_entries_failed++;
                continue;
            }

            // Check per-tenant quota (honour limits even during warmup).
            if (!checkTenantQuota(tenant_id, decoded.size())) {
                THEMIS_DEBUG("warmupFromLog: line {}: tenant '{}' quota exceeded, skipping", line_number, tenant_id);
                total_skipped.fetch_add(1, std::memory_order_relaxed);
                enhanced_metrics_.warmup_entries_skipped++;
                continue;
            }

            // Decide target level: L1 (up to cap) or L2.
            const std::string cache_key
                = (config_.enable_tenant_isolation && !tenant_id.empty()) ? makeTenantKey(key, tenant_id) : key;

            // Reserve one global warmup slot atomically so max_entries is exact,
            // even with multiple workers.
            bool reserved_slot = false;
            if (max_entries > 0) {
                size_t cur = total_loaded.load(std::memory_order_relaxed);
                for (;;) {
                    if (cur >= max_entries) {
                        break;
                    }
                    if (total_loaded.compare_exchange_weak(cur, cur + 1, std::memory_order_relaxed,
                                                           std::memory_order_relaxed)) {
                        reserved_slot = true;
                        break;
                    }
                }
                if (!reserved_slot) {
                    break;
                }
            }

            bool inserted = false;

            if (l1_warmed.load(std::memory_order_relaxed) < l1_warmup_cap
                && decoded.size() <= config_.l1_max_entry_size) {
                // Store in L1 (per-shard insertion under l1_mutex_).
                auto entry    = std::make_unique<L1Entry>();
                entry->result = value_json;
                entry->created_at_ms.store(now_ms, std::memory_order_relaxed);
                entry->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
                entry->access_count.store(1, std::memory_order_relaxed);
                entry->ttl_seconds.store(ttl_remaining_s, std::memory_order_relaxed);

                {
                    std::unique_lock<std::shared_mutex> lk(l1_mutex_);
                    if (l1_cache_.count(cache_key) == 0) {
                        if (static_cast<int>(l1_cache_.size()) > = config_.l1_max_entries) {
                            evictLRU(CacheLevel::HOT);
                        }
                        l1_cache_[cache_key] = std::move(entry);
                        l1_eviction_strategy_->onInsert(cache_key, static_cast<uint64_t>(now_ms));
                        l1_warmed.fetch_add(1, std::memory_order_relaxed);
                        enhanced_metrics_.total_bytes_cached += decoded.size();
                        inserted = true;
                    }
                }
            } else {
                // Store in L2 (compressed).
                auto compressed = utils::zstd_compress(decoded, config_.l2_compression_level);
                if (compressed.empty()) {
                    // ZSTD can be unavailable in some builds. Fall back to
                    // plain L1 insertion so warmup still succeeds.
                    auto entry    = std::make_unique<L1Entry>();
                    entry->result = value_json;
                    entry->created_at_ms.store(now_ms, std::memory_order_relaxed);
                    entry->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
                    entry->access_count.store(1, std::memory_order_relaxed);
                    entry->ttl_seconds.store(ttl_remaining_s, std::memory_order_relaxed);

                    {
                        std::unique_lock<std::shared_mutex> lk(l1_mutex_);
                        if (l1_cache_.count(cache_key) == 0) {
                            if (static_cast<int>(l1_cache_.size()) > = config_.l1_max_entries) {
                                evictLRU(CacheLevel::HOT);
                            }
                            l1_cache_[cache_key] = std::move(entry);
                            l1_eviction_strategy_->onInsert(cache_key, static_cast<uint64_t>(now_ms));
                            l1_warmed.fetch_add(1, std::memory_order_relaxed);
                            enhanced_metrics_.total_bytes_cached += decoded.size();
                            inserted = true;
                        }
                    }
                } else {
                    L2Entry entry;
                    entry.compressed_result = std::move(compressed);
                    entry.created_at_ms     = now_ms;
                    entry.last_accessed_ms  = now_ms;
                    entry.access_count      = 1;
                    entry.ttl_seconds       = ttl_remaining_s;

                    size_t compressed_size = entry.compressed_result.size();
                    {
                        std::lock_guard<std::mutex> lk(l2_mutex_);
                        if (l2_cache_.count(cache_key) == 0) {
                            if (static_cast<int>(l2_cache_.size()) > = config_.l2_max_entries) {
                                evictLRU(CacheLevel::WARM);
                            }
                            l2_cache_[cache_key] = std::move(entry);
                            l2_eviction_strategy_->onInsert(cache_key, static_cast<uint64_t>(now_ms));
                            enhanced_metrics_.total_bytes_cached += decoded.size();
                            enhanced_metrics_.total_bytes_compressed += compressed_size;
                            inserted = true;
                        }
                    }
                }
            }

            if (inserted) {
                // Update tenant quota tracking only when the entry was inserted.
                if (config_.enable_tenant_isolation && !tenant_id.empty()) {
                    std::lock_guard<std::mutex> tlk(tenant_mutex_);
                    tenant_metrics_[tenant_id].bytes_used += decoded.size();
                }
                if (!reserved_slot) {
                    total_loaded.fetch_add(1, std::memory_order_relaxed);
                }
                enhanced_metrics_.warmup_entries_loaded++;
            } else {
                // Duplicate key already present in cache; count as skipped.
                if (reserved_slot) {
                    total_loaded.fetch_sub(1, std::memory_order_relaxed);
                }
                total_skipped.fetch_add(1, std::memory_order_relaxed);
                enhanced_metrics_.warmup_entries_skipped++;
            }
        }
    };

    // Dispatch workers. Use a single-threaded fast path when there is only
    // one worker or the log is trivially small to avoid async overhead.
    if (num_workers <= 1 || total_lines == 0) {
        processChunk(0, total_lines);
    } else {
        std::vector<std::future<void>> futures;
        futures.reserve(num_workers);
        for (uint32_t w = 0; w < num_workers; ++w) {
            const size_t start = w * chunk_size;
            if (start >= total_lines) {
                break;
            }
            const size_t end = std::min(start + chunk_size, total_lines);
            futures.push_back(std::async(std::launch::async, processChunk, start, end));
        }
        for (auto &f : futures) {
            f.get();
        }
    }

    // Compute elapsed time and throughput.
    // Use microsecond precision internally so sub-millisecond runs still
    // produce a meaningful entries-per-second value.
    const auto t1             = std::chrono::steady_clock::now();
    const int64_t duration_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    const int64_t duration_ms = duration_us / 1000;
    const size_t loaded       = total_loaded.load();
    const size_t skipped      = total_skipped.load();
    const size_t failed       = total_failed.load();
    // Throughput: 0.0 when no time has elapsed (e.g. empty log) to avoid
    // reporting a bogus infinity-like value.
    const double entries_per_second
        = (duration_us > 0) ? static_cast<double>(loaded) / (static_cast<double>(duration_us) / 1'000'000.0) : 0.0;

    THEMIS_INFO("warmupFromLog: loaded={}, skipped={}, failed={} from '{}' "
                "in {}ms ({:.0f} entries/s, {} workers)",
                loaded, skipped, failed, log_path, duration_ms, entries_per_second, num_workers);

    result.entries_loaded            = loaded;
    result.entries_skipped           = skipped + failed;
    result.entries_total             = loaded + skipped + failed;
    result.warmup_duration_ms        = duration_ms;
    result.warmup_entries_per_second = entries_per_second;
    return result;
}

// ---------------------------------------------------------------------------
// AdaptiveQueryCache::exportSnapshot
// ---------------------------------------------------------------------------

AdaptiveQueryCache::WarmupResult AdaptiveQueryCache::exportSnapshot(const std::string &out_path) const {
    WarmupResult result;
    std::ofstream file(out_path, std::ios::trunc);
    if (!file.is_open()) {
        THEMIS_WARN("exportSnapshot: cannot open output file '{}'", out_path);
        result.ok    = false;
        result.error = "cannot open output file: " + out_path;
        return result;
    }

    size_t exported      = 0;
    const int64_t now_ms = getCurrentTimeMs();

    // Export L1 entries.
    {
        std::shared_lock<std::shared_mutex> lk(l1_mutex_);
        for (const auto &[key, entry] : l1_cache_) {
            if (!entry) {
                continue;
            }

            const int64_t created_at_ms = entry->created_at_ms.load(std::memory_order_relaxed);
            const int ttl_seconds       = entry->ttl_seconds.load(std::memory_order_relaxed);
            if (isExpired(created_at_ms, ttl_seconds)) {
                continue;
            }

            int ttl_remaining_s = ttl_seconds - static_cast<int>((now_ms - created_at_ms) / 1000);
            if (ttl_remaining_s <= 0) {
                continue;
            }

            std::string value_json = entry->result.dump();
            std::string value_b64  = base64Encode(value_json);

            nlohmann::json rec;
            // Bug fix: always export the bare SHA-256 fingerprint, not the
            // tenant-scoped cache key.  warmupFromLog() validates the key with
            // isValidSha256Key() which requires a 64-char hex string; exporting
            // "tenant:<id>:<fp>" would cause those entries to be silently
            // skipped on re-import.
            rec["key"]             = extractFingerprintFromKey(key);
            rec["value_b64"]       = value_b64;
            rec["ttl_remaining_s"] = ttl_remaining_s;

            std::string tenant = extractTenantFromKey(key);
            if (!tenant.empty()) {
                rec["tenant"] = tenant;
            }

            file << rec.dump() << '\n';
            ++exported;
        }
    }

    // Export L2 entries.
    {
        std::lock_guard<std::mutex> lk(l2_mutex_);
        for (const auto &[key, entry] : l2_cache_) {
            if (isExpired(entry.created_at_ms, entry.ttl_seconds)) {
                continue;
            }

            int ttl_remaining_s = entry.ttl_seconds - static_cast<int>((now_ms - entry.created_at_ms) / 1000);
            if (ttl_remaining_s <= 0) {
                continue;
            }

            auto decompressed = utils::zstd_decompress(entry.compressed_result);
            if (decompressed.empty()) {
                continue;
            }

            std::string value_json(decompressed.begin(), decompressed.end());
            std::string value_b64 = base64Encode(value_json);

            nlohmann::json rec;
            rec["key"]             = extractFingerprintFromKey(key);
            rec["value_b64"]       = value_b64;
            rec["ttl_remaining_s"] = ttl_remaining_s;

            std::string tenant = extractTenantFromKey(key);
            if (!tenant.empty()) {
                rec["tenant"] = tenant;
            }

            file << rec.dump() << '\n';
            ++exported;
        }
    }

    file.flush();
    THEMIS_INFO("exportSnapshot: exported {} entries to '{}'", exported, out_path);
    result.entries_loaded  = exported;
    result.entries_written = exported;
    result.entries_total   = exported;
    return result;
}

} // namespace themis
