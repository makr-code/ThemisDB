#include "server/rpc/blob_transfer_handler.h"
#include <zstd.h>
#include <lz4.h>
#include <snappy.h>
#include <crc32c/crc32c.h>
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <iomanip>

namespace themis {
namespace rpc {

namespace fs = std::filesystem;

// Implementation class
class BlobTransferHandler::Impl {
public:
    Impl() 
        : total_bytes_(0)
        , transferred_bytes_(0)
        , total_chunks_(0)
        , transferred_chunks_(0)
        , cancelled_(false)
        , start_time_(std::chrono::steady_clock::now()) {}
    
    BlobStatus StartTransfer(const BlobConfig& config) {
        config_ = config;
        
        if (!fs::exists(config_.source_path)) {
            return BlobStatus::ERROR_BLOB_NOT_FOUND;
        }
        
        total_bytes_ = fs::file_size(config_.source_path);
        total_chunks_ = (total_bytes_ + (config_.chunk_size_mb * 1024 * 1024) - 1) / 
                       (config_.chunk_size_mb * 1024 * 1024);
        
        return BlobStatus::OK;
    }
    
    BlobStatus StreamChunks(BlobChunkCallback callback) {
        std::ifstream file(config_.source_path, std::ios::binary);
        if (!file) {
            return BlobStatus::ERROR_IO_ERROR;
        }
        
        std::vector<char> buffer(config_.chunk_size_mb * 1024 * 1024);
        uint32_t chunk_index = 0;
        uint64_t file_offset = 0;
        
        while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
            if (cancelled_) {
                break;
            }
            
            size_t bytes_read = file.gcount();
            
            // Create chunk
            themis::sharding::BlobChunk chunk;
            chunk.set_blob_id(config_.blob_id);
            chunk.set_chunk_index(chunk_index++);
            chunk.set_total_chunks(total_chunks_);
            chunk.set_offset(file_offset);
            chunk.set_is_last_chunk(false);
            
            // Compress
            std::string compressed_data;
            BlobStatus status = CompressData(
                std::string(buffer.data(), bytes_read),
                &compressed_data
            );
            if (status != BlobStatus::OK) {
                return status;
            }
            
            chunk.set_data(compressed_data);
            chunk.set_uncompressed_size(bytes_read);
            chunk.set_compressed_size(compressed_data.size());
            chunk.set_compression_type(config_.compression_type);
            
            // Checksum
            chunk.set_checksum(CalculateChecksum(compressed_data));
            chunk.set_checksum_type(config_.checksum_type);
            
            // Metadata (first chunk only)
            if (chunk_index == 1) {
                for (const auto& kv : config_.metadata) {
                    (*chunk.mutable_metadata())[kv.first] = kv.second;
                }
            }
            
            // Temporal metadata
            chunk.set_snapshot_timestamp_ns(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            );
            
            callback(chunk);
            
            transferred_bytes_ += bytes_read;
            transferred_chunks_++;
            file_offset += bytes_read;
        }
        
        // Final chunk
        if (!cancelled_ && chunk_index > 0) {
            themis::sharding::BlobChunk final_chunk;
            final_chunk.set_blob_id(config_.blob_id);
            final_chunk.set_is_last_chunk(true);
            final_chunk.set_total_chunks(chunk_index);
            final_chunk.set_checksum(CalculateBlobHash());
            final_chunk.set_checksum_type(themis::sharding::CHECKSUM_SHA256);
            callback(final_chunk);
        }
        
        return BlobStatus::OK;
    }
    
    BlobStatus VerifyBlob(const std::string& expected_hash) {
        if (CalculateBlobHash() != expected_hash) {
            return BlobStatus::ERROR_CHECKSUM_MISMATCH;
        }
        return BlobStatus::OK;
    }
    
    BlobStatus ReceiveChunk(const themis::sharding::BlobChunk& chunk) {
        // Verify checksum
        if (CalculateChecksum(chunk.data()) != chunk.checksum()) {
            return BlobStatus::ERROR_CHECKSUM_MISMATCH;
        }
        
        // Decompress
        std::string decompressed;
        BlobStatus status = DecompressData(chunk.data(), &decompressed);
        if (status != BlobStatus::OK) {
            return status;
        }
        
        // Write to file
        if (!output_file_.is_open()) {
            output_file_.open(config_.dest_path, std::ios::binary);
        }
        
        output_file_.write(decompressed.data(), decompressed.size());
        
        transferred_bytes_ += decompressed.size();
        transferred_chunks_++;
        
        return BlobStatus::OK;
    }
    
    BlobStatus FinalizeBlob() {
        if (output_file_.is_open()) {
            output_file_.close();
        }
        return BlobStatus::OK;
    }
    
    BlobProgress GetProgress() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time_).count();
        
        BlobProgress progress;
        progress.total_bytes = total_bytes_;
        progress.transferred_bytes = transferred_bytes_;
        progress.total_chunks = total_chunks_;
        progress.transferred_chunks = transferred_chunks_;
        progress.elapsed_ms = elapsed;
        
        if (transferred_bytes_ > 0 && elapsed > 0) {
            progress.compression_ratio = static_cast<double>(total_bytes_) / 
                                        transferred_bytes_;
            progress.transfer_speed_mbps = 
                (transferred_bytes_ / 1024.0 / 1024.0) / (elapsed / 1000.0);
            
            double bytes_per_ms = static_cast<double>(transferred_bytes_) / elapsed;
            progress.estimated_remaining_ms = 
                (total_bytes_ - transferred_bytes_) / bytes_per_ms;
        }
        
        return progress;
    }
    
    std::string CreateCheckpoint() {
        // Save current state
        checkpoint_.transferred_bytes = transferred_bytes_;
        checkpoint_.transferred_chunks = transferred_chunks_;
        checkpoint_.checkpoint_id = GenerateCheckpointId();
        return checkpoint_.checkpoint_id;
    }
    
    BlobStatus ResumeTransfer(const std::string& checkpoint_id) {
        // TODO: Load checkpoint state
        return BlobStatus::OK;
    }
    
    void Cancel() {
        cancelled_ = true;
    }

private:
    BlobStatus CompressData(const std::string& input, std::string* output) {
        switch (config_.compression_type) {
            case themis::sharding::COMPRESSION_NONE:
                *output = input;
                return BlobStatus::OK;
                
            case themis::sharding::COMPRESSION_ZSTD: {
                size_t max_size = ZSTD_compressBound(input.size());
                output->resize(max_size);
                size_t size = ZSTD_compress(
                    &(*output)[0], max_size,
                    input.data(), input.size(),
                    config_.compression_level
                );
                if (ZSTD_isError(size)) {
                    return BlobStatus::ERROR_COMPRESSION_FAILED;
                }
                output->resize(size);
                return BlobStatus::OK;
            }
            
            default:
                return BlobStatus::ERROR_INVALID_CONFIG;
        }
    }
    
    BlobStatus DecompressData(const std::string& input, std::string* output) {
        switch (config_.compression_type) {
            case themis::sharding::COMPRESSION_NONE:
                *output = input;
                return BlobStatus::OK;
                
            case themis::sharding::COMPRESSION_ZSTD: {
                size_t size = ZSTD_getFrameContentSize(input.data(), input.size());
                output->resize(size);
                size_t actual = ZSTD_decompress(
                    &(*output)[0], size,
                    input.data(), input.size()
                );
                if (ZSTD_isError(actual)) {
                    return BlobStatus::ERROR_COMPRESSION_FAILED;
                }
                return BlobStatus::OK;
            }
            
            default:
                return BlobStatus::ERROR_INVALID_CONFIG;
        }
    }
    
    std::string CalculateChecksum(const std::string& data) {
        switch (config_.checksum_type) {
            case themis::sharding::CHECKSUM_CRC32: {
                return std::to_string(crc32c::Crc32c(data.data(), data.size()));
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
            default:
                return "";
        }
    }
    
    std::string CalculateBlobHash() {
        std::ifstream file(config_.source_path, std::ios::binary);
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        
        std::vector<char> buffer(1024 * 1024);
        while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
            SHA256_Update(&sha256, buffer.data(), file.gcount());
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
    
    std::string GenerateCheckpointId() {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
        return config_.blob_id + "_" + std::to_string(timestamp);
    }
    
    struct Checkpoint {
        std::string checkpoint_id;
        uint64_t transferred_bytes;
        uint32_t transferred_chunks;
    };
    
    BlobConfig config_;
    uint64_t total_bytes_;
    uint64_t transferred_bytes_;
    uint32_t total_chunks_;
    uint32_t transferred_chunks_;
    bool cancelled_;
    std::chrono::steady_clock::time_point start_time_;
    std::ofstream output_file_;
    Checkpoint checkpoint_;
};

// Public API
BlobTransferHandler::BlobTransferHandler()
    : impl_(std::make_unique<Impl>()) {}

BlobTransferHandler::~BlobTransferHandler() = default;

BlobStatus BlobTransferHandler::StartTransfer(const BlobConfig& config) {
    return impl_->StartTransfer(config);
}

BlobStatus BlobTransferHandler::StreamChunks(BlobChunkCallback callback) {
    return impl_->StreamChunks(callback);
}

BlobStatus BlobTransferHandler::VerifyBlob(const std::string& expected_hash) {
    return impl_->VerifyBlob(expected_hash);
}

BlobStatus BlobTransferHandler::ReceiveChunk(const themis::sharding::BlobChunk& chunk) {
    return impl_->ReceiveChunk(chunk);
}

BlobStatus BlobTransferHandler::FinalizeBlob() {
    return impl_->FinalizeBlob();
}

BlobProgress BlobTransferHandler::GetProgress() const {
    return impl_->GetProgress();
}

std::string BlobTransferHandler::CreateCheckpoint() {
    return impl_->CreateCheckpoint();
}

BlobStatus BlobTransferHandler::ResumeTransfer(const std::string& checkpoint_id) {
    return impl_->ResumeTransfer(checkpoint_id);
}

void BlobTransferHandler::Cancel() {
    impl_->Cancel();
}

} // namespace rpc
} // namespace themis
