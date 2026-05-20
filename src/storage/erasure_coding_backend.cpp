/*
 * ThemisDB | File: erasure_coding_backend.cpp | Version: 0.0.13 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 277
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=79 | delta=76 | status=divergent
 * External Severity (v3): C=7, H=58, M=14
 * PR: #4336 docs(storage): correct SECURITY.md â€” erasure coding is implemente... (2026-03-19T05:35:37Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * ThemisDB Erasure Coding Backend — Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/erasure_coding_backend.h"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <algorithm>

namespace themisdb {
namespace storage {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

ErasureCodingBackend::ErasureCodingBackend(const ErasureCodingConfig& config)
    : config_(config)
{
    if (config_.data_shards < 2) {
        throw std::invalid_argument(
            "ErasureCodingBackend: data_shards must be >= 2 (got " +
            std::to_string(config_.data_shards) + ")");
    }
    if (config_.parity_shards < 1) {
        throw std::invalid_argument(
            "ErasureCodingBackend: parity_shards must be >= 1 (got " +
            std::to_string(config_.parity_shards) + ")");
    }

    // Map the storage-module enum to the sharding-module enum and create the
    // appropriate coder.
    themis::sharding::ErasureCodingAlgorithm algo;
    switch (config_.algorithm) {
        case ErasureCodingAlgorithm::CAUCHY:
            algo = themis::sharding::ErasureCodingAlgorithm::CAUCHY;
            break;
        case ErasureCodingAlgorithm::REED_SOLOMON:
        case ErasureCodingAlgorithm::LRC:
        default:
            algo = themis::sharding::ErasureCodingAlgorithm::REED_SOLOMON;
            break;
    }

    coder_ = themis::sharding::ErasureCoder::create(algo);

    spdlog::debug("ErasureCodingBackend: RS({},{}) algorithm={} overhead={:.1f}x",
                  config_.data_shards, config_.parity_shards,
                  static_cast<int>(config_.algorithm),
                  storageOverhead());
}

ErasureCodingBackend::~ErasureCodingBackend() = default;

// ---------------------------------------------------------------------------
// Low-level encode
// ---------------------------------------------------------------------------

std::vector<EncodedShard> ErasureCodingBackend::encode(
    const std::string&          blob_id,
    const std::vector<uint8_t>& data
) const {
    if (data.empty()) {
        throw std::invalid_argument(
            "ErasureCodingBackend::encode: cannot encode empty data for blob '" +
            blob_id + "'");
    }

    const uint32_t k = config_.data_shards;
    const uint32_t m = config_.parity_shards;
    const uint64_t original_size = data.size();

    // encode() from ReedSolomonCoder splits data into k+m chunks
    std::vector<std::vector<uint8_t>> raw_chunks;
    try {
        raw_chunks = coder_->encode(data, k, m);
    } catch (const std::exception& ex) {
        throw std::runtime_error(
            "ErasureCodingBackend::encode: coder failed for blob '" + blob_id +
            "': " + ex.what());
    }

    if (raw_chunks.size() != static_cast<size_t>(k + m)) {
        throw std::runtime_error(
            "ErasureCodingBackend::encode: expected " +
            std::to_string(k + m) + " chunks, got " +
            std::to_string(raw_chunks.size()) + " for blob '" + blob_id + "'");
    }

    std::vector<EncodedShard> shards;
    shards.reserve(raw_chunks.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(raw_chunks.size()); ++i) {
        EncodedShard s;
        s.shard_index   = i;
        s.is_parity     = (i >= k);
        s.original_size = original_size;
        s.data          = std::move(raw_chunks[i]);
        shards.push_back(std::move(s));
    }

    spdlog::debug("ErasureCodingBackend::encode: blob='{}' size={} shards={} "
                  "shard_size={}",
                  blob_id, original_size, shards.size(),
                  shards.empty() ? 0u : static_cast<unsigned>(shards[0].data.size()));
    return shards;
}

// ---------------------------------------------------------------------------
// Low-level decode
// ---------------------------------------------------------------------------

std::vector<uint8_t> ErasureCodingBackend::decode(
    const std::string&                      blob_id,
    const std::map<uint32_t, EncodedShard>& shards,
    uint64_t                                original_size
) const {
    const uint32_t k = config_.data_shards;
    const uint32_t m = config_.parity_shards;

    if (shards.size() < static_cast<size_t>(k)) {
        throw std::runtime_error(
            "ErasureCodingBackend::decode: need at least " +
            std::to_string(k) + " shards for blob '" + blob_id +
            "', only " + std::to_string(shards.size()) + " available");
    }

    // Determine original_size from shards if not provided by caller
    if (original_size == 0) {
        for (const auto& [idx, shard] : shards) {
            if (shard.original_size > 0) {
                original_size = shard.original_size;
                break;
            }
        }
    }

    // Build the map<uint32_t, vector<uint8_t>> expected by ErasureCoder::decode
    std::map<uint32_t, std::vector<uint8_t>> chunk_map;
    for (const auto& [idx, shard] : shards) {
        chunk_map[idx] = shard.data;
    }

    // Determine which shard indices are missing
    std::vector<uint32_t> missing;
    for (uint32_t i = 0; i < k + m; ++i) {
        if (chunk_map.find(i) == chunk_map.end()) {
            missing.push_back(i);
        }
    }

    auto recovered = coder_->decode(chunk_map, missing, k, m);

    // Trim trailing padding to restore exact original size
    if (original_size > 0 && recovered.size() > original_size) {
        recovered.resize(original_size);
    }

    spdlog::debug("ErasureCodingBackend::decode: blob='{}' available={} missing={} "
                  "recovered={}",
                  blob_id, shards.size(), missing.size(), recovered.size());
    return recovered;
}

// ---------------------------------------------------------------------------
// High-level put / get
// ---------------------------------------------------------------------------

void ErasureCodingBackend::put(
    const std::string&          blob_id,
    const std::vector<uint8_t>& data
) {
    auto shards = encode(blob_id, data);

    std::lock_guard<std::mutex> lock(store_mutex_);
    BlobEntry& entry = store_[blob_id];
    entry.original_size = data.size();
    entry.chunks.clear();
    for (auto& s : shards) {
        entry.chunks[s.shard_index] = std::move(s.data);
    }
}

std::optional<std::vector<uint8_t>> ErasureCodingBackend::get(
    const std::string& blob_id
) const {
    std::lock_guard<std::mutex> lock(store_mutex_);

    auto it = store_.find(blob_id);
    if (it == store_.end()) {
        return std::nullopt;
    }

    const BlobEntry& entry = it->second;

    // Verify we have enough shards for reconstruction
    if (entry.chunks.size() < static_cast<size_t>(config_.data_shards)) {
        spdlog::warn("ErasureCodingBackend::get: blob='{}' only {}/{} shards "
                     "available (need {}); cannot reconstruct",
                     blob_id, entry.chunks.size(),
                     config_.totalShards(), config_.data_shards);
        return std::nullopt;
    }

    // Build EncodedShard map for decode()
    std::map<uint32_t, EncodedShard> shard_map;
    for (const auto& [idx, chunk] : entry.chunks) {
        EncodedShard s;
        s.shard_index   = idx;
        s.is_parity     = (idx >= config_.data_shards);
        s.original_size = entry.original_size;
        s.data          = chunk;
        shard_map[idx]  = std::move(s);
    }

    try {
        return decode(blob_id, shard_map, entry.original_size);
    } catch (const std::exception& ex) {
        spdlog::error("ErasureCodingBackend::get: decode failed for '{}': {}",
                      blob_id, ex.what());
        return std::nullopt;
    }
}

void ErasureCodingBackend::remove(const std::string& blob_id) {
    std::lock_guard<std::mutex> lock(store_mutex_);
    store_.erase(blob_id);
}

bool ErasureCodingBackend::dropShard(
    const std::string& blob_id,
    uint32_t           shard_index
) {
    std::lock_guard<std::mutex> lock(store_mutex_);

    auto it = store_.find(blob_id);
    if (it == store_.end()) {
        return false;
    }

    auto& chunks = it->second.chunks;
    auto  chunk_it = chunks.find(shard_index);
    if (chunk_it == chunks.end()) {
        return false;
    }

    chunks.erase(chunk_it);
    return true;
}

uint32_t ErasureCodingBackend::availableShardCount(
    const std::string& blob_id
) const {
    std::lock_guard<std::mutex> lock(store_mutex_);

    auto it = store_.find(blob_id);
    if (it == store_.end()) {
        return 0u;
    }
    return static_cast<uint32_t>(it->second.chunks.size());
}

} // namespace storage
} // namespace themisdb

