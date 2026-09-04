/**
 * @file stream_protocol.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 83/100
 * @note Gap Summary: total=7; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=2, Debt=1, C=6, H=15, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Inter-Shard Streaming Protocol Implementation
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sharding/stream_protocol.h"
#include "utils/thread_join_utils.h"
#include <openssl/evp.h>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <random>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>

// Optional: LZ4 compression (conditional compilation)
#ifdef THEMIS_HAS_LZ4
#include <lz4.h>
#endif

// Optional: Zstd compression (conditional compilation)
#ifdef THEMIS_HAS_ZSTD
#include <zstd.h>
#endif

namespace themisdb {
namespace streaming {

// ============================================================================
// CRC32 Helper
// ============================================================================

namespace {

// CRC32 lookup table
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD706B3,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

uint32_t calculateCRC32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief Helper function to execute an operation with exponential backoff retry.
 * 
 * @tparam Func Callable that returns bool (true = success, false = transient failure)
 * @param func Operation to retry
 * @param max_retries Maximum number of retry attempts (default: 3)
 * @param initial_delay_ms Initial backoff delay in milliseconds (default: 100)
 * @param max_delay_ms Maximum backoff delay cap (default: 5000)
 * @return true if operation succeeded, false if all retries exhausted
 */
template <typename Func>
inline bool retryWithBackoff(
    Func&& func,
    int max_retries = 3,
    uint64_t initial_delay_ms = 100,
    uint64_t max_delay_ms = 5000
) {
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        try {
            if (func()) {
                return true;  // Success
            }
            // Transient failure: prepare to retry
        } catch (const std::exception&) {
            // Exception indicates transient failure; retry
        }
        
        if (attempt < max_retries - 1) {
            // Exponential backoff: 100ms, 200ms, 400ms, ...
            uint64_t delay_ms = initial_delay_ms * (1ULL << attempt);
            delay_ms = std::min(delay_ms, max_delay_ms);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
    
    return false;  // All retries exhausted
}

/** @brief Generate pseudo-random per-session identifier. */
uint32_t generateSessionId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;
    return dis(gen);
}

} // anonymous namespace

// ============================================================================
// StreamMessageHeader Implementation
// ============================================================================

/** @brief Serialize stream message header into fixed-size wire bytes. */
std::vector<uint8_t> StreamMessageHeader::serialize() const {
    std::vector<uint8_t> result(SIZE);
    size_t pos = 0;
    
    result[pos++] = version;
    result[pos++] = static_cast<uint8_t>(type);
    
    // session_id (big-endian)
    result[pos++] = (session_id >> 24) & 0xFF;
    result[pos++] = (session_id >> 16) & 0xFF;
    result[pos++] = (session_id >> 8) & 0xFF;
    result[pos++] = session_id & 0xFF;
    
    // sequence_number (big-endian)
    for (int i = 7; i >= 0; --i) {
        result[pos++] = (sequence_number >> (i * 8)) & 0xFF;
    }
    
    // payload_length (big-endian)
    result[pos++] = (payload_length >> 24) & 0xFF;
    result[pos++] = (payload_length >> 16) & 0xFF;
    result[pos++] = (payload_length >> 8) & 0xFF;
    result[pos++] = payload_length & 0xFF;
    
    // flags (big-endian)
    result[pos++] = (flags >> 24) & 0xFF;
    result[pos++] = (flags >> 16) & 0xFF;
    result[pos++] = (flags >> 8) & 0xFF;
    result[pos++] = flags & 0xFF;
    
    // checksum (big-endian)
    result[pos++] = (checksum >> 24) & 0xFF;
    result[pos++] = (checksum >> 16) & 0xFF;
    result[pos++] = (checksum >> 8) & 0xFF;
    result[pos++] = checksum & 0xFF;
    
    return result;
}

/**
 * @brief Deserialize stream message header from wire bytes.
 * @param data Serialized header bytes.
 * @return Parsed header or std::nullopt for malformed/short input.
 */
std::optional<StreamMessageHeader> StreamMessageHeader::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < SIZE) {
        return std::nullopt;
    }
    
    StreamMessageHeader header;
    size_t pos = 0;
    
    header.version = data[pos++];
    header.type = static_cast<StreamMessageType>(data[pos++]);
    
    header.session_id = (static_cast<uint32_t>(data[pos]) << 24) |
                        (static_cast<uint32_t>(data[pos+1]) << 16) |
                        (static_cast<uint32_t>(data[pos+2]) << 8) |
                        static_cast<uint32_t>(data[pos+3]);
    pos += 4;
    
    header.sequence_number = 0;
    for (int i = 0; i < 8; ++i) {
        header.sequence_number = (header.sequence_number << 8) | data[pos++];
    }
    
    header.payload_length = (static_cast<uint32_t>(data[pos]) << 24) |
                           (static_cast<uint32_t>(data[pos+1]) << 16) |
                           (static_cast<uint32_t>(data[pos+2]) << 8) |
                           static_cast<uint32_t>(data[pos+3]);
    pos += 4;
    
    header.flags = (static_cast<uint32_t>(data[pos]) << 24) |
                   (static_cast<uint32_t>(data[pos+1]) << 16) |
                   (static_cast<uint32_t>(data[pos+2]) << 8) |
                   static_cast<uint32_t>(data[pos+3]);
    pos += 4;
    
    header.checksum = (static_cast<uint32_t>(data[pos]) << 24) |
                      (static_cast<uint32_t>(data[pos+1]) << 16) |
                      (static_cast<uint32_t>(data[pos+2]) << 8) |
                      static_cast<uint32_t>(data[pos+3]);
    
    return header;
}

// ============================================================================
// StreamChunk Implementation
// ============================================================================

/**
 * @brief Verify chunk payload checksum.
 * @return true when checksum matches serialized payload data.
 */
bool StreamChunk::verify() const {
    if (data.empty()) {
        return checksum == 0;
    }
    return calculateCRC32(data.data(), data.size()) == checksum;
}

/** @brief Serialize chunk metadata and payload into transport bytes. */
std::vector<uint8_t> StreamChunk::serialize() const {
    std::vector<uint8_t> result;
    
    // Reserve space
    result.reserve(24 + data.size());
    
    // file_offset (8 bytes, big-endian)
    for (int i = 7; i >= 0; --i) {
        result.push_back((file_offset >> (i * 8)) & 0xFF);
    }
    
    // chunk_index (4 bytes)
    result.push_back((chunk_index >> 24) & 0xFF);
    result.push_back((chunk_index >> 16) & 0xFF);
    result.push_back((chunk_index >> 8) & 0xFF);
    result.push_back(chunk_index & 0xFF);
    
    // uncompressed_size (4 bytes)
    result.push_back((uncompressed_size >> 24) & 0xFF);
    result.push_back((uncompressed_size >> 16) & 0xFF);
    result.push_back((uncompressed_size >> 8) & 0xFF);
    result.push_back(uncompressed_size & 0xFF);
    
    // compressed_size (4 bytes)
    result.push_back((compressed_size >> 24) & 0xFF);
    result.push_back((compressed_size >> 16) & 0xFF);
    result.push_back((compressed_size >> 8) & 0xFF);
    result.push_back(compressed_size & 0xFF);
    
    // checksum (4 bytes)
    result.push_back((checksum >> 24) & 0xFF);
    result.push_back((checksum >> 16) & 0xFF);
    result.push_back((checksum >> 8) & 0xFF);
    result.push_back(checksum & 0xFF);
    
    // data
    result.insert(result.end(), data.begin(), data.end());
    
    return result;
}

/**
 * @brief Deserialize chunk from transport bytes and validate metadata bounds.
 * @param data Serialized chunk bytes.
 * @return Parsed chunk or std::nullopt for malformed/inconsistent input.
 */
std::optional<StreamChunk> StreamChunk::deserialize(const std::vector<uint8_t>& data) {
    // W2-S03: Chunk metadata validation - fail-closed on malformed inputs
    if (data.size() < 24) {
        return std::nullopt;
    }
    
    StreamChunk chunk;
    size_t pos = 0;
    
    // file_offset
    chunk.file_offset = 0;
    for (int i = 0; i < 8; ++i) {
        chunk.file_offset = (chunk.file_offset << 8) | data[pos++];
    }
    
    // chunk_index
    chunk.chunk_index = (static_cast<uint32_t>(data[pos]) << 24) |
                        (static_cast<uint32_t>(data[pos+1]) << 16) |
                        (static_cast<uint32_t>(data[pos+2]) << 8) |
                        static_cast<uint32_t>(data[pos+3]);
    pos += 4;
    
    // uncompressed_size
    chunk.uncompressed_size = (static_cast<uint32_t>(data[pos]) << 24) |
                              (static_cast<uint32_t>(data[pos+1]) << 16) |
                              (static_cast<uint32_t>(data[pos+2]) << 8) |
                              static_cast<uint32_t>(data[pos+3]);
    pos += 4;
    
    // compressed_size
    chunk.compressed_size = (static_cast<uint32_t>(data[pos]) << 24) |
                            (static_cast<uint32_t>(data[pos+1]) << 16) |
                            (static_cast<uint32_t>(data[pos+2]) << 8) |
                            static_cast<uint32_t>(data[pos+3]);
    pos += 4;
    
    // checksum
    chunk.checksum = (static_cast<uint32_t>(data[pos]) << 24) |
                     (static_cast<uint32_t>(data[pos+1]) << 16) |
                     (static_cast<uint32_t>(data[pos+2]) << 8) |
                     static_cast<uint32_t>(data[pos+3]);
    pos += 4;
    
    // W2-S03: Validate chunk metadata consistency
    // Fail-closed if uncompressed_size exceeds 1GB (impossibly large single chunk)
    constexpr uint32_t MAX_UNCOMPRESSED_SIZE = 1024u * 1024u * 1024u;  // 1GB
    if (chunk.uncompressed_size == 0 || chunk.uncompressed_size > MAX_UNCOMPRESSED_SIZE) {
        return std::nullopt;
    }
    
    // Fail-closed if compressed_size doesn't match payload size
    size_t payload_size = data.size() - pos;
    if (chunk.compressed_size != payload_size) {
        return std::nullopt;
    }
    
    // Fail-closed if uncompressed_size < compressed_size (invalid compression claim)
    // Only allow when uncompressed and compressed are the same (no compression)
    if (chunk.compressed_size > 0 && chunk.uncompressed_size < chunk.compressed_size) {
        return std::nullopt;
    }
    
    // data
    if (data.size() > pos) {
        chunk.data.assign(data.begin() + pos, data.end());
    }
    
    return chunk;
}

// ============================================================================
// StreamFileProgress Implementation
// ============================================================================

/**
 * @brief Compute transfer throughput over observed activity window.
 * @return Average bytes per second since start_time.
 */
double StreamFileProgress::getThroughputBytesPerSecond() const {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        last_activity - start_time
    ).count();
    
    if (elapsed <= 0) {
      return 0.0;
    }
    return (bytes_transferred * 1000.0) / elapsed;
}

// ============================================================================
// StreamingStats Implementation
// ============================================================================

/** @brief Export streaming counters in Prometheus exposition format. */
std::string StreamingStats::toPrometheusFormat() const {
    std::ostringstream oss;
    
    oss << "# HELP themisdb_streaming_sessions_total Total streaming sessions\n"
        << "# TYPE themisdb_streaming_sessions_total counter\n"
        << "themisdb_streaming_sessions_total " << sessions_total.load() << "\n\n";
    
    oss << "# HELP themisdb_streaming_sessions_successful Successful streaming sessions\n"
        << "# TYPE themisdb_streaming_sessions_successful counter\n"
        << "themisdb_streaming_sessions_successful " << sessions_successful.load() << "\n\n";
    
    oss << "# HELP themisdb_streaming_sessions_failed Failed streaming sessions\n"
        << "# TYPE themisdb_streaming_sessions_failed counter\n"
        << "themisdb_streaming_sessions_failed " << sessions_failed.load() << "\n\n";
    
    oss << "# HELP themisdb_streaming_bytes_total Total bytes streamed\n"
        << "# TYPE themisdb_streaming_bytes_total counter\n"
        << "themisdb_streaming_bytes_sent_total " << bytes_sent_total.load() << "\n"
        << "themisdb_streaming_bytes_received_total " << bytes_received_total.load() << "\n\n";
    
    oss << "# HELP themisdb_streaming_chunks_total Total chunks streamed\n"
        << "# TYPE themisdb_streaming_chunks_total counter\n"
        << "themisdb_streaming_chunks_sent_total " << chunks_sent_total.load() << "\n"
        << "themisdb_streaming_chunks_received_total " << chunks_received_total.load() << "\n\n";
    
    oss << "# HELP themisdb_streaming_chunk_retries_total Total chunk retries\n"
        << "# TYPE themisdb_streaming_chunk_retries_total counter\n"
        << "themisdb_streaming_chunk_retries_total " << chunk_retries_total.load() << "\n\n";
    
    oss << "# HELP themisdb_streaming_compression_bytes_saved Bytes saved by compression\n"
        << "# TYPE themisdb_streaming_compression_bytes_saved counter\n"
        << "themisdb_streaming_compression_bytes_saved " << compression_bytes_saved.load() << "\n";
    
    return oss.str();
}

// ============================================================================
// StreamCompressor Implementation
// ============================================================================

/**
 * @brief Compress payload using selected algorithm, with passthrough fallback.
 * @return Compressed bytes, or original bytes when algorithm is unavailable/fails.
 */
std::vector<uint8_t> StreamCompressor::compress(
    const std::vector<uint8_t>& data,
    CompressionAlgorithm algorithm,
    [[maybe_unused]] int level) {
    
    if (data.empty() || algorithm == CompressionAlgorithm::NONE) {
        return data;
    }
    
#ifdef THEMIS_HAS_LZ4
    if (algorithm == CompressionAlgorithm::LZ4) {
        int max_dst_size = LZ4_compressBound(static_cast<int>(data.size()));
        std::vector<uint8_t> compressed(max_dst_size);
        
        int compressed_size = LZ4_compress_default(
            reinterpret_cast<const char*>(data.data()),
            reinterpret_cast<char*>(compressed.data()),
            static_cast<int>(data.size()),
            max_dst_size
        );
        
        if (compressed_size > 0) {
            compressed.resize(compressed_size);
            return compressed;
        }
    }
#endif

#ifdef THEMIS_HAS_ZSTD
    if (algorithm == CompressionAlgorithm::ZSTD) {
        size_t max_dst_size = ZSTD_compressBound(data.size());
        std::vector<uint8_t> compressed(max_dst_size);
        
        size_t compressed_size = ZSTD_compress(
            compressed.data(),
            max_dst_size,
            data.data(),
            data.size(),
            level
        );
        
        if (!ZSTD_isError(compressed_size)) {
            compressed.resize(compressed_size);
            return compressed;
        }
    }
#endif

    // Fallback: return uncompressed
    return data;
}

/**
 * @brief Decompress payload to expected size, with passthrough fallback.
 * @return Decompressed bytes, or original bytes when algorithm is unavailable/fails.
 */
std::vector<uint8_t> StreamCompressor::decompress(
    const std::vector<uint8_t>& data,
    CompressionAlgorithm algorithm,
    [[maybe_unused]] size_t uncompressed_size) {
    
    if (data.empty() || algorithm == CompressionAlgorithm::NONE) {
        return data;
    }
    
#ifdef THEMIS_HAS_LZ4
    if (algorithm == CompressionAlgorithm::LZ4) {
        std::vector<uint8_t> decompressed(uncompressed_size);
        
        int decompressed_size = LZ4_decompress_safe(
            reinterpret_cast<const char*>(data.data()),
            reinterpret_cast<char*>(decompressed.data()),
            static_cast<int>(data.size()),
            static_cast<int>(uncompressed_size)
        );
        
        if (decompressed_size > 0) {
            decompressed.resize(decompressed_size);
            return decompressed;
        }
    }
#endif

#ifdef THEMIS_HAS_ZSTD
    if (algorithm == CompressionAlgorithm::ZSTD) {
        std::vector<uint8_t> decompressed(uncompressed_size);
        
        size_t decompressed_size = ZSTD_decompress(
            decompressed.data(),
            uncompressed_size,
            data.data(),
            data.size()
        );
        
        if (!ZSTD_isError(decompressed_size)) {
            decompressed.resize(decompressed_size);
            return decompressed;
        }
    }
#endif

    // Fallback: return as-is
    return data;
}

/** @brief Return whether algorithm is compiled into current binary. */
bool StreamCompressor::isSupported(CompressionAlgorithm algorithm) {
    switch (algorithm) {
        case CompressionAlgorithm::NONE:
            return true;
#ifdef THEMIS_HAS_LZ4
        case CompressionAlgorithm::LZ4:
            return true;
#endif
#ifdef THEMIS_HAS_ZSTD
        case CompressionAlgorithm::ZSTD:
            return true;
#endif
        default:
            return false;
    }
}

// ============================================================================
// StreamRateLimiter Implementation
// ============================================================================

StreamRateLimiter::StreamRateLimiter(uint64_t bytes_per_second)
    : bytes_per_second_(bytes_per_second)
    , available_tokens_(bytes_per_second)
    , last_refill_(std::chrono::steady_clock::now()) {
}

std::chrono::milliseconds StreamRateLimiter::acquire([[maybe_unused]] size_t bytes) {
    if (bytes_per_second_.load() == 0) {
        return std::chrono::milliseconds(0);  // Unlimited
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Refill tokens based on time elapsed
    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_refill_
    ).count();
    
    if (elapsed_ms > 0) {
        uint64_t new_tokens = (bytes_per_second_.load() * elapsed_ms) / 1000;
        available_tokens_ = std::min(
            available_tokens_.load() + new_tokens,
            bytes_per_second_.load()  // Cap at 1 second worth
        );
        last_refill_ = now;
    }
    
    // Check if we have enough tokens
    if (available_tokens_.load() >= bytes) {
        available_tokens_ -= bytes;
        return std::chrono::milliseconds(0);
    }
    
    // Calculate wait time
    uint64_t needed = bytes - available_tokens_.load();
    uint64_t wait_ms = (needed * 1000) / bytes_per_second_.load();
    
    return std::chrono::milliseconds(wait_ms + 1);
}

void StreamRateLimiter::setRate([[maybe_unused]] uint64_t bytes_per_second) {
    bytes_per_second_.store(bytes_per_second);
}

// ============================================================================
// StreamSession Implementation
// ============================================================================

StreamSession::StreamSession(const StreamSessionConfig& config)
    : config_(config)
    , session_id_(generateSessionId()) {
    
    if (config_.throttle.max_bytes_per_second > 0) {
        rate_limiter_ = std::make_shared<StreamRateLimiter>(
            config_.throttle.max_bytes_per_second
        );
    }
}

StreamSession::~StreamSession() {
    abort("Session destroyed");
}

bool StreamSession::initialize() {
    if (state_.load() != StreamSessionState::INITIALIZED) {
        return false;
    }
    
    transitionState(StreamSessionState::PREPARING);

    // Validate that a remote endpoint is configured — without one, no
    // PREPARE_REQUEST can ever be sent (even with a real transport).
    if (config_.remote_endpoint.empty()) {
        transitionState(StreamSessionState::ABORTED);
        return false;
    }

    // If a preparation callback has been injected (e.g. a real mTLS transport
    // that exchanges PREPARE_REQUEST / PREPARE_ACK), delegate to it.
    std::function<bool()> prepare_callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        prepare_callback = prepare_callback_;
    }
    if ([[maybe_unused]] prepare_callback) {
        const bool prepared = prepare_callback();
        if (!prepared) {
            transitionState(StreamSessionState::ABORTED);
        }
        return prepared;
    }

    // No transport callback wired yet.
    // NON-PRODUCTION PATH (Simulation/Stub/Mockup)
    // Purpose: Allow the StreamSession state machine to advance past PREPARING
    //          while the mTLS PREPARE_REQUEST/PREPARE_ACK exchange is not yet
    //          wired up.  The remote endpoint is validated above to catch
    //          misconfigured sessions early.
    // Activation: `prepare_callback_` not set (no real transport injected).
    // Production Delta: No actual connection is established; the remote side
    //                   has no matching prepared session, so subsequent data
    //                   transfers will fail on the remote end.
    // Removal Plan: Inject a real mTLS preparation callback via
    //               `setPrepareTransferCallback()` and remove this fallback.
    //               See src/sharding/FUTURE_ENHANCEMENTS.md §Stream Protocol PrepareTransfer.
    
    spdlog::warn("StreamSession::initialize() using in-process simulation (no mTLS prepare callback). "
                 "This is a test-only configuration. Remote endpoint: {}. "
                 "Subsequent data transfers will fail unless a real transport callback is injected.",
                 config_.remote_endpoint);
    return true;
}

void StreamSession::addFile(const StreamFileInfo& file) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // W2-S06: Consensus validation — validate file before adding to stream
    if (file.file_id.empty()) {
        spdlog::error("StreamSession::addFile: file has empty file_id, rejecting");
        return;
    }
    
    if (file.collection_name.empty()) {
        spdlog::error("StreamSession::addFile: file has empty collection_name, rejecting");
        return;
    }
    
    files_.push_back(file);
}

bool StreamSession::start() {
    if (state_.load() != StreamSessionState::PREPARING) {
        return false;
    }
    
    transitionState(StreamSessionState::STREAMING);
    running_.store(true);
    
    // Start worker threads
    session_thread_ = std::thread(&StreamSession::sessionLoop, this);
    heartbeat_thread_ = std::thread(&StreamSession::heartbeatLoop, this);
    
    return true;
}

void StreamSession::pause() {
    std::vector<StreamTransferTask*> tasks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks.reserve(transfer_tasks_.size());
        for (auto& [id, task] : transfer_tasks_) {
            tasks.push_back(task.get());
        }
    }

    for (auto* task : tasks) {
        if (task) {
            task->pause();
        }
    }
}

void StreamSession::resume() {
    std::vector<StreamTransferTask*> tasks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks.reserve(transfer_tasks_.size());
        for (auto& [id, task] : transfer_tasks_) {
            tasks.push_back(task.get());
        }
    }

    for (auto* task : tasks) {
        if (task) {
            task->resume();
        }
    }
}

void StreamSession::abort(const std::string& reason) {
    if (!running_.exchange(false)) {
        return;
    }
    
    transitionState(StreamSessionState::ABORTED);

    std::vector<StreamTransferTask*> transfer_task_ptrs;
    std::vector<StreamReceiveTask*> receive_task_ptrs;
    StreamCompletionCallback completion_callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        transfer_task_ptrs.reserve(transfer_tasks_.size());
        for (auto& [id, task] : transfer_tasks_) {
            transfer_task_ptrs.push_back(task.get());
        }
        receive_task_ptrs.reserve(receive_tasks_.size());
        for (auto& [id, task] : receive_tasks_) {
            receive_task_ptrs.push_back(task.get());
        }
        completion_callback = completion_callback_;
    }
    
    for (auto* task : transfer_task_ptrs) {
        if (task) {
            task->abort();
        }
    }
    for (auto* task : receive_task_ptrs) {
        if (task) {
            task->abort();
        }
    }
    
    cv_.notify_all();
    
    if (session_thread_.joinable()) {
        const bool joined = themis::utils::joinThreadWithin(session_thread_);
        if (!joined) {
            spdlog::warn("StreamSession shutdown join timed out for session thread");
        }
    }
    if (heartbeat_thread_.joinable()) {
        const bool joined = themis::utils::joinThreadWithin(heartbeat_thread_);
        if (!joined) {
            spdlog::warn("StreamSession shutdown join timed out for heartbeat thread");
        }
    }
    
    if ([[maybe_unused]] completion_callback) {
        completion_callback(session_id_, false, reason);
    }
}

StreamSessionProgress StreamSession::getProgress() const {
    StreamSessionProgress progress;
    progress.session_id = session_id_;
    progress.state = state_.load();
    progress.remote_shard_id = config_.remote_shard_id;
    progress.direction = config_.direction;
    
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    
    progress.files_total = static_cast<uint32_t>(files_.size());
    
    if (config_.direction == StreamDirection::OUTGOING) {
        for (const auto& [id, task] : transfer_tasks_) {
            auto file_progress = task->getProgress();
            progress.file_progress.push_back(file_progress);
            progress.bytes_transferred += file_progress.bytes_transferred;
            progress.total_bytes += file_progress.total_bytes;
            if (task->isComplete()) {
                progress.files_completed++;
            }
        }
    } else {
        for (const auto& [id, task] : receive_tasks_) {
            auto file_progress = task->getProgress();
            progress.file_progress.push_back(file_progress);
            progress.bytes_transferred += file_progress.bytes_transferred;
            progress.total_bytes += file_progress.total_bytes;
            if (task->isComplete()) {
                progress.files_completed++;
            }
        }
    }
    
    return progress;
}

void StreamSession::setProgressCallback([[maybe_unused]] StreamProgressCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    progress_callback_ = std::move([[maybe_unused]] callback);
}

void StreamSession::setCompletionCallback([[maybe_unused]] StreamCompletionCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    completion_callback_ = std::move([[maybe_unused]] callback);
}

void StreamSession::setPrepareTransferCallback([[maybe_unused]] std::function<bool()> cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    prepare_callback_ = std::move([[maybe_unused]] cb);
}

bool StreamSession::isActive() const {
    auto state = state_.load();
    return state == StreamSessionState::PREPARING || 
           state == StreamSessionState::STREAMING;
}

void StreamSession::sessionLoop() {
    // W5-Sharding: Guard session state transitions and task completion checks
    // Ensures all state updates are protected from concurrent access
    while (running_.load(std::memory_order_acquire)) {
        // Check for completion
        bool all_complete = true;
        bool any_failed = false;
        
        bool has_files = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            has_files = !files_.empty();
            
            if (config_.direction == StreamDirection::OUTGOING) {
                for (const auto& [id, task] : transfer_tasks_) {
                    if (!task->isComplete()) {
                        all_complete = false;
                    }
                    if (task->isFailed()) {
                        any_failed = true;
                    }
                }
            } else {
                for (const auto& [id, task] : receive_tasks_) {
                    if (!task->isComplete()) {
                        all_complete = false;
                    }
                }
            }
        }
        
        if (any_failed) {
            // W5-Sharding: State transition guarded by atomic store
            transitionState(StreamSessionState::FAILED);
            running_.store(false, std::memory_order_release);

            StreamCompletionCallback completion_callback;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                completion_callback = completion_callback_;
            }
            if ([[maybe_unused]] completion_callback) {
                completion_callback(session_id_, false, "Transfer task failed");
            }
            break;
        }
        
        if (all_complete && has_files) {
            // W5-Sharding: State transition guarded by atomic store
            transitionState(StreamSessionState::COMPLETE);
            running_.store(false, std::memory_order_release);

            StreamCompletionCallback completion_callback;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                completion_callback = completion_callback_;
            }
            if ([[maybe_unused]] completion_callback) {
                completion_callback(session_id_, true, "");
            }
            break;
        }
        
        // Report progress
        notifyProgress();
        
        // Sleep briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void StreamSession::heartbeatLoop() {
    // W5-Sharding: Guard heartbeat loop with proper memory ordering
    while (running_.load(std::memory_order_acquire)) {
        // Send heartbeat
        sendMessage(StreamMessageType::HEARTBEAT, {});
        
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS), [this] {
            return !running_.load(std::memory_order_acquire);
        });
    }
}

void StreamSession::notifyProgress() {
    StreamProgressCallback progress_callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        progress_callback = progress_callback_;
    }
    if ([[maybe_unused]] progress_callback) {
        progress_callback([[maybe_unused]] getProgress());
    }
}

void StreamSession::transitionState(StreamSessionState new_state) {
    state_.store(new_state);
}

bool StreamSession::sendMessage([[maybe_unused]] StreamMessageType type, [[maybe_unused]] const std::vector<uint8_t>& payload) {
    // NON-PRODUCTION PATH (Simulation/Stub/Mockup)
    // Purpose: Allow testing of the heartbeat mechanism when mTLS transport not wired.
    // Activation: No real mTLS client injected.
    // Production Delta: No actual message transmission over network; local stub always succeeds.
    // Removal Plan: Replace with real mTLS client implementation (see FUTURE_ENHANCEMENTS.md).
    
    // In real implementation, this would send over the mTLS connection with retry logic:
    // return retryWithBackoff([this, &type, &payload]() -> bool {
    //     try {
    //         auto result = mtls_client_->send(payload);
    //         return result.ok();
    //     } catch (const std::exception&) {
    //         return false;  // Transient failure, retry
    //     }
    // }, 3, 100, 5000);
    
    return true;
}

// ============================================================================
// StreamCoordinator Implementation (Singleton)
// ============================================================================

StreamCoordinator& StreamCoordinator::getInstance() {
    static StreamCoordinator instance;
    return instance;
}

void StreamCoordinator::initialize(const StreamThrottleConfig& throttle_config) {
    if (initialized_.exchange(true)) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    throttle_config_ = throttle_config;

    if (throttle_config_.max_bytes_per_second > 0) {
        global_rate_limiter_ = std::make_shared<StreamRateLimiter>(
            throttle_config_.max_bytes_per_second
        );
    }
}

void StreamCoordinator::shutdown() {
    if (!initialized_.exchange(false)) {
        return;
    }

    std::vector<std::shared_ptr<StreamPlan>> active_plans;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_plans = active_plans_;
        active_plans_.clear();
    }

    for (auto& plan : active_plans) {
        if (plan) {
            plan->abort();
        }
    }
}

std::shared_ptr<StreamPlan> StreamCoordinator::createPlan(const StreamPlanConfig& config) {
    auto plan = std::make_shared<StreamPlan>(config);
    
    std::lock_guard<std::mutex> lock(mutex_);
    active_plans_.push_back(plan);
    
    return plan;
}

std::vector<std::shared_ptr<StreamPlan>> StreamCoordinator::getActivePlans() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_plans_;
}

void StreamCoordinator::updateThrottleConfig(const StreamThrottleConfig& config) {
    std::shared_ptr<StreamRateLimiter> global_rate_limiter;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        throttle_config_ = config;
        global_rate_limiter = global_rate_limiter_;
    }
    
    if (global_rate_limiter && config.max_bytes_per_second > 0) {
        global_rate_limiter->setRate(config.max_bytes_per_second);
    }
}

// ============================================================================
// StreamPlan Implementation
// ============================================================================

StreamPlan::StreamPlan(const StreamPlanConfig& config)
    : config_(config) {
}

StreamPlan::~StreamPlan() {
    abort();
}

void StreamPlan::addSession(std::unique_ptr<StreamSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // W2-S06: Consensus validation — validate session before adding to plan
    if (!session) {
        spdlog::error("StreamPlan::addSession: session is null, rejecting");
        return;
    }
    
    if (session->getSessionId() == 0) {
        spdlog::error("StreamPlan::addSession: session has invalid session_id=0, rejecting");
        return;
    }
    
    sessions_.push_back(std::move(session));
}

bool StreamPlan::execute() {
    if (running_.exchange(true)) {
        return false;
    }
    
    executor_thread_ = std::thread(&StreamPlan::executorLoop, this);
    return true;
}

void StreamPlan::abort() {
    if (!running_.exchange(false)) {
        return;
    }
    
    cv_.notify_all();
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& session : sessions_) {
            session->abort("Plan aborted");
        }
    }
    
    if (executor_thread_.joinable()) {
        const bool joined = themis::utils::joinThreadWithin(executor_thread_);
        if (!joined) {
            spdlog::warn("StreamPlan shutdown join timed out for executor thread");
        }
    }
}

bool StreamPlan::waitForCompletion(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (timeout == std::chrono::milliseconds::max()) {
        cv_.wait(lock, [this] { return complete_.load(); });
    } else {
        cv_.wait_for(lock, timeout, [this] { return complete_.load(); });
    }
    
    return successful_.load();
}

std::vector<StreamSessionProgress> StreamPlan::getProgress() const {
    std::vector<StreamSessionProgress> progress;
    
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    for (const auto& session : sessions_) {
        progress.push_back(session->getProgress());
    }
    
    return progress;
}

void StreamPlan::addListener([[maybe_unused]] std::shared_ptr<IStreamListener> listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    listeners_.push_back([[maybe_unused]] listener);
}

void StreamPlan::executorLoop() {
    // Initialize all sessions
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& session : sessions_) {
            session->initialize();
        }
    }
    
    // Start sessions (up to max_concurrent)
    size_t active_count = 0;
    size_t next_session = 0;
    
    while (running_.load()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Start new sessions if we have capacity
            while (active_count < config_.max_concurrent_sessions && 
                   next_session < sessions_.size()) {
                sessions_[next_session]->start();
                next_session++;
                active_count++;
            }
            
            // Check session status
            bool all_complete = true;
            bool any_failed = false;
            
            for (const auto& session : sessions_) {
                auto state = session->getState();
                
                if (state == StreamSessionState::COMPLETE) {
                    // Session completed
                } else if (state == StreamSessionState::FAILED || 
                          state == StreamSessionState::ABORTED) {
                    any_failed = true;
                } else if (session->isActive()) {
                    all_complete = false;
                }
            }
            
            if (all_complete && next_session >= sessions_.size()) {
                successful_.store(!any_failed);
                complete_.store(true);
                running_.store(false);
                cv_.notify_all();
                break;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void StreamPlan::notifyListeners(std::function<void(IStreamListener&)> callback) {
    std::vector<std::shared_ptr<IStreamListener>> listeners;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners = listeners_;
    }

    for ([[maybe_unused]] auto& listener : listeners) {
        if ([[maybe_unused]] listener) {
            callback([[maybe_unused]] *listener);
        }
    }
}

// ============================================================================
// StreamTransferTask Implementation
// ============================================================================

StreamTransferTask::StreamTransferTask(
    const StreamFileInfo& file,
    std::shared_ptr<StreamRateLimiter> rate_limiter,
    const StreamSessionConfig& config
) : file_(file), rate_limiter_(rate_limiter), config_(config) {
    const auto total_size = file_.file_size;
    chunks_acked_.resize((total_size + config.chunk_size - 1) / config.chunk_size, false);
}

StreamTransferTask::~StreamTransferTask() {
    abort();
    if (transfer_thread_.joinable()) {
        const bool joined = themis::utils::joinThreadWithin(transfer_thread_);
        if (!joined) {
            spdlog::warn("StreamTransferTask join timed out during destruction");
        }
    }
}

bool StreamTransferTask::start() {
    running_.store(true, std::memory_order_release);
    transfer_thread_ = std::thread(&StreamTransferTask::transferLoop, this);
    return true;
}

void StreamTransferTask::pause() {
    paused_.store(true, std::memory_order_release);
}

void StreamTransferTask::resume() {
    paused_.store(false, std::memory_order_release);
    cv_.notify_all();
}

void StreamTransferTask::abort() {
    running_.store(false, std::memory_order_release);
    failed_.store(true, std::memory_order_release);
    cv_.notify_all();
}

void StreamTransferTask::onChunkAck([[maybe_unused]] uint32_t chunk_index) {
    {
        std::lock_guard<std::mutex> lock(progress_mutex_);
        if (chunk_index < chunks_acked_.size()) {
            chunks_acked_[chunk_index] = true;
        }
    }
    cv_.notify_all();
}

void StreamTransferTask::onRetryRequest([[maybe_unused]] uint32_t chunk_index) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_retries_.push(chunk_index);
    }
    cv_.notify_all();
}

StreamFileProgress StreamTransferTask::getProgress() const {
    std::lock_guard<std::mutex> lock(progress_mutex_);
    return progress_;
}

void StreamTransferTask::transferLoop() {
    while (running_.load(std::memory_order_acquire)) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // Use wait_for with the session timeout to prevent indefinite blocking
            // if paused_ is set but never cleared (e.g. caller forgets to resume).
            const auto wait_dur = std::chrono::milliseconds(
                config_.timeout_ms > 0 ? config_.timeout_ms : DEFAULT_TIMEOUT_MS);
            cv_.wait_for(lock, wait_dur, [this] { 
                return !paused_.load(std::memory_order_acquire) || !running_.load(std::memory_order_acquire); 
            });
        }
        
        if (!running_.load(std::memory_order_acquire)) {
          break;
        }

        for (;;) {
            uint32_t chunk_index = 0;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (pending_retries_.empty()) {
                    break;
                }
                chunk_index = pending_retries_.front();
                pending_retries_.pop();
            }

            if (auto chunk = createChunk(chunk_index)) {
                sendChunk(*chunk);
            }
        }
        
        uint32_t chunk_index = 0;
        bool has_chunk = false;
        bool already_acked = false;
        bool transfer_complete = false;
        {
            std::lock_guard<std::mutex> lock(progress_mutex_);
            if (next_chunk_to_send_ < chunks_acked_.size()) {
                chunk_index = next_chunk_to_send_;
                already_acked = chunks_acked_[chunk_index];
                has_chunk = true;
                if (already_acked) {
                    ++next_chunk_to_send_;
                }
            } else {
                transfer_complete = true;
            }
        }

        if (transfer_complete) {
            complete_.store(true, std::memory_order_release);
            break;
        }

        if (has_chunk && !already_acked) {
            if (auto chunk = createChunk(chunk_index)) {
                if (sendChunk(*chunk)) {
                    std::lock_guard<std::mutex> lock(progress_mutex_);
                    if (next_chunk_to_send_ == chunk_index) {
                        ++next_chunk_to_send_;
                    }
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::optional<StreamChunk> StreamTransferTask::createChunk([[maybe_unused]] uint32_t chunk_index) {
    StreamChunk chunk;
    chunk.chunk_index = chunk_index;
    chunk.file_offset = static_cast<uint64_t>(chunk_index) * config_.chunk_size;

    const uint64_t remaining = file_.file_size > chunk.file_offset
        ? file_.file_size - chunk.file_offset
        : 0;

    chunk.uncompressed_size = static_cast<uint32_t>(
        std::min<uint64_t>(config_.chunk_size, remaining));

    if (chunk.uncompressed_size == 0) {
        return std::nullopt;
    }

    // Read data from source file
    chunk.data.resize(chunk.uncompressed_size);
    std::ifstream file(file_.source_path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << file_.source_path << std::endl;
        return std::nullopt;
    }

    file.seekg(chunk.file_offset);
    file.read(reinterpret_cast<char*>(chunk.data.data()), chunk.uncompressed_size);
    if (!file) {
        std::cerr << "Failed to read chunk at offset " << chunk.file_offset << std::endl;
        return std::nullopt;
    }

    // Calculate checksum
    chunk.checksum = calculateCRC32(chunk.data.data(), chunk.uncompressed_size);

    // Compress if enabled; only keep compressed if it's smaller
    if (config_.compression != CompressionAlgorithm::NONE) {
        auto compressed = StreamCompressor::compress(
            chunk.data, config_.compression, config_.compression_level);
        if (compressed.size() < chunk.uncompressed_size) {
            chunk.compressed_size = static_cast<uint32_t>(compressed.size());
            chunk.data = std::move(compressed);
        } else {
            chunk.compressed_size = chunk.uncompressed_size;
        }
    } else {
        chunk.compressed_size = chunk.uncompressed_size;
    }

    return chunk;
}

bool StreamTransferTask::sendChunk(const StreamChunk& chunk) {
    // W2-S07: Document stream transfer semantics
    // - Local staging: written to temporary file without replication
    // - Remote setup: would require consensus before durable write
    // - Recovery: based on source file and transaction log, not staging files
    
    // Local implementation: write chunk to temporary staging area
    // In distributed setup, this would send via RPC/network to target shard
    
    // Security: Use platform-specific temporary directory instead of hardcoded /tmp
    std::filesystem::path staging_dir = std::filesystem::temp_directory_path() / "themis_stream_staging";
    if (!std::filesystem::exists(staging_dir)) {
        try {
            std::filesystem::create_directories(staging_dir);
            // Set restrictive permissions (owner read/write only) on Unix-like systems
            #ifndef _WIN32
            std::filesystem::permissions(staging_dir, 
                                       std::filesystem::perms::owner_read | 
                                       std::filesystem::perms::owner_write | 
                                       std::filesystem::perms::owner_exec,
                                       std::filesystem::perm_options::replace);
            #endif
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Failed to create staging directory: " << e.what() << std::endl;
            return false;
        }
    }

    // W2-S07: Validate chunk before writing to persistent storage
    if (chunk.chunk_index == std::numeric_limits<uint32_t>::max()) {
        spdlog::error("Invalid chunk index for staging write");
        return false;
    }
    if (chunk.data.size() > 1024 * 1024 * 1024) {  // 1GB max chunk
        spdlog::error("Chunk data exceeds maximum size ({})", chunk.data.size());
        return false;
    }
    
    // BATCH 5: Add retry logic for transient I/O failures with exponential backoff
    // Transient errors: file system temporarily unavailable, permission denied temporarily, etc.
    std::filesystem::path chunk_file = staging_dir / 
        (file_.file_id + "_chunk_" + std::to_string(chunk.chunk_index) + ".dat");
    
    // Lambda to perform chunk write operation with retry capability
    auto writeChunkWithRetry = [&chunk, &chunk_file]() -> bool {
        try {
            std::ofstream out(chunk_file, std::ios::binary);
            if (!out) {
                spdlog::warn("Failed to open chunk file for writing: {}", chunk_file.string());
                return false;  // Transient failure - retry
            }

            // Write chunk metadata and data
            out.write(reinterpret_cast<const char*>(&chunk.chunk_index), sizeof(chunk.chunk_index));
            out.write(reinterpret_cast<const char*>(&chunk.file_offset), sizeof(chunk.file_offset));
            out.write(reinterpret_cast<const char*>(&chunk.uncompressed_size), sizeof(chunk.uncompressed_size));
            out.write(reinterpret_cast<const char*>(&chunk.compressed_size), sizeof(chunk.compressed_size));
            out.write(reinterpret_cast<const char*>(&chunk.checksum), sizeof(chunk.checksum));
            out.write(reinterpret_cast<const char*>(chunk.data.data()), chunk.data.size());

            if (!out.good()) {
                spdlog::warn("Failed to write chunk data to file: {}", chunk_file.string());
                return false;  // Transient failure - retry
            }
            
            return true;  // Success
        } catch (const std::filesystem::filesystem_error& e) {
            spdlog::warn("Filesystem error writing chunk {}: {}", chunk_file.string(), e.what());
            return false;  // Transient failure - retry
        } catch (const std::exception& e) {
            spdlog::error("Unexpected exception writing chunk {}: {}", chunk_file.string(), e.what());
            return false;  // Transient failure - retry
        }
    };
    
    // BATCH 5: Retry with exponential backoff (max 3 attempts, 50-1000ms delays)
    return retryWithBackoff(
        writeChunkWithRetry,
        3,      // max_retries
        50,     // initial_delay_ms (shorter for I/O operations)
        1000    // max_delay_ms
    );
}

// ============================================================================
// StreamReceiveTask Implementation
// ============================================================================

StreamReceiveTask::StreamReceiveTask(
    const StreamFileInfo& file,
    const std::string& output_path,
    const StreamSessionConfig& config
) : file_(file), output_path_(output_path), config_(config) {
    const auto total_size = file_.file_size;
    chunks_received_.resize((total_size + config.chunk_size - 1) / config.chunk_size, false);
}

StreamReceiveTask::~StreamReceiveTask() {
    abort();
}

bool StreamReceiveTask::start() {
    running_.store(true, std::memory_order_release);
    
    // Create output directory if it doesn't exist
    std::filesystem::path out_path(output_path_);
    std::filesystem::path parent_dir = out_path.parent_path();
    
    if (!parent_dir.empty() && !std::filesystem::exists(parent_dir)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(parent_dir, ec)) {
            std::cerr << "Failed to create output directory: " << parent_dir 
                      << " - " << ec.message() << std::endl;
            failed_.store(true, std::memory_order_release);
            return false;
        }
    }
    
    // Open output file for writing
    std::ofstream test_file(output_path_, std::ios::binary | std::ios::trunc);
    if (!test_file.is_open()) {
        std::cerr << "Failed to open output file for writing: " << output_path_ << std::endl;
        failed_.store(true, std::memory_order_release);
        return false;
    }
    test_file.close();
    
    return true;
}

bool StreamReceiveTask::onChunkReceived(const StreamChunk& chunk) {
    if (!running_.load(std::memory_order_acquire) || 
        failed_.load(std::memory_order_acquire) || 
        complete_.load(std::memory_order_acquire)) {
        return false;
    }

    if (chunk.chunk_index >= chunks_received_.size()) {
        std::cerr << "Rejecting chunk with out-of-range index " << chunk.chunk_index
                  << " for file " << file_.file_id << std::endl;
        failed_.store(true, std::memory_order_release);
        return false;
    }

    if (chunk.compressed_size != chunk.data.size() || chunk.uncompressed_size == 0 ||
        chunk.compressed_size > chunk.uncompressed_size) {
        std::cerr << "Rejecting chunk " << chunk.chunk_index
                  << " due to inconsistent size metadata" << std::endl;
        failed_.store(true, std::memory_order_release);
        return false;
    }

    const uint64_t expected_offset = static_cast<uint64_t>(chunk.chunk_index) * config_.chunk_size;
    const uint64_t remaining = file_.file_size > expected_offset
        ? file_.file_size - expected_offset
        : 0;
    const uint32_t expected_uncompressed = static_cast<uint32_t>(
        std::min<uint64_t>(config_.chunk_size, remaining));

    if (chunk.file_offset != expected_offset || expected_uncompressed == 0 ||
        chunk.uncompressed_size != expected_uncompressed) {
        std::cerr << "Rejecting chunk " << chunk.chunk_index
                  << " due to unexpected offset/size metadata" << std::endl;
        failed_.store(true, std::memory_order_release);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(write_mutex_);

        if (chunk.chunk_index < next_expected_chunk_ || chunks_received_[chunk.chunk_index]) {
            std::cerr << "Rejecting stale or duplicate chunk " << chunk.chunk_index
                      << " for file " << file_.file_id << std::endl;
            failed_.store(true, std::memory_order_release);
            return false;
        }

        if (chunk.chunk_index > next_expected_chunk_ &&
            out_of_order_chunks_.count(chunk.chunk_index) > 0) {
            std::cerr << "Rejecting duplicate buffered chunk " << chunk.chunk_index
                      << " for file " << file_.file_id << std::endl;
            failed_.store(true, std::memory_order_release);
            return false;
        }

        if (chunk.chunk_index == next_expected_chunk_) {
            if (!writeChunk(chunk)) {
                chunks_received_[chunk.chunk_index] = false;
                std::cerr << "Failed to write chunk " << chunk.chunk_index
                          << " for file " << file_.file_id << std::endl;
                failed_.store(true, std::memory_order_release);
                return false;
            }
            chunks_received_[chunk.chunk_index] = true;
            next_expected_chunk_++;
            
            while (out_of_order_chunks_.count(next_expected_chunk_)) {
                if (!writeChunk(out_of_order_chunks_[next_expected_chunk_])) {
                    chunks_received_[next_expected_chunk_] = false;
                    std::cerr << "Failed to flush buffered chunk " << next_expected_chunk_
                              << " for file " << file_.file_id << std::endl;
                    failed_.store(true, std::memory_order_release);
                    return false;
                }
                chunks_received_[next_expected_chunk_] = true;
                out_of_order_chunks_.erase(next_expected_chunk_);
                next_expected_chunk_++;
            }
        } else {
            out_of_order_chunks_[chunk.chunk_index] = chunk;
            chunks_received_[chunk.chunk_index] = true;
        }

        const bool all_received = std::all_of(
            chunks_received_.begin(), chunks_received_.end(), []([[maybe_unused]] bool received) {
                return received;
            });
        if (all_received) {
            complete_.store(true, std::memory_order_release);
        }
    }
    
    return true;
}

void StreamReceiveTask::abort() {
    running_.store(false, std::memory_order_release);
    failed_.store(true, std::memory_order_release);
}

StreamFileProgress StreamReceiveTask::getProgress() const {
    std::lock_guard<std::mutex> lock(progress_mutex_);
    return progress_;
}

bool StreamReceiveTask::verifyIntegrity() const {
    if (!std::filesystem::exists(output_path_)) {
        std::cerr << "Output file does not exist: " << output_path_ << std::endl;
        return false;
    }
    
    // Check file size matches expected size
    auto file_size = std::filesystem::file_size(output_path_);
    if (file_size != file_.file_size) {
        std::cerr << "File size mismatch: expected " << file_.file_size 
                  << " but got " << file_size << std::endl;
        return false;
    }
    
    // File-level checksum verification is disabled here because StreamFileInfo
    // no longer contains a numeric checksum field. Per-chunk CRC checks remain in place.
    
    return true;
}

bool StreamReceiveTask::writeChunk(const StreamChunk& chunk) {
    if (chunk.compressed_size != chunk.data.size() || chunk.uncompressed_size == 0 ||
        chunk.compressed_size > chunk.uncompressed_size) {
        std::cerr << "Rejecting chunk " << chunk.chunk_index
                  << " due to invalid payload metadata" << std::endl;
        return false;
    }

    if (chunk.file_offset > file_.file_size ||
        file_.file_size - chunk.file_offset < chunk.uncompressed_size) {
        std::cerr << "Rejecting chunk " << chunk.chunk_index
                  << " due to out-of-bounds file range" << std::endl;
        return false;
    }

    // Decompress if needed
    std::vector<uint8_t> write_data = {};

    if (chunk.compressed_size < chunk.uncompressed_size) {
        // Data is compressed, decompress it
        write_data = StreamCompressor::decompress(
            chunk.data, config_.compression, chunk.uncompressed_size);
        if (write_data.size() != chunk.uncompressed_size) {
            std::cerr << "Failed to decompress chunk " << chunk.chunk_index << std::endl;
            return false;
        }
    } else {
        write_data = chunk.data;
    }

    // Verify checksum
    uint32_t computed_checksum = calculateCRC32(write_data.data(), write_data.size());
    if (computed_checksum != chunk.checksum) {
        std::cerr << "Checksum mismatch for chunk " << chunk.chunk_index 
                  << " (expected: " << chunk.checksum 
                  << ", got: " << computed_checksum << ")" << std::endl;
        return false;
    }

    // Write to target file
    const std::string& target_path = output_path_.empty() ? file_.target_path : output_path_;
    if (target_path.empty()) {
        std::cerr << "No target path configured for chunk " << chunk.chunk_index << std::endl;
        return false;
    }

    std::ofstream file(target_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) {
        // File doesn't exist, create it
        file.open(target_path, std::ios::binary | std::ios::out);
        if (!file) {
            std::cerr << "Failed to open target file: " << target_path << std::endl;
            return false;
        }
    }

    file.seekp(chunk.file_offset);
    file.write(reinterpret_cast<const char*>(write_data.data()), write_data.size());
    
    if (!file.good()) {
        std::cerr << "Failed to write chunk " << chunk.chunk_index 
                  << " to file at offset " << chunk.file_offset << std::endl;
        return false;
    }

    return true;
}

void StreamReceiveTask::requestRetry([[maybe_unused]] uint32_t chunk_index) {
    // Request retry for a specific chunk
    // In a real implementation, this would send a network message to the sender
    // For now, we log the retry request
    
    std::cerr << "Requesting retry for chunk " << chunk_index 
              << " of file_id " << file_.file_id << std::endl;
    
    // In production, this would:
    // 1. Create a RETRY_REQUEST message
    // 2. Include the session_id, file_id, and chunk_index
    // 3. Send via the network layer to the sender shard
    // 
    // Example structure:
    // StreamMessage retry_msg;
    // retry_msg.type = StreamMessageType::RETRY_REQUEST;
    // retry_msg.session_id = config_.session_id;
    // retry_msg.file_id = file_.file_id;
    // retry_msg.chunk_index = chunk_index;
    // network_->sendMessage(config_.peer_address, retry_msg);
    
    // Mark chunk as not received so it can be processed again
    if (chunk_index < chunks_received_.size()) {
        std::lock_guard<std::mutex> lock(write_mutex_);
        chunks_received_[chunk_index] = false;
    }
}

} // namespace streaming
} // namespace themisdb

