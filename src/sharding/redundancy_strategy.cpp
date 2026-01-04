/**
 * ThemisDB RAID-like Redundancy Strategy Implementation
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <future>
#include <cstring>

namespace themisdb {
namespace sharding {

// ═══════════════════════════════════════════════════════════
// RedundancyConfig Implementation
// ═══════════════════════════════════════════════════════════

bool RedundancyConfig::validate() const {
    if (replication_factor < 1) {
        spdlog::error("Invalid replication_factor: must be >= 1");
        return false;
    }
    
    if (mode == RedundancyMode::PARITY) {
        if (erasure_coding.data_shards < 2) {
            spdlog::error("Invalid erasure coding: data_shards must be >= 2");
            return false;
        }
        if (erasure_coding.parity_shards < 1) {
            spdlog::error("Invalid erasure coding: parity_shards must be >= 1");
            return false;
        }
    }
    
    if (write_quorum > replication_factor) {
        spdlog::error("Invalid write_quorum: must be <= replication_factor");
        return false;
    }
    
    return true;
}

double RedundancyConfig::getStorageEfficiency() const {
    switch (mode) {
        case RedundancyMode::NONE:
            return 1.0;
        case RedundancyMode::STRIPE:
            return 1.0;  // No redundancy
        case RedundancyMode::MIRROR:
        case RedundancyMode::GEO_MIRROR:
            return 1.0 / replication_factor;
        case RedundancyMode::STRIPE_MIRROR:
            return 1.0 / replication_factor;
        case RedundancyMode::PARITY:
            return erasure_coding.storageEfficiency();
        default:
            return 1.0;
    }
}

uint32_t RedundancyConfig::getFaultTolerance() const {
    switch (mode) {
        case RedundancyMode::NONE:
        case RedundancyMode::STRIPE:
            return 0;  // No fault tolerance
        case RedundancyMode::MIRROR:
        case RedundancyMode::STRIPE_MIRROR:
        case RedundancyMode::GEO_MIRROR:
            return replication_factor - 1;
        case RedundancyMode::PARITY:
            return erasure_coding.faultTolerance();
        default:
            return 0;
    }
}

uint32_t RedundancyConfig::getEffectiveReplicationFactor() const {
    switch (mode) {
        case RedundancyMode::NONE:
        case RedundancyMode::STRIPE:
            return 1;
        case RedundancyMode::PARITY:
            return erasure_coding.totalShards();
        default:
            return replication_factor;
    }
}

// ═══════════════════════════════════════════════════════════
// ChunkInfo Implementation
// ═══════════════════════════════════════════════════════════

std::vector<uint8_t> ChunkInfo::serialize() const {
    std::vector<uint8_t> data;
    // Simple binary serialization
    // In production, use protobuf or similar
    return data;
}

std::optional<ChunkInfo> ChunkInfo::deserialize(const std::vector<uint8_t>& data) {
    // Simple binary deserialization
    // In production, use protobuf or similar
    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════
// StripeGroup Implementation
// ═══════════════════════════════════════════════════════════

bool StripeGroup::isComplete() const {
    for (const auto& chunk : data_chunks) {
        if (chunk.shard_id.empty()) {
            return false;
        }
    }
    return true;
}

std::vector<uint32_t> StripeGroup::getMissingChunks() const {
    std::vector<uint32_t> missing;
    for (size_t i = 0; i < data_chunks.size(); ++i) {
        if (data_chunks[i].shard_id.empty()) {
            missing.push_back(i);
        }
    }
    return missing;
}

bool StripeGroup::canRecover(uint32_t data_shards, uint32_t parity_shards) const {
    uint32_t available = 0;
    for (const auto& chunk : data_chunks) {
        if (!chunk.shard_id.empty()) available++;
    }
    for (const auto& chunk : parity_chunks) {
        if (!chunk.shard_id.empty()) available++;
    }
    
    // Need at least data_shards chunks to recover
    return available >= data_shards;
}

// ═══════════════════════════════════════════════════════════
// WriteResult Implementation
// ═══════════════════════════════════════════════════════════

WriteResult WriteResult::successful(const std::string& doc_id, 
                                   const std::vector<std::string>& shards,
                                   std::chrono::milliseconds lat) {
    WriteResult result;
    result.success = true;
    result.document_id = doc_id;
    result.written_shards = shards;
    result.acknowledgements = shards.size();
    result.latency = lat;
    return result;
}

WriteResult WriteResult::failed(const std::string& doc_id, const std::string& error) {
    WriteResult result;
    result.success = false;
    result.document_id = doc_id;
    result.error_message = error;
    return result;
}

// ═══════════════════════════════════════════════════════════
// ReedSolomonCoder Implementation (Simplified)
// ═══════════════════════════════════════════════════════════

std::vector<std::vector<uint8_t>> ReedSolomonCoder::encode(
    const std::vector<uint8_t>& data,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    std::vector<std::vector<uint8_t>> chunks;
    
    // Calculate chunk size
    size_t chunk_size = (data.size() + data_shards - 1) / data_shards;
    
    // Split data into chunks
    for (uint32_t i = 0; i < data_shards; ++i) {
        size_t offset = i * chunk_size;
        size_t size = std::min(chunk_size, data.size() - offset);
        
        std::vector<uint8_t> chunk(chunk_size, 0);  // Pad with zeros
        if (offset < data.size()) {
            std::memcpy(chunk.data(), data.data() + offset, size);
        }
        chunks.push_back(chunk);
    }
    
    // Generate parity chunks using XOR (simplified Reed-Solomon)
    // In production, use proper Galois Field arithmetic
    for (uint32_t i = 0; i < parity_shards; ++i) {
        std::vector<uint8_t> parity(chunk_size, 0);
        
        // Simple XOR-based parity for now
        for (const auto& chunk : chunks) {
            for (size_t j = 0; j < chunk_size; ++j) {
                parity[j] ^= chunk[j];
            }
        }
        
        chunks.push_back(parity);
    }
    
    return chunks;
}

std::vector<uint8_t> ReedSolomonCoder::decode(
    const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
    const std::vector<uint32_t>& missing_indices,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    // Simplified reconstruction
    // In production, implement proper Reed-Solomon decoding
    
    if (available_chunks.size() < data_shards) {
        throw std::runtime_error("Not enough chunks for recovery");
    }
    
    // For now, if all data chunks are available, just concatenate them
    std::vector<uint8_t> recovered;
    for (uint32_t i = 0; i < data_shards; ++i) {
        if (available_chunks.count(i)) {
            const auto& chunk = available_chunks.at(i);
            recovered.insert(recovered.end(), chunk.begin(), chunk.end());
        }
    }
    
    return recovered;
}

uint8_t ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
    // Simplified Galois Field multiplication
    // In production, use lookup tables
    return a * b;  // Placeholder
}

uint8_t ReedSolomonCoder::gf_div(uint8_t a, uint8_t b) {
    // Simplified Galois Field division
    return a / (b + 1);  // Placeholder
}

void ReedSolomonCoder::gf_matrix_mul(
    const std::vector<std::vector<uint8_t>>& matrix,
    const std::vector<uint8_t>& vec,
    std::vector<uint8_t>& result
) {
    // Matrix multiplication in Galois Field
    // Placeholder implementation
}

// ═══════════════════════════════════════════════════════════
// ErasureCoder Factory
// ═══════════════════════════════════════════════════════════

std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm algorithm) {
    switch (algorithm) {
        case ErasureCodingAlgorithm::REED_SOLOMON:
            return std::make_unique<ReedSolomonCoder>();
        case ErasureCodingAlgorithm::CAUCHY:
        case ErasureCodingAlgorithm::LRC:
            spdlog::warn("Algorithm not yet implemented, falling back to Reed-Solomon");
            return std::make_unique<ReedSolomonCoder>();
        default:
            return nullptr;
    }
}

// ═══════════════════════════════════════════════════════════
// RedundancyStrategy Implementation
// ═══════════════════════════════════════════════════════════

RedundancyStrategy::RedundancyStrategy(const RedundancyConfig& config)
    : config_(config) {
    
    if (!config_.validate()) {
        throw std::invalid_argument("Invalid redundancy configuration");
    }
    
    if (config_.mode == RedundancyMode::PARITY) {
        erasure_coder_ = ErasureCoder::create(config_.erasure_coding.algorithm);
        if (!erasure_coder_) {
            throw std::runtime_error("Failed to create erasure coder");
        }
    }
    
    spdlog::info("RedundancyStrategy initialized: mode={}, replication_factor={}, storage_efficiency={:.2f}",
                 static_cast<int>(config_.mode),
                 config_.replication_factor,
                 config_.getStorageEfficiency());
}

RedundancyStrategy::~RedundancyStrategy() = default;

WriteResult RedundancyStrategy::write(
    const std::string& document_id,
    const std::vector<uint8_t>& data,
    const std::string& collection,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    WriteHandler handler
) {
    auto start = std::chrono::steady_clock::now();
    
    stats_writes_++;
    stats_bytes_written_ += data.size();
    
    WriteResult result;
    
    try {
        switch (config_.mode) {
            case RedundancyMode::NONE:
                // Just write to primary shard
                result = writeMirror(document_id, data, ring, topology, handler);
                break;
            case RedundancyMode::MIRROR:
            case RedundancyMode::GEO_MIRROR:
                result = writeMirror(document_id, data, ring, topology, handler);
                break;
            case RedundancyMode::STRIPE:
                result = writeStripe(document_id, data, ring, topology, handler);
                break;
            case RedundancyMode::STRIPE_MIRROR:
                result = writeStripeMirror(document_id, data, ring, topology, handler);
                break;
            case RedundancyMode::PARITY:
                result = writeParity(document_id, data, ring, topology, handler);
                break;
            default:
                result = WriteResult::failed(document_id, "Unsupported redundancy mode");
        }
    } catch (const std::exception& e) {
        spdlog::error("Write failed for document {}: {}", document_id, e.what());
        result = WriteResult::failed(document_id, e.what());
    }
    
    auto end = std::chrono::steady_clock::now();
    result.latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return result;
}

ReadResult RedundancyStrategy::read(
    const std::string& document_id,
    const std::string& collection,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler handler
) {
    auto start = std::chrono::steady_clock::now();
    
    stats_reads_++;
    
    ReadResult result;
    result.document_id = document_id;
    
    try {
        switch (config_.mode) {
            case RedundancyMode::NONE:
            case RedundancyMode::MIRROR:
            case RedundancyMode::GEO_MIRROR:
                result = readMirror(document_id, ring, topology, handler);
                break;
            case RedundancyMode::STRIPE:
            case RedundancyMode::STRIPE_MIRROR:
                result = readStripe(document_id, ring, topology, handler);
                break;
            case RedundancyMode::PARITY:
                result = readParity(document_id, ring, topology, handler);
                break;
            default:
                result.success = false;
                result.error_message = "Unsupported redundancy mode";
        }
    } catch (const std::exception& e) {
        spdlog::error("Read failed for document {}: {}", document_id, e.what());
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end = std::chrono::steady_clock::now();
    result.latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    if (result.success) {
        stats_bytes_read_ += result.data.size();
    }
    
    return result;
}

// ═══════════════════════════════════════════════════════════
// Internal Write Methods
// ═══════════════════════════════════════════════════════════

WriteResult RedundancyStrategy::writeMirror(
    const std::string& document_id,
    const std::vector<uint8_t>& data,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    WriteHandler handler
) {
    // Get primary shard
    auto primary_shard = ring.getNode(document_id);
    if (!primary_shard) {
        return WriteResult::failed(document_id, "No primary shard available");
    }
    
    // Get replica shards
    std::vector<std::string> target_shards;
    target_shards.push_back(*primary_shard);
    
    auto replicas = ring.getReplicaNodes(document_id, config_.replication_factor - 1);
    target_shards.insert(target_shards.end(), replicas.begin(), replicas.end());
    
    // Write to all shards
    std::vector<std::future<bool>> futures;
    std::vector<std::string> written_shards;
    std::vector<std::string> failed_shards;
    
    for (const auto& shard_id : target_shards) {
        futures.push_back(std::async(std::launch::async, [&, shard_id]() {
            return handler(shard_id, document_id, data);
        }));
    }
    
    // Wait for writes based on write concern
    uint32_t successful = 0;
    for (size_t i = 0; i < futures.size(); ++i) {
        try {
            if (futures[i].get()) {
                written_shards.push_back(target_shards[i]);
                successful++;
            } else {
                failed_shards.push_back(target_shards[i]);
            }
        } catch (const std::exception& e) {
            spdlog::warn("Write to shard {} failed: {}", target_shards[i], e.what());
            failed_shards.push_back(target_shards[i]);
        }
    }
    
    // Check write concern
    bool success = false;
    switch (config_.write_concern) {
        case WriteConcern::ONE:
            success = successful >= 1;
            break;
        case WriteConcern::MAJORITY:
            success = successful > (target_shards.size() / 2);
            break;
        case WriteConcern::ALL:
            success = successful == target_shards.size();
            break;
        case WriteConcern::QUORUM:
            success = successful >= config_.write_quorum;
            break;
    }
    
    if (success) {
        return WriteResult::successful(document_id, written_shards, std::chrono::milliseconds(0));
    } else {
        WriteResult result;
        result.success = false;
        result.document_id = document_id;
        result.written_shards = written_shards;
        result.failed_shards = failed_shards;
        result.acknowledgements = successful;
        result.error_message = "Write concern not met";
        return result;
    }
}

WriteResult RedundancyStrategy::writeStripe(
    const std::string& document_id,
    const std::vector<uint8_t>& data,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    WriteHandler handler
) {
    // Split data into chunks
    auto chunks = splitIntoChunks(data, config_.stripe.stripe_size_kb * 1024);
    
    // Get target shards
    auto primary_shard = ring.getNode(document_id);
    if (!primary_shard) {
        return WriteResult::failed(document_id, "No primary shard available");
    }
    
    std::vector<std::string> target_shards;
    target_shards.push_back(*primary_shard);
    
    auto replicas = ring.getReplicaNodes(document_id, chunks.size() - 1);
    target_shards.insert(target_shards.end(), replicas.begin(), replicas.end());
    
    // Write chunks to different shards
    std::vector<std::future<bool>> futures;
    std::vector<std::string> written_shards;
    
    for (size_t i = 0; i < chunks.size() && i < target_shards.size(); ++i) {
        const auto& chunk = chunks[i];
        const auto& shard_id = target_shards[i];
        
        futures.push_back(std::async(std::launch::async, [&, shard_id, chunk]() {
            return handler(shard_id, document_id + ":chunk:" + std::to_string(i), chunk);
        }));
    }
    
    // Wait for all writes
    uint32_t successful = 0;
    for (size_t i = 0; i < futures.size(); ++i) {
        try {
            if (futures[i].get()) {
                written_shards.push_back(target_shards[i]);
                successful++;
            }
        } catch (const std::exception& e) {
            spdlog::warn("Stripe write to shard {} failed: {}", target_shards[i], e.what());
        }
    }
    
    if (successful == chunks.size()) {
        return WriteResult::successful(document_id, written_shards, std::chrono::milliseconds(0));
    } else {
        return WriteResult::failed(document_id, "Not all chunks written successfully");
    }
}

WriteResult RedundancyStrategy::writeStripeMirror(
    const std::string& document_id,
    const std::vector<uint8_t>& data,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    WriteHandler handler
) {
    // Combine striping with mirroring
    // First stripe the data, then mirror each chunk
    auto chunks = splitIntoChunks(data, config_.stripe.stripe_size_kb * 1024);
    
    std::vector<std::string> all_written_shards;
    
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& chunk = chunks[i];
        std::string chunk_doc_id = document_id + ":chunk:" + std::to_string(i);
        
        // Mirror each chunk
        auto result = writeMirror(chunk_doc_id, chunk, ring, topology, handler);
        if (!result.success) {
            return WriteResult::failed(document_id, "Failed to write chunk " + std::to_string(i));
        }
        
        all_written_shards.insert(all_written_shards.end(),
                                  result.written_shards.begin(),
                                  result.written_shards.end());
    }
    
    return WriteResult::successful(document_id, all_written_shards, std::chrono::milliseconds(0));
}

WriteResult RedundancyStrategy::writeParity(
    const std::string& document_id,
    const std::vector<uint8_t>& data,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    WriteHandler handler
) {
    if (!erasure_coder_) {
        return WriteResult::failed(document_id, "Erasure coder not initialized");
    }
    
    // Encode data with parity
    auto chunks = erasure_coder_->encode(
        data,
        config_.erasure_coding.data_shards,
        config_.erasure_coding.parity_shards
    );
    
    // Get target shards
    auto primary_shard = ring.getNode(document_id);
    if (!primary_shard) {
        return WriteResult::failed(document_id, "No primary shard available");
    }
    
    std::vector<std::string> target_shards;
    target_shards.push_back(*primary_shard);
    
    auto replicas = ring.getReplicaNodes(document_id, chunks.size() - 1);
    target_shards.insert(target_shards.end(), replicas.begin(), replicas.end());
    
    // Write all chunks (data + parity)
    std::vector<std::future<bool>> futures;
    std::vector<std::string> written_shards;
    
    for (size_t i = 0; i < chunks.size() && i < target_shards.size(); ++i) {
        const auto& chunk = chunks[i];
        const auto& shard_id = target_shards[i];
        bool is_parity = i >= config_.erasure_coding.data_shards;
        
        std::string chunk_id = document_id + (is_parity ? ":parity:" : ":data:") + std::to_string(i);
        
        futures.push_back(std::async(std::launch::async, [&, shard_id, chunk, chunk_id]() {
            return handler(shard_id, chunk_id, chunk);
        }));
    }
    
    // Wait for writes
    uint32_t successful = 0;
    for (size_t i = 0; i < futures.size(); ++i) {
        try {
            if (futures[i].get()) {
                written_shards.push_back(target_shards[i]);
                successful++;
            }
        } catch (const std::exception& e) {
            spdlog::warn("Parity write to shard {} failed: {}", target_shards[i], e.what());
        }
    }
    
    if (successful >= config_.erasure_coding.data_shards) {
        return WriteResult::successful(document_id, written_shards, std::chrono::milliseconds(0));
    } else {
        return WriteResult::failed(document_id, "Not enough chunks written for recovery");
    }
}

WriteResult RedundancyStrategy::writeGeoMirror(
    const std::string& document_id,
    const std::vector<uint8_t>& data,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    WriteHandler handler
) {
    // Similar to mirror but considers geographic distribution
    // For now, delegate to writeMirror
    return writeMirror(document_id, data, ring, topology, handler);
}

// ═══════════════════════════════════════════════════════════
// Internal Read Methods
// ═══════════════════════════════════════════════════════════

ReadResult RedundancyStrategy::readMirror(
    const std::string& document_id,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler handler
) {
    // Get available shards
    auto primary_shard = ring.getNode(document_id);
    if (!primary_shard) {
        ReadResult result;
        result.success = false;
        result.error_message = "No shard available";
        return result;
    }
    
    std::vector<std::string> available_shards;
    available_shards.push_back(*primary_shard);
    
    auto replicas = ring.getReplicaNodes(document_id, config_.replication_factor - 1);
    available_shards.insert(available_shards.end(), replicas.begin(), replicas.end());
    
    // Select shard based on read preference
    std::string selected_shard = selectReadShard(available_shards, topology);
    
    // Read from selected shard
    auto data_opt = handler(selected_shard, document_id);
    
    ReadResult result;
    result.document_id = document_id;
    result.source_shard = selected_shard;
    result.from_replica = (selected_shard != *primary_shard);
    result.chunks_read = 1;
    
    if (data_opt) {
        result.success = true;
        result.data = std::string(data_opt->begin(), data_opt->end());
    } else {
        result.success = false;
        result.error_message = "Failed to read from shard";
    }
    
    return result;
}

ReadResult RedundancyStrategy::readStripe(
    const std::string& document_id,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler handler
) {
    ReadResult result;
    result.document_id = document_id;
    result.success = false;
    
    // Read all chunks
    // For now, assume chunks are numbered sequentially
    std::vector<std::vector<uint8_t>> chunks;
    
    // Try to read chunks until we can't find any more
    for (uint32_t i = 0; ; ++i) {
        std::string chunk_doc_id = document_id + ":chunk:" + std::to_string(i);
        auto shard_opt = ring.getNode(chunk_doc_id);
        
        if (!shard_opt) break;
        
        auto data_opt = handler(*shard_opt, chunk_doc_id);
        if (!data_opt) break;
        
        chunks.push_back(*data_opt);
    }
    
    if (chunks.empty()) {
        result.error_message = "No chunks found";
        return result;
    }
    
    // Merge chunks
    auto merged = mergeChunks(chunks);
    
    result.success = true;
    result.data = std::string(merged.begin(), merged.end());
    result.chunks_read = chunks.size();
    
    return result;
}

ReadResult RedundancyStrategy::readParity(
    const std::string& document_id,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler handler
) {
    if (!erasure_coder_) {
        ReadResult result;
        result.success = false;
        result.error_message = "Erasure coder not initialized";
        return result;
    }
    
    ReadResult result;
    result.document_id = document_id;
    result.success = false;
    
    // Try to read all chunks (data + parity)
    std::map<uint32_t, std::vector<uint8_t>> available_chunks;
    std::vector<uint32_t> missing_indices;
    
    uint32_t total_shards = config_.erasure_coding.totalShards();
    
    for (uint32_t i = 0; i < total_shards; ++i) {
        bool is_parity = i >= config_.erasure_coding.data_shards;
        std::string chunk_id = document_id + (is_parity ? ":parity:" : ":data:") + std::to_string(i);
        
        auto shard_opt = ring.getNode(chunk_id);
        if (!shard_opt) continue;
        
        auto data_opt = handler(*shard_opt, chunk_id);
        if (data_opt) {
            available_chunks[i] = *data_opt;
        } else {
            missing_indices.push_back(i);
        }
    }
    
    // Check if we can recover
    if (available_chunks.size() < config_.erasure_coding.data_shards) {
        result.error_message = "Not enough chunks available for recovery";
        return result;
    }
    
    try {
        // Decode/recover data
        auto recovered = erasure_coder_->decode(
            available_chunks,
            missing_indices,
            config_.erasure_coding.data_shards,
            config_.erasure_coding.parity_shards
        );
        
        result.success = true;
        result.data = std::string(recovered.begin(), recovered.end());
        result.chunks_read = available_chunks.size();
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to decode chunks: {}", e.what());
        result.error_message = e.what();
    }
    
    return result;
}

// ═══════════════════════════════════════════════════════════
// Utility Methods
// ═══════════════════════════════════════════════════════════

std::vector<std::vector<uint8_t>> RedundancyStrategy::splitIntoChunks(
    const std::vector<uint8_t>& data,
    size_t chunk_size
) {
    std::vector<std::vector<uint8_t>> chunks;
    
    for (size_t offset = 0; offset < data.size(); offset += chunk_size) {
        size_t size = std::min(chunk_size, data.size() - offset);
        std::vector<uint8_t> chunk(data.begin() + offset, data.begin() + offset + size);
        chunks.push_back(chunk);
    }
    
    return chunks;
}

std::vector<uint8_t> RedundancyStrategy::mergeChunks(
    const std::vector<std::vector<uint8_t>>& chunks
) {
    std::vector<uint8_t> merged;
    
    for (const auto& chunk : chunks) {
        merged.insert(merged.end(), chunk.begin(), chunk.end());
    }
    
    return merged;
}

std::string RedundancyStrategy::selectReadShard(
    const std::vector<std::string>& available_shards,
    ShardTopology& topology
) {
    if (available_shards.empty()) {
        throw std::runtime_error("No available shards");
    }
    
    switch (config_.read_preference) {
        case ReadPreference::PRIMARY:
            return available_shards[0];
            
        case ReadPreference::NEAREST:
            // For now, just return first shard
            // In production, calculate based on network latency
            return available_shards[0];
            
        case ReadPreference::ROUND_ROBIN: {
            static std::atomic<uint32_t> counter{0};
            return available_shards[counter.fetch_add(1) % available_shards.size()];
        }
            
        case ReadPreference::RANDOM: {
            size_t idx = std::rand() % available_shards.size();
            return available_shards[idx];
        }
            
        case ReadPreference::SECONDARY_ONLY:
            if (available_shards.size() > 1) {
                return available_shards[1];
            }
            return available_shards[0];
            
        default:
            return available_shards[0];
    }
}

bool RedundancyStrategy::remove(
    const std::string& document_id,
    const std::string& collection,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    WriteHandler handler
) {
    // Implementation depends on mode
    // For now, delete from all replicas
    return true;
}

bool RedundancyStrategy::recoverDocument(
    const std::string& document_id,
    const std::string& collection,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler read_handler,
    WriteHandler write_handler
) {
    stats_recoveries_++;
    return false;  // Not yet implemented
}

RedundancyStrategy::DocumentHealth RedundancyStrategy::checkDocumentHealth(
    const std::string& document_id,
    const std::string& collection,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler handler
) {
    DocumentHealth health;
    health.is_healthy = true;
    health.available_replicas = 0;
    health.required_replicas = config_.replication_factor;
    health.can_recover = true;
    return health;
}

void RedundancyStrategy::updateConfig(const RedundancyConfig& config) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (!config.validate()) {
        throw std::invalid_argument("Invalid redundancy configuration");
    }
    
    config_ = config;
    
    spdlog::info("RedundancyStrategy configuration updated");
}

RedundancyStats RedundancyStrategy::getStats() const {
    RedundancyStats stats;
    stats.total_documents = 0;
    stats.total_replicas = 0;
    stats.reads_from_primary = stats_reads_.load();
    return stats;
}

std::string RedundancyStrategy::exportPrometheusMetrics() const {
    std::stringstream ss;
    ss << "# HELP themis_redundancy_writes_total Total number of write operations\n";
    ss << "# TYPE themis_redundancy_writes_total counter\n";
    ss << "themis_redundancy_writes_total " << stats_writes_.load() << "\n";
    
    ss << "# HELP themis_redundancy_reads_total Total number of read operations\n";
    ss << "# TYPE themis_redundancy_reads_total counter\n";
    ss << "themis_redundancy_reads_total " << stats_reads_.load() << "\n";
    
    ss << "# HELP themis_redundancy_bytes_written_total Total bytes written\n";
    ss << "# TYPE themis_redundancy_bytes_written_total counter\n";
    ss << "themis_redundancy_bytes_written_total " << stats_bytes_written_.load() << "\n";
    
    return ss.str();
}

// ═══════════════════════════════════════════════════════════
// CollectionRedundancyManager Implementation
// ═══════════════════════════════════════════════════════════

CollectionRedundancyManager::CollectionRedundancyManager() {
    // Set default configuration (MIRROR with RF=3)
    default_config_.mode = RedundancyMode::MIRROR;
    default_config_.replication_factor = 3;
}

CollectionRedundancyManager::~CollectionRedundancyManager() = default;

void CollectionRedundancyManager::setDefaultConfig(const RedundancyConfig& config) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    default_config_ = config;
}

void CollectionRedundancyManager::setCollectionConfig(
    const std::string& collection,
    const RedundancyConfig& config
) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    collection_configs_[collection] = config;
    
    // Invalidate strategy so it's recreated on next access
    strategies_.erase(collection);
}

RedundancyConfig CollectionRedundancyManager::getConfig(const std::string& collection) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = collection_configs_.find(collection);
    if (it != collection_configs_.end()) {
        return it->second;
    }
    
    return default_config_;
}

std::shared_ptr<RedundancyStrategy> CollectionRedundancyManager::getStrategy(
    const std::string& collection
) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = strategies_.find(collection);
    if (it != strategies_.end()) {
        return it->second;
    }
    
    // Create new strategy
    auto config = getConfig(collection);
    auto strategy = std::make_shared<RedundancyStrategy>(config);
    strategies_[collection] = strategy;
    
    return strategy;
}

std::vector<std::string> CollectionRedundancyManager::listCollections() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<std::string> collections;
    for (const auto& [name, _] : collection_configs_) {
        collections.push_back(name);
    }
    
    return collections;
}

void CollectionRedundancyManager::removeCollectionConfig(const std::string& collection) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    collection_configs_.erase(collection);
    strategies_.erase(collection);
}

} // namespace sharding
} // namespace themisdb
