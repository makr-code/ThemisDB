/**
 * @file rocksdb_token_blacklist.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class RocksDBTokenBlacklist final : public ITokenBlacklist {
public:
    struct Config {
        std::string db_path;
        std::string column_family = "token_blacklist";
        uint32_t purge_interval_seconds = 300;
    };

    /**
     * @brief Rocks DBToken Blacklist.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit RocksDBTokenBlacklist(const Config& config);

    ~RocksDBTokenBlacklist() override;

    RocksDBTokenBlacklist(const RocksDBTokenBlacklist&) = delete;
    RocksDBTokenBlacklist& operator=(const RocksDBTokenBlacklist&) = delete;

    // -----------------------------------------------------------------------
    // ITokenBlacklist interface
    // -----------------------------------------------------------------------

    void add(const std::string& jti,
             std::chrono::system_clock::time_point expiry) override;

    bool isRevoked(const std::string& jti) const override;

    void purgeExpired() override;

private:
    Config config_;

    // RocksDB state
    // DB ownership is managed with raw pointer; destruction handled manually.
    rocksdb::DB* db_{nullptr};
    rocksdb::ColumnFamilyHandle* cf_{nullptr};

    // Background purge thread
    std::thread              purge_thread_;
    std::atomic<bool>        running_{false};
    std::mutex               cv_mutex_;
    std::condition_variable  cv_;

    std::vector<rocksdb::ColumnFamilyHandle*> other_cf_handles_;

    /**
     * @brief Purge Loop.
     */
    void purgeLoop();

    /**
     * @brief Encode Expiry.
     * @param[in] tp Input parameter.
     * @return Return value.
     */
    static std::string encodeExpiry(std::chrono::system_clock::time_point tp);

    /**
     * @brief Decode Expiry.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    static std::chrono::system_clock::time_point decodeExpiry(const std::string& val);
};

} // namespace auth
} // namespace themis
