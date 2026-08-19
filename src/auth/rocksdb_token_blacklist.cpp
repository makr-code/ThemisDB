/**
 * @file rocksdb_token_blacklist.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/rocksdb_token_blacklist.h"

#include <chrono>
#include <cstring>
#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/write_batch.h>
#include <stdexcept>

#include "utils/logger.h"

namespace themis {
namespace auth {

// ============================================================================
// Expiry encoding helpers
// ============================================================================

std::string RocksDBTokenBlacklist::encodeExpiry(std::chrono::system_clock::time_point tp) {
    int64_t secs
        = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count());

    // Store as big-endian 8 bytes so byte-wise ordering matches time ordering
    std::string buf(8, '\0');
    for (int i = 7; i >= 0; --i) {
        buf[i] = static_cast<char>(secs & 0xFF);
        secs >>= 8;
    }
    return buf;
}

std::chrono::system_clock::time_point RocksDBTokenBlacklist::decodeExpiry(const std::string &val) {
    if (val.size() < 8) {
        return std::chrono::system_clock::time_point{};
    }
    int64_t secs = 0;
    for (int i = 0; i < 8; ++i) {
        secs = (secs << 8) | static_cast<uint8_t>(val[i]);
    }
    return std::chrono::system_clock::time_point{std::chrono::seconds{secs}};
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

RocksDBTokenBlacklist::RocksDBTokenBlacklist(const Config &config) : config_(config) {
    if (config_.db_path.empty()) {
        throw std::invalid_argument("RocksDBTokenBlacklist: db_path must not be empty");
    }

    // -----------------------------------------------------------------------
    // Open the DB; create the dedicated column family if it does not exist
    // -----------------------------------------------------------------------
    rocksdb::Options opts;
    opts.create_if_missing              = true;
    opts.create_missing_column_families = true;

    // Determine which column families exist in the on-disk database.
    // On a brand-new database, ListColumnFamilies will return "path not found"
    // or a similar error — we fall back to the "default" CF only.
    std::vector<std::string> existing_cfs;
    {
        rocksdb::Status ls = rocksdb::DB::ListColumnFamilies(rocksdb::DBOptions{opts}, config_.db_path, &existing_cfs);
        if (!ls.ok()) {
            existing_cfs = {"default"};
        }
    }
    if (existing_cfs.empty()) {
        existing_cfs = {"default"};
    }

    // Ensure our blacklist CF is in the open request so it gets created if absent.
    bool has_blacklist_cf = false;
    for (const auto &cf : existing_cfs) {
        if (cf == config_.column_family) {
            has_blacklist_cf = true;
            break;
        }
    }
    if (!has_blacklist_cf) {
        existing_cfs.push_back(config_.column_family);
    }

    std::vector<rocksdb::ColumnFamilyDescriptor> cf_descs;
    cf_descs.reserve(existing_cfs.size());
    for (const auto &cf : existing_cfs) {
        cf_descs.emplace_back(cf, rocksdb::ColumnFamilyOptions{});
    }

    std::vector<rocksdb::ColumnFamilyHandle *> cf_handles;
    rocksdb::DB* db_raw = nullptr;
    rocksdb::Status s = rocksdb::DB::Open(rocksdb::DBOptions{opts}, config_.db_path, cf_descs, &cf_handles, &db_raw);
    db_.reset(db_raw);
    if (!s.ok()) {
        throw std::runtime_error("RocksDBTokenBlacklist: failed to open DB at '" + config_.db_path
                                 + "': " + s.ToString());
    }

    // Identify the blacklist CF handle; keep all others for proper cleanup.
    for (size_t i = 0; i < existing_cfs.size(); ++i) {
        if (existing_cfs[i] == config_.column_family) {
            cf_ = cf_handles[i];
        } else {
            other_cf_handles_.push_back(cf_handles[i]);
        }
    }

    if (!cf_) {
        // Unexpected: CF should have been created by create_missing_column_families.
        for (auto *h : other_cf_handles_) {
            db_->DestroyColumnFamilyHandle(h);
        }
        other_cf_handles_.clear();
        db_.reset();
        throw std::runtime_error("RocksDBTokenBlacklist: blacklist CF '" + config_.column_family
                                 + "' not found after open");
    }

    THEMIS_INFO("RocksDBTokenBlacklist: opened DB at '{}' (CF '{}')", config_.db_path, config_.column_family);

    // Start background purge thread
    running_.store(true);
    purge_thread_ = std::thread([this] { purgeLoop(); });
}

RocksDBTokenBlacklist::~RocksDBTokenBlacklist() {
    // Signal and join the background thread
    {
        std::lock_guard<std::mutex> lk(cv_mutex_);
        running_.store(false);
    }
    cv_.notify_all();
    if (purge_thread_.joinable()) {
        purge_thread_.join();
    }

    // Close DB: destroy all CF handles first, then delete the DB pointer.
    if (cf_) {
        db_->DestroyColumnFamilyHandle(cf_);
        cf_ = nullptr;
    }
    for (auto *h : other_cf_handles_) {
        db_->DestroyColumnFamilyHandle(h);
    }
    other_cf_handles_.clear();
    if (db_) {
        db_.reset();
    }

    THEMIS_INFO("RocksDBTokenBlacklist: closed DB at '{}'", config_.db_path);
}

// ============================================================================
// ITokenBlacklist interface
// ============================================================================

void RocksDBTokenBlacklist::add(const std::string &jti, std::chrono::system_clock::time_point expiry) {
    if (jti.empty()) {
        return;
    }

    rocksdb::WriteOptions wo;
    wo.sync = false; // WAL is still active; sync=false defers fsync for perf

    rocksdb::Status s = db_->Put(wo, cf_, rocksdb::Slice(jti), rocksdb::Slice(encodeExpiry(expiry)));
    if (!s.ok()) {
        THEMIS_WARN("RocksDBTokenBlacklist::add: Put failed for JTI '{}': {}", jti, s.ToString());
    } else {
        THEMIS_DEBUG("RocksDBTokenBlacklist: revoked JTI '{}'", jti);
    }
}

bool RocksDBTokenBlacklist::isRevoked(const std::string &jti) const {
    if (jti.empty()) {
        return false;
    }

    std::string value;
    rocksdb::ReadOptions ro;
    rocksdb::Status s = db_->Get(ro, cf_, rocksdb::Slice(jti), &value);

    if (s.IsNotFound()) {
        return false;
    }
    if (!s.ok()) {
        THEMIS_WARN("RocksDBTokenBlacklist::isRevoked: Get failed for JTI '{}': {}", jti, s.ToString());
        return false;
    }

    auto expiry = decodeExpiry(value);
    return expiry > std::chrono::system_clock::now();
}

void RocksDBTokenBlacklist::purgeExpired() {
    auto now = std::chrono::system_clock::now();

    // Use a chunk size to bound WriteBatch memory usage and write-stall risk for
    // large column families.
    static constexpr size_t kChunkSize = 1000;

    rocksdb::ReadOptions ro;
    rocksdb::WriteOptions wo;
    rocksdb::WriteBatch batch;
    size_t pending = 0;

    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(ro, cf_));
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        auto expiry = decodeExpiry(it->value().ToString());
        if (expiry <= now) {
            batch.Delete(cf_, it->key());
            ++pending;

            if (pending >= kChunkSize) {
                rocksdb::Status s = db_->Write(wo, &batch);
                if (!s.ok()) {
                    THEMIS_WARN("RocksDBTokenBlacklist::purgeExpired: Write failed: {}", s.ToString());
                }
                batch.Clear();
                pending = 0;
            }
        }
    }

    if (!it->status().ok()) {
        THEMIS_WARN("RocksDBTokenBlacklist::purgeExpired: iterator error: {}", it->status().ToString());
        return;
    }

    // Flush any remaining deletes.
    if (pending > 0) {
        rocksdb::Status s = db_->Write(wo, &batch);
        if (!s.ok()) {
            THEMIS_WARN("RocksDBTokenBlacklist::purgeExpired: Write failed: {}", s.ToString());
            return;
        }
    }

    THEMIS_DEBUG("RocksDBTokenBlacklist: purgeExpired completed");
}

// ============================================================================
// Background purge thread
// ============================================================================

void RocksDBTokenBlacklist::purgeLoop() {
    while (running_.load()) {
        std::unique_lock<std::mutex> lk(cv_mutex_);
        cv_.wait_for(lk, std::chrono::seconds(config_.purge_interval_seconds), [this] { return !running_.load(); });

        if (!running_.load()) {
            break;
        }
        lk.unlock();

        THEMIS_DEBUG("RocksDBTokenBlacklist: running scheduled purge");
        purgeExpired();
    }
}

} // namespace auth
} // namespace themis
