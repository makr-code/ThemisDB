/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            snapshot_transfer_handler.h                        ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:26:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     203                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 43ea0ace66  2026-03-26  fix: Fix 4+5 – XXH64 checksum, FinalizeSnapshot restore, ... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <atomic>
#include "shard_rpc.pb.h"

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
    uint64_t total_bytes;
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
