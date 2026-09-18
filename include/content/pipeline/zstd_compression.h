/**
 * @file zstd_compression.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "utils/zstd_codec.h"

namespace themis::content::pipeline {

class ZstdCompression {
public:
    using StreamCallback = std::function<void(size_t processed, size_t total)>;

    ZstdCompression() = default;
    ~ZstdCompression() = default;

    /**
     * @brief Compress.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data);

    /**
     * @brief Decompress.
     * @param[in] compressed_data Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data);

    std::vector<uint8_t> compress_streaming(
        const std::vector<uint8_t>& data,
        size_t chunk_size = 1024 * 1024,
        StreamCallback callback = nullptr
    );

    std::vector<uint8_t> decompress_streaming(
        const std::vector<uint8_t>& compressed_data,
        StreamCallback callback = nullptr
    );

    /**
     * @brief Set compression level.
     * @param[in] level Input parameter.
     */
    void set_compression_level(int level);

    /**
     * @brief Get compression level.
     * @return Return value.
     */
    int get_compression_level() const;

private:
    int compression_level_ = 3;  // Default ZSTD compression level
};

}  // namespace themis::content::pipeline
