/**
 * @file snapshot_transfer_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    std::string snapshot_id = {};
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

class SnapshotTransferHandler {
public:
    SnapshotTransferHandler();
    ~SnapshotTransferHandler();
    
    // Non-copyable
    SnapshotTransferHandler(const SnapshotTransferHandler&) = delete;
    SnapshotTransferHandler& operator=(const SnapshotTransferHandler&) = delete;
    
    /**
     * @brief Create Snapshot.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    SnapshotStatus CreateSnapshot(const SnapshotConfig& config);
    
    /**
     * @brief Stream Chunks.
     * @param[in] callback Input parameter.
     * @return Return value.
     */
    SnapshotStatus StreamChunks(ChunkCallback callback);
    
    /**
     * @brief Verify Snapshot.
     * @param[in] expected_hash Input parameter.
     * @return Return value.
     */
    SnapshotStatus VerifySnapshot(const std::string& expected_hash);
    
    /**
     * @brief Receive Chunk.
     * @param[in] chunk Input parameter.
     * @return Return value.
     */
    SnapshotStatus ReceiveChunk(const shard_proto::SnapshotChunk& chunk);
    
    /**
     * @brief Finalize Snapshot.
     * @return Return value.
     */
    SnapshotStatus FinalizeSnapshot();
    
    /**
     * @brief Get Progress.
     * @return Return value.
     */
    SnapshotProgress GetProgress() const;
    
    /**
     * @brief Cancel.
     */
    void Cancel();

    /**
     * @brief Set DB.
     * @param[in,out] db Input/output parameter.
     */
    void SetDB(rocksdb::DB* db);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rpc
} // namespace themis
