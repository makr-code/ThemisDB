/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            connection_compression.cpp                         ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-14 06:30:00                                ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    : cfg_(cfg)
{}

ZstdDictionaryCompressor::~ZstdDictionaryCompressor() {
    freeDicts();
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
      ddict_(other.ddict_)
{
    other.dict_id_ = 0;
    other.cdict_   = nullptr;
    other.ddict_   = nullptr;
}

ZstdDictionaryCompressor& ZstdDictionaryCompressor::operator=(
    ZstdDictionaryCompressor&& other) noexcept
{
    if (this != &other) {
        freeDicts();
        cfg_        = other.cfg_;
        dict_bytes_ = std::move(other.dict_bytes_);
        dict_id_    = other.dict_id_;
        cdict_      = other.cdict_;
        ddict_      = other.ddict_;
        other.dict_id_ = 0;
        other.cdict_   = nullptr;
        other.ddict_   = nullptr;
    }
    return *this;
}

bool ZstdDictionaryCompressor::train(
    const std::vector<std::vector<uint8_t>>& samples,
    size_t max_dict_size)
{
    if (samples.empty()) return false;

    if (max_dict_size == 0) max_dict_size = cfg_.dict_max_size;

    // Concatenate all sample buffers.
    size_t total_size = 0;
    for (const auto& s : samples) total_size += s.size();
    if (total_size == 0) return false;

    std::vector<uint8_t> concat(total_size);
    std::vector<size_t>  sample_sizes(samples.size());
    size_t offset = 0;
    for (size_t i = 0; i < samples.size(); ++i) {
        std::memcpy(concat.data() + offset, samples[i].data(), samples[i].size());
        sample_sizes[i] = samples[i].size();
        offset += samples[i].size();
    }

    std::vector<uint8_t> dict_buf(max_dict_size);
    const size_t dict_size = ZDICT_trainFromBuffer(
        dict_buf.data(), dict_buf.size(),
        concat.data(), sample_sizes.data(), samples.size());

    if (ZDICT_isError(dict_size)) return false;

    dict_buf.resize(dict_size);
    return loadDictionary(dict_buf);
}

bool ZstdDictionaryCompressor::loadDictionary(
    const std::vector<uint8_t>& dict_bytes)
{
    if (dict_bytes.empty()) return false;

    // Build CDict and DDict.
    ZSTD_CDict* new_cdict = ZSTD_createCDict(
        dict_bytes.data(), dict_bytes.size(), cfg_.compression_level);
    if (!new_cdict) return false;

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
        ZSTD_CCtx* cctx = ZSTD_createCCtx();
        if (!cctx) return {};
        compressed = ZSTD_compress_usingCDict(
            cctx,
            out.data() + DICT_PREFIX_SIZE, bound,
            data.data(), data.size(),
            cdict_);
        ZSTD_freeCCtx(cctx);
    } else {
        // Fall back to plain Zstd if no dictionary.
        compressed = ZSTD_compress(
            out.data() + DICT_PREFIX_SIZE, bound,
            data.data(), data.size(),
            cfg_.compression_level);
    }

    if (ZSTD_isError(compressed)) return {};

    // Skip if the compressed form is not smaller.
    if (compressed >= data.size()) return {};

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
        ZSTD_DCtx* dctx = ZSTD_createDCtx();
        if (!dctx) return {};
        result = ZSTD_decompress_usingDDict(
            dctx,
            out.data(), out.size(),
            data.data() + DICT_PREFIX_SIZE, data.size() - DICT_PREFIX_SIZE,
            ddict_);
        ZSTD_freeDCtx(dctx);
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
