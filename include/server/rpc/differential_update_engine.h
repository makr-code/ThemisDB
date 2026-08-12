/**
 * @file differential_update_engine.h
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
    uint64_t size;
    std::string blob_type;
    uint64_t base_version_size;
    double estimated_change_rate;  // 0.0 - 1.0
};

/**
 * Engine for differential updates using hash-based deduplication.
 * Implements rsync-like differential transfer for binary blobs.
 * 
 * Features:
 * - Content-Defined Chunking (CDC) with Rabin fingerprinting
 * - Fixed-block differential
 * - Binary diff (bsdiff) for minimal changes
 * - Smart strategy selection
 * - 90-98% bandwidth savings for typical updates
 */
class DifferentialUpdateEngine {
public:
    DifferentialUpdateEngine();
    ~DifferentialUpdateEngine();
    
    // Non-copyable
    DifferentialUpdateEngine(const DifferentialUpdateEngine&) = delete;
    DifferentialUpdateEngine& operator=(const DifferentialUpdateEngine&) = delete;
    
    /**
     * Generate chunk manifest for a blob.
     * 
     * @param blob_path Path to blob file
     * @param mode Chunking mode (CDC, FIXED_BLOCK, etc.)
     * @param chunk_size_kb Chunk size for FIXED_BLOCK mode
     * @return Vector of chunk information
     */
    std::vector<ChunkInfo> GenerateManifest(
        const std::string& blob_path,
        themis::sharding::proto::DifferentialMode mode,
        uint32_t chunk_size_kb = 64
    );
    
    /**
     * Compute delta between base and target manifests.
     * 
     * @param base_manifest Manifest of base version
     * @param target_manifest Manifest of target version
     * @return Delta result with chunks to transfer
     */
    DeltaResult ComputeDelta(
        const std::vector<ChunkInfo>& base_manifest,
        const std::vector<ChunkInfo>& target_manifest
    );
    
    /**
     * Select optimal differential strategy based on metadata.
     * 
     * Strategy selection:
     * - < 5% change: Binary Diff (bsdiff)
     * - 5-30% change: Fixed-Block
     * - 30-90% change: CDC
     * - > 90% change: Full Transfer (no diff)
     * 
     * @param metadata Blob metadata
     * @return Recommended differential mode
     */
    themis::sharding::proto::DifferentialMode SelectStrategy(
        const BlobMetadata& metadata
    );
    
    /**
     * Extract chunks from blob based on manifest.
     * 
     * @param blob_path Path to blob file
        * @param chunk_indices Chunk indices to extract
     * @return Map of chunk index to chunk data
     */
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
