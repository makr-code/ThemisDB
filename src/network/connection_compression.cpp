/**
 * @file connection_compression.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Wire Protocol – Dictionary Compression (ZstdDictionaryCompressor)

#include "network/connection_compression.h"

#include <cstring>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace themis {
namespace network {

// =============================================================================
// ZstdDictionaryCompressor
// =============================================================================

// Wire-format prefix size: [dict_id: 4B][original_size: 4B]
static constexpr size_t DICT_PREFIX_SIZE = 8;

ZstdDictionaryCompressor::ZstdDictionaryCompressor(const Config& cfg)
    : cfg_(cfg),
      cctx_(ZSTD_createCCtx()),
      dctx_(ZSTD_createDCtx())
{}

ZstdDictionaryCompressor::~ZstdDictionaryCompressor() {
    freeDicts();
    if (cctx_) { ZSTD_freeCCtx(cctx_); cctx_ = nullptr; }
    if (dctx_) { ZSTD_freeDCtx(dctx_); dctx_ = nullptr; }
}

void ZstdDictionaryCompressor::freeDicts() noexcept {
    if (cdict_) { ZSTD_freeCDict(cdict_); cdict_ = nullptr; }
    if (ddict_) { ZSTD_freeDDict(ddict_); ddict_ = nullptr; }
}

ZstdDictionaryCompressor::ZstdDictionaryCompressor(
    ZstdDictionaryCompressor&& other) noexcept
    : cfg_(other.cfg_),
      dict_bytes_(std::move(other.dict_bytes_)),
      dict_id_(other.dict_id_),
      cdict_(other.cdict_),
      ddict_(other.ddict_),
      cctx_(other.cctx_),
      dctx_(other.dctx_)
{
    other.dict_id_ = 0;
    other.cdict_   = nullptr;
    other.ddict_   = nullptr;
    other.cctx_    = nullptr;
    other.dctx_    = nullptr;
}

ZstdDictionaryCompressor& ZstdDictionaryCompressor::operator=(
    ZstdDictionaryCompressor&& other) noexcept
{
    if (this != &other) {
        freeDicts();
        if (cctx_) { ZSTD_freeCCtx(cctx_); cctx_ = nullptr; }
        if (dctx_) { ZSTD_freeDCtx(dctx_); dctx_ = nullptr; }
        cfg_        = other.cfg_;
        dict_bytes_ = std::move(other.dict_bytes_);
        dict_id_    = other.dict_id_;
        cdict_      = other.cdict_;
        ddict_      = other.ddict_;
        cctx_       = other.cctx_;
        dctx_       = other.dctx_;
        other.dict_id_ = 0;
        other.cdict_   = nullptr;
        other.ddict_   = nullptr;
        other.cctx_    = nullptr;
        other.dctx_    = nullptr;
    }
    return *this;
}

bool ZstdDictionaryCompressor::train(
    const std::vector<std::vector<uint8_t>>& samples,
    size_t max_dict_size)
{
    if (samples.empty()) {
      return false;
    }

    if (max_dict_size == 0) {
      max_dict_size = cfg_.dict_max_size;
    }

    // Concatenate all sample buffers.
    size_t total_size = 0;
    for (const auto& s : samples) {
      total_size += s.size();
    }
    if (total_size == 0) {
      return false;
    }

    std::vector<uint8_t> concat(total_size);
    std::vector<size_t>  sample_sizes(samples.size());
    size_t offset = 0;
    for (size_t i = 0; i < samples.size(); ++i) {
        // R12: Add overflow-safe bounds checks before memcpy to prevent
        // buffer overflow even when sample sizes are extreme.
        if (offset > concat.size()) {
            return false;
        }
        if (samples[i].size() > (concat.size() - offset)) {
            // Buffer overflow detected: sample would exceed destination
            return false;
        }
        std::memcpy(concat.data() + offset, samples[i].data(), samples[i].size());
        sample_sizes[i] = samples[i].size();
        offset += samples[i].size();
    }

    std::vector<uint8_t> dict_buf(max_dict_size);
    const size_t dict_size = ZDICT_trainFromBuffer(
        dict_buf.data(), dict_buf.size(),
        concat.data(), sample_sizes.data(),
        static_cast<unsigned>(samples.size()));

    if (ZDICT_isError(dict_size)) {
      return false;
    }

    dict_buf.resize(dict_size);
    return loadDictionary(dict_buf);
}

bool ZstdDictionaryCompressor::loadDictionary(
    const std::vector<uint8_t>& dict_bytes)
{
    if (dict_bytes.empty()) {
      return false;
    }

    // Build CDict and DDict.
    ZSTD_CDict* new_cdict = ZSTD_createCDict(
        dict_bytes.data(), dict_bytes.size(), cfg_.compression_level);
    if (!new_cdict) {
      return false;
    }

    ZSTD_DDict* new_ddict = ZSTD_createDDict(
        dict_bytes.data(), dict_bytes.size());
    if (!new_ddict) {
        ZSTD_freeCDict(new_cdict);
        return false;
    }

    freeDicts();
    cdict_      = new_cdict;
    ddict_      = new_ddict;
    dict_bytes_ = dict_bytes;
    dict_id_    = ZSTD_getDictID_fromDict(dict_bytes.data(), dict_bytes.size());
    return true;
}

std::vector<uint8_t> ZstdDictionaryCompressor::compress(
    const std::vector<uint8_t>& data) const
{
    if (data.size() < cfg_.min_compress_bytes) return {};

    const size_t bound = ZSTD_compressBound(data.size());
    if (ZSTD_isError(bound)) return {};

    std::vector<uint8_t> out(DICT_PREFIX_SIZE + bound);

    // Write prefix: [dict_id: uint32_t LE][original_size: uint32_t LE]
    const uint32_t did      = dict_id_;
    const uint32_t orig_le  = static_cast<uint32_t>(data.size());
    std::memcpy(out.data(),     &did,     4);
    std::memcpy(out.data() + 4, &orig_le, 4);

    size_t compressed = 0;

    if (cdict_) {
        // cctx_ is created in the constructor; null only if allocation failed
        // (out-of-memory at construction time). Return empty to signal failure.
        if (!cctx_) return {};
        // Reset the cached context to initial state and compress with dict.
        ZSTD_CCtx_reset(cctx_, ZSTD_reset_session_only);
        compressed = ZSTD_compress_usingCDict(
            cctx_,
            out.data() + DICT_PREFIX_SIZE, bound,
            data.data(), data.size(),
            cdict_);
    } else {
        // Fall back to plain Zstd if no dictionary.
        compressed = ZSTD_compress(
            out.data() + DICT_PREFIX_SIZE, bound,
            data.data(), data.size(),
            cfg_.compression_level);
    }

    if (ZSTD_isError(compressed)) return {};

    // Skip if the total output (prefix + compressed data) is not smaller than input.
    if (DICT_PREFIX_SIZE + compressed >= data.size()) return {};

    out.resize(DICT_PREFIX_SIZE + compressed);
    return out;
}

std::vector<uint8_t> ZstdDictionaryCompressor::decompress(
    const std::vector<uint8_t>& data) const
{
    if (data.size() < DICT_PREFIX_SIZE) return {};

    uint32_t dict_id_in = 0;
    uint32_t orig_size  = 0;
    std::memcpy(&dict_id_in, data.data(),     4);
    std::memcpy(&orig_size,  data.data() + 4, 4);

    if (orig_size == 0 || orig_size > wire::V2_MAX_PAYLOAD) return {};

    std::vector<uint8_t> out(static_cast<size_t>(orig_size));
    size_t result = 0;

    if (ddict_ && dict_id_in != 0) {
        // dctx_ is created in the constructor; null only if allocation failed.
        if (!dctx_) return {};
        // Reset the cached context before reuse.
        ZSTD_DCtx_reset(dctx_, ZSTD_reset_session_only);
        result = ZSTD_decompress_usingDDict(
            dctx_,
            out.data(), out.size(),
            data.data() + DICT_PREFIX_SIZE, data.size() - DICT_PREFIX_SIZE,
            ddict_);
    } else {
        // Fall back to plain Zstd (dict_id == 0 means no dictionary was used).
        result = ZSTD_decompress(
            out.data(), out.size(),
            data.data() + DICT_PREFIX_SIZE, data.size() - DICT_PREFIX_SIZE);
    }

    if (ZSTD_isError(result) || result != static_cast<size_t>(orig_size))
        return {};

    return out;
}

} // namespace network
} // namespace themis
