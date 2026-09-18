/**
 * @file stream_writer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/stream_writer.h"
#include <stdexcept>
#include "exporters/exporter_errors.h"
#ifdef THEMIS_HAS_ZSTD
#include <zstd.h>
#endif
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

/**
 * @brief Write.
 * @param[in] data Input parameter.
 * @details Calls: data(), size().
 */
void StreamWriter::write(const std::string& data) {
    write(data.data(), data.size());
}

/**
 * @brief Write.
 * @param[in] data Input parameter.
 * @param[in] size Input parameter.
 * @throws SizeLimitException if an error occurs.
 * @details Calls: compressAndWrite(), size(), writeBuffer(), std::memcpy(), data().
 */
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
            file_.write(data, static_cast<std::streamsize>(size));
            compressed_bytes_written_ += size;
        } else {
            std::memcpy(buffer_.data() + buffer_pos_, data, size);
            buffer_pos_ += size;
        }
    }
}

/**
 * @brief Flush.
 * @details Calls: data(), size(), ZSTD_flushStream(), write(), writeBuffer().
 */
void StreamWriter::flush() {
    if (config_.compression != CompressionType::NONE) {
        // For compression, finalize current block
#ifdef THEMIS_HAS_ZSTD
        if (compression_state_) {
            ZSTD_CStream* cstream = static_cast<ZSTD_CStream*>(compression_state_);
            size_t remaining = 0;
            do {
                ZSTD_outBuffer out_buf = { buffer_.data(), buffer_.size(), 0 };
                remaining = ZSTD_flushStream(cstream, &out_buf);
                if (out_buf.pos > 0) {
                    file_.write(buffer_.data(), static_cast<std::streamsize>(out_buf.pos));
                    compressed_bytes_written_ += out_buf.pos;
                }
            } while (remaining > 0);
        }
#endif
    } else {
        writeBuffer();
    }
    file_.flush();
}

/**
 * @brief Close.
 * @details Calls: is_open(), finalizeCompression(), writeBuffer().
 */
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

/**
 * @brief Init Compression.
 * @throws ExportIOException if an error occurs.
 * @details Calls: ZSTD_createCStream(), ZSTD_initCStream(), ZSTD_isError(), ZSTD_freeCStream().
 */
void StreamWriter::initCompression() {
    // Both GZIP and ZSTD requests use ZSTD as the sole compression backend.
    // GZIP is accepted for backward compatibility but produces ZSTD output.
    // For tools requiring gzip format, pipe through: zstd -d | gzip (or pigz).
#ifdef THEMIS_HAS_ZSTD
    ZSTD_CStream* cstream = ZSTD_createCStream();
    if (!cstream) {
        throw ExportIOException("Failed to create zstd compression stream", config_.output_path, 0);
    }
    // Clamp level to valid zstd range [1, 22]
    int level = config_.compression_level;
    if (level < 1) {
      level = 1;
    }
    if (level > 22) {
      level = 22;
    }
    size_t init_result = ZSTD_initCStream(cstream, level);
    if (ZSTD_isError(init_result)) {
        ZSTD_freeCStream(cstream);
        throw ExportIOException("Failed to initialize zstd compression stream",
                                config_.output_path, static_cast<int>(init_result));
    }
    compression_state_ = cstream;
#else
    throw ExportIOException("ZSTD compression not available (not compiled with THEMIS_HAS_ZSTD)",
                            config_.output_path, 0);
#endif
}

/**
 * @brief Write Buffer.
 * @details Calls: write(), data().
 */
void StreamWriter::writeBuffer() {
    if (buffer_pos_ > 0) {
        file_.write(buffer_.data(), static_cast<std::streamsize>(buffer_pos_));
        compressed_bytes_written_ += buffer_pos_;
        buffer_pos_ = 0;
    }
}

/**
 * @brief Compress And Write.
 * @param[in] data Input parameter.
 * @param[in] size Input parameter.
 * @throws ExportIOException if an error occurs.
 * @details Calls: data(), size(), ZSTD_compressStream(), ZSTD_isError(), write().
 */
void StreamWriter::compressAndWrite(const char* data, size_t size) {
#ifdef THEMIS_HAS_ZSTD
    if (compression_state_) {
        ZSTD_CStream* cstream = static_cast<ZSTD_CStream*>(compression_state_);
        ZSTD_inBuffer in_buf = { data, size, 0 };

        while (in_buf.pos < in_buf.size) {
            ZSTD_outBuffer out_buf = { buffer_.data(), buffer_.size(), 0 };
            size_t ret = ZSTD_compressStream(cstream, &out_buf, &in_buf);
            if (ZSTD_isError(ret)) {
                throw ExportIOException("ZSTD compression stream error", config_.output_path,
                                        static_cast<int>(ret));
            }
            if (out_buf.pos > 0) {
                file_.write(buffer_.data(), static_cast<std::streamsize>(out_buf.pos));
                compressed_bytes_written_ += out_buf.pos;
            }
        }
    }
#endif
}

/**
 * @brief Finalize Compression.
 * @details Calls: data(), size(), ZSTD_endStream(), write(), ZSTD_freeCStream().
 */
void StreamWriter::finalizeCompression() {
#ifdef THEMIS_HAS_ZSTD
    if (compression_state_) {
        ZSTD_CStream* cstream = static_cast<ZSTD_CStream*>(compression_state_);
        size_t remaining = 0;
        do {
            ZSTD_outBuffer out_buf = { buffer_.data(), buffer_.size(), 0 };
            remaining = ZSTD_endStream(cstream, &out_buf);
            if (out_buf.pos > 0) {
                file_.write(buffer_.data(), static_cast<std::streamsize>(out_buf.pos));
                compressed_bytes_written_ += out_buf.pos;
            }
        } while (remaining > 0);
        ZSTD_freeCStream(cstream);
        compression_state_ = nullptr;
    }
#endif
}

} // namespace themis::exporters


