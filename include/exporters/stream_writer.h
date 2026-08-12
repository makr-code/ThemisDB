/**
 * @file stream_writer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <vector>

namespace themis::exporters {

/// Compression type
enum class CompressionType {
    NONE,
    GZIP,   ///< Accepted for backward compatibility; produces ZSTD output (not gzip format).
            ///< For gzip output, pipe through: zstd -d | gzip  (or pigz).
    ZSTD
};

/// Streaming output writer with optional compression
class StreamWriter {
public:
    struct Config {
        std::string output_path;
        CompressionType compression = CompressionType::NONE;
        int compression_level = 3;  // 1-22 for zstd; level 3 is the default (good speed/compression ratio)
        size_t buffer_size = 8192;
        size_t max_file_size = 0;  // 0 = unlimited
    };
    
    explicit StreamWriter(const Config& config);
    ~StreamWriter();
    
    /// Write data to stream
    void write(const std::string& data);
    void write(const char* data, size_t size);
    
    /// Flush buffered data
    void flush();
    
    /// Close stream
    void close();
    
    /// Get bytes written (before compression)
    size_t getBytesWritten() const { return bytes_written_; }
    
    /// Get compressed bytes written
    size_t getCompressedBytesWritten() const { return compressed_bytes_written_; }
    
    /// Check if size limit reached
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
    
    void initCompression();
    void writeBuffer();
    void compressAndWrite([[maybe_unused]] const char* data, [[maybe_unused]] size_t size);
    void finalizeCompression();
};

} // namespace themis::exporters
