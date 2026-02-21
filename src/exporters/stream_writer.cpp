/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stream_writer.cpp                                  ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:38:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     204                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c61448a6c  2026-02-19  Exporters production readiness: tenant isolation, PII red... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "exporters/stream_writer.h"
#include "exporters/exporter_errors.h"
#include <zlib.h>
#include <cstring>

namespace themis::exporters {

StreamWriter::StreamWriter(const Config& config) 
    : config_(config), buffer_(config.buffer_size) {
    
    file_.open(config.output_path, std::ios::binary);
    if (!file_.is_open()) {
        throw ExportIOException(
            "Failed to open output file for streaming",
            config.output_path,
            errno
        );
    }
    
    if (config_.compression != CompressionType::NONE) {
        initCompression();
    }
}

StreamWriter::~StreamWriter() {
    try {
        close();
    } catch (...) {
        // Suppress exceptions in destructor
    }
}

void StreamWriter::write(const std::string& data) {
    write(data.data(), data.size());
}

void StreamWriter::write(const char* data, size_t size) {
    bytes_written_ += size;
    
    // Check size limit
    if (config_.max_file_size > 0 && compressed_bytes_written_ >= config_.max_file_size) {
        throw SizeLimitException(
            "Export file size limit exceeded",
            compressed_bytes_written_,
            config_.max_file_size
        );
    }
    
    if (config_.compression != CompressionType::NONE) {
        compressAndWrite(data, size);
    } else {
        // Write directly if buffer would overflow
        if (buffer_pos_ + size > buffer_.size()) {
            writeBuffer();
        }
        
        // If data is larger than buffer, write directly
        if (size > buffer_.size()) {
            file_.write(data, size);
            compressed_bytes_written_ += size;
        } else {
            std::memcpy(buffer_.data() + buffer_pos_, data, size);
            buffer_pos_ += size;
        }
    }
}

void StreamWriter::flush() {
    if (config_.compression != CompressionType::NONE) {
        // For compression, finalize current block
        if (compression_state_) {
            z_stream* strm = static_cast<z_stream*>(compression_state_);
            strm->avail_in = 0;
            strm->next_in = Z_NULL;
            
            int ret = Z_OK;
            do {
                strm->avail_out = buffer_.size();
                strm->next_out = reinterpret_cast<Bytef*>(buffer_.data());
                ret = deflate(strm, Z_SYNC_FLUSH);
                
                size_t have = buffer_.size() - strm->avail_out;
                if (have > 0) {
                    file_.write(buffer_.data(), have);
                    compressed_bytes_written_ += have;
                }
            } while (ret == Z_OK && strm->avail_out == 0);
        }
    } else {
        writeBuffer();
    }
    file_.flush();
}

void StreamWriter::close() {
    if (!file_.is_open()) {
        return;
    }
    
    if (config_.compression != CompressionType::NONE) {
        finalizeCompression();
    } else {
        writeBuffer();
    }
    
    file_.close();
}

void StreamWriter::initCompression() {
    if (config_.compression == CompressionType::GZIP) {
        // Use unique_ptr for automatic cleanup
        auto strm = std::make_unique<z_stream>();
        strm->zalloc = Z_NULL;
        strm->zfree = Z_NULL;
        strm->opaque = Z_NULL;
        
        // Use deflateInit2 with gzip header (window bits + 16)
        int ret = deflateInit2(strm.get(), config_.compression_level, Z_DEFLATED,
                              15 + 16, 8, Z_DEFAULT_STRATEGY);
        if (ret != Z_OK) {
            throw ExportIOException("Failed to initialize gzip compression", config_.output_path, ret);
        }
        
        compression_state_ = strm.release();  // Transfer ownership
    }
}

void StreamWriter::writeBuffer() {
    if (buffer_pos_ > 0) {
        file_.write(buffer_.data(), buffer_pos_);
        compressed_bytes_written_ += buffer_pos_;
        buffer_pos_ = 0;
    }
}

void StreamWriter::compressAndWrite(const char* data, size_t size) {
    if (config_.compression == CompressionType::GZIP && compression_state_) {
        z_stream* strm = static_cast<z_stream*>(compression_state_);
        strm->avail_in = size;
        strm->next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data));
        
        do {
            strm->avail_out = buffer_.size();
            strm->next_out = reinterpret_cast<Bytef*>(buffer_.data());
            
            int ret = deflate(strm, Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR) {
                throw ExportIOException("Compression stream error", config_.output_path, ret);
            }
            
            size_t have = buffer_.size() - strm->avail_out;
            if (have > 0) {
                file_.write(buffer_.data(), have);
                compressed_bytes_written_ += have;
            }
        } while (strm->avail_out == 0);
    }
}

void StreamWriter::finalizeCompression() {
    if (config_.compression == CompressionType::GZIP && compression_state_) {
        z_stream* strm = static_cast<z_stream*>(compression_state_);
        strm->avail_in = 0;
        strm->next_in = Z_NULL;
        
        int ret;
        do {
            strm->avail_out = buffer_.size();
            strm->next_out = reinterpret_cast<Bytef*>(buffer_.data());
            ret = deflate(strm, Z_FINISH);
            
            size_t have = buffer_.size() - strm->avail_out;
            if (have > 0) {
                file_.write(buffer_.data(), have);
                compressed_bytes_written_ += have;
            }
        } while (ret == Z_OK);
        
        deflateEnd(strm);
        delete strm;
        compression_state_ = nullptr;
    }
}

} // namespace themis::exporters
