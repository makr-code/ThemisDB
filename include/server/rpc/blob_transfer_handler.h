/**
 * @file blob_transfer_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <functional>
#include <map>
#include <atomic>
#if __has_include("shard_rpc.pb.h")
#include "shard_rpc.pb.h"
#elif __has_include("proto_generated/shard_rpc.pb.h")
#include "proto_generated/shard_rpc.pb.h"
#else
#error "Required protobuf header shard_rpc.pb.h not found."
#endif

namespace themis {
namespace rpc {

// Configuration for blob transfer
struct BlobConfig {
    std::string blob_id;
    std::string blob_type;  // e.g., "lora_adapter", "model_checkpoint"
    std::string source_path;
    std::string dest_path;
    
    // Compression settings
    themis::sharding::proto::CompressionType compression_type;
    int compression_level;
    
    // Chunking settings
    uint32_t chunk_size_mb;  // 1-100 MB
    themis::sharding::proto::ChecksumType checksum_type;
    
    // Metadata
    std::map<std::string, std::string> metadata;
    
    // Resume support
    bool enable_resume = {};
    std::string checkpoint_id;
    
    BlobConfig()
        : compression_type(themis::sharding::proto::COMPRESSION_ZSTD)
        , compression_level(6)
        , chunk_size_mb(10)
        , checksum_type(themis::sharding::proto::CHECKSUM_CRC32)
        , enable_resume(true) {}
};

// Progress information for blob transfer
struct BlobProgress {
    uint64_t total_bytes = 0;
    uint64_t transferred_bytes;
    uint32_t total_chunks;
    uint32_t transferred_chunks;
    double compression_ratio;
    uint64_t elapsed_ms;
    uint64_t estimated_remaining_ms;
    double transfer_speed_mbps;
};

// Status codes
enum class BlobStatus {
    OK = 0,
    ERROR_BLOB_NOT_FOUND,
    ERROR_COMPRESSION_FAILED,
    ERROR_CHECKSUM_MISMATCH,
    ERROR_IO_ERROR,
    ERROR_INVALID_CONFIG,
    ERROR_NETWORK_ERROR,
    ERROR_RESUME_FAILED
};

// Callback for chunk streaming
using BlobChunkCallback = std::function<void(const themis::sharding::proto::BlobChunk&)>;

/**
 * Handler for large binary blob transfers (LoRA adapters, models, etc.).
 * Provides efficient transfer with compression, resume support, and progress tracking.
 * 
 * Features:
 * - Optimized for large files (100 MB - 10 GB)
 * - High compression ratios (3-6x with Zstd)
 * - Resume support for interrupted transfers
 * - Progress tracking and monitoring
 * - Metadata attachment
 */
class BlobTransferHandler {
public:
    using ChecksumFn = std::function<std::string(const std::string&,
                                                 themis::sharding::proto::ChecksumType)>;

    BlobTransferHandler();
    ~BlobTransferHandler();
    
    // Non-copyable
    BlobTransferHandler(const BlobTransferHandler&) = delete;
    BlobTransferHandler& operator=(const BlobTransferHandler&) = delete;
    
    /**
     * Start a blob transfer.
     * 
     * @param config Blob transfer configuration
     * @return Status code
     */
    BlobStatus StartTransfer(const BlobConfig& config);
    
    /**
     * Stream blob chunks to the callback.
     * 
     * @param callback Function to receive each chunk
     * @return Status code
     */
    BlobStatus StreamChunks(BlobChunkCallback callback);
    
    /**
     * Verify blob integrity after transfer.
     * 
     * @param expected_hash Expected SHA256 hash
     * @return Status code
     */
    BlobStatus VerifyBlob(const std::string& expected_hash);
    
    /**
     * Receive and save blob chunks.
     * 
     * @param chunk Received blob chunk
     * @return Status code
     */
    BlobStatus ReceiveChunk(const themis::sharding::proto::BlobChunk& chunk);
    
    /**
     * Finalize blob after all chunks received.
     * 
     * @return Status code
     */
    BlobStatus FinalizeBlob();
    
    /**
     * Get current transfer progress.
     * 
     * @return Progress information
     */
    BlobProgress GetProgress() const;
    
    /**
     * Create a checkpoint for resume support.
     * 
     * @return Checkpoint ID
     */
    std::string CreateCheckpoint();
    
    /**
     * Resume transfer from a checkpoint.
     * 
     * @param checkpoint_id Checkpoint to resume from
     * @return Status code
     */
    BlobStatus ResumeTransfer(const std::string& checkpoint_id);
    
    /**
     * Cancel an in-progress transfer.
     */
    void Cancel();

    static void setChecksumFn(ChecksumFn fn);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rpc
} // namespace themis
