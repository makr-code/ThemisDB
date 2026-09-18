/**
 * @file change_stream_compressor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * Change stream compression for high-volume CDC feeds.
 *
 * Provides batch compression of CDC change events for efficient transport
 * over high-volume SSE or WebSocket streams.  A batch of ChangeEvent records
 * is serialised to a JSON array and compressed with Zstandard before being
 * handed to the transport layer; the transport writes the opaque bytes and the
 * consumer decompresses them before delivering individual events.
 *
 * Wire format (CompressedBatch::serialize()):
 * @code
 * +----------+---------+-----------+---------------+-------------+---...---+
 * | magic    | version | algorithm | original_size | event_count | payload |
 * | 4 bytes  | 1 byte  | 1 byte    | 4 bytes LE    | 4 bytes LE  | N bytes |
 * +----------+---------+-----------+---------------+-------------+---...---+
 * @endcode
 *
 * magic        = 0x43 0x44 0x43 0x5A ("CDCZ")
 * version      = 0x01 (current)
 * algorithm    = 0x00 (NONE / uncompressed) | 0x01 (ZSTD)
 * original_size = byte-length of the uncompressed JSON payload
 * event_count   = number of ChangeEvent records in the batch
 * payload       = zstd-compressed UTF-8 JSON array (or raw JSON when algorithm=NONE)
 *
 * When the raw JSON is smaller than Config::min_compression_size_bytes the
 * batch is stored uncompressed (algorithm = NONE) to avoid overhead.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"
#include "utils/zstd_codec.h"
#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace cdc {

// ── Wire-format constants ─────────────────────────────────────────────────────

inline constexpr uint8_t kCdcBatchMagic[4] = {0x43, 0x44, 0x43, 0x5A}; // "CDCZ"
inline constexpr uint8_t kCdcBatchVersion   = 0x01;

// ── Algorithm identifiers ─────────────────────────────────────────────────────

enum class StreamCompressionAlgorithm : uint8_t {
    NONE = 0x00, ///< Uncompressed (raw JSON bytes)
    ZSTD = 0x01, ///< Zstandard (default)
};

// ── CompressedBatch ───────────────────────────────────────────────────────────

struct CompressedBatch {
    uint8_t                    version       = kCdcBatchVersion;
    StreamCompressionAlgorithm algorithm     = StreamCompressionAlgorithm::ZSTD;
    uint32_t                   original_size = 0; ///< Uncompressed payload length
    uint32_t                   event_count   = 0; ///< Number of events in the batch
    std::vector<uint8_t>       payload;           ///< Compressed (or raw) event bytes

    std::vector<uint8_t> serialize() const {
        constexpr size_t kHeaderSize = 4 + 1 + 1 + 4 + 4; // 14 bytes
        std::vector<uint8_t> out = {};

        out.reserve(kHeaderSize + payload.size());

        // magic
        out.insert(out.end(), std::begin(kCdcBatchMagic), std::end(kCdcBatchMagic));
        // version
        out.push_back(version);
        // algorithm
        out.push_back(static_cast<uint8_t>(algorithm));
        // original_size (little-endian)
        out.push_back(static_cast<uint8_t>(original_size & 0xFF));
        out.push_back(static_cast<uint8_t>((original_size >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>((original_size >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((original_size >> 24) & 0xFF));
        // event_count (little-endian)
        out.push_back(static_cast<uint8_t>(event_count & 0xFF));
        out.push_back(static_cast<uint8_t>((event_count >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>((event_count >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((event_count >> 24) & 0xFF));
        // payload
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }

    /**
     * @brief Deserialize.
     * @param[in] bytes Input parameter.
     * @return Return value.
     * @details Calls: size(), assign(), begin(), end().
     */
    static std::optional<CompressedBatch> deserialize(const std::vector<uint8_t>& bytes) {
        constexpr size_t kHeaderSize = 4 + 1 + 1 + 4 + 4; // 14 bytes
        if (bytes.size() < kHeaderSize) {
            return std::nullopt;
        }
        // Validate magic
        if (bytes[0] != kCdcBatchMagic[0] || bytes[1] != kCdcBatchMagic[1] ||
            bytes[2] != kCdcBatchMagic[2] || bytes[3] != kCdcBatchMagic[3]) {
            return std::nullopt;
        }
        CompressedBatch batch;
        batch.version   = bytes[4];
        batch.algorithm = static_cast<StreamCompressionAlgorithm>(bytes[5]);
        batch.original_size =
            static_cast<uint32_t>(bytes[6])        |
            (static_cast<uint32_t>(bytes[7])  << 8) |
            (static_cast<uint32_t>(bytes[8])  << 16)|
            (static_cast<uint32_t>(bytes[9])  << 24);
        batch.event_count =
            static_cast<uint32_t>(bytes[10])        |
            (static_cast<uint32_t>(bytes[11]) << 8) |
            (static_cast<uint32_t>(bytes[12]) << 16)|
            (static_cast<uint32_t>(bytes[13]) << 24);
        batch.payload.assign(bytes.begin() + kHeaderSize, bytes.end());
        return batch;
    }
};

// ── ChangeStreamCompressor ────────────────────────────────────────────────────

class ChangeStreamCompressor {
public:
    // ── Configuration ────────────────────────────────────────────────────────

    struct Config {
        StreamCompressionAlgorithm algorithm = StreamCompressionAlgorithm::ZSTD;
        int    level                         = 3;    ///< Zstd compression level (1–22)
        size_t min_compression_size_bytes    = 256;  ///< Skip compression for smaller payloads
    };

    // ── Statistics ────────────────────────────────────────────────────────────

    struct Stats {
        uint64_t batches_compressed  = 0; ///< Total compress() calls
        uint64_t batches_skipped     = 0; ///< Batches stored uncompressed (below threshold)
        uint64_t events_compressed   = 0; ///< Total events processed by compress()
        uint64_t bytes_in            = 0; ///< Total uncompressed JSON bytes submitted
        uint64_t bytes_out           = 0; ///< Total compressed payload bytes produced
        uint64_t batches_decompressed = 0;///< Total decompress() calls
        uint64_t decompress_errors   = 0; ///< Failed decompress() calls

        double compression_ratio() const noexcept {
            return (bytes_out > 0) ? static_cast<double>(bytes_in) / static_cast<double>(bytes_out)
                                   : 1.0;
        }
    };

    // ── Constructor / destructor ──────────────────────────────────────────────

    explicit ChangeStreamCompressor(Config config = Config{});  // defined below

    // Non-copyable (atomics), movable.
    ChangeStreamCompressor(const ChangeStreamCompressor&) = delete;
    ChangeStreamCompressor& operator=(const ChangeStreamCompressor&) = delete;

    /**
     * @brief ── Compression ───────────────────────────────────────────────────────────
     * @param[in] events Input parameter.
     * @return Return value.
     * @details Calls: lk(), nlohmann::json::array(), push_back(), toJson(), dump(), size(), fetch_add(), utils::zstd_compress().
     */

    CompressedBatch compress(const std::vector<Changefeed::ChangeEvent>& events) {
        // Snapshot config under the lock to avoid races with setConfig().
        const Config cfg = [&] {
            std::lock_guard<std::mutex> lk(config_mutex_);
            return config_;
        }();

        // Build JSON payload
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& ev : events) {
            arr.push_back(ev.toJson());
        }
        const std::string json_str = arr.dump();

        CompressedBatch batch;
        batch.event_count   = static_cast<uint32_t>(events.size());
        batch.original_size = static_cast<uint32_t>(json_str.size());

        // Update input stats
        stats_batches_compressed_.fetch_add(1, std::memory_order_relaxed);
        stats_events_compressed_.fetch_add(events.size(), std::memory_order_relaxed);
        stats_bytes_in_.fetch_add(json_str.size(), std::memory_order_relaxed);

        // Decide whether to compress
        if (cfg.algorithm == StreamCompressionAlgorithm::ZSTD &&
            json_str.size() >= cfg.min_compression_size_bytes) {
            auto compressed = utils::zstd_compress(json_str, cfg.level);
            if (!compressed.empty()) {
                batch.algorithm = StreamCompressionAlgorithm::ZSTD;
                batch.payload   = std::move(compressed);
                stats_bytes_out_.fetch_add(batch.payload.size(), std::memory_order_relaxed);
                return batch;
            }
            // Compression failed — fall through to uncompressed path
        }

        // Uncompressed fallback
        batch.algorithm = StreamCompressionAlgorithm::NONE;
        batch.payload.assign(json_str.begin(), json_str.end());
        stats_bytes_out_.fetch_add(batch.payload.size(), std::memory_order_relaxed);
        stats_batches_skipped_.fetch_add(1, std::memory_order_relaxed);
        return batch;
    }

    /**
     * @brief ── Decompression ─────────────────────────────────────────────────────────
     * @param[in] batch Input parameter.
     * @return Return value.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: fetch_add(), utils::zstd_decompress(), empty(), assign(), begin(), end(), reserve(), nlohmann::json::parse().
     */

    std::vector<Changefeed::ChangeEvent> decompress(const CompressedBatch& batch) {
        stats_batches_decompressed_.fetch_add(1, std::memory_order_relaxed);

        std::string json_str = {};

        if (batch.algorithm == StreamCompressionAlgorithm::ZSTD) {
            auto decompressed = utils::zstd_decompress(batch.payload);
            if (decompressed.empty() && batch.original_size > 0) {
                stats_decompress_errors_.fetch_add(1, std::memory_order_relaxed);
                throw std::runtime_error("ChangeStreamCompressor: zstd decompression failed");
            }
            json_str.assign(decompressed.begin(), decompressed.end());
        } else {
            // NONE: payload is raw JSON
            json_str.assign(batch.payload.begin(), batch.payload.end());
        }

        std::vector<Changefeed::ChangeEvent> events;
        events.reserve(batch.event_count);

        try {
            const auto arr = nlohmann::json::parse(json_str);
            if (!arr.is_array()) {
                stats_decompress_errors_.fetch_add(1, std::memory_order_relaxed);
                throw std::runtime_error("ChangeStreamCompressor: expected JSON array");
            }
            for (const auto& item : arr) {
                events.push_back(Changefeed::ChangeEvent::fromJson(item));
            }
        } catch (const nlohmann::json::exception& e) {
            stats_decompress_errors_.fetch_add(1, std::memory_order_relaxed);
            throw std::runtime_error(
                std::string("ChangeStreamCompressor: JSON parse error: ") + e.what());
        }

        return events;
    }

    // ── Statistics ────────────────────────────────────────────────────────────

    Stats getStats() const noexcept {
        Stats s;
        s.batches_compressed   = stats_batches_compressed_.load(std::memory_order_relaxed);
        s.batches_skipped      = stats_batches_skipped_.load(std::memory_order_relaxed);
        s.events_compressed    = stats_events_compressed_.load(std::memory_order_relaxed);
        s.bytes_in             = stats_bytes_in_.load(std::memory_order_relaxed);
        s.bytes_out            = stats_bytes_out_.load(std::memory_order_relaxed);
        s.batches_decompressed = stats_batches_decompressed_.load(std::memory_order_relaxed);
        s.decompress_errors    = stats_decompress_errors_.load(std::memory_order_relaxed);
        return s;
    }

    void resetStats() noexcept {
        stats_batches_compressed_.store(0, std::memory_order_relaxed);
        stats_batches_skipped_.store(0, std::memory_order_relaxed);
        stats_events_compressed_.store(0, std::memory_order_relaxed);
        stats_bytes_in_.store(0, std::memory_order_relaxed);
        stats_bytes_out_.store(0, std::memory_order_relaxed);
        stats_batches_decompressed_.store(0, std::memory_order_relaxed);
        stats_decompress_errors_.store(0, std::memory_order_relaxed);
    }

    // ── Configuration ─────────────────────────────────────────────────────────

    Config getConfig() const {
        /**
         * @brief Lock.
         * @param[in] config_mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(config_mutex_);
        return config_;
    }

    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Calls: lock().
     */
    void setConfig(const Config& config) {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_ = config;
    }

private:
    Config config_;
    mutable std::mutex config_mutex_; ///< Guards config_ updates

    // Atomic stats — updated without holding config_mutex_
    std::atomic<uint64_t> stats_batches_compressed_{0};
    std::atomic<uint64_t> stats_batches_skipped_{0};
    std::atomic<uint64_t> stats_events_compressed_{0};
    std::atomic<uint64_t> stats_bytes_in_{0};
    std::atomic<uint64_t> stats_bytes_out_{0};
    std::atomic<uint64_t> stats_batches_decompressed_{0};
    std::atomic<uint64_t> stats_decompress_errors_{0};
};

} // namespace cdc
} // namespace themis

// Out-of-line constructor definition: required to avoid GCC "default member
// initializer … required before end of its enclosing class" diagnostic that
// is triggered when a nested struct with defaulted members is used as a
// default argument inside the class body.
namespace themis {
namespace cdc {
/**
 * @brief Change Stream Compressor.
 * @param[in] config Input parameter.
 * @return Return value.
 */
inline ChangeStreamCompressor::ChangeStreamCompressor(Config config)
    : config_(std::move(config)) {}
} // namespace cdc
} // namespace themis
