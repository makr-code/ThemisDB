/**
 * @file snapshot_transfer_handler.h
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
#include <vector>
#include <atomic>
#if __has_include("shard_rpc.pb.h")
#include "shard_rpc.pb.h"
#elif __has_include("proto_generated/shard_rpc.pb.h")
#include "proto_generated/shard_rpc.pb.h"
#else
#error "Required protobuf header shard_rpc.pb.h not found."
#endif

// Forward-declare RocksDB to avoid pulling in heavy headers in consumers.
namespace rocksdb { class DB; }

namespace themis {
namespace rpc {

namespace shard_proto = themis::sharding::proto;

// Configuration for snapshot transfer
struct SnapshotConfig {
    std::string shard_id;
    std::string snapshot_id;
    bool is_incremental;
    std::string base_snapshot_id;  // For incremental snapshots
    
    // Compression settings
    shard_proto::CompressionType compression_type;
    int compression_level;  // 1-9 for Zstd, ignored for others
    
    // Chunking settings
    uint32_t chunk_size_mb;  // 1-100 MB
    shard_proto::ChecksumType checksum_type;
    
    // Snapshot isolation
    shard_proto::SnapshotIsolation isolation_level;
    bool is_immutable;  // True if source is frozen during transfer
    
    SnapshotConfig()
        : is_incremental(false)
        , compression_type(shard_proto::COMPRESSION_ZSTD)
        , compression_level(6)
        , chunk_size_mb(10)
        , checksum_type(shard_proto::CHECKSUM_CRC32)
        , isolation_level(shard_proto::SNAPSHOT_MVCC)
        , is_immutable(false) {}
};

// Progress information for snapshot transfer
struct SnapshotProgress {
    uint64_t total_bytes = 0;
    uint64_t transferred_bytes;
    uint32_t total_chunks;
    uint32_t transferred_chunks;
    double compression_ratio;
    uint64_t elapsed_ms;
    uint64_t estimated_remaining_ms;
};

// Status codes
enum class SnapshotStatus {
    OK = 0,
    ERROR_SNAPSHOT_NOT_FOUND,
    ERROR_COMPRESSION_FAILED,
    ERROR_CHECKSUM_MISMATCH,
    ERROR_ROCKSDB_ERROR,
    ERROR_INVALID_CONFIG,
    ERROR_NETWORK_ERROR,
    ERROR_SECURITY_PATH_TRAVERSAL  // Path traversal attempt detected
};

// Callback for chunk streaming
using ChunkCallback = std::function<void(const shard_proto::SnapshotChunk&)>;

/**
 * Handler for RocksDB snapshot transfer operations.
 * Provides efficient snapshot-based bulk data migration between shards.
 * 
 * Features:
 * - MVCC-aware snapshot creation for consistency
 * - Chunked transfer with configurable compression
 * - Incremental and full snapshot support
 * - Checksum verification for data integrity
 * - Progress tracking and monitoring
 */
class SnapshotTransferHandler {
public:
    SnapshotTransferHandler();
    ~SnapshotTransferHandler();
    
    // Non-copyable
    SnapshotTransferHandler(const SnapshotTransferHandler&) = delete;
    SnapshotTransferHandler& operator=(const SnapshotTransferHandler&) = delete;
    
    /**
     * Create a snapshot with the specified configuration.
     * 
     * For full snapshots: Creates a new RocksDB checkpoint
     * For incremental: Uses RocksDB WAL and SST deltas since base snapshot
     * 
     * @param config Snapshot configuration
     * @return Status code
     */
    SnapshotStatus CreateSnapshot(const SnapshotConfig& config);
    
    /**
     * Stream snapshot chunks to the callback.
     * 
     * Each chunk is compressed and checksummed according to config.
     * Chunks are streamed in order for sequential reconstruction.
     * 
     * @param callback Function to receive each chunk
     * @return Status code
     */
    SnapshotStatus StreamChunks(ChunkCallback callback);
    
    /**
     * Verify snapshot integrity after transfer.
     * 
     * Validates:
     * - Per-chunk checksums
     * - Overall snapshot hash
     * - Chunk sequence completeness
     * 
     * @param expected_hash Expected SHA256 hash of complete snapshot
     * @return Status code
     */
    SnapshotStatus VerifySnapshot(const std::string& expected_hash);
    
    /**
     * Receive and apply snapshot chunks.
     * 
     * SECURITY: This method validates file paths using canonical path resolution
     * to prevent path traversal attacks (CWE-22). User-supplied file paths are
     * verified to be within the snapshot directory before any file operations.
     * 
     * Path validation includes:
     * - Canonical path resolution with fs::canonical()
     * - Verification that resolved path is within snapshot directory
     * - Handling of non-existent parent directories
     * - Rejection of absolute paths and .. traversal attempts
     * - Logging of security violations
     * 
     * @param chunk Received snapshot chunk with file path and data
     * @return Status code (ERROR_SECURITY_PATH_TRAVERSAL on path traversal attempt)
     */
    SnapshotStatus ReceiveChunk(const shard_proto::SnapshotChunk& chunk);
    
    /**
     * Finalize snapshot after all chunks received.
     * 
     * @return Status code
     */
    SnapshotStatus FinalizeSnapshot();
    
    /**
     * Get current transfer progress.
     * 
     * @return Progress information
     */
    SnapshotProgress GetProgress() const;
    
    /**
     * Cancel an in-progress snapshot transfer.
     */
    void Cancel();

    /**
     * Inject the RocksDB instance to use for snapshot creation and restore.
     * Must be called before CreateSnapshot() or FinalizeSnapshot().
     *
     * @param db Pointer to the open RocksDB instance (not owned by this handler).
     */
    void SetDB(rocksdb::DB* db);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rpc
} // namespace themis
