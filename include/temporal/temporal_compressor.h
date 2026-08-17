/**
 * @file temporal_compressor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Data Compressor
 *
 * Compresses historical version payloads stored in SystemVersionedTable
 * using four strategies:
 *
 *  • DELTA    – stores field-level JSON diff between consecutive versions
 *  • ZSTD     – general-purpose LZ-family compressor (inline implementation)
 *  • GORILLA  – XOR-delta encoding for numeric time-series columns
 *  • DICTIONARY – value-table encoding for high-cardinality string columns
 *
 * All algorithms are self-contained and operate purely on `nlohmann::json`
 * payloads, avoiding any external library dependencies.  The `compressHistory`
 * method returns a `CompressionStats` snapshot for observability.
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include "temporal/system_versioned_table.h"
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

namespace themisdb {
namespace temporal {

// ============================================================================
// CompressionAlgorithm
// ============================================================================

/** Discriminator for the compression strategy applied to history payloads. */
enum class CompressionAlgorithm {
    DELTA,      ///< JSON field-level delta between consecutive versions
    ZSTD,       ///< General-purpose LZ-family byte-level compression
    GORILLA,    ///< XOR-delta encoding for numeric (double) columns
    DICTIONARY, ///< Value-table encoding for repeated string fields
    LZ4         ///< LZ4 block compression — high-throughput, low-latency path
};

// ============================================================================
// CompressionConfig
// ============================================================================

/**
 * Configuration for a single compression pass.
 *
 * `delay_before_compression` controls a grace window: versions younger than
 * this age are skipped so that hot / recently-written history is not
 * compressed before it might be needed for conflict resolution.
 */
struct CompressionConfig {
    /// Algorithm to apply.
    CompressionAlgorithm algorithm{CompressionAlgorithm::DELTA};

    /// Compression level (1–9); interpreted per algorithm.
    /// For DELTA and DICTIONARY this is ignored.
    int compression_level{3};

    /// When true, all versions in the range are compressed immediately
    /// regardless of age.  When false, `delay_before_compression` applies.
    bool compress_immediately{false};

    /// Versions younger than this age are left untouched.
    std::chrono::seconds delay_before_compression{24 * 3600};
};

// ============================================================================
// CompressionStats
// ============================================================================

/** Result snapshot from a `compressHistory` call. */
struct CompressionStats {
    /// Number of versions processed (including skipped ones).
    size_t versions_processed{0};

    /// Number of versions that were actually compressed.
    size_t versions_compressed{0};

    /// Number of versions skipped because they were too recent.
    size_t versions_skipped{0};

    /// Sum of original payload sizes in bytes (UTF-8 JSON).
    size_t original_size_bytes{0};

    /// Sum of compressed payload sizes in bytes.
    size_t compressed_size_bytes{0};

    /// Ratio: original / compressed (>1 means space was saved).
    double compression_ratio{1.0};

    /// Wall-clock time for the entire compression pass.
    std::chrono::milliseconds compression_time{0};

    /// Any per-version errors encountered (key → message).
    std::vector<std::pair<std::string, std::string>> errors;

    nlohmann::json toJson() const {
        nlohmann::json err_arr = nlohmann::json::array();
        for (const auto& [k, msg] : errors)
            err_arr.push_back({{"key", k}, {"error", msg}});
        return {
            {"versions_processed",  versions_processed},
            {"versions_compressed", versions_compressed},
            {"versions_skipped",    versions_skipped},
            {"original_size_bytes", original_size_bytes},
            {"compressed_size_bytes", compressed_size_bytes},
            {"compression_ratio",   compression_ratio},
            {"compression_time_ms", compression_time.count()},
            {"errors",              err_arr}
        };
    }
};

// ============================================================================
// TemporalCompressor
// ============================================================================

/**
 * @brief Compresses historical version payloads in a SystemVersionedTable.
 *
 * Usage:
 * ```cpp
 * TemporalCompressor compressor;
 * CompressionConfig cfg;
 * cfg.algorithm = CompressionAlgorithm::DELTA;
 * cfg.compress_immediately = true;
 * auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
 * ```
 *
 * ## Algorithm details
 *
 * ### DELTA
 * For each consecutive pair of historical versions of the same key, the
 * payload is replaced with a JSON patch object:
 * ```json
 * {"__compressed": "delta", "__base": <key+version>, "__patch": {<field>: <new_value>, ...}}
 * ```
 * Fields unchanged between versions are omitted.  Decompression walks the
 * chain from the earliest baseline and applies patches in order.
 *
 * ### ZSTD
 * The JSON string is run-length encoded using a simple byte-level LZ77
 * variant (no external library).  The payload is base64-encoded and stored
 * with a `__compressed: "zstd"` marker.
 *
 * ### GORILLA
 * Each numeric (double/int) field is extracted across all versions of a key,
 * XOR-delta encoded as a sequence, and stored compactly.  Non-numeric fields
 * use verbatim JSON.
 *
 * ### DICTIONARY
 * All unique string values for each field in the range are collected into a
 * per-field dictionary; payloads are replaced with integer indices.
 */
class TemporalCompressor {
public:
    TemporalCompressor() = default;

    // Non-copyable; movable
    TemporalCompressor(const TemporalCompressor&)            = delete;
    TemporalCompressor& operator=(const TemporalCompressor&) = delete;
    TemporalCompressor(TemporalCompressor&&)                 noexcept = default;
    TemporalCompressor& operator=(TemporalCompressor&&)      noexcept = default;

    // ── Primary API ───────────────────────────────────────────────────────────

    /**
     * Compress historical versions of all keys in the given time range.
     *
     * Versions whose `sys_start` timestamp is within the
     * `delay_before_compression` grace window are silently skipped unless
     * `config.compress_immediately == true`.
     *
     * @param table   The system-versioned table whose history is compressed.
     * @param range   Only versions with sys_start in [range.start, range.end)
     *                are considered.
     * @param config  Algorithm and tuning parameters.
     * @return        Aggregated statistics for the compression pass.
     */
    CompressionStats compressHistory(SystemVersionedTable& table,
                                     const TimeRange& range,
                                     const CompressionConfig& config);

    /**
     * Decompress a payload that was previously compressed by this class.
     * Returns the original JSON document on success, or the input unchanged
     * if it is not a compressed payload.
     */
    static nlohmann::json decompress(const nlohmann::json& compressed);

    // ── Utility ───────────────────────────────────────────────────────────────

    /** Return the algorithm name as a string. */
    static std::string algorithmName(CompressionAlgorithm algo);

private:
    // ── Per-algorithm compressors ─────────────────────────────────────────────

    static nlohmann::json applyDelta(const nlohmann::json& base,
                                      const nlohmann::json& current,
                                      const std::string& base_ref);

    static nlohmann::json applyZstd(const nlohmann::json& doc,
                                     int level);

    static nlohmann::json decompressZstd(const nlohmann::json& doc);

    /// Compress a JSON document with LZ4 block format.
    static nlohmann::json applyLz4(const nlohmann::json& doc);

    /// Decompress a payload that was compressed with applyLz4().
    static nlohmann::json decompressLz4(const nlohmann::json& doc);

    /// Build a Gorilla-encoded payload from a vector of (timestamp, value) pairs.
    static nlohmann::json applyGorilla(
        const std::string& field_name,
        const std::vector<std::pair<Timestamp, double>>& series);

    static nlohmann::json applyDictionary(
        const nlohmann::json& doc,
        std::unordered_map<std::string, std::unordered_map<std::string, int>>& dicts);

    // ── Helpers ───────────────────────────────────────────────────────────────

    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);

    static std::string rlEncode(const std::string& input);
    static std::string rlDecode(const std::string& input);

    mutable std::mutex mutex_;
};

} // namespace temporal
} // namespace themisdb

