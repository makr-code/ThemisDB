/**
 * @file semantic_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cache/semantic_cache.h"

#include <chrono>
#include <openssl/sha.h>
#include <rocksdb/iterator.h>

#include "utils/hash_util.h"
#include "utils/logger.h"

namespace themis {

// CacheEntry JSON serialization
nlohmann::json SemanticCache::CacheEntry::toJson() const {
    return {
        {"response", response}, {"metadata", metadata}, {"timestamp_ms", timestamp_ms}, {"ttl_seconds", ttl_seconds}};
}

std::optional<SemanticCache::CacheEntry> SemanticCache::CacheEntry::fromJson(const nlohmann::json &j) {
    try {
        CacheEntry entry;
        entry.response     = j.at("response").get<std::string>();
        entry.metadata     = j.value("metadata", nlohmann::json::object());
        entry.timestamp_ms = j.at("timestamp_ms").get<int64_t>();
        entry.ttl_seconds  = j.at("ttl_seconds").get<int>();
        return entry;
    } catch (const std::string& ex) {
        // generic_catch fix: log swallowed non-standard exception types
        THEMIS_DEBUG("semantic_cache::fromJson: std::string exception: {}", ex);
        return std::nullopt;
    } catch (const char* ex) {
        THEMIS_DEBUG("semantic_cache::fromJson: c-string exception: {}", (ex ? ex : "<null>"));
        return std::nullopt;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    } catch (...) {
        THEMIS_DEBUG("semantic_cache: unhandled exception caught");
        return std::nullopt;
    }
}

// Stats JSON serialization
nlohmann::json SemanticCache::Stats::toJson() const {
    return {{"hit_count", hit_count},         {"miss_count", miss_count},
            {"total_entries", total_entries}, {"total_size_bytes", total_size_bytes},
            {"hit_rate", hit_rate},           {"avg_latency_ms", avg_latency_ms}};
}

// Constructor
SemanticCache::SemanticCache(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf_handle,
                             int default_ttl_seconds, int bg_expiry_interval_s)
    : db_(db), cf_handle_(cf_handle), default_ttl_seconds_(default_ttl_seconds) {
    // F-014: launch background expiry thread when an interval is requested.
    // The thread wakes every bg_expiry_interval_s seconds and calls clearExpired().
    // A stop flag + condition variable allow the destructor to wake and join immediately.
    if (bg_expiry_interval_s > 0) {
        const auto interval = std::chrono::seconds(bg_expiry_interval_s);
        bg_expiry_thread_   = std::thread([this, interval]() {
            while (!bg_stop_.load(std::memory_order_acquire)) {
                std::unique_lock<std::mutex> lk(bg_cv_mutex_);
                bg_cv_.wait_for(lk, interval, [this]() { return bg_stop_.load(std::memory_order_acquire); });
                if (!bg_stop_.load(std::memory_order_acquire)) {
                    clearExpired();
                }
            }
        });
    }
}

SemanticCache::~SemanticCache() {
    // Signal the background thread to stop and wake it immediately so the
    // destructor doesn't need to wait for the full interval to elapse.
    bg_stop_.store(true, std::memory_order_release);
    bg_cv_.notify_all();
    if (bg_expiry_thread_.joinable()) {
        bg_expiry_thread_.join();
    }
}

// Compute SHA256 hash of prompt + params
std::string SemanticCache::computeKey(const std::string &prompt, const nlohmann::json &params) const {
    std::string input = prompt + params.dump();

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.c_str()),static_cast<int>(input.size()), hash);

    return themis::hash::bytes_to_hex(hash, SHA256_DIGEST_LENGTH);
}

int64_t SemanticCache::getCurrentTimestampMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

bool SemanticCache::isExpired(const CacheEntry &entry) const {
    if (entry.ttl_seconds <= 0) {
        return false; // No expiry
    }

    int64_t now_ms    = getCurrentTimestampMs();
    int64_t expiry_ms = entry.timestamp_ms + (entry.ttl_seconds * 1000);
    return now_ms > expiry_ms;
}

bool SemanticCache::put(const std::string &prompt, const nlohmann::json &params, const std::string &response,
                        const nlohmann::json &metadata, int ttl_seconds) {
    std::string key = computeKey(prompt, params);

    int effective_ttl = (ttl_seconds == 0) ? default_ttl_seconds_ : ttl_seconds;

    CacheEntry entry;
    entry.response     = response;
    entry.metadata     = metadata;
    entry.timestamp_ms = getCurrentTimestampMs();
    entry.ttl_seconds  = effective_ttl;

    std::string value = entry.toJson().dump();

    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;
    if (cf_handle_) {
        s = db_->Put(write_opts, cf_handle_, key, value);
    } else {
        s = db_->Put(write_opts, key, value);
    }

    if (s.ok()) {
        // Update in-memory size counters so getStats() avoids a full RocksDB scan.
        entry_count_.fetch_add(1, std::memory_order_relaxed);
        total_bytes_.fetch_add(static_cast<int>(key.size()) + value.size(), std::memory_order_relaxed);
    }

    return s.ok();
}

std::optional<SemanticCache::CacheEntry> SemanticCache::query(const std::string &prompt, const nlohmann::json &params) {
    auto start = std::chrono::steady_clock::now();

    std::string key = computeKey(prompt, params);
    std::string value = {};

    rocksdb::ReadOptions read_opts;
    rocksdb::Status s;
    if (cf_handle_) {
        s = db_->Get(read_opts, cf_handle_, key, &value);
    } else {
        s = db_->Get(read_opts, key, &value);
    }

    auto end = std::chrono::steady_clock::now();
    uint64_t latency_us
        = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    total_query_latency_us_.fetch_add(latency_us, std::memory_order_relaxed);

    if (!s.ok()) {
        miss_count_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(value);
    } catch (const std::string&) {
        miss_count_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    } catch (const char*) {
        miss_count_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    } catch (const nlohmann::json::exception&) {
        miss_count_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    } catch (...) {
        THEMIS_DEBUG("semantic_cache: unhandled exception caught");
        miss_count_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }

    auto entry = CacheEntry::fromJson(j);
    if (!entry || isExpired(*entry)) {
        miss_count_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }

    hit_count_.fetch_add(1, std::memory_order_relaxed);
    return entry;
}

SemanticCache::Stats SemanticCache::getStats() const {
    Stats stats;
    stats.hit_count  = hit_count_.load(std::memory_order_relaxed);
    stats.miss_count = miss_count_.load(std::memory_order_relaxed);

    uint64_t total_queries = stats.hit_count + stats.miss_count;
    if (total_queries > 0) {
        stats.hit_rate = static_cast<double>(stats.hit_count) / total_queries;
        // Convert accumulated microseconds back to milliseconds for the public API.
        stats.avg_latency_ms
            = static_cast<double>(total_query_latency_us_.load(std::memory_order_relaxed)) / total_queries / 1000.0;
    }

    // Read the in-memory counters maintained by put() / clearExpired() / clear().
    // This avoids the O(N) RocksDB key-scan that the previous implementation required.
    stats.total_entries    = entry_count_.load(std::memory_order_relaxed);
    stats.total_size_bytes = total_bytes_.load(std::memory_order_relaxed);

    return stats;
}

uint64_t SemanticCache::clearExpired() {
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it(cf_handle_ ? db_->NewIterator(read_opts, cf_handle_)
                                                     : db_->NewIterator(read_opts));

    // Check for null iterator before use
    if (!it) {
        THEMIS_ERROR("Failed to create iterator for semantic cache expiration cleanup");
        return 0; // Return 0 entries removed on iterator creation failure
    }

    rocksdb::WriteBatch batch;
    uint64_t removed = 0;

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string value = it->value().ToString();

        try {
            nlohmann::json j = nlohmann::json::parse(value);
            auto entry       = CacheEntry::fromJson(j);

            if (entry && isExpired(*entry)) {
                if (cf_handle_) {
                    batch.Delete(cf_handle_, it->key());
                } else {
                    batch.Delete(it->key());
                }
                removed++;
            }
        } catch (const nlohmann::json::exception&) {
            // Invalid entry, remove it.
            if (cf_handle_) {
                batch.Delete(cf_handle_, it->key());
            } else {
                batch.Delete(it->key());
            }
            removed++;
        } catch (const std::string&) {
            // Invalid entry, remove it.
            if (cf_handle_) {
                batch.Delete(cf_handle_, it->key());
            } else {
                batch.Delete(it->key());
            }
            removed++;
        } catch (const char*) {
            // Invalid entry, remove it
            if (cf_handle_) {
                batch.Delete(cf_handle_, it->key());
            } else {
                batch.Delete(it->key());
            }
            removed++;
        } catch (...) {
            THEMIS_WARN("semantic_cache::it: unhandled exception caught");
            // Invalid entry, remove it.
            if (cf_handle_) {
                batch.Delete(cf_handle_, it->key());
            } else {
                batch.Delete(it->key());
            }
            removed++;
        }
    }

    if (removed > 0) {
        rocksdb::WriteOptions write_opts;
        db_->Write(write_opts, &batch);
        // Saturating subtract via CAS loop — same pattern as numa_memory_manager
        // to prevent concurrent put() from being accidentally zeroed out when we
        // clamp an underflow.  Relaxed ordering is safe: entry_count_ is used
        // for statistics only, not for memory ordering between other variables.
        uint64_t expected = entry_count_.load(std::memory_order_relaxed);
        uint64_t desired = 0;
        do {
            desired = (expected >= removed) ? expected - removed : 0u;
        } while (!entry_count_.compare_exchange_weak(expected, desired, std::memory_order_relaxed,
                                                     std::memory_order_relaxed));
    }

    return removed;
}

bool SemanticCache::clear() {
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it(cf_handle_ ? db_->NewIterator(read_opts, cf_handle_)
                                                     : db_->NewIterator(read_opts));

    // Check for null iterator before use
    if (!it) {
        THEMIS_ERROR("Failed to create iterator for semantic cache clearing");
        return false; // Return failure on iterator creation failure
    }

    rocksdb::WriteBatch batch;

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (cf_handle_) {
            batch.Delete(cf_handle_, it->key());
        } else {
            batch.Delete(it->key());
        }
    }

    rocksdb::WriteOptions write_opts;
    rocksdb::Status s = db_->Write(write_opts, &batch);

    // Reset metrics
    hit_count_.store(0, std::memory_order_relaxed);
    miss_count_.store(0, std::memory_order_relaxed);
    total_query_latency_us_.store(0, std::memory_order_relaxed);
    entry_count_.store(0, std::memory_order_relaxed);
    total_bytes_.store(0, std::memory_order_relaxed);

    return s.ok();
}

} // namespace themis

