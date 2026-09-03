/**
 * @file rocksdb_token_blacklist.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/token_blacklist.h"

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <memory>

// RocksDB forward declarations
namespace rocksdb {
    class DB;
    class ColumnFamilyHandle;
    struct Options;
    struct ReadOptions;
    struct WriteOptions;
}

namespace themis {
namespace auth {

/**
 * @brief RocksDB-backed token blacklist for single-node persistent deployments.
 *
 * Each revoked JTI is stored as a key/value pair in a dedicated RocksDB column
 * family:
 *   Key:   <jti>
 *   Value: <expiry as int64 seconds-since-epoch, big-endian>
 *
 * A background thread wakes every purge_interval_seconds to iterate the column
 * family and delete expired entries, allowing RocksDB to reclaim space over
 * time through its normal compaction mechanisms.
 *
 * Durability contract: entries survive process restart because RocksDB writes
 * use the default WriteOptions (WAL enabled).
 *
 * Thread-safety: all public methods are thread-safe.
 */
class RocksDBTokenBlacklist final : public ITokenBlacklist {
public:
    struct Config {
        /// Path to the RocksDB database directory.
        std::string db_path;
        /// Name of the dedicated column family for the blacklist.
        std::string column_family = "token_blacklist";
        /// How often the background thread purges expired entries (seconds).
        uint32_t purge_interval_seconds = 300;
    };

    /**
     * @brief Open (or create) the RocksDB database and start the background
     *        purge thread.
     *
     * @throws std::runtime_error if the database cannot be opened.
     */
    explicit RocksDBTokenBlacklist(const Config& config);

    /**
     * @brief Stop the background purge thread and close the database.
     */
    ~RocksDBTokenBlacklist() override;

    RocksDBTokenBlacklist(const RocksDBTokenBlacklist&) = delete;
    RocksDBTokenBlacklist& operator=(const RocksDBTokenBlacklist&) = delete;

    // -----------------------------------------------------------------------
    // ITokenBlacklist interface
    // -----------------------------------------------------------------------

    /**
     * @brief Persist a JTI revocation to RocksDB.
     *
     * The expiry timestamp is stored alongside the JTI so the background
     * purge thread can reclaim space without relying on TTL support.
     */
    void add(const std::string& jti,
             std::chrono::system_clock::time_point expiry) override;

    /**
     * @brief Check revocation by reading from RocksDB and verifying expiry.
     *
     * @return true if the JTI is present and its stored expiry is in the future.
     */
    bool isRevoked(const std::string& jti) const override;

    /**
     * @brief Delete all entries whose expiry timestamp is in the past.
     *
     * Also called by the background purge thread; safe to call manually.
     */
    void purgeExpired() override;

private:
    Config config_;

    // RocksDB state
    // The RocksDB C++ API expects a raw DB* out-parameter for Open()/OpenForReadOnly()
    // and owns the DB lifetime through a unique_ptr after the handle is created.
    std::unique_ptr<rocksdb::DB> db_{nullptr};
    rocksdb::ColumnFamilyHandle* cf_{nullptr};

    // Background purge thread
    std::thread              purge_thread_;
    std::atomic<bool>        running_{false};
    std::mutex               cv_mutex_;
    std::condition_variable  cv_;

    /// Other CF handles that must be kept open until DB is closed.
    std::vector<rocksdb::ColumnFamilyHandle*> other_cf_handles_;

    void purgeLoop();

    /// Encode expiry as 8-byte big-endian int64 (seconds since Unix epoch).
    static std::string encodeExpiry(std::chrono::system_clock::time_point tp);

    /// Decode an expiry value produced by encodeExpiry().
    static std::chrono::system_clock::time_point decodeExpiry(const std::string& val);
};

} // namespace auth
} // namespace themis
