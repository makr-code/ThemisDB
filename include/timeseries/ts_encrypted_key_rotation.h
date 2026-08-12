/**
 * @file ts_encrypted_key_rotation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "timeseries/encrypted_chunk_store.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace rocksdb {
class TransactionDB;
class ColumnFamilyHandle;
} // namespace rocksdb

namespace themis {

/**
 * @brief Configuration for TsEncryptedKeyRotation.
 */
struct TsEncryptedKeyRotationConfig {
    /// How often the worker polls for stale chunks.
    std::chrono::seconds check_interval{3600};
    /// Maximum chunks to re-encrypt per rotation pass (0 = unlimited).
    size_t max_chunks_per_pass{0};
};

/**
 * @brief Background key-rotation worker for encrypted time-series chunks.
 *
 * TsEncryptedKeyRotation scans all encrypted Gorilla chunks stored under the
 * "tsc:" key prefix, identifies any chunk whose embedded key_id no longer
 * matches the current master key, and re-encrypts those chunks in-place using
 * the new key.
 *
 * The rotation is non-blocking with respect to reads:
 *   1. A new, freshly encrypted value is written atomically (RocksDB Put).
 *   2. Concurrent readers that already hold the old value in their RocksDB
 *      iterator snapshot continue to use it until they release the snapshot.
 *   3. No chunk is ever deleted before its replacement is committed.
 *
 * The background thread runs on a configurable interval and stops gracefully
 * when stop() is called.
 *
 * Usage:
 * @code
 *   TsEncryptedKeyRotation rotator(db, cf, enc_store,
 *       TsEncryptedKeyRotationConfig{
 *           .check_interval = std::chrono::minutes(60)
 *       });
 *   rotator.start();
 *   // ... application runs ...
 *   rotator.stop();
 * @endcode
 */
class TsEncryptedKeyRotation {
public:
    /// Alias kept for backwards compatibility.
    using Config = TsEncryptedKeyRotationConfig;

    /**
     * @brief Construct the rotator.
     *
     * @param db           RocksDB TransactionDB (non-owning).
     * @param cf           Column family handle (nullptr = default CF).
     * @param enc_store    Shared EncryptedChunkStore that owns key callbacks.
     * @param config       Rotation configuration.
     */
    TsEncryptedKeyRotation(rocksdb::TransactionDB*               db,
                           rocksdb::ColumnFamilyHandle*           cf,
                           std::shared_ptr<EncryptedChunkStore>  enc_store,
                           Config                                config = Config{});

    ~TsEncryptedKeyRotation();

    // Non-copyable.
    TsEncryptedKeyRotation(const TsEncryptedKeyRotation&)            = delete;
    TsEncryptedKeyRotation& operator=(const TsEncryptedKeyRotation&) = delete;

    /**
     * @brief Start the background rotation worker thread.
     *
     * No-op if the worker is already running.
     */
    void start();

    /**
     * @brief Stop the background worker and join the thread.
     *
     * Blocks until the worker exits.  Safe to call even if start() was never
     * called.
     */
    void stop();

    /**
     * @brief Returns true while the background thread is active.
     */
    bool isRunning() const noexcept { return running_.load(std::memory_order_acquire); }

    /**
     * @brief Run one rotation pass synchronously (for testing / manual trigger).
     *
     * @return Number of chunks re-encrypted.
     */
    size_t runOnce();

    /**
     * @brief Total number of chunks re-encrypted across all passes.
     */
    uint64_t totalReencrypted() const noexcept {
        return total_reencrypted_.load(std::memory_order_relaxed);
    }

private:
    void rotationLoop();

    rocksdb::TransactionDB*              db_;
    rocksdb::ColumnFamilyHandle*         cf_;
    std::shared_ptr<EncryptedChunkStore> enc_store_;
    Config                               config_;

    std::atomic<bool>       running_{false};
    std::thread             thread_;
    std::mutex              cv_mu_;
    std::condition_variable cv_;
    bool                    stop_flag_{false};

    std::atomic<uint64_t>   total_reencrypted_{0};

    static constexpr const char* GORILLA_CHUNK_PREFIX = "tsc:";
    static constexpr const char* ENCRYPTED_MARKER     = "aes-256-gcm";
};

} // namespace themis
