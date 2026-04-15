/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            differential_update_engine.h                       ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     138                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include "shard_rpc.pb.h"

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
     * @param chunks Chunk indices to extract
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
