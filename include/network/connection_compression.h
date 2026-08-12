/**
 * @file connection_compression.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Connection-level payload compression for the Wire Protocol V2.
//
// Wire format for all compressed payloads:
//   [original_size: uint32_t, little-endian] [compressed_data...]
//
// The 4-byte prefix allows decoders to pre-allocate the output buffer
// without relying on codec-internal metadata.

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include <lz4.h>
#include <zstd.h>
#include <zdict.h>

#include "themis/network/wire_protocol_v2.hpp"

namespace themis {
namespace network {

/// @brief Compress @p data with LZ4.
///
/// Returns the compressed payload (with 4-byte original-size prefix) when
/// compression reduces the size and the input meets the minimum threshold.
/// Returns an empty vector when compression is skipped or fails.
///
/// @param data     Input bytes to compress.
/// @param min_size Skip compression when input is smaller than this.
inline std::vector<uint8_t> compressLZ4(
    const std::vector<uint8_t>& data, uint32_t min_size = 256)
{
    if (data.size() < min_size) return {};
    const int src_size = static_cast<int>(data.size());
    const int bound    = LZ4_compressBound(src_size);
    if (bound <= 0) return {};

    std::vector<uint8_t> out(4 + static_cast<size_t>(bound));
    const uint32_t orig_le = static_cast<uint32_t>(src_size);
    std::memcpy(out.data(), &orig_le, 4);

    const int compressed = LZ4_compress_default(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(out.data() + 4),
        src_size, bound);
    if (compressed <= 0) return {};

    out.resize(4 + static_cast<size_t>(compressed));
    return out;
}

/// @brief Decompress an LZ4-compressed payload (with 4-byte original-size prefix).
///
/// Returns the decompressed bytes, or an empty vector on error.
inline std::vector<uint8_t> decompressLZ4(const std::vector<uint8_t>& data)
{
    if (data.size() < 4) return {};
    uint32_t orig_le = 0;
    std::memcpy(&orig_le, data.data(), 4);
    const int orig_size = static_cast<int>(orig_le);
    if (orig_size <= 0 ||
        static_cast<size_t>(orig_size) > wire::V2_MAX_PAYLOAD) return {};

    std::vector<uint8_t> out(static_cast<size_t>(orig_size));
    const int result = LZ4_decompress_safe(
        reinterpret_cast<const char*>(data.data() + 4),
        reinterpret_cast<char*>(out.data()),
        static_cast<int>(data.size() - 4),
        orig_size);
    if (result != orig_size) return {};
    return out;
}

/// @brief Compress @p data with Zstd.
///
/// Returns the compressed payload (with 4-byte original-size prefix) when
/// compression reduces the size and the input meets the minimum threshold.
/// Returns an empty vector when compression is skipped or fails.
///
/// @param data      Input bytes to compress.
/// @param min_size  Skip compression when input is smaller than this.
/// @param level     Zstd compression level (1–22, default 3).
inline std::vector<uint8_t> compressZstd(
    const std::vector<uint8_t>& data,
    uint32_t min_size = 256,
    int      level    = ZSTD_CLEVEL_DEFAULT)
{
    if (data.size() < min_size) return {};
    const size_t bound = ZSTD_compressBound(data.size());
    if (ZSTD_isError(bound)) return {};

    std::vector<uint8_t> out(4 + bound);
    const uint32_t orig_le = static_cast<uint32_t>(data.size());
    std::memcpy(out.data(), &orig_le, 4);

    const size_t compressed = ZSTD_compress(
        out.data() + 4, bound,
        data.data(), data.size(),
        level);
    if (ZSTD_isError(compressed)) return {};

    out.resize(4 + compressed);
    return out;
}

/// @brief Decompress a Zstd-compressed payload (with 4-byte original-size prefix).
///
/// Returns the decompressed bytes, or an empty vector on error.
inline std::vector<uint8_t> decompressZstd(const std::vector<uint8_t>& data)
{
    if (data.size() < 4) return {};
    uint32_t orig_le = 0;
    std::memcpy(&orig_le, data.data(), 4);
    const size_t orig_size = static_cast<size_t>(orig_le);
    if (orig_size == 0 || orig_size > wire::V2_MAX_PAYLOAD) return {};

    std::vector<uint8_t> out(orig_size);
    const size_t result = ZSTD_decompress(
        out.data(), orig_size,
        data.data() + 4, data.size() - 4);
    if (ZSTD_isError(result) || result != orig_size) return {};
    return out;
}

// =============================================================================
// ZstdDictionaryCompressor – Zstd dictionary-based compression
// =============================================================================

/**
 * @brief Adaptive Zstd compressor that uses a pre-trained dictionary.
 *
 * Dictionary compression achieves significantly better ratios than generic
 * Zstd for payloads that share a common structure (e.g. JSON key names,
 * Protobuf field tags).  A dictionary is trained once from representative
 * sample data and then reused for all subsequent compress/decompress calls.
 *
 * Wire format for dictionary-compressed payloads:
 * @code
 *   [dict_id: uint32_t LE][original_size: uint32_t LE][compressed_data...]
 * @endcode
 *
 * Usage:
 * @code
 *   // Training (one-time, offline or at startup)
 *   ZstdDictionaryCompressor comp;
 *   comp.train(sample_buffers, 112 * 1024); // 112 KiB dict
 *
 *   // Compression
 *   auto compressed = comp.compress(payload);
 *
 *   // Decompression (same instance, or one constructed from the dict bytes)
 *   auto restored = comp.decompress(compressed);
 * @endcode
 *
 * @note Training requires at least a few thousand bytes of representative
 *       samples.  If training fails or the dictionary is too small to be
 *       useful, compress() falls back to regular Zstd compression.
 */
class ZstdDictionaryCompressor {
public:
    struct Config {
        int    compression_level  = ZSTD_CLEVEL_DEFAULT; ///< Zstd level (1–22)
        size_t min_compress_bytes = 256;                 ///< Skip if smaller
        size_t dict_max_size      = 112 * 1024;          ///< Max dict size bytes
        static Config defaults() { return {}; }
    };

    explicit ZstdDictionaryCompressor(const Config& cfg = Config::defaults());
    ~ZstdDictionaryCompressor();

    // Non-copyable due to ZSTD_CDict/ZSTD_DDict ownership.
    ZstdDictionaryCompressor(const ZstdDictionaryCompressor&)            = delete;
    ZstdDictionaryCompressor& operator=(const ZstdDictionaryCompressor&) = delete;

    ZstdDictionaryCompressor(ZstdDictionaryCompressor&& other) noexcept;
    ZstdDictionaryCompressor& operator=(ZstdDictionaryCompressor&& other) noexcept;

    /**
     * @brief Train a dictionary from representative @p samples.
     *
     * Concatenates all samples and calls ZSTD_trainFromBuffer().
     *
     * @param samples         Collection of representative payload buffers.
     * @param max_dict_size   Maximum dictionary size in bytes.
     * @return true if training succeeded and the dictionary is ready;
     *         false if there was insufficient sample data or ZSTD failed.
     */
    bool train(const std::vector<std::vector<uint8_t>>& samples,
               size_t max_dict_size = 0);

    /**
     * @brief Load a pre-built dictionary from raw bytes.
     *
     * Allows dictionaries serialised and stored offline to be reloaded at
     * startup without re-training.
     *
     * @return true on success; false if ZSTD rejected the bytes.
     */
    bool loadDictionary(const std::vector<uint8_t>& dict_bytes);

    /**
     * @brief Compress @p data with the trained dictionary (if available).
     *
     * Falls back to plain Zstd if no dictionary has been trained yet or if
     * the payload is smaller than `min_compress_bytes`.
     *
     * @return Compressed bytes with 8-byte prefix, or empty on error/skip.
     */
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) const;

    /**
     * @brief Decompress a dictionary-compressed payload.
     *
     * The dictionary ID embedded in the header is validated against the
     * loaded dictionary.  Plain Zstd payloads (no dict prefix) are handled
     * transparently when no dictionary is loaded.
     *
     * @return Decompressed bytes, or empty on error.
     */
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& data) const;

    /**
     * @brief Return the raw dictionary bytes (for serialisation / peer sharing).
     *
     * Returns an empty vector if no dictionary has been trained.
     */
    const std::vector<uint8_t>& dictionaryBytes() const noexcept {
        return dict_bytes_;
    }

    /**
     * @brief Return the Zstd dictionary ID, or 0 if none is loaded.
     */
    uint32_t dictionaryId() const noexcept { return dict_id_; }

    /// Return true if a valid dictionary is ready.
    bool hasDictionary() const noexcept { return cdict_ != nullptr; }

    const Config& config() const noexcept { return cfg_; }

private:
    Config cfg_;

    std::vector<uint8_t> dict_bytes_;
    uint32_t             dict_id_ = 0;

    ZSTD_CDict* cdict_ = nullptr; ///< Compression dictionary context
    ZSTD_DDict* ddict_ = nullptr; ///< Decompression dictionary context

    /// Cached compression context – reused across compress() calls to avoid
    /// per-call allocation overhead (reset via ZSTD_CCtx_reset).
    ZSTD_CCtx*  cctx_  = nullptr;

    /// Cached decompression context – reused across decompress() calls.
    ZSTD_DCtx*  dctx_  = nullptr;

    void freeDicts() noexcept;
};

} // namespace network
} // namespace themis

