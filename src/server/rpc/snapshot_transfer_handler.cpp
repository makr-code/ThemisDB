#include "server/rpc/snapshot_transfer_handler.h"
#include <rocksdb/db.h>
#include <rocksdb/checkpoint.h>
#include <rocksdb/options.h>
#include <zstd.h>
#include <lz4.h>
#include <snappy.h>
#include <crc32c/crc32c.h>
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace themis {
namespace rpc {

namespace fs = std::filesystem;

// Implementation class (PIMPL pattern)
class SnapshotTransferHandler::Impl {
public:
    Impl() 
        : db_(nullptr)
        , checkpoint_(nullptr)
        , total_bytes_(0)
        , transferred_bytes_(0)
        , total_chunks_(0)
        , transferred_chunks_(0)
        , cancelled_(false)
        , start_time_(std::chrono::steady_clock::now()) {}
    
    ~Impl() {
        if (checkpoint_) {
            delete checkpoint_;
        }
    }
    
    SnapshotStatus CreateSnapshot(const SnapshotConfig& config) {
        config_ = config;
        
        // Validate configuration
        if (config_.shard_id.empty()) {
            return SnapshotStatus::ERROR_INVALID_CONFIG;
        }
        
        // TODO: Get RocksDB instance from ThemisDB
        // For now, this is a placeholder
        // db_ = themis::storage::GetShardDB(config_.shard_id);
        
        if (!db_) {
            return SnapshotStatus::ERROR_ROCKSDB_ERROR;
        }
        
        // Create checkpoint directory
        snapshot_dir_ = "/tmp/themis_snapshots/" + config_.snapshot_id;
        fs::create_directories(snapshot_dir_);
        
        // Create RocksDB checkpoint
        rocksdb::Status s = rocksdb::Checkpoint::Create(db_, &checkpoint_);
        if (!s.ok()) {
            return SnapshotStatus::ERROR_ROCKSDB_ERROR;
        }
        
        if (config_.is_incremental && !config_.base_snapshot_id.empty()) {
            // For incremental: Export WAL and new SST files since base snapshot
            s = checkpoint_->CreateCheckpoint(snapshot_dir_, 0, &snapshot_sequence_);
        } else {
            // For full: Create complete checkpoint
            s = checkpoint_->CreateCheckpoint(snapshot_dir_);
        }
        
        if (!s.ok()) {
            return SnapshotStatus::ERROR_ROCKSDB_ERROR;
        }
        
        // Calculate total size and chunk count
        total_bytes_ = CalculateDirectorySize(snapshot_dir_);
        total_chunks_ = (total_bytes_ + (config_.chunk_size_mb * 1024 * 1024) - 1) / 
                       (config_.chunk_size_mb * 1024 * 1024);
        
        return SnapshotStatus::OK;
    }
    
    SnapshotStatus StreamChunks(ChunkCallback callback) {
        if (snapshot_dir_.empty()) {
            return SnapshotStatus::ERROR_SNAPSHOT_NOT_FOUND;
        }
        
        // Iterate through all files in snapshot directory
        std::vector<fs::path> files;
        for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
        
        // Sort files for deterministic ordering
        std::sort(files.begin(), files.end());
        
        uint32_t chunk_index = 0;
        uint64_t file_offset = 0;
        
        for (const auto& file_path : files) {
            if (cancelled_) {
                break;
            }
            
            std::ifstream file(file_path, std::ios::binary);
            if (!file) {
                return SnapshotStatus::ERROR_ROCKSDB_ERROR;
            }
            
            // Read file in chunks
            std::vector<char> buffer(config_.chunk_size_mb * 1024 * 1024);
            
            while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
                size_t bytes_read = file.gcount();
                
                // Create chunk message
                themis::sharding::SnapshotChunk chunk;
                chunk.set_snapshot_id(config_.snapshot_id);
                chunk.set_shard_id(config_.shard_id);
                chunk.set_chunk_index(chunk_index++);
                chunk.set_total_chunks(total_chunks_);
                chunk.set_is_last_chunk(false);  // Will update for last chunk
                
                // Set file metadata
                chunk.set_file_path(file_path.filename().string());
                chunk.set_file_offset(file_offset);
                
                // Compress data
                std::string compressed_data;
                SnapshotStatus status = CompressData(
                    std::string(buffer.data(), bytes_read),
                    &compressed_data
                );
                
                if (status != SnapshotStatus::OK) {
                    return status;
                }
                
                chunk.set_data(compressed_data);
                chunk.set_uncompressed_size(bytes_read);
                chunk.set_compressed_size(compressed_data.size());
                chunk.set_compression_type(config_.compression_type);
                
                // Calculate checksum
                std::string checksum = CalculateChecksum(compressed_data);
                chunk.set_checksum(checksum);
                chunk.set_checksum_type(config_.checksum_type);
                
                // Set temporal snapshot metadata
                chunk.set_snapshot_timestamp_ns(snapshot_timestamp_ns_);
                chunk.set_snapshot_sequence(snapshot_sequence_);
                chunk.set_isolation_level(config_.isolation_level);
                chunk.set_is_immutable(config_.is_immutable);
                
                if (!config_.base_snapshot_id.empty()) {
                    chunk.set_base_version(config_.base_snapshot_id);
                }
                
                // Send chunk
                callback(chunk);
                
                transferred_bytes_ += bytes_read;
                transferred_chunks_++;
                file_offset += bytes_read;
            }
            
            file_offset = 0;  // Reset for next file
        }
        
        // Send final chunk marker
        if (!cancelled_ && chunk_index > 0) {
            themis::sharding::SnapshotChunk final_chunk;
            final_chunk.set_snapshot_id(config_.snapshot_id);
            final_chunk.set_is_last_chunk(true);
            final_chunk.set_total_chunks(chunk_index);
            
            // Calculate overall snapshot hash
            std::string snapshot_hash = CalculateSnapshotHash();
            final_chunk.set_checksum(snapshot_hash);
            final_chunk.set_checksum_type(themis::sharding::CHECKSUM_SHA256);
            
            callback(final_chunk);
        }
        
        return SnapshotStatus::OK;
    }
    
    SnapshotStatus VerifySnapshot(const std::string& expected_hash) {
        std::string actual_hash = CalculateSnapshotHash();
        
        if (actual_hash != expected_hash) {
            return SnapshotStatus::ERROR_CHECKSUM_MISMATCH;
        }
        
        return SnapshotStatus::OK;
    }
    
    SnapshotStatus ReceiveChunk(const themis::sharding::SnapshotChunk& chunk) {
        // Verify checksum
        std::string calculated_checksum = CalculateChecksum(chunk.data());
        if (calculated_checksum != chunk.checksum()) {
            return SnapshotStatus::ERROR_CHECKSUM_MISMATCH;
        }
        
        // Decompress data
        std::string decompressed_data;
        SnapshotStatus status = DecompressData(chunk.data(), &decompressed_data);
        if (status != SnapshotStatus::OK) {
            return status;
        }
        
        // Verify decompressed size
        if (decompressed_data.size() != chunk.uncompressed_size()) {
            return SnapshotStatus::ERROR_COMPRESSION_FAILED;
        }
        
        // SECURITY: Validate path to prevent directory traversal (CWE-22)
        // Step 1: Get canonical directory path
        fs::path snapshot_dir_canonical;
        try {
            snapshot_dir_canonical = fs::canonical(snapshot_dir_);
        } catch (const fs::filesystem_error& e) {
            spdlog::error("Failed to canonicalize snapshot directory: {}", e.what());
            return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
        }
        
        // Step 2: Construct target path from user-supplied file path
        fs::path file_path = snapshot_dir_canonical / chunk.file_path();
        
        // Step 3: Validate canonical path is within snapshot directory
        fs::path canonical_file_path;
        try {
            canonical_file_path = fs::canonical(file_path);
        } catch (const fs::filesystem_error& e) {
            // File doesn't exist yet - construct canonical parent path
            if (e.code() == std::errc::no_such_file_or_directory) {
                fs::path parent = file_path.parent_path();
                if (!fs::exists(parent)) {
                    // Try to create parent directories
                    try {
                        fs::create_directories(parent);
                    } catch (const fs::filesystem_error& create_err) {
                        spdlog::error("Failed to create parent directory: {}", create_err.what());
                        return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
                    }
                }
                
                // Canonicalize parent and append filename
                try {
                    canonical_file_path = fs::canonical(parent) / file_path.filename();
                } catch (const fs::filesystem_error& canon_err) {
                    spdlog::error("Failed to canonicalize parent path: {}", canon_err.what());
                    return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
                }
            } else {
                spdlog::error("Canonicalization failed: {}", e.what());
                return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
            }
        }
        
        // Step 4: Verify canonical path starts with snapshot directory
        const std::string canonical_str = canonical_file_path.string();
        const std::string dir_str = snapshot_dir_canonical.string();
        
        // Check if canonical path is within snapshot directory
        if (canonical_str.find(dir_str) != 0 || 
            (canonical_str.length() > dir_str.length() && 
             canonical_str[dir_str.length()] != fs::path::preferred_separator)) {
            spdlog::error("Path traversal attempt detected: {} not under {}", 
                         canonical_str, dir_str);
            return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
        }
        
        // Step 5: Now safe to use canonical_file_path
        std::ofstream file(canonical_file_path, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            spdlog::error("Failed to open file for writing: {}", canonical_file_path.string());
            return SnapshotStatus::ERROR_ROCKSDB_ERROR;
        }
        
        file.write(decompressed_data.data(), decompressed_data.size());
        
        transferred_bytes_ += decompressed_data.size();
        transferred_chunks_++;
        
        return SnapshotStatus::OK;
    }
    
    SnapshotStatus FinalizeSnapshot() {
        // TODO: Restore RocksDB from checkpoint directory
        // This would involve copying files to RocksDB data directory
        // and opening the database
        
        return SnapshotStatus::OK;
    }
    
    SnapshotProgress GetProgress() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time_).count();
        
        SnapshotProgress progress;
        progress.total_bytes = total_bytes_;
        progress.transferred_bytes = transferred_bytes_;
        progress.total_chunks = total_chunks_;
        progress.transferred_chunks = transferred_chunks_;
        progress.elapsed_ms = elapsed;
        
        if (transferred_bytes_ > 0) {
            progress.compression_ratio = static_cast<double>(total_bytes_) / 
                                        transferred_bytes_;
            
            // Estimate remaining time
            double bytes_per_ms = static_cast<double>(transferred_bytes_) / elapsed;
            uint64_t remaining_bytes = total_bytes_ - transferred_bytes_;
            progress.estimated_remaining_ms = 
                static_cast<uint64_t>(remaining_bytes / bytes_per_ms);
        } else {
            progress.compression_ratio = 1.0;
            progress.estimated_remaining_ms = 0;
        }
        
        return progress;
    }
    
    void Cancel() {
        cancelled_ = true;
    }

private:
    SnapshotStatus CompressData(const std::string& input, std::string* output) {
        switch (config_.compression_type) {
            case themis::sharding::COMPRESSION_NONE:
                *output = input;
                return SnapshotStatus::OK;
                
            case themis::sharding::COMPRESSION_LZ4: {
                int max_size = LZ4_compressBound(input.size());
                output->resize(max_size);
                int compressed_size = LZ4_compress_default(
                    input.data(),
                    &(*output)[0],
                    input.size(),
                    max_size
                );
                if (compressed_size <= 0) {
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                output->resize(compressed_size);
                return SnapshotStatus::OK;
            }
            
            case themis::sharding::COMPRESSION_ZSTD: {
                size_t max_size = ZSTD_compressBound(input.size());
                output->resize(max_size);
                size_t compressed_size = ZSTD_compress(
                    &(*output)[0],
                    max_size,
                    input.data(),
                    input.size(),
                    config_.compression_level
                );
                if (ZSTD_isError(compressed_size)) {
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                output->resize(compressed_size);
                return SnapshotStatus::OK;
            }
            
            case themis::sharding::COMPRESSION_SNAPPY: {
                size_t compressed_size;
                snappy::RawCompress(input.data(), input.size(),
                                   &(*output)[0], &compressed_size);
                output->resize(compressed_size);
                return SnapshotStatus::OK;
            }
            
            default:
                return SnapshotStatus::ERROR_INVALID_CONFIG;
        }
    }
    
    SnapshotStatus DecompressData(const std::string& input, std::string* output) {
        switch (config_.compression_type) {
            case themis::sharding::COMPRESSION_NONE:
                *output = input;
                return SnapshotStatus::OK;
                
            case themis::sharding::COMPRESSION_LZ4: {
                // Note: For production, would need to know uncompressed size beforehand
                output->resize(input.size() * 10);  // Estimate
                int decompressed_size = LZ4_decompress_safe(
                    input.data(),
                    &(*output)[0],
                    input.size(),
                    output->size()
                );
                if (decompressed_size < 0) {
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                output->resize(decompressed_size);
                return SnapshotStatus::OK;
            }
            
            case themis::sharding::COMPRESSION_ZSTD: {
                size_t decompressed_size = ZSTD_getFrameContentSize(
                    input.data(), input.size());
                if (decompressed_size == ZSTD_CONTENTSIZE_ERROR ||
                    decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                output->resize(decompressed_size);
                size_t actual_size = ZSTD_decompress(
                    &(*output)[0],
                    decompressed_size,
                    input.data(),
                    input.size()
                );
                if (ZSTD_isError(actual_size)) {
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                return SnapshotStatus::OK;
            }
            
            case themis::sharding::COMPRESSION_SNAPPY: {
                if (!snappy::Uncompress(input.data(), input.size(), output)) {
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                return SnapshotStatus::OK;
            }
            
            default:
                return SnapshotStatus::ERROR_INVALID_CONFIG;
        }
    }
    
    std::string CalculateChecksum(const std::string& data) {
        switch (config_.checksum_type) {
            case themis::sharding::CHECKSUM_CRC32: {
                uint32_t crc = crc32c::Crc32c(data.data(), data.size());
                return std::to_string(crc);
            }
            
            case themis::sharding::CHECKSUM_SHA256: {
                unsigned char hash[SHA256_DIGEST_LENGTH];
                SHA256(reinterpret_cast<const unsigned char*>(data.data()),
                      data.size(), hash);
                std::stringstream ss;
                for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                    ss << std::hex << std::setw(2) << std::setfill('0')
                       << static_cast<int>(hash[i]);
                }
                return ss.str();
            }
            
            case themis::sharding::CHECKSUM_XXH64: {
                // TODO: Implement XXH64
                return "";
            }
            
            default:
                return "";
        }
    }
    
    std::string CalculateSnapshotHash() {
        // Calculate SHA256 hash of all files in snapshot directory
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        
        std::vector<fs::path> files;
        for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());
        
        for (const auto& file_path : files) {
            std::ifstream file(file_path, std::ios::binary);
            std::vector<char> buffer(1024 * 1024);  // 1 MB buffer
            
            while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
                SHA256_Update(&sha256, buffer.data(), file.gcount());
            }
        }
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &sha256);
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(hash[i]);
        }
        return ss.str();
    }
    
    uint64_t CalculateDirectorySize(const fs::path& dir) {
        uint64_t size = 0;
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                size += entry.file_size();
            }
        }
        return size;
    }
    
    SnapshotConfig config_;
    rocksdb::DB* db_;
    rocksdb::Checkpoint* checkpoint_;
    std::string snapshot_dir_;
    uint64_t snapshot_timestamp_ns_;
    uint64_t snapshot_sequence_;
    
    uint64_t total_bytes_;
    uint64_t transferred_bytes_;
    uint32_t total_chunks_;
    uint32_t transferred_chunks_;
    std::atomic<bool> cancelled_;  // THREAD-SAFE: Use atomic for cancellation flag
    std::chrono::steady_clock::time_point start_time_;
};

// Public API implementation
SnapshotTransferHandler::SnapshotTransferHandler()
    : impl_(std::make_unique<Impl>()) {}

SnapshotTransferHandler::~SnapshotTransferHandler() = default;

SnapshotStatus SnapshotTransferHandler::CreateSnapshot(const SnapshotConfig& config) {
    return impl_->CreateSnapshot(config);
}

SnapshotStatus SnapshotTransferHandler::StreamChunks(ChunkCallback callback) {
    return impl_->StreamChunks(callback);
}

SnapshotStatus SnapshotTransferHandler::VerifySnapshot(const std::string& expected_hash) {
    return impl_->VerifySnapshot(expected_hash);
}

SnapshotStatus SnapshotTransferHandler::ReceiveChunk(
    const themis::sharding::SnapshotChunk& chunk) {
    return impl_->ReceiveChunk(chunk);
}

SnapshotStatus SnapshotTransferHandler::FinalizeSnapshot() {
    return impl_->FinalizeSnapshot();
}

SnapshotProgress SnapshotTransferHandler::GetProgress() const {
    return impl_->GetProgress();
}

void SnapshotTransferHandler::Cancel() {
    impl_->Cancel();
}

} // namespace rpc
} // namespace themis
