/**
 * @file stream_writer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <vector>

namespace themis::exporters {

enum class CompressionType {
    NONE,
    GZIP,   ///< Accepted for backward compatibility; produces ZSTD output (not gzip format).
    ZSTD
};

class StreamWriter {
public:
    struct Config {
        std::string output_path;
        CompressionType compression = CompressionType::NONE;
        int compression_level = 3;  // 1-22 for zstd; level 3 is the default (good speed/compression ratio)
        size_t buffer_size = 8192;
        size_t max_file_size = 0;  // 0 = unlimited
    };
    
    /**
     * @brief Stream Writer.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit StreamWriter(const Config& config);
    ~StreamWriter();
    
    /**
     * @brief Write.
     * @param[in] data Input parameter.
     */
    void write(const std::string& data);
    /**
     * @brief Write.
     * @param[in] data Input parameter.
     * @param[in] size Input parameter.
     */
    void write(const char* data, size_t size);
    
    /**
     * @brief Flush.
     */
    void flush();
    
    /**
     * @brief Close.
     */
    void close();
    
    size_t getBytesWritten() const { return bytes_written_; }
    
    size_t getCompressedBytesWritten() const { return compressed_bytes_written_; }
    
    bool isLimitReached() const {
        return config_.max_file_size > 0 && compressed_bytes_written_ >= config_.max_file_size;
    }
    
private:
    Config config_;
    std::ofstream file_;
    std::vector<char> buffer_;
    size_t buffer_pos_ = 0;
    size_t bytes_written_ = 0;
    size_t compressed_bytes_written_ = 0;
    
    // Compression state
    void* compression_state_ = nullptr;  // zstd stream state
    
    /**
     * @brief Init Compression.
     */
    void initCompression();
    /**
     * @brief Write Buffer.
     */
    void writeBuffer();
    void compressAndWrite([[maybe_unused]] const char* data, [[maybe_unused]] size_t size);
    /**
     * @brief Finalize Compression.
     */
    void finalizeCompression();
};

} // namespace themis::exporters
