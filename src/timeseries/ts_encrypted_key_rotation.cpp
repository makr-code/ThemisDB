/**
 * @file ts_encrypted_key_rotation.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/ts_encrypted_key_rotation.h"
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/write_batch.h>
#include <spdlog/spdlog.h>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

TsEncryptedKeyRotation::TsEncryptedKeyRotation(
    rocksdb::TransactionDB*              db,
    rocksdb::ColumnFamilyHandle*         cf,
    std::shared_ptr<EncryptedChunkStore> enc_store,
    Config                               config)
    : db_(db)
    , cf_(cf)
    , enc_store_(std::move(enc_store))
    , config_(config)
{
    if (!db_) {
        throw std::invalid_argument("TsEncryptedKeyRotation: db must not be null");
    }
    if (!enc_store_) {
        throw std::invalid_argument("TsEncryptedKeyRotation: enc_store must not be null");
    }
}

TsEncryptedKeyRotation::~TsEncryptedKeyRotation()
{
    stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// start / stop
// ─────────────────────────────────────────────────────────────────────────────

void TsEncryptedKeyRotation::start()
{
    if (running_.exchange(true, std::memory_order_acq_rel)) {
        return; // already running
    }

    {
        std::lock_guard<std::mutex> lk(cv_mu_);
        stop_flag_ = false;
    }

    thread_ = std::thread(&TsEncryptedKeyRotation::rotationLoop, this);
}

void TsEncryptedKeyRotation::stop()
{
    {
        std::lock_guard<std::mutex> lk(cv_mu_);
        stop_flag_ = true;
    }
    cv_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }
    running_.store(false, std::memory_order_release);
}

// ─────────────────────────────────────────────────────────────────────────────
// rotationLoop
// ─────────────────────────────────────────────────────────────────────────────

void TsEncryptedKeyRotation::rotationLoop()
{
    while (true) {
        {
            std::unique_lock<std::mutex> lk(cv_mu_);
            bool stopped = cv_.wait_for(lk, config_.check_interval,
                                        [this] { return stop_flag_; });
            if (stopped) {
              break;
            }
        }

        try {
            size_t n = runOnce();
            if (n > 0) {
                spdlog::info("TsEncryptedKeyRotation: re-encrypted {} chunks", n);
            }
        } catch (const std::exception& e) {
            spdlog::error("TsEncryptedKeyRotation: error during rotation pass: {}", e.what());
        } catch (...) {
            spdlog::error("TsEncryptedKeyRotation: unknown error during rotation pass");
        }
    }

    running_.store(false, std::memory_order_release);
}

// ─────────────────────────────────────────────────────────────────────────────
// runOnce
// ─────────────────────────────────────────────────────────────────────────────

size_t TsEncryptedKeyRotation::runOnce()
{
    // Determine the current master key_id without performing any encryption.
    std::string current_key_id = enc_store_->getCurrentKeyId();
    if (current_key_id.empty()) {
        return 0;
    }

    // Scan all "tsc:" prefixed keys.
    rocksdb::ReadOptions read_opts;
    read_opts.fill_cache = false;
    std::unique_ptr<rocksdb::Iterator> it(
        cf_ ? db_->NewIterator(read_opts, cf_)
            : db_->NewIterator(read_opts));

    const std::string prefix = GORILLA_CHUNK_PREFIX;
    const std::string end_marker = std::string(GORILLA_CHUNK_PREFIX) + "\xFF";

    size_t reencrypted = 0;
    it->Seek(prefix);

    while (it->Valid()) {
        std::string key = it->key().ToString();
        if (key >= end_marker || key.compare(0, prefix.size(), prefix) != 0) {
          break;
        }

        try {
            nlohmann::json chunk_meta = nlohmann::json::parse(it->value().ToString());

            // Only process encrypted chunks that are using a stale key.
            if (!chunk_meta.contains("encryption") ||
                chunk_meta["encryption"] != ENCRYPTED_MARKER) {
                it->Next();
                continue;
            }

            std::string stored_key_id = chunk_meta.value("key_id", "");
            if (stored_key_id == current_key_id) {
                // Already encrypted with the current key — skip.
                it->Next();
                continue;
            }

            // Extract series_id from chunk key:
            //   tsc:{metric}:{entity}:{first_ts}:{last_ts}
            size_t pos1 = prefix.size();
            size_t pos2 = key.find(':', pos1);
            size_t pos3 = (pos2 != std::string::npos) ? key.find(':', pos2 + 1) : std::string::npos;
            if (pos2 == std::string::npos || pos3 == std::string::npos) {
                it->Next();
                continue;
            }
            std::string series_id = key.substr(pos1, pos3 - pos1);

            // Extract chunk range for audit log.
            size_t pos4 = key.find(':', pos3 + 1);
            if (pos4 == std::string::npos) {
                // Malformed chunk key — skip.
                spdlog::warn("TsEncryptedKeyRotation: malformed chunk key (missing last_ts): {}", key);
                it->Next();
                continue;
            }
            std::string chunk_range = "[" + key.substr(pos3 + 1, pos4 - pos3 - 1)
                                    + "," + key.substr(pos4 + 1) + "]";

            // Decrypt with old key.
            std::vector<uint8_t> encrypted_data = {};

            if (chunk_meta.contains("data") && chunk_meta["data"].is_binary()) {
                encrypted_data = chunk_meta["data"].get<std::vector<uint8_t>>();
            } else if (chunk_meta.contains("data") && chunk_meta["data"].is_array()) {
                encrypted_data = chunk_meta["data"].get<std::vector<uint8_t>>();
            } else if (chunk_meta.contains("data") && chunk_meta["data"].is_object() &&
                       chunk_meta["data"].contains("bytes") &&
                       chunk_meta["data"]["bytes"].is_array()) {
                encrypted_data = chunk_meta["data"]["bytes"].get<std::vector<uint8_t>>();
            } else {
                throw std::runtime_error("Unsupported encrypted chunk data encoding");
            }
            std::vector<uint8_t> plaintext =
                enc_store_->decryptChunk(series_id, encrypted_data, chunk_range);

            // Re-encrypt with the current key.
            // encryptChunk() fetches the key in a single call and returns the
            // key_id it actually used, eliminating any race between key snapshot
            // and encryption that could make chunk_meta["key_id"] inconsistent.
            auto enc_result =
                enc_store_->encryptChunk(series_id, plaintext, chunk_range);

            // Update the JSON envelope with the key_id from the fresh encryption.
            chunk_meta["key_id"] = enc_result.key_id;
            chunk_meta["data"]   = nlohmann::json::binary(enc_result.blob);

            std::string new_value = chunk_meta.dump();

            // Write atomically — readers holding the old snapshot are unaffected.
            rocksdb::WriteOptions wo;
            rocksdb::Status s;
            if (cf_) {
                rocksdb::WriteBatch batch;
                batch.Put(cf_, key, new_value);
                s = db_->Write(wo, &batch);
            } else {
                rocksdb::WriteBatch batch;
                batch.Put(key, new_value);
                s = db_->Write(wo, &batch);
            }

            if (s.ok()) {
                ++reencrypted;
                total_reencrypted_.fetch_add(1, std::memory_order_relaxed);
            } else {
                spdlog::warn("TsEncryptedKeyRotation: failed to write re-encrypted chunk {}: {}",
                             key, s.ToString());
            }

        } catch (const std::exception& e) {
            spdlog::warn("TsEncryptedKeyRotation: error processing chunk {}: {}", key, e.what());
        }

        if (config_.max_chunks_per_pass > 0 && reencrypted >= config_.max_chunks_per_pass) {
            break;
        }

        it->Next();
    }

    return reencrypted;
}

} // namespace themis


