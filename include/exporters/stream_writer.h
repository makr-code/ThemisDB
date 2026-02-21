/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stream_writer.h                                    ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     88                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    GZIP,
    ZSTD
};

/// Streaming output writer with optional compression
class StreamWriter {
public:
    struct Config {
        std::string output_path;
        CompressionType compression = CompressionType::NONE;
        int compression_level = 6;  // 1-9 for gzip (zstd support planned)
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
    void* compression_state_ = nullptr;  // zlib or zstd state
    
    void initCompression();
    void writeBuffer();
    void compressAndWrite(const char* data, size_t size);
    void finalizeCompression();
};

} // namespace themis::exporters
