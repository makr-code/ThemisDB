/**
 * @file differential_update_engine.h
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
#include <vector>
#include <map>
#if __has_include("shard_rpc.pb.h")
#include "shard_rpc.pb.h"
#elif __has_include("proto_generated/shard_rpc.pb.h")
#include "proto_generated/shard_rpc.pb.h"
#else
#error "Required protobuf header shard_rpc.pb.h not found."
#endif

namespace themis {
namespace rpc {

// Chunk manifest for hash-based deduplication
struct ChunkInfo {
    std::string hash;        // SHA256 hash
    uint64_t offset;         // Offset in blob
    uint32_t size;           // Chunk size
    uint32_t index;          // Chunk index
};

// Delta result
struct DeltaResult {
    std::vector<uint32_t> unchanged_chunks;  // Chunks to keep from base
    std::vector<uint32_t> changed_chunks;    // Chunks to transfer
    uint64_t total_bytes_saved;
    double savings_percentage;
};

// Blob metadata for strategy selection
struct BlobMetadata {
    uint64_t size = 0;
    std::string blob_type;
    uint64_t base_version_size;
    double estimated_change_rate;  // 0.0 - 1.0
};

class DifferentialUpdateEngine {
public:
    DifferentialUpdateEngine();
    ~DifferentialUpdateEngine();
    
    // Non-copyable
    DifferentialUpdateEngine(const DifferentialUpdateEngine&) = delete;
    DifferentialUpdateEngine& operator=(const DifferentialUpdateEngine&) = delete;
    
    std::vector<ChunkInfo> GenerateManifest(
        const std::string& blob_path,
        themis::sharding::proto::DifferentialMode mode,
        uint32_t chunk_size_kb = 64
    );
    
    /**
     * @brief Compute Delta.
     * @param[in] base_manifest Input parameter.
     * @param[in] target_manifest Input parameter.
     * @return Return value.
     */
    DeltaResult ComputeDelta(
        const std::vector<ChunkInfo>& base_manifest,
        const std::vector<ChunkInfo>& target_manifest
    );
    
    /**
     * @brief Select Strategy.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    themis::sharding::proto::DifferentialMode SelectStrategy(
        const BlobMetadata& metadata
    );
    
    std::map<uint32_t, std::string> ExtractChunks(
        const std::string& blob_path,
        const std::vector<uint32_t>& chunk_indices
    );

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rpc
} // namespace themis
