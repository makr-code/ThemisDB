/**
 * @file differential_update_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/rpc/differential_update_engine.h"
#include <openssl/sha.h>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace themis {
namespace rpc {

// Rabin fingerprinting for Content-Defined Chunking
/** @brief Rabin fingerprinting for Content-Defined Chunking. */
class RabinFingerprint {
public:
    RabinFingerprint() : window_size_(48), avg_chunk_size_(64 * 1024) {}
    
    std::vector<uint64_t> FindChunkBoundaries(const std::string& data) {
        std::vector<uint64_t> boundaries;
        boundaries.push_back(0);  // Start
        
        const uint64_t mask = (1ULL << 13) - 1;  // For ~8KB average chunks
        uint64_t fingerprint = 0;
        
        for (size_t i = 0; i < data.size(); i++) {
            fingerprint = (fingerprint << 1) + static_cast<uint8_t>(data[i]);
            
            // Check if this is a boundary
            if ((fingerprint & mask) == 0 || 
                (i - boundaries.back()) >= (avg_chunk_size_ * 4)) {
                boundaries.push_back(i);
            }
        }
        
        boundaries.push_back(data.size());  // End
        return boundaries;
    }

private:
    size_t window_size_;
    size_t avg_chunk_size_;
};

// Implementation class
/** @brief Implementation class. */
class DifferentialUpdateEngine::Impl {
public:
    Impl() : rabin_(std::make_unique<RabinFingerprint>()) {}
    
    std::vector<ChunkInfo> GenerateManifest(
        const std::string& blob_path,
        themis::sharding::proto::DifferentialMode mode,
        uint32_t chunk_size_kb
    ) {
        std::vector<ChunkInfo> manifest;
        std::ifstream file(blob_path, std::ios::binary);
        
        if (!file) {
            return manifest;
        }
        
        // Read entire file (for CDC mode)
        std::string data((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
        file.close();
        
        switch (mode) {
            case themis::sharding::proto::DIFFERENTIAL_CDC:
                manifest = GenerateManifestCDC(data);
                break;
                
            case themis::sharding::proto::DIFFERENTIAL_FIXED_BLOCK:
                manifest = GenerateManifestFixedBlock(data, chunk_size_kb);
                break;
                
            case themis::sharding::proto::DIFFERENTIAL_BSDIFF:
                // For bsdiff, treat whole file as single chunk
                manifest = GenerateManifestWhole(data);
                break;
                
            default:
                break;
        }
        
        return manifest;
    }
    
    DeltaResult ComputeDelta(
        const std::vector<ChunkInfo>& base_manifest,
        const std::vector<ChunkInfo>& target_manifest
    ) {
        DeltaResult result;
        
        // Build hash set of base chunks
        std::map<std::string, uint32_t> base_hashes = {};

        for (const auto& chunk : base_manifest) {
            base_hashes[chunk.hash] = chunk.index;
        }
        
        // Find changed and unchanged chunks
        uint64_t unchanged_bytes = 0;
        uint64_t total_bytes = 0;
        
        for (const auto& chunk : target_manifest) {
            total_bytes += chunk.size;
            
            auto it = base_hashes.find(chunk.hash);
            if (it != base_hashes.end()) {
                // Chunk unchanged
                result.unchanged_chunks.push_back(chunk.index);
                unchanged_bytes += chunk.size;
            } else {
                // Chunk changed - need to transfer
                result.changed_chunks.push_back(chunk.index);
            }
        }
        
        result.total_bytes_saved = unchanged_bytes;
        result.savings_percentage = total_bytes > 0 ?
            (100.0 * unchanged_bytes / total_bytes) : 0.0;
        
        return result;
    }
    
    themis::sharding::proto::DifferentialMode SelectStrategy(
        const BlobMetadata& metadata
    ) {
        double change_rate = metadata.estimated_change_rate;
        
        if (change_rate < 0.05) {
            // < 5% change: Binary diff is optimal
            return themis::sharding::proto::DIFFERENTIAL_BSDIFF;
        } else if (change_rate < 0.30) {
            // 5-30% change: Fixed-block works well
            return themis::sharding::proto::DIFFERENTIAL_FIXED_BLOCK;
        } else if (change_rate < 0.90) {
            // 30-90% change: CDC gives best deduplication
            return themis::sharding::proto::DIFFERENTIAL_CDC;
        } else {
            // > 90% change: Full transfer is more efficient
            return themis::sharding::proto::DIFFERENTIAL_NONE;
        }
    }
    
    std::map<uint32_t, std::string> ExtractChunks(
        const std::string& blob_path,
        const std::vector<uint32_t>& chunk_indices
    ) {
        std::map<uint32_t, std::string> chunks;

        if (chunk_indices.empty()) { return chunks; }

        // Read the entire blob (manifest is already held by the caller; we
        // re-derive boundaries from the default CDC mode so we can seek to the
        // right offsets without requiring the caller to pass a manifest).
        std::ifstream file(blob_path, std::ios::binary);
        if (!file) { return chunks; }

        std::string data((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
        file.close();

        // Build a full manifest (CDC for non-trivial blobs, whole-file otherwise)
        // so we can map chunk_index → (offset, size).
        auto boundaries = rabin_->FindChunkBoundaries(data);
        std::vector<ChunkInfo> manifest = {};

        if (boundaries.size() >= 2) {
            for (size_t i = 0; i < boundaries.size() - 1; i++) {
                uint64_t start = boundaries[i];
                uint64_t end   = boundaries[i + 1];
                uint32_t sz    = static_cast<uint32_t>(end - start);
                ChunkInfo info;
                info.offset = start;
                info.size   = sz;
                info.index  = static_cast<uint32_t>(i);
                info.hash   = CalculateHash(data.substr(start, sz));
                manifest.push_back(info);
            }
        } else {
            // Trivial: whole blob is one chunk
            ChunkInfo info;
            info.offset = 0;
            info.size   = static_cast<uint32_t>(data.size());
            info.index  = 0;
            info.hash   = CalculateHash(data);
            manifest.push_back(info);
        }

        // Build index → ChunkInfo lookup
        std::unordered_map<uint32_t, const ChunkInfo*> by_index = {};

        for (const auto& c : manifest) { by_index[c.index] = &c; }

        for (uint32_t idx : chunk_indices) {
            auto it = by_index.find(idx);
            if (it == by_index.end()) { continue; }
            const ChunkInfo& ci = *it->second;
            if (ci.offset + ci.size > data.size()) { continue; }
            chunks[idx] = data.substr(ci.offset, ci.size);
        }

        return chunks;
    }

private:
    std::vector<ChunkInfo> GenerateManifestCDC(const std::string& data) {
        std::vector<ChunkInfo> manifest;
        
        // Find chunk boundaries using Rabin fingerprinting
        auto boundaries = rabin_->FindChunkBoundaries(data);
        
        for (size_t i = 0; i < boundaries.size() - 1; i++) {
            uint64_t start = boundaries[i];
            uint64_t end = boundaries[i + 1];
            uint32_t size = static_cast<uint32_t>(end - start);
            
            ChunkInfo info;
            info.offset = start;
            info.size = size;
            info.index = static_cast<uint32_t>(i);
            info.hash = CalculateHash(data.substr(start, size));
            
            manifest.push_back(info);
        }
        
        return manifest;
    }
    
    std::vector<ChunkInfo> GenerateManifestFixedBlock(
        const std::string& data,
        uint32_t chunk_size_kb
    ) {
        std::vector<ChunkInfo> manifest;
        uint64_t chunk_size = chunk_size_kb * 1024;
        uint32_t index = 0;
        
        for (uint64_t offset = 0; offset < data.size(); offset += chunk_size) {
            uint32_t size = static_cast<uint32_t>(std::min(chunk_size, static_cast<uint64_t>(data.size() - offset)));
            
            ChunkInfo info;
            info.offset = offset;
            info.size = size;
            info.index = index++;
            info.hash = CalculateHash(data.substr(offset, size));
            
            manifest.push_back(info);
        }
        
        return manifest;
    }
    
    std::vector<ChunkInfo> GenerateManifestWhole(const std::string& data) {
        std::vector<ChunkInfo> manifest;
        
        ChunkInfo info;
        info.offset = 0;
        info.size = static_cast<uint32_t>(data.size());
        info.index = 0;
        info.hash = CalculateHash(data);
        
        manifest.push_back(info);
        return manifest;
    }
    
    std::string CalculateHash(const std::string& data) {
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
    
    std::unique_ptr<RabinFingerprint> rabin_;
};

// Public API
DifferentialUpdateEngine::DifferentialUpdateEngine()
    : impl_(std::make_unique<Impl>()) {}

DifferentialUpdateEngine::~DifferentialUpdateEngine() = default;

std::vector<ChunkInfo> DifferentialUpdateEngine::GenerateManifest(
    const std::string& blob_path,
    themis::sharding::proto::DifferentialMode mode,
    uint32_t chunk_size_kb
) {
    return impl_->GenerateManifest(blob_path, mode, chunk_size_kb);
}

DeltaResult DifferentialUpdateEngine::ComputeDelta(
    const std::vector<ChunkInfo>& base_manifest,
    const std::vector<ChunkInfo>& target_manifest
) {
    return impl_->ComputeDelta(base_manifest, target_manifest);
}

themis::sharding::proto::DifferentialMode DifferentialUpdateEngine::SelectStrategy(
    const BlobMetadata& metadata
) {
    return impl_->SelectStrategy(metadata);
}

std::map<uint32_t, std::string> DifferentialUpdateEngine::ExtractChunks(
    const std::string& blob_path,
    const std::vector<uint32_t>& chunk_indices
) {
    return impl_->ExtractChunks(blob_path, chunk_indices);
}

} // namespace rpc
} // namespace themis

