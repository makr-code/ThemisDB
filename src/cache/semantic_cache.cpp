/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            semantic_cache.cpp                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     290                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "cache/semantic_cache.h"
#include "utils/logger.h"
#include "utils/hash_util.h"
#include <openssl/sha.h>
#include <chrono>
#include <rocksdb/iterator.h>

namespace themis {

// CacheEntry JSON serialization
nlohmann::json SemanticCache::CacheEntry::toJson() const {
    return {
        {"response", response},
        {"metadata", metadata},
        {"timestamp_ms", timestamp_ms},
        {"ttl_seconds", ttl_seconds}
    };
}

std::optional<SemanticCache::CacheEntry> SemanticCache::CacheEntry::fromJson(const nlohmann::json& j) {
    try {
        CacheEntry entry;
        entry.response = j.at("response").get<std::string>();
        entry.metadata = j.value("metadata", nlohmann::json::object());
        entry.timestamp_ms = j.at("timestamp_ms").get<int64_t>();
        entry.ttl_seconds = j.at("ttl_seconds").get<int>();
        return entry;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// Stats JSON serialization
nlohmann::json SemanticCache::Stats::toJson() const {
    return {
        {"hit_count", hit_count},
        {"miss_count", miss_count},
        {"total_entries", total_entries},
        {"total_size_bytes", total_size_bytes},
        {"hit_rate", hit_rate},
        {"avg_latency_ms", avg_latency_ms}
    };
}

// Constructor
SemanticCache::SemanticCache(
    rocksdb::TransactionDB* db,
    rocksdb::ColumnFamilyHandle* cf_handle,
    int default_ttl_seconds
) : db_(db), cf_handle_(cf_handle), default_ttl_seconds_(default_ttl_seconds) {}

// Compute SHA256 hash of prompt + params
std::string SemanticCache::computeKey(const std::string& prompt, const nlohmann::json& params) const {
    std::string input = prompt + params.dump();
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    
    return themis::hash::bytes_to_hex(hash, SHA256_DIGEST_LENGTH);
}

int64_t SemanticCache::getCurrentTimestampMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

bool SemanticCache::isExpired(const CacheEntry& entry) const {
    if (entry.ttl_seconds <= 0) {
        return false; // No expiry
    }
    
    int64_t now_ms = getCurrentTimestampMs();
    int64_t expiry_ms = entry.timestamp_ms + (entry.ttl_seconds * 1000);
    return now_ms > expiry_ms;
}

bool SemanticCache::put(
    const std::string& prompt,
    const nlohmann::json& params,
    const std::string& response,
    const nlohmann::json& metadata,
    int ttl_seconds
) {
    std::string key = computeKey(prompt, params);
    
    int effective_ttl = (ttl_seconds == 0) ? default_ttl_seconds_ : ttl_seconds;
    
    CacheEntry entry;
    entry.response = response;
    entry.metadata = metadata;
    entry.timestamp_ms = getCurrentTimestampMs();
    entry.ttl_seconds = effective_ttl;
    
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
        total_bytes_.fetch_add(key.size() + value.size(), std::memory_order_relaxed);
    }
    
    return s.ok();
}

std::optional<SemanticCache::CacheEntry> SemanticCache::query(
    const std::string& prompt,
    const nlohmann::json& params
) {
    auto start = std::chrono::steady_clock::now();
    
    std::string key = computeKey(prompt, params);
    std::string value;
    
    rocksdb::ReadOptions read_opts;
    rocksdb::Status s;
    if (cf_handle_) {
        s = db_->Get(read_opts, cf_handle_, key, &value);
    } else {
        s = db_->Get(read_opts, key, &value);
    }
    
    auto end = std::chrono::steady_clock::now();
    uint64_t latency_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    total_query_latency_us_.fetch_add(latency_us, std::memory_order_relaxed);
    
    if (!s.ok()) {
        miss_count_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }
    
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(value);
    } catch (const std::exception&) {
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
        stats.hit_rate     = static_cast<double>(stats.hit_count) / total_queries;
        // Convert accumulated microseconds back to milliseconds for the public API.
        stats.avg_latency_ms = static_cast<double>(
            total_query_latency_us_.load(std::memory_order_relaxed)) /
            total_queries / 1000.0;
    }
    
    // Read the in-memory counters maintained by put() / clearExpired() / clear().
    // This avoids the O(N) RocksDB key-scan that the previous implementation required.
    stats.total_entries    = entry_count_.load(std::memory_order_relaxed);
    stats.total_size_bytes = total_bytes_.load(std::memory_order_relaxed);
    
    return stats;
}

uint64_t SemanticCache::clearExpired() {
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it(
        cf_handle_ ? db_->NewIterator(read_opts, cf_handle_) : db_->NewIterator(read_opts)
    );
    
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
            auto entry = CacheEntry::fromJson(j);
            
            if (entry && isExpired(*entry)) {
                if (cf_handle_) batch.Delete(cf_handle_, it->key());
                else batch.Delete(it->key());
                removed++;
            }
        } catch (const std::exception&) {
            // Invalid entry, remove it
            if (cf_handle_) batch.Delete(cf_handle_, it->key());
            else batch.Delete(it->key());
            removed++;
        }
    }
    
    if (removed > 0) {
        rocksdb::WriteOptions write_opts;
        db_->Write(write_opts, &batch);
        // Saturating fetch_sub: atomically decrement entry_count_ and clamp at zero.
        // Concurrent put() calls may temporarily make the counter drift; it is
        // self-correcting and is only used for statistics, not for correctness.
        uint64_t prev = entry_count_.fetch_sub(removed, std::memory_order_relaxed);
        if (prev < removed) {
            // Wrapped below zero — restore to zero.  A concurrent put() between
            // the fetch_sub and this store is benign: it would have re-incremented
            // the counter and the store would incorrectly reset it.  We accept this
            // rare statistical inaccuracy rather than incurring a CAS loop here.
            entry_count_.store(0, std::memory_order_relaxed);
        }
    }
    
    return removed;
}

bool SemanticCache::clear() {
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it(
        cf_handle_ ? db_->NewIterator(read_opts, cf_handle_) : db_->NewIterator(read_opts)
    );
    
    // Check for null iterator before use
    if (!it) {
        THEMIS_ERROR("Failed to create iterator for semantic cache clearing");
        return false; // Return failure on iterator creation failure
    }
    
    rocksdb::WriteBatch batch;
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (cf_handle_) batch.Delete(cf_handle_, it->key());
        else batch.Delete(it->key());
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
