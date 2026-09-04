/**
 * @file redundancy_strategy.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=13, H=21, M=62, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB RAID-like Redundancy Strategy Implementation
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/raft_shard_manager.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <limits>
#include <numeric>
#include <future>
#include <cstring>
#include <iterator>
#include <unordered_set>

namespace themis {
namespace sharding {

namespace {

// W2-S04: Version tracking — hybrid timestamp + counter for monotonic version tokens
// This provides better version tracking than pure timestamps by:
// 1. Using atomic counter for monotonicity within a process
// 2. Combining with timestamp for approximate temporal ordering
// 3. Avoiding clock skew issues between shards
// Format: [48-bit timestamp (microseconds) | 16-bit counter]
inline uint64_t makeVersionToken() {
    static std::atomic<uint16_t> version_counter{0};
    
    // Get current timestamp in microseconds (48 bits)
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    
    // Increment counter (16 bits) - wraps around at 65536
    uint16_t counter = version_counter.fetch_add(1, std::memory_order_relaxed);
    
    // Combine: upper 48 bits = timestamp, lower 16 bits = counter
    // This gives us monotonic ordering within a process and approximate temporal ordering
    return (static_cast<uint64_t>(micros) << 16) | counter;
}

/**
 * @brief BATCH 5: Retry helper for transient replication failures with exponential backoff.
 * 
 * Used for replication operations that may fail transiently due to:
 * - Temporary network issues (STORAGE_AHEAD, CACHE_AHEAD)
 * - Temporary coordinator unavailability
 * - Transient lock contention
 * 
 * @tparam Func Callable that returns bool (true = success, false = transient failure)
 * @param func Operation to retry
 * @param max_retries Maximum retry attempts (default: 3)
 * @param initial_delay_ms Initial backoff delay in milliseconds (default: 50)
 * @param max_delay_ms Maximum backoff delay cap (default: 2000)
 * @return true if operation succeeded, false if all retries exhausted
 */
template <typename Func>
inline bool retryReplicationWithBackoff(
    Func&& func,
    int max_retries = 3,
    uint64_t initial_delay_ms = 50,
    uint64_t max_delay_ms = 2000
) {
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        try {
            if (func()) {
                return true;  // Success
            }
            // Transient failure: prepare to retry
        } catch (const std::exception& e) {
            spdlog::debug("Replication attempt {} failed: {}", attempt + 1, e.what());
            // Exception indicates transient failure; retry
        }
        
        if (attempt < max_retries - 1) {
            // Exponential backoff: 50ms, 100ms, 200ms, ...
            uint64_t delay_ms = initial_delay_ms * (1 << attempt);
            delay_ms = std::min(delay_ms, max_delay_ms);
            
            spdlog::debug("Replication retry in {}ms (attempt {}/{})", 
                         delay_ms, attempt + 1, max_retries);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
    
    spdlog::warn("Replication exhausted {} retries", max_retries);
    return false;  // All retries exhausted
}

}  // namespace

// ═══════════════════════════════════════════════════════════
// RedundancyConfig Implementation
// ═══════════════════════════════════════════════════════════

/** @brief Validate redundancy configuration invariants and mode-specific constraints. */
bool RedundancyConfig::validate() const {
    if (replication_factor < 1) {
        spdlog::error("Invalid replication_factor: must be >= 1");
        return false;
    }
    
    if (mode == RedundancyMode::PARITY || mode == RedundancyMode::RAID6) {
        if (erasure_coding.data_shards < 2) {
            spdlog::error("Invalid erasure coding: data_shards must be >= 2");
            return false;
        }
        if (erasure_coding.parity_shards < 1) {
            spdlog::error("Invalid erasure coding: parity_shards must be >= 1");
            return false;
        }
        if (mode == RedundancyMode::RAID6 && erasure_coding.parity_shards < 2) {
            spdlog::error("RAID6 requires at least 2 parity shards");
            return false;
        }
    }
    
    if (write_quorum > replication_factor) {
        spdlog::error("Invalid write_quorum: must be <= replication_factor");
        return false;
    }

    if (mode == RedundancyMode::GEO_MIRROR) {
        // Each per-region write quorum must not exceed the replication factor
        for (const auto& [region, q] : geo_replication.region_write_quorums) {
            if (q == 0) {
                spdlog::error("GEO_MIRROR: write quorum for region '{}' must be >= 1", region);
                return false;
            }
            if (q > replication_factor) {
                spdlog::error("GEO_MIRROR: write quorum for region '{}' ({}) exceeds "
                              "replication_factor ({})", region, q, replication_factor);
                return false;
            }
        }
        // Same for read quorums
        for (const auto& [region, q] : geo_replication.region_read_quorums) {
            if (q == 0) {
                spdlog::error("GEO_MIRROR: read quorum for region '{}' must be >= 1", region);
                return false;
            }
            if (q > replication_factor) {
                spdlog::error("GEO_MIRROR: read quorum for region '{}' ({}) exceeds "
                              "replication_factor ({})", region, q, replication_factor);
                return false;
            }
        }
        // region_failure_threshold must be in (0, 1]
        if (geo_replication.enable_geo_failover &&
            (geo_replication.region_failure_threshold <= 0.0 ||
             geo_replication.region_failure_threshold > 1.0)) {
            spdlog::error("GEO_MIRROR: region_failure_threshold must be in (0, 1], got {}",
                          geo_replication.region_failure_threshold);
            return false;
        }
    }
    
    return true;
}

/** @brief Compute logical-to-physical storage efficiency for configured mode. */
double RedundancyConfig::getStorageEfficiency() const {
    switch (mode) {
        case RedundancyMode::NONE:
            return 1.0;
        case RedundancyMode::STRIPE:
            return 1.0;  // No redundancy
        case RedundancyMode::MIRROR:
        [[fallthrough]];\n        case RedundancyMode::GEO_MIRROR:
            return 1.0 / replication_factor;
        case RedundancyMode::STRIPE_MIRROR:
            return 1.0 / replication_factor;
        case RedundancyMode::PARITY:
        [[fallthrough]];\n        case RedundancyMode::RAID6:
            return erasure_coding.storageEfficiency();
        default:
            return 1.0;
    }
}

/** @brief Return maximum tolerable shard failures under configured mode. */
uint32_t RedundancyConfig::getFaultTolerance() const {
    switch (mode) {
        case RedundancyMode::NONE:
        [[fallthrough]];\n        case RedundancyMode::STRIPE:
            return 0;  // No fault tolerance
        case RedundancyMode::MIRROR:
        [[fallthrough]];\n        case RedundancyMode::STRIPE_MIRROR:
        [[fallthrough]];\n        case RedundancyMode::GEO_MIRROR:
            return replication_factor - 1;
        case RedundancyMode::PARITY:
        [[fallthrough]];\n        case RedundancyMode::RAID6:
            return erasure_coding.faultTolerance();
        default:
            return 0;
    }
}

/** @brief Return effective shard fanout/replication factor for writes. */
uint32_t RedundancyConfig::getEffectiveReplicationFactor() const {
    switch (mode) {
        case RedundancyMode::NONE:
        [[fallthrough]];\n        case RedundancyMode::STRIPE:
            return 1;
        case RedundancyMode::PARITY:
        [[fallthrough]];\n        case RedundancyMode::RAID6:
            return erasure_coding.totalShards();
        default:
            return replication_factor;
    }
}

// ═══════════════════════════════════════════════════════════
// ChunkInfo Implementation
// ═══════════════════════════════════════════════════════════

/** @brief Serialize chunk metadata into binary payload (placeholder wire format). */
std::vector<uint8_t> ChunkInfo::serialize() const {
    std::vector<uint8_t> data;
    // Simple binary serialization
    // In production, use protobuf or similar
    return data;
}

/** @brief Deserialize chunk metadata from binary payload (placeholder parser). */
std::optional<ChunkInfo> ChunkInfo::deserialize([[maybe_unused]] const std::vector<uint8_t>& data) {
    // Simple binary deserialization
    // In production, use protobuf or similar
    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════
// StripeGroup Implementation
// ═══════════════════════════════════════════════════════════

/** @brief Return whether all data chunks are assigned to concrete shard ids. */
bool StripeGroup::isComplete() const {
    for (const auto& chunk : data_chunks) {
        if (chunk.shard_id.empty()) {
            return false;
        }
    }
    return true;
}

/** @brief Return indices of data chunks currently missing shard assignment. */
std::vector<uint32_t> StripeGroup::getMissingChunks() const {
    std::vector<uint32_t> missing = {};

    for (size_t i = 0; i <static_cast<int>(data_chunks.size()); ++i) {
        if (data_chunks[i].shard_id.empty()) {
            missing.push_back(static_cast<uint32_t>(i));
        }
    }
    return missing;
}

/** @brief Return whether available data+parity chunks are sufficient for recovery. */
bool StripeGroup::canRecover(uint32_t data_shards, [[maybe_unused]] uint32_t parity_shards) const {
    uint32_t available = 0;
    for (const auto& chunk : data_chunks) {
        if (!chunk.shard_id.empty()) {
          available++;
        }
    }
    for (const auto& chunk : parity_chunks) {
        if (!chunk.shard_id.empty()) {
          available++;
        }
    }
    
    // Need at least data_shards chunks to recover
    return available >= data_shards;
}

// ═══════════════════════════════════════════════════════════
// WriteResult Implementation
// ═══════════════════════════════════════════════════════════

/** @brief Build successful write-result payload helper. */
WriteResult WriteResult::successful(const std::string& doc_id, 
                                   const std::vector<std::string>& shards,
                                   std::chrono::milliseconds lat) {
    WriteResult result;
    result.success = true;
    result.document_id = doc_id;
    result.written_shards = shards;
    result.acknowledgements = static_cast<uint32_t>(shards.size());
    result.latency = lat;
    return result;
}

/** @brief Build failed write-result payload helper. */
WriteResult WriteResult::failed(const std::string& doc_id, const std::string& error) {
    WriteResult result;
    result.success = false;
    result.document_id = doc_id;
    result.error_message = error;
    return result;
}

// ═══════════════════════════════════════════════════════════
// ReedSolomonCoder Implementation
// Systematic Vandermonde-based encoding that supports recovery of up to
// parity_shards simultaneously lost chunks (data or parity).
// ═══════════════════════════════════════════════════════════

// Build Vandermonde parity matrix (parity_shards x data_shards).
// V[p][j] = gf_pow(p+1, j) so each row uses a distinct evaluation point {1,2,...,m}.
std::vector<std::vector<uint8_t>> ReedSolomonCoder::buildVandermondeMatrix(
    uint32_t rows, uint32_t cols
) {
    if (rows + cols > 255) {
        throw std::invalid_argument("Too many shards: rows + cols must be <= 255");
    }
    std::vector<std::vector<uint8_t>> matrix(rows, std::vector<uint8_t>(cols));
    for (uint32_t p = 0; p < rows; ++p) {
        uint8_t base = static_cast<uint8_t>(p + 1);  // evaluation point: 1..m
        for (uint32_t j = 0; j < cols; ++j) {
            matrix[p][j] = gf_pow(base, static_cast<uint8_t>(j));
        }
    }
    return matrix;
}

// Gaussian elimination in GF(2^8) to invert an n×n matrix in-place.
bool ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
    const size_t n = matrix.size();
    // Augment with identity matrix
    std::vector<std::vector<uint8_t>> aug(n, std::vector<uint8_t>(2 * n, 0));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            aug[i][j] = matrix[i][j];
        }
        aug[i][n + i] = 1;
    }
    for (size_t col = 0; col < n; ++col) {
        // Find pivot
        size_t pivot = n;
        for (size_t row = col; row < n; ++row) {
            if (aug[row][col] != 0) { pivot = row; break; }
        }
        if (pivot == n) {
          return false;
        }
        std::swap(aug[col], aug[pivot]);
        uint8_t inv_pivot = gf_inv(aug[col][col]);
        for (size_t j = 0; j < 2 * n; ++j) {
            aug[col][j] = gf_mul(aug[col][j], inv_pivot);
        }
        for (size_t row = 0; row < n; ++row) {
            if (row == col || aug[row][col] == 0) {
              continue;
            }
            uint8_t factor = aug[row][col];
            for (size_t j = 0; j < 2 * n; ++j) {
                aug[row][j] ^= gf_mul(factor, aug[col][j]);
            }
        }
    }
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            matrix[i][j] = aug[i][n + j];
        }
    }
    return true;
}

std::vector<std::vector<uint8_t>> ReedSolomonCoder::encode(
    const std::vector<uint8_t>& data,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    // Calculate chunk size (pad last chunk with zeros if needed)
    size_t chunk_size = (static_cast<int>(data.size()) + data_shards - 1) / data_shards;

    // Split data into k chunks (data shards)
    std::vector<std::vector<uint8_t>> chunks;
    chunks.reserve(data_shards + parity_shards);
    for (uint32_t i = 0; i < data_shards; ++i) {
        size_t offset = i * chunk_size;
        std::vector<uint8_t> chunk(chunk_size, 0);
        if (static_cast<int>(data.size()) > offset) {
            size_t sz = std::min(chunk_size, static_cast<int>(data.size()) - offset);
            std::memcpy(chunk.data(), data.data() + offset, sz);
        }
        chunks.push_back(std::move(chunk));
    }

    // Build Vandermonde parity matrix and compute parity chunks
    auto vandermonde = buildVandermondeMatrix(parity_shards, data_shards);
    for (uint32_t p = 0; p < parity_shards; ++p) {
        std::vector<uint8_t> parity(chunk_size, 0);
        for (uint32_t j = 0; j < data_shards; ++j) {
            uint8_t coeff = vandermonde[p][j];
            if (coeff == 0) {
              continue;
            }
            for (size_t x = 0; x < chunk_size; ++x) {
                parity[x] ^= gf_mul(coeff, chunks[j][x]);
            }
        }
        chunks.push_back(std::move(parity));
    }

    return chunks;
}

std::vector<uint8_t> ReedSolomonCoder::decode(
    const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
    const std::vector<uint32_t>& missing_indices,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    // Validate that the number of missing chunks does not exceed the fault tolerance
    if (static_cast<int>(missing_indices.size()) > parity_shards) {
        throw std::runtime_error("Too many missing chunks: " +
                                 std::to_string(missing_indices.size()) +
                                 " missing, but only " + std::to_string(parity_shards) +
                                 " parity shard(s) available");
    }
    if (static_cast<int>(available_chunks.size()) < data_shards) {
        throw std::runtime_error("Not enough chunks for recovery");
    }

    // Fast path: all data chunks present — just concatenate
    bool all_data_available = true;
    for (uint32_t i = 0; i < data_shards; ++i) {
        if (available_chunks.find(i) == available_chunks.end()) {
            all_data_available = false;
            break;
        }
    }
    if (all_data_available) {
        std::vector<uint8_t> recovered = {};

        for (uint32_t i = 0; i < data_shards; ++i) {
            const auto& chunk = available_chunks.at(i);
            recovered.insert(recovered.end(), chunk.begin(), chunk.end());
        }
        return recovered;
    }

    // Full erasure recovery using Vandermonde matrix inversion.
    // Build full (k+m) × k encoding matrix:
    //   Rows 0..k-1:   identity (data chunks)
    //   Rows k..k+m-1: Vandermonde parity matrix
    const uint32_t total_shards = data_shards + parity_shards;
    auto vandermonde = buildVandermondeMatrix(parity_shards, data_shards);

    std::vector<std::vector<uint8_t>> full_matrix(total_shards,
                                                   std::vector<uint8_t>(data_shards, 0));
    for (uint32_t i = 0; i < data_shards; ++i) {
        full_matrix[i][i] = 1;
    }
    for (uint32_t p = 0; p < parity_shards; ++p) {
        full_matrix[data_shards + p] = vandermonde[p];
    }

    // Select k rows corresponding to available chunks
    std::vector<uint32_t> available_indices;
    available_indices.reserve(data_shards);
    for (const auto& [idx, _] : available_chunks) {
        if (static_cast<int>(available_indices.size()) < data_shards) {
            available_indices.push_back(idx);
        }
    }

    std::vector<std::vector<uint8_t>> decode_matrix(data_shards,
                                                     std::vector<uint8_t>(data_shards));
    for (size_t i = 0; i < data_shards; ++i) {
        decode_matrix[i] = full_matrix[available_indices[i]];
    }

    if (!invertMatrix(decode_matrix)) {
        throw std::runtime_error("Failed to invert decode matrix for Reed-Solomon recovery");
    }

    // Apply inverse matrix byte-by-byte to recover original data chunks
    size_t chunk_size = available_chunks.begin()-> static_cast<int>(second.size());
    std::vector<std::vector<uint8_t>> recovered_data(data_shards,
                                                      std::vector<uint8_t>(chunk_size, 0));
    for (size_t x = 0; x < chunk_size; ++x) {
        std::vector<uint8_t> available_bytes(data_shards);
        for (size_t i = 0; i < data_shards; ++i) {
            available_bytes[i] = available_chunks.at(available_indices[i])[x];
        }
        std::vector<uint8_t> recovered_bytes;
        gf_matrix_mul(decode_matrix, available_bytes, recovered_bytes);
        for (size_t i = 0; i < data_shards; ++i) {
            recovered_data[i][x] = recovered_bytes[i];
        }
    }

    std::vector<uint8_t> result;
    result.reserve(data_shards * chunk_size);
    for (const auto& chunk : recovered_data) {
        result.insert(result.end(), chunk.begin(), chunk.end());
    }
    return result;
}

uint8_t ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
    // Galois Field GF(2^8) multiplication using Russian Peasant algorithm
    // Irreducible polynomial: x^8 + x^4 + x^3 + x^2 + 1 (0x1d)
    uint8_t p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) {
          p ^= a;
        }
        const uint8_t hi = a & 0x80;
        a <<= 1;
        if (hi) {
          a ^= 0x1d;
        }
        b >>= 1;
    }
    return p;
}

uint8_t ReedSolomonCoder::gf_inv([[maybe_unused]] uint8_t a) {
    if (a == 0) {
      return 0;
    }
    // Fermat's Little Theorem for finite fields: a^(p-1) = 1 in GF(p),
    // so a^(2^8 - 2) = a^(-1) in GF(2^8).
    uint8_t result = 1;
    for (int i = 7; i >= 0; i--) {
        result = gf_mul(result, result);
        if ((254 >> i) & 1) {
          result = gf_mul(result, a);
        }
    }
    return result;
}

uint8_t ReedSolomonCoder::gf_div(uint8_t a, uint8_t b) {
    return gf_mul(a, gf_inv(b));
}

uint8_t ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp) {
    uint8_t result = 1;
    for (uint8_t i = 0; i < exp; ++i) {
        result = gf_mul(result, a);
    }
    return result;
}

void ReedSolomonCoder::gf_matrix_mul(
    const std::vector<std::vector<uint8_t>>& matrix,
    const std::vector<uint8_t>& vec,
    std::vector<uint8_t>& result
) {
    const size_t rows = matrix.size();
    result.assign(rows, 0);
    for (size_t i = 0; i < rows; i++) {
        uint8_t sum = 0;
        for (size_t j = 0; j < matrix[i].size()  && static_cast<size_t>(j) <static_cast<int>(vec.size()); j++) {
            sum ^= gf_mul(matrix[i][j], vec[j]);
        }
        result[i] = sum;
    }
}

// ═══════════════════════════════════════════════════════════
// CauchyReedSolomonCoder Implementation
// ═══════════════════════════════════════════════════════════

// Galois Field (GF(2^8)) multiplication using Russian Peasant algorithm
uint8_t CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    uint8_t hi_bit_set;
    
    for (int i = 0; i < 8; i++) {
        if (b & 1) {
            p ^= a;
        }
        hi_bit_set = a & 0x80;
        a <<= 1;
        if (hi_bit_set) {
            a ^= 0x1d;  // x^8 + x^4 + x^3 + x^2 + 1 polynomial
        }
        b >>= 1;
    }
    
    return p;
}

// Galois Field inverse using Extended Euclidean algorithm
uint8_t CauchyReedSolomonCoder::gf_inv([[maybe_unused]] uint8_t a) {
    if (a == 0) {
      return 0;
    }
    
    // Use Fermat's Little Theorem: a^(2^8 - 2) = a^254 = a^(-1) in GF(2^8)
    // Compute using repeated squaring
    uint8_t result = 1;
    
    // Exponent 254 = 11111110 in binary
    // Start from the highest bit and work down
    for (int i = 7; i >= 0; i--) {
        result = gf_mul(result, result);  // Square
        if ((254 >> i) & 1) {
            result = gf_mul(result, a);
        }
    }
    
    return result;
}

// Build Cauchy matrix for erasure coding
std::vector<std::vector<uint8_t>> CauchyReedSolomonCoder::buildCauchyMatrix(
    uint32_t rows, uint32_t cols
) {
    std::vector<std::vector<uint8_t>> matrix(rows, std::vector<uint8_t>(cols));
    
    // Cauchy matrix: M[i][j] = 1 / (x[i] XOR y[j])
    // where x and y are distinct elements from GF(2^8)
    
    // Ensure rows + cols doesn't exceed 256 to avoid duplicates
    if (rows + cols > 256) {
        throw std::invalid_argument("Too many shards: rows + cols must be <= 256");
    }
    
    std::vector<uint8_t> x(rows);
    std::vector<uint8_t> y(cols);
    
    // Initialize x and y with distinct values
    // Use first 'rows' values for x, next 'cols' values for y
    for (uint32_t i = 0; i < rows; i++) {
        x[i] = static_cast<uint8_t>(i);
    }
    for (uint32_t j = 0; j < cols; j++) {
        y[j] = static_cast<uint8_t>(rows + j);
    }
    
    // Build Cauchy matrix
    for (uint32_t i = 0; i < rows; i++) {
        for (uint32_t j = 0; j < cols; j++) {
            uint8_t diff = x[i] ^ y[j];
            
            // Ensure diff is non-zero (x and y should be distinct)
            if (diff == 0) {
                throw std::runtime_error("Invalid Cauchy matrix: x[i] == y[j]");
            }
            
            matrix[i][j] = gf_inv(diff);
        }
    }
    
    return matrix;
}

// Matrix-vector multiplication in GF(2^8)
void CauchyReedSolomonCoder::gf_matrix_mul(
    const std::vector<std::vector<uint8_t>>& matrix,
    const std::vector<uint8_t>& vec,
    std::vector<uint8_t>& result
) {
    size_t rows = matrix.size();
    size_t cols = matrix[0].size();
    
    result.resize(rows, 0);
    
    for (size_t i = 0; i < rows; i++) {
        uint8_t sum = 0;
        for (size_t j = 0; j < cols  && static_cast<size_t>(j) <static_cast<int>(vec.size()); j++) {
            sum ^= gf_mul(matrix[i][j], vec[j]);
        }
        result[i] = sum;
    }
}

// Gauss-Jordan elimination for matrix inversion in GF(2^8)
bool CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
    size_t n = matrix.size();
    if (n == 0 || matrix[0].size() != n) {
      return false;
    }
    
    // Create augmented matrix [A | I]
    std::vector<std::vector<uint8_t>> augmented(n, std::vector<uint8_t>(2 * n, 0));
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            augmented[i][j] = matrix[i][j];
        }
        augmented[i][n + i] = 1;  // Identity matrix
    }
    
    // Forward elimination
    for (size_t i = 0; i < n; i++) {
        // Find pivot
        size_t pivot_row = i;
        for (size_t j = i + 1; j < n; j++) {
            if (augmented[j][i] != 0) {
                pivot_row = j;
                break;
            }
        }
        
        if (augmented[pivot_row][i] == 0) {
            return false;  // Matrix is singular
        }
        
        // Swap rows
        if (pivot_row != i) {
            std::swap(augmented[i], augmented[pivot_row]);
        }
        
        // Scale pivot row
        uint8_t pivot = augmented[i][i];
        uint8_t pivot_inv = gf_inv(pivot);
        for (size_t j = 0; j < 2 * n; j++) {
            augmented[i][j] = gf_mul(augmented[i][j], pivot_inv);
        }
        
        // Eliminate column
        for (size_t j = 0; j < n; j++) {
            if (j != i && augmented[j][i] != 0) {
                uint8_t factor = augmented[j][i];
                for (size_t k = 0; k < 2 * n; k++) {
                    augmented[j][k] ^= gf_mul(factor, augmented[i][k]);
                }
            }
        }
    }
    
    // Extract inverse from augmented matrix
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            matrix[i][j] = augmented[i][n + j];
        }
    }
    
    return true;
}

std::vector<std::vector<uint8_t>> CauchyReedSolomonCoder::encode(
    const std::vector<uint8_t>& data,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    std::vector<std::vector<uint8_t>> chunks;
    
    // Calculate chunk size
    size_t chunk_size = (static_cast<int>(data.size()) + data_shards - 1) / data_shards;
    
    // Split data into chunks
    for (uint32_t i = 0; i < data_shards; ++i) {
        size_t offset = i * chunk_size;
        size_t size = std::min(chunk_size, static_cast<int>(data.size()) - offset);
        
        std::vector<uint8_t> chunk(chunk_size, 0);  // Pad with zeros
        if (static_cast<int>(data.size()) > offset) {
            std::memcpy(chunk.data(), data.data() + offset, size);
        }
        chunks.push_back(chunk);
    }
    
    // Build Cauchy matrix for parity generation
    auto cauchy_matrix = buildCauchyMatrix(parity_shards, data_shards);
    
    // Generate parity chunks using Cauchy matrix
    for (uint32_t p = 0; p < parity_shards; ++p) {
        std::vector<uint8_t> parity(chunk_size, 0);
        
        // For each byte position in the chunk
        for (size_t byte_pos = 0; byte_pos < chunk_size; ++byte_pos) {
            // Collect data bytes at this position
            std::vector<uint8_t> data_bytes(data_shards);
            for (uint32_t d = 0; d < data_shards; ++d) {
                data_bytes[d] = chunks[d][byte_pos];
            }
            
            // Apply Cauchy matrix row to compute parity byte
            uint8_t parity_byte = 0;
            for (uint32_t d = 0; d < data_shards; ++d) {
                parity_byte ^= gf_mul(cauchy_matrix[p][d], data_bytes[d]);
            }
            parity[byte_pos] = parity_byte;
        }
        
        chunks.push_back(parity);
    }
    
    return chunks;
}

std::vector<uint8_t> CauchyReedSolomonCoder::decode(
    const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
    const std::vector<uint32_t>& missing_indices,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    // Validate that the number of missing chunks does not exceed the fault tolerance
    if (static_cast<int>(missing_indices.size()) > parity_shards) {
        throw std::runtime_error("Too many missing chunks: " +
                                 std::to_string(missing_indices.size()) +
                                 " missing, but only " + std::to_string(parity_shards) +
                                 " parity shard(s) available");
    }
    // Check if we have enough chunks
    if (static_cast<int>(available_chunks.size()) < data_shards) {
        throw std::runtime_error("Not enough chunks for recovery");
    }
    
    // If all data chunks are available, just concatenate them
    bool all_data_available = true;
    for (uint32_t i = 0; i < data_shards; ++i) {
        if (available_chunks.find(i) == available_chunks.end()) {
            all_data_available = false;
            break;
        }
    }
    
    if (all_data_available) {
        std::vector<uint8_t> recovered = {};

        for (uint32_t i = 0; i < data_shards; ++i) {
            const auto& chunk = available_chunks.at(i);
            recovered.insert(recovered.end(), chunk.begin(), chunk.end());
        }
        return recovered;
    }
    
    // Need to use erasure decoding
    size_t chunk_size = available_chunks.begin()-> static_cast<int>(second.size());
    uint32_t total_shards = data_shards + parity_shards;
    
    // Build full Cauchy matrix (identity for data, Cauchy for parity)
    std::vector<std::vector<uint8_t>> full_matrix(total_shards, std::vector<uint8_t>(data_shards));
    
    // Identity portion (for data shards)
    for (uint32_t i = 0; i < data_shards; ++i) {
        for (uint32_t j = 0; j < data_shards; ++j) {
            full_matrix[i][j] = (i == j) ? 1 : 0;
        }
    }
    
    // Cauchy portion (for parity shards)
    auto cauchy_matrix = buildCauchyMatrix(parity_shards, data_shards);
    for (uint32_t i = 0; i < parity_shards; ++i) {
        for (uint32_t j = 0; j < data_shards; ++j) {
            full_matrix[data_shards + i][j] = cauchy_matrix[i][j];
        }
    }
    
    // Extract rows for available chunks
    std::vector<std::vector<uint8_t>> decode_matrix(data_shards, std::vector<uint8_t>(data_shards));
    std::vector<uint32_t> available_indices;
    
    for (const auto& [idx, _] : available_chunks) {
        if (static_cast<int>(available_indices.size()) < data_shards) {
            available_indices.push_back(idx);
        }
    }
    
    for (size_t i = 0; i < data_shards; ++i) {
        for (size_t j = 0; j < data_shards; ++j) {
            decode_matrix[i][j] = full_matrix[available_indices[i]][j];
        }
    }
    
    // Invert the decode matrix
    if (!invertMatrix(decode_matrix)) {
        throw std::runtime_error("Failed to invert decode matrix");
    }
    
    // Recover data chunks byte by byte
    std::vector<std::vector<uint8_t>> recovered_data(data_shards, std::vector<uint8_t>(chunk_size));
    
    for (size_t byte_pos = 0; byte_pos < chunk_size; ++byte_pos) {
        // Collect available bytes
        std::vector<uint8_t> available_bytes(data_shards);
        for (size_t i = 0; i < data_shards; ++i) {
            available_bytes[i] = available_chunks.at(available_indices[i])[byte_pos];
        }
        
        // Apply inverse matrix to recover original data bytes
        std::vector<uint8_t> recovered_bytes;
        gf_matrix_mul(decode_matrix, available_bytes, recovered_bytes);
        
        // Store recovered bytes
        for (size_t i = 0; i < data_shards; ++i) {
            recovered_data[i][byte_pos] = recovered_bytes[i];
        }
    }
    
    // Concatenate recovered data chunks
    std::vector<uint8_t> result = {};

    for (const auto& chunk : recovered_data) {
        result.insert(result.end(), chunk.begin(), chunk.end());
    }
    
    return result;
}

// ═══════════════════════════════════════════════════════════
// LocallyRepairableCoder (LRC) Implementation
// ═══════════════════════════════════════════════════════════

namespace {  // anonymous — GF helpers shared with LRC

static constexpr uint8_t LRC_GF_POLY = 0x1d;  // x^8+x^4+x^3+x^2+1

static uint8_t lrc_gf_mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1) {
          p ^= a;
        }
        bool carry = (a & 0x80) != 0;
        a <<= 1;
        if (carry) {
          a ^= LRC_GF_POLY;
        }
        b >>= 1;
    }
    return p;
}

static uint8_t lrc_gf_pow(uint8_t a, uint8_t exp) {
    uint8_t r = 1;
    for (uint8_t i = 0; i < exp; ++i) {
      r = lrc_gf_mul(r, a);
    }
    return r;
}

static uint8_t lrc_gf_inv([[maybe_unused]] uint8_t a) {
    // Extended Euclidean / brute-force for GF(2^8)
    for (int b = 1; b < 256; ++b)
        if (lrc_gf_mul(a, static_cast<uint8_t>(b)) == 1)
            return static_cast<uint8_t>(b);
    return 0;
}

[[maybe_unused]] static void lrc_gf_matrix_mul(const std::vector<std::vector<uint8_t>>& m,
                                                const std::vector<uint8_t>& v,
                                                std::vector<uint8_t>& result) {
    const std::size_t rows = m.size(), cols = v.size();
    result.assign(rows, 0);
    for (std::size_t r = 0; r < rows; ++r)
        for (std::size_t c = 0; c < cols; ++c)
            result[r] ^= lrc_gf_mul(m[r][c], v[c]);
}

static std::vector<std::vector<uint8_t>> lrc_buildVandermonde(uint32_t rows, uint32_t cols) {
    std::vector<std::vector<uint8_t>> mat(rows, std::vector<uint8_t>(cols));
    for (uint32_t r = 0; r < rows; ++r)
        for (uint32_t c = 0; c < cols; ++c)
            mat[r][c] = lrc_gf_pow(static_cast<uint8_t>(r + 1), static_cast<uint8_t>(c));
    return mat;
}

static bool lrc_invertMatrix(std::vector<std::vector<uint8_t>>& mat) {
    const uint32_t n = static_cast<uint32_t>(mat.size());
    std::vector<std::vector<uint8_t>> id(n, std::vector<uint8_t>(n, 0));
    for (uint32_t i = 0; i < n; ++i) {
      id[i][i] = 1;
    }
    for (uint32_t col = 0; col < n; ++col) {
        // Pivot
        uint32_t pivot = n;
        for (uint32_t row = col; row < n; ++row)
            if (mat[row][col]) { pivot = row; break; }
        if (pivot == n) {
          return false;
        }
        std::swap(mat[col], mat[pivot]);
        std::swap(id[col], id[pivot]);
        // Scale
        const uint8_t inv_lead = lrc_gf_inv(mat[col][col]);
        for (uint32_t j = 0; j < n; ++j) {
            mat[col][j] = lrc_gf_mul(mat[col][j], inv_lead);
            id[col][j]  = lrc_gf_mul(id[col][j],  inv_lead);
        }
        // Eliminate
        for (uint32_t row = 0; row < n; ++row) {
            if (row == col || !mat[row][col]) {
              continue;
            }
            const uint8_t f = mat[row][col];
            for (uint32_t j = 0; j < n; ++j) {
                mat[row][j] ^= lrc_gf_mul(f, mat[col][j]);
                id[row][j]  ^= lrc_gf_mul(f, id[col][j]);
            }
        }
    }
    mat = id;
    return true;
}

} // anonymous namespace

// ── LocallyRepairableCoder helpers ──────────────────────────────────────────

uint32_t LocallyRepairableCoder::localGroupCount(uint32_t data_shards,
                                                  uint32_t parity_shards) {
    const uint32_t groups = (data_shards + kDefaultLocalGroupSize - 1) / kDefaultLocalGroupSize;
    // Reserve at least 1 parity for global coverage; cap local group count.
    return std::min(groups, parity_shards > 1 ? parity_shards - 1 : parity_shards);
}

// ── encode ───────────────────────────────────────────────────────────────────

std::vector<std::vector<uint8_t>> LocallyRepairableCoder::encode(
    const std::vector<uint8_t>& data,
    uint32_t data_shards,
    uint32_t parity_shards)
{
    if (data_shards == 0 || parity_shards == 0)
        throw std::invalid_argument("LRC encode: shard counts must be > 0");
    if (data.empty())
        throw std::invalid_argument("LRC encode: data must not be empty");

    const uint32_t shard_size = static_cast<uint32_t>(
        (static_cast<int>(data.size()) + data_shards - 1) / data_shards);
    const uint32_t n_local = localGroupCount(data_shards, parity_shards);
    const uint32_t n_global = parity_shards - n_local;

    // Split data into equal-size shards (zero-padded)
    std::vector<std::vector<uint8_t>> shards(data_shards + parity_shards,
                                              std::vector<uint8_t>(shard_size, 0));
    for (uint32_t s = 0; s < data_shards; ++s) {
        const uint32_t start = s * shard_size;
        const uint32_t end   = std::min<uint32_t>(start + shard_size,
                                                   static_cast<uint32_t>(data.size()));
        if (start < static_cast<uint32_t>(data.size()))
            std::copy(data.begin() + start, data.begin() + end, shards[s].begin());
    }

    // Local parity shards: XOR each group
    const uint32_t local_start = data_shards;  // local parities begin here
    for (uint32_t g = 0; g < n_local; ++g) {
        const uint32_t grp_begin = g * kDefaultLocalGroupSize;
        const uint32_t grp_end   = std::min(grp_begin + kDefaultLocalGroupSize, data_shards);
        shards[local_start + g].assign(shard_size, 0);
        for (uint32_t s = grp_begin; s < grp_end; ++s)
            for (uint32_t b = 0; b < shard_size; ++b)
                shards[local_start + g][b] ^= shards[s][b];
    }

    // Global parity shards: Vandermonde RS over all data shards
    if (n_global > 0) {
        auto gp_matrix = lrc_buildVandermonde(n_global, data_shards);
        const uint32_t global_start = data_shards + n_local;
        for (uint32_t p = 0; p < n_global; ++p) {
            shards[global_start + p].assign(shard_size, 0);
            for (uint32_t b = 0; b < shard_size; ++b) {
                for (uint32_t s = 0; s < data_shards; ++s)
                    shards[global_start + p][b] ^=
                        lrc_gf_mul(gp_matrix[p][s], shards[s][b]);
            }
        }
    }

    return shards;
}

// ── decode ───────────────────────────────────────────────────────────────────

std::vector<uint8_t> LocallyRepairableCoder::decode(
    const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
    const std::vector<uint32_t>& missing_indices,
    uint32_t data_shards,
    uint32_t parity_shards)
{
    if (missing_indices.empty()) {
        // All data shards present — just concatenate
        std::vector<uint8_t> result = {};

        for (uint32_t s = 0; s < data_shards; ++s) {
            // W2-S06: Iterator safety — use at() for bounds checking instead of find()+access
            try {
                const auto& chunk = available_chunks.at(s);
                result.insert(result.end(), chunk.begin(), chunk.end());
            } catch (const std::out_of_range&) {
                throw std::runtime_error("LRC decode: missing shard with no missing_indices entry");
            }
        }
        return result;
    }

    const uint32_t n_total   = data_shards + parity_shards;
    const uint32_t n_local   = localGroupCount(data_shards, parity_shards);
    const uint32_t shard_size = static_cast<uint32_t>(
        available_chunks.begin()-> static_cast<int>(second.size()));

    // Build full shard array (fill known shards; zeros for missing)
    std::vector<std::vector<uint8_t>> shards(n_total,
                                              std::vector<uint8_t>(shard_size, 0));
    for (const auto& [idx, data] : available_chunks)
        if (idx < n_total) {
          shards[idx] = data;
        }

    // Attempt local group repair for each missing data shard
    std::vector<bool> recovered(n_total, false);
    for (uint32_t mi : missing_indices)
        recovered[mi] = false;
    for (const auto& [idx, _] : available_chunks)
        recovered[idx] = true;

    // Try local repair: for each missing data shard, check if only it is missing
    // from its local group (data + local-parity shard available)
    bool any_local = true;
    while (any_local) {
        any_local = false;
        for (uint32_t s = 0; s < data_shards; ++s) {
            if (recovered[s]) {
              continue;
            }
            const uint32_t g          = s / kDefaultLocalGroupSize;
            const uint32_t grp_begin  = g * kDefaultLocalGroupSize;
            const uint32_t grp_end    = std::min(grp_begin + kDefaultLocalGroupSize, data_shards);
            const uint32_t local_par  = data_shards + g;

            // Count missing in group (data + local parity)
            int missing_in_group = 0;
            for (uint32_t m = grp_begin; m < grp_end; ++m) {
              if (!recovered[m]) ++missing_in_group;
            }
            if (!recovered[local_par]) {
              ++missing_in_group;
            }

            if (missing_in_group == 1) {
                // Recover via XOR of group
                shards[s].assign(shard_size, 0);
                for (uint32_t m = grp_begin; m < grp_end; ++m)
                    if (m != s)
                        for (uint32_t b = 0; b < shard_size; ++b)
                            shards[s][b] ^= shards[m][b];
                if (recovered[local_par])
                    for (uint32_t b = 0; b < shard_size; ++b)
                        shards[s][b] ^= shards[local_par][b];
                recovered[s] = true;
                any_local = true;
            }
        }
    }

    // If still missing, fall back to global RS recovery
    std::vector<uint32_t> still_missing = {};

    for (uint32_t s = 0; s < data_shards; ++s)
        if (!recovered[s]) {
          still_missing.push_back(s);
        }

    if (!still_missing.empty()) {
        const uint32_t n_global      = parity_shards - n_local;
        const uint32_t global_start  = data_shards + n_local;

        // Collect available rows (data + global parity)
        std::vector<uint32_t> avail_rows = {};

        for (uint32_t s = 0; s < data_shards; ++s)
            if (recovered[s]) {
              avail_rows.push_back(s);
            }
        for (uint32_t p = 0; p < n_global && static_cast<int>(avail_rows.size()) < data_shards; ++p)
            if (recovered[global_start + p]) {
              avail_rows.push_back(data_shards + p);
            }

        if (static_cast<int>(avail_rows.size()) < data_shards)
            throw std::runtime_error("LRC decode: insufficient shards for recovery");

        // Build encode matrix: identity (data) + Vandermonde (global)
        auto vand = lrc_buildVandermonde(n_global, data_shards);
        std::vector<std::vector<uint8_t>> full_mat(data_shards + n_global,
                                                    std::vector<uint8_t>(data_shards, 0));
        for (uint32_t s = 0; s < data_shards; ++s) {
          full_mat[s][s] = 1;
        }
        for (uint32_t p = 0; p < n_global; ++p) {
          full_mat[data_shards + p] = vand[p];
        }

        // Select decode matrix rows
        std::vector<std::vector<uint8_t>> dec_mat(data_shards,
                                                   std::vector<uint8_t>(data_shards));
        std::vector<std::vector<uint8_t>> rhs(data_shards,
                                               std::vector<uint8_t>(shard_size, 0));
        for (uint32_t i = 0; i < data_shards; ++i) {
            const uint32_t row_idx = avail_rows[i];
            dec_mat[i] = row_idx < data_shards
                       ? full_mat[row_idx]
                       : full_mat[data_shards + (row_idx - data_shards)];
            rhs[i]     = row_idx < data_shards
                       ? shards[row_idx]
                       : shards[global_start + (row_idx - data_shards)];
        }

        if (!lrc_invertMatrix(dec_mat))
            throw std::runtime_error("LRC decode: could not invert recovery matrix");

        for (uint32_t s = 0; s < data_shards; ++s) {
            shards[s].assign(shard_size, 0);
            for (uint32_t b = 0; b < shard_size; ++b)
                for (uint32_t j = 0; j < data_shards; ++j)
                    shards[s][b] ^= lrc_gf_mul(dec_mat[s][j], rhs[j][b]);
        }
    }

    // Concatenate recovered data shards
    std::vector<uint8_t> result;
    result.reserve(static_cast<std::size_t>(data_shards) * shard_size);
    for (uint32_t s = 0; s < data_shards; ++s)
        result.insert(result.end(), shards[s].begin(), shards[s].end());
    return result;
}

// ═══════════════════════════════════════════════════════════
// HammingCoder Implementation
// ═══════════════════════════════════════════════════════════

// Parity shard p (0-indexed) covers data shard j (0-indexed) when bit p is
// set in the 1-based position (j+1):  ((j + 1) >> p) & 1 == 1.
//
// This mirrors the classical Hamming parity-bit assignment but operates at
// shard granularity with pure XOR — no Galois-Field arithmetic is required.

// Returns true when parity shard `p` covers data shard `j` (both 0-indexed).
static inline bool hammingCovers(uint32_t j, uint32_t p) noexcept {
    return (((j + 1u) >> p) & 1u) != 0u;
}

std::vector<std::vector<uint8_t>> HammingCoder::encode(
    const std::vector<uint8_t>& data,
    uint32_t data_shards,
    uint32_t parity_shards)
{
    if (data_shards == 0 || parity_shards == 0)
        throw std::invalid_argument("HammingCoder::encode: shard counts must be > 0");
    if (data.empty())
        throw std::invalid_argument("HammingCoder::encode: data must not be empty");

    const uint32_t shard_size = static_cast<uint32_t>(
        (static_cast<int>(data.size()) + data_shards - 1) / data_shards);

    // Initialise all shards to zero (data shards will be filled below)
    const uint32_t total_shards = data_shards + parity_shards;
    std::vector<std::vector<uint8_t>> shards(total_shards,
                                              std::vector<uint8_t>(shard_size, 0));

    // Fill data shards (zero-padded if data is not a multiple of shard_size)
    for (uint32_t s = 0; s < data_shards; ++s) {
        const uint32_t start = s * shard_size;
        const uint32_t end = std::min(start + shard_size,
                                      static_cast<uint32_t>(data.size()));
        if (start < static_cast<uint32_t>(data.size()))
            std::copy(data.begin() + start, data.begin() + end, shards[s].begin());
    }

    // Compute parity shards via XOR
    for (uint32_t p = 0; p < parity_shards; ++p) {
        std::vector<uint8_t>& parity = shards[data_shards + p];
        for (uint32_t j = 0; j < data_shards; ++j) {
            if (hammingCovers(j, p)) {
                for (uint32_t b = 0; b < shard_size; ++b)
                    parity[b] ^= shards[j][b];
            }
        }
    }

    return shards;
}

std::vector<uint8_t> HammingCoder::decode(
    const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
    const std::vector<uint32_t>& missing_indices,
    uint32_t data_shards,
    uint32_t parity_shards)
{
    if (available_chunks.empty())
        throw std::runtime_error("HammingCoder::decode: no chunks available");

    const uint32_t total_shards = data_shards + parity_shards;
    const uint32_t shard_size =
        static_cast<uint32_t>(available_chunks.begin()-> static_cast<int>(second.size()));

    // Fast path: all data shards present
    if (missing_indices.empty()) {
        std::vector<uint8_t> result;
        result.reserve(static_cast<size_t>(data_shards) * shard_size);
        for (uint32_t s = 0; s < data_shards; ++s) {
            // W2-S06: Iterator safety — use at() for bounds checking instead of find()+access
            try {
                const auto& chunk = available_chunks.at(s);
                result.insert(result.end(), chunk.begin(), chunk.end());
            } catch (const std::out_of_range&) {
                throw std::runtime_error(
                    "HammingCoder::decode: data shard " + std::to_string(s) +
                    " missing but not listed in missing_indices");
            }
        }
        return result;
    }

    // Build working shard array (zeros for missing shards)
    std::vector<std::vector<uint8_t>> shards(total_shards,
                                              std::vector<uint8_t>(shard_size, 0));
    std::vector<bool> present(total_shards, false);
    for (const auto& [idx, chunk] : available_chunks) {
        if (idx < total_shards) {
            shards[idx] = chunk;
            present[idx] = true;
        }
    }

    // Iterative repair: in each pass try to recover a missing shard using a
    // parity shard that covers exactly one absent shard (the target itself).
    // The loop terminates when a full pass completes without recovering any
    // new shard; at that point the remaining missing shards cannot be repaired
    // with the available parity coverage.
    bool progress = true;
    while (progress) {
        progress = false;

        for (uint32_t target = 0; target < total_shards; ++target) {
            if (present[target]) {
              continue;
            }

            if (target >= data_shards) {
                // Missing parity shard: recompute directly from data shards
                const uint32_t p = target - data_shards;
                bool can_recompute = true;
                for (uint32_t j = 0; j < data_shards; ++j) {
                    if (!present[j] && hammingCovers(j, p)) {
                        can_recompute = false;
                        break;
                    }
                }
                if (can_recompute) {
                    shards[target].assign(shard_size, 0);
                    for (uint32_t j = 0; j < data_shards; ++j) {
                        if (hammingCovers(j, p)) {
                            for (uint32_t b = 0; b < shard_size; ++b)
                                shards[target][b] ^= shards[j][b];
                        }
                    }
                    present[target] = true;
                    progress = true;
                }
            } else {
                // Missing data shard j = target.
                // Look for a parity shard p that covers it and has all its
                // other covered shards already recovered.
                for (uint32_t p = 0; p < parity_shards; ++p) {
                    if (!present[data_shards + p]) continue;       // parity absent
                    if (!hammingCovers(target, p))  continue;      // parity doesn't cover target

                    // Check that every other data shard covered by p is present
                    bool all_others_present = true;
                    for (uint32_t j = 0; j < data_shards; ++j) {
                        if (j == target) {
                          continue;
                        }
                        if (hammingCovers(j, p) && !present[j]) {
                            all_others_present = false;
                            break;
                        }
                    }
                    if (!all_others_present) {
                      continue;
                    }

                    // Recover target = parity[p] XOR (XOR of other covered data shards)
                    shards[target] = shards[data_shards + p];
                    for (uint32_t j = 0; j < data_shards; ++j) {
                        if (j == target) {
                          continue;
                        }
                        if (hammingCovers(j, p)) {
                            for (uint32_t b = 0; b < shard_size; ++b)
                                shards[target][b] ^= shards[j][b];
                        }
                    }
                    present[target] = true;
                    progress = true;
                    break;
                }
            }
        }
    }

    // Verify all data shards are recovered
    for (uint32_t s = 0; s < data_shards; ++s) {
        if (!present[s])
            throw std::runtime_error(
                "HammingCoder::decode: cannot recover data shard " +
                std::to_string(s) + " — too many simultaneous failures");
    }

    // Concatenate and return
    std::vector<uint8_t> result;
    result.reserve(static_cast<size_t>(data_shards) * shard_size);
    for (uint32_t s = 0; s < data_shards; ++s)
        result.insert(result.end(), shards[s].begin(), shards[s].end());
    return result;
}

// ═══════════════════════════════════════════════════════════
// ErasureCoder Factory
// ═══════════════════════════════════════════════════════════

std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm algorithm) {
    switch (algorithm) {
        case ErasureCodingAlgorithm::REED_SOLOMON:
            return std::make_unique<ReedSolomonCoder>();
        case ErasureCodingAlgorithm::CAUCHY:
            return std::make_unique<CauchyReedSolomonCoder>();
        case ErasureCodingAlgorithm::LRC:
            return std::make_unique<LocallyRepairableCoder>();
        case ErasureCodingAlgorithm::HAMMING:
            return std::make_unique<HammingCoder>();
        default:
            return nullptr;
    }
}

// ═══════════════════════════════════════════════════════════
// RedundancyStrategy Implementation
// ═══════════════════════════════════════════════════════════

/**
 * @brief Construct redundancy strategy and initialize mode-specific coder state.
 * @param config Validated redundancy configuration.
 * @throws std::invalid_argument if configuration invariants are violated.
 * @throws std::runtime_error if erasure-coder creation fails for parity modes.
 */
RedundancyStrategy::RedundancyStrategy(const RedundancyConfig& config)
    : config_(config) {
    
    if (!config_.validate()) {
        throw std::invalid_argument("Invalid redundancy configuration");
    }
    
    if (config_.mode == RedundancyMode::PARITY || config_.mode == RedundancyMode::RAID6) {
        erasure_coder_ = ErasureCoder::create(config_.erasure_coding.algorithm);
        if (!erasure_coder_) {
            throw std::runtime_error("Failed to create erasure coder");
        }
    }
    
    // Initialize TrueTime for globally consistent timestamps
    TrueTime::Config tt_config;
    tt_config.base_uncertainty_us = 1000;  // 1ms base uncertainty
    tt_config.max_drift_us = 100000;     // 100ms max drift
    tt_config.sync_interval_s = 30;     // Sync every 30 seconds
    truetime_ = std::make_unique<TrueTime>(tt_config);
    truetime_->startSyncThread();
    
    spdlog::info("RedundancyStrategy initialized: mode={}, replication_factor={}, storage_efficiency={:.2f}",
                 static_cast<int>(config_.mode),
                 config_.replication_factor,
                 config_.getStorageEfficiency());
}

/** @brief Destroy strategy and release coder/manager resources. */
RedundancyStrategy::~RedundancyStrategy() {
    if (truetime_) {
        truetime_->stopSyncThread();
    }
}

/** @brief Attach optional Raft manager used for leader-enforced writes. */
void RedundancyStrategy::setRaftShardManager(std::shared_ptr<themisdb::sharding::RaftShardManager> raft_manager) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    raft_manager_ = raft_manager;
    spdlog::info("RaftShardManager set for RedundancyStrategy");
}

/** @brief Update shard-latency EWMA used by ReadPreference::NEAREST routing. */
void RedundancyStrategy::recordShardLatency(const std::string& shard_id, double latency_ms) {
    std::lock_guard<std::mutex> lock(latency_mutex_);
    auto it = shard_latency_ewma_ms_.find(shard_id);
    if (it == shard_latency_ewma_ms_.end()) {
        shard_latency_ewma_ms_[shard_id] = latency_ms;
    } else {
        // Exponential moving average: new_ewma = α * sample + (1 - α) * old_ewma
        it->second = kLatencyEwmaAlpha * latency_ms + (1.0 - kLatencyEwmaAlpha) * it->second;
    }
}

/**
 * @brief Execute write path for configured redundancy mode.
 * @return WriteResult with success flag, replica/chunk fanout and measured latency.
 */
WriteResult RedundancyStrategy::write(
    const std::string& document_id,
    const std::vector<uint8_t>& data,
    const std::string& collection [[maybe_unused]],
    ConsistentHashRing& ring,
    ShardTopology& topology,
    WriteHandler handler
) {
    auto start = std::chrono::steady_clock::now();
    
    // W2-S02: Input validation guards — fail-closed on invalid document_id or data
    if (document_id.empty()) {
        spdlog::error("RedundancyStrategy::write: document_id is empty, rejecting write");
        return WriteResult::failed(document_id, "document_id is empty");
    }
    
    if (data.empty()) {
        spdlog::error("RedundancyStrategy::write: data is empty, rejecting write");
        return WriteResult::failed(document_id, "data is empty");
    }
    
    stats_writes_++;
    stats_bytes_written_ += data.size();
    
    WriteResult result;
    
    try {
        // W2-S06: Timeout enforcement — check deadline before operation
        auto deadline = start + config_.replication_timeout;
        
        switch (config_.mode) {
            case RedundancyMode::NONE:
                // Just write to primary shard
                result = writeMirror(document_id, data, ring, topology, handler);
                break;
            case RedundancyMode::MIRROR:
                result = writeMirror(document_id, data, ring, topology, handler);
                break;
            case RedundancyMode::GEO_MIRROR:
                result = writeGeoMirror(document_id, data, ring, topology, handler);
                break;
            case RedundancyMode::STRIPE:
                result = writeStripe(document_id, data, ring, topology, handler);
                break;
            case RedundancyMode::STRIPE_MIRROR:
                result = writeStripeMirror(document_id, data, ring, topology, handler);
                break;
            case RedundancyMode::PARITY:
            [[fallthrough]];\n            case RedundancyMode::RAID6:
                result = writeParity(document_id, data, ring, topology, handler);
                break;
            default:
                result = WriteResult::failed(document_id, "Unsupported redundancy mode");
        }
        
        // Check if operation exceeded timeout
        auto now = std::chrono::steady_clock::now();
        if (now > deadline && result.success) {
            spdlog::warn("write: operation for document {} completed but exceeded timeout "
                        "(deadline exceeded by {}ms)",
                        document_id,
                        std::chrono::duration_cast<std::chrono::milliseconds>(now - deadline).count());
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Write failed for document {}: {}", document_id, e.what());
        result = WriteResult::failed(document_id, e.what());
    }
    
    auto end = std::chrono::steady_clock::now();
    result.latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return result;
}

/**
 * @brief Execute read path for configured redundancy mode and read preference.
 * @return ReadResult including data (if found), source shards, and error context.
 */
ReadResult RedundancyStrategy::read(
    const std::string& document_id,
    const std::string& collection [[maybe_unused]],
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler handler
) {
    auto start = std::chrono::steady_clock::now();
    
    // W2-S02: Input validation guards — fail-closed on invalid document_id
    ReadResult result;
    result.document_id = document_id;
    // Use TrueTime for globally consistent version tokens
    if (truetime_) {
        auto tt_now = truetime_->now();
        result.version_token = tt_now.midpoint().count();
    } else {
        result.version_token = makeVersionToken();
    }
    
    if (document_id.empty()) {
        spdlog::error("RedundancyStrategy::read: document_id is empty, rejecting read");
        result.success = false;
        result.error_message = "document_id is empty";
        return result;
    }
    
    stats_reads_++;

    RedundancyMode mode;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        mode = config_.mode;
    }
    
    try {
        switch (mode) {
            case RedundancyMode::NONE:
            [[fallthrough]];\n            case RedundancyMode::MIRROR:
                result = readMirror(document_id, ring, topology, handler);
                break;
            case RedundancyMode::GEO_MIRROR:
                result = readGeoMirror(document_id, ring, topology, handler);
                break;
            case RedundancyMode::STRIPE:
            [[fallthrough]];\n            case RedundancyMode::STRIPE_MIRROR:
                result = readStripe(document_id, ring, topology, handler);
                break;
            case RedundancyMode::PARITY:
            [[fallthrough]];\n            case RedundancyMode::RAID6:
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
    ShardTopology& topology [[maybe_unused]],
    WriteHandler handler
) {
    // Get primary shard
    auto primary_shard = ring.getNode(document_id);
    if (!primary_shard) {
        return WriteResult::failed(document_id, "No primary shard available");
    }
    
    // Get replica shards
    std::vector<std::string> target_shards;
    target_shards.reserve(static_cast<size_t>(std::max<uint32_t>(1, config_.replication_factor)));
    target_shards.push_back(*primary_shard);
    
    auto replicas = ring.getReplicaNodes(document_id, config_.replication_factor - 1);
    // CONSENSUS-AWARE: Validate replicas before adding to target list
    if (!replicas.empty()) {
        for (const auto& replica : replicas) {
            // W2-S06: Consensus validation — fail-closed if replica is invalid
            if (replica.empty()) {
                spdlog::error("writeMirror: received empty replica ID, rejecting write for document {}", 
                             document_id);
                return WriteResult::failed(document_id, "Invalid replica shard identifier");
            }
        }
    }
    
    target_shards.insert(
        target_shards.end(),
        std::make_move_iterator(replicas.begin()),
        std::make_move_iterator(replicas.end()));

    // W2-S06: Consensus validation — determine required acknowledgments based on write concern
    const uint32_t configured_targets = std::max<uint32_t>(1, config_.replication_factor);
    uint32_t required_acks = 1;
    switch (config_.write_concern) {
        case WriteConcern::ONE:
            required_acks = 1;
            break;
        case WriteConcern::MAJORITY:
            required_acks = (configured_targets / 2) + 1;
            break;
        case WriteConcern::ALL:
            required_acks = configured_targets;
            break;
        case WriteConcern::QUORUM:
            // W2-S02: Fail-closed on invalid write_quorum
            if (config_.write_quorum == 0 && static_cast<int>(target_shards.size()) > 1) {
                spdlog::error("writeMirror: write_quorum is 0 with {} target shards, rejecting write", 
                             target_shards.size());
                WriteResult result;
                result.success = false;
                result.document_id = document_id;
                result.error_message = "write_quorum is 0 with active replicas";
                return result;
            }
            required_acks = config_.write_quorum;
            break;
    }

    if (static_cast<int>(target_shards.size()) < required_acks) {
        WriteResult result;
        result.success = false;
        result.document_id = document_id;
        result.error_message = "Insufficient replica targets to satisfy write concern";
        return result;
    }

    // Fast path: single target shard should be handled synchronously to avoid
    // unnecessary async machinery and potential blocking edge cases.
    if (static_cast<int>(target_shards.size()) == 1) {
        const auto& shard_id = target_shards.front();
        bool ok = false;
        try {
            ok = handler(shard_id, document_id, data);
        } catch (const std::exception& e) {
            spdlog::warn("Write to shard {} failed: {}", shard_id, e.what());
            ok = false;
        }

        if (ok) {
            return WriteResult::successful(document_id, {shard_id}, std::chrono::milliseconds(0));
        }

        WriteResult result;
        result.success = false;
        result.document_id = document_id;
        result.failed_shards = {shard_id};
        result.acknowledgements = 0;
        result.error_message = "Write concern not met";
        return result;
    }
    
    // W2-S06: Distributed write with replication consensus — send to all replicas in parallel
    std::vector<std::future<bool>> futures;
    std::vector<std::string> written_shards;
    std::vector<std::string> failed_shards = {};

    futures.reserve(target_shards.size());
    written_shards.reserve(target_shards.size());
    failed_shards.reserve(target_shards.size());

    for (const auto& shard_id : target_shards) {
        futures.push_back(std::async(std::launch::async, [&, shard_id]() {
            return handler(shard_id, document_id, data);
        }));
    }
    
    // Wait for writes based on write concern and enforce deadline
    uint32_t successful = 0;
    const auto wait_timeout = config_.replication_timeout;
    for (size_t i = 0; i <static_cast<int>(futures.size()); ++i) {
        try {
            auto status = futures[i].wait_for(wait_timeout);
            if (status != std::future_status::ready) {
                spdlog::warn("Write to shard {} timed out", target_shards[i]);
                failed_shards.push_back(target_shards[i]);
                continue;
            }

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
            success = successful >= required_acks;
            break;
        case WriteConcern::ALL:
            success = successful >= required_acks;
            break;
        case WriteConcern::QUORUM:
            success = successful >= required_acks;
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

// ═══════════════════════════════════════════════════════════
// Raft Consensus Integration Helper Methods
// ═══════════════════════════════════════════════════════════

bool RedundancyStrategy::shouldUseRaftConsensus(const std::string& shard_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    // Check if Raft is enabled in configuration
    if (!config_.enable_raft_consensus) {
        return false;
    }
    
    // Check if RaftShardManager is set
    if (!raft_manager_) {
        return false;
    }
    
    // Check if shard has Raft instance
    auto raft_info = raft_manager_->getShardRaftInfo(shard_id);
    return raft_info.has_value();
}

bool RedundancyStrategy::proposeRaftWrite(const std::string& shard_id,
                                         const std::string& document_id,
                                         const std::vector<uint8_t>& data) {
    // W2-S02: Input validation guards — fail-closed on invalid inputs
    if (shard_id.empty()) {
        spdlog::error("proposeRaftWrite: shard_id is empty, rejecting write");
        return false;
    }
    
    if (document_id.empty()) {
        spdlog::error("proposeRaftWrite: document_id is empty, rejecting write");
        return false;
    }
    
    if (data.empty()) {
        spdlog::error("proposeRaftWrite: data is empty, rejecting write");
        return false;
    }
    
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (!raft_manager_) {
        spdlog::error("RaftShardManager not set, cannot propose Raft write");
        return false;
    }
    
    // Check if this node is the leader for the shard
    if (!raft_manager_->isShardLeader(shard_id)) {
        std::string leader = raft_manager_->getShardLeader(shard_id);
        if (!leader.empty()) {
            spdlog::warn("Not the leader for shard {}, leader is: {}", shard_id, leader);
        } else {
            spdlog::warn("Not the leader for shard {}, no known leader", shard_id);
        }
        return false;
    }
    
    // Serialize write command
    // Format: "WRITE|<doc_id>|<data_size>|<data>"
    // W2-S06: Command injection hardening — validate document_id for delimiter use
    // and use explicit field size to prevent delimiter ambiguity
    if (document_id.find('|') != std::string::npos) {
        spdlog::error("proposeRaftWrite: document_id '{}' contains reserved delimiter '|', "
                      "aborting to prevent command injection", document_id);
        return false;
    }
    
    // Build command with explicit field lengths to prevent injection attacks
    std::string command = {};
    command.reserve(20 + static_cast<int>(document_id.size()) + data.size());
    
    // Field 0: command type
    command.append("WRITE|");
    
    // Field 1: document_id (validated to not contain '|')
    command.append(std::to_string(document_id.size()));
    command.append(":");
    command.append(document_id);
    command.append("|");
    
    // Field 2: data_size
    command.append(std::to_string(data.size()));
    command.append("|");
    
    // Field 3: raw data
    command.append(reinterpret_cast<const char*>(data.data()),static_cast<int>(data.size()));
    
    // Propose write through Raft
    auto future = raft_manager_->proposeWrite(shard_id, command);
    
    try {
        // Wait for commit with timeout
        auto status = future.wait_for(config_.replication_timeout);
        if (status == std::future_status::timeout) {
            spdlog::error("Raft write proposal timeout for shard: {}", shard_id);
            return false;
        }
        
        bool committed = future.get();
        if (!committed) {
            spdlog::error("Raft write proposal failed for shard: {}", shard_id);
            return false;
        }
        
        spdlog::debug("Raft write committed for document {} on shard {}", document_id, shard_id);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception during Raft write proposal: {}", e.what());
        return false;
    }
}

WriteResult RedundancyStrategy::writeStripe(
    const std::string& document_id,
    const std::vector<uint8_t>& data,
    ConsistentHashRing& ring,
    ShardTopology& topology [[maybe_unused]],
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
    
    auto replicas = ring.getReplicaNodes(document_id, static_cast<int>(chunks.size()) - 1);
    target_shards.insert(target_shards.end(), replicas.begin(), replicas.end());
    
    // W2-S06: Consensus validation — determine required acknowledgments based on write concern
    const uint32_t configured_targets = std::max<uint32_t>(1,static_cast<int>(chunks.size()));
    uint32_t required_acks = 1;
    switch (config_.write_concern) {
        case WriteConcern::ONE:
            required_acks = 1;
            break;
        case WriteConcern::MAJORITY:
            required_acks = (configured_targets / 2) + 1;
            break;
        case WriteConcern::ALL:
            required_acks = configured_targets;
            break;
        case WriteConcern::QUORUM:
            if (config_.write_quorum == 0 && static_cast<int>(target_shards.size()) > 1) {
                spdlog::error("writeStripe: write_quorum is 0 with {} target shards, rejecting write", 
                             target_shards.size());
                WriteResult result;
                result.success = false;
                result.document_id = document_id;
                result.error_message = "write_quorum is 0 with active replicas";
                return result;
            }
            required_acks = config_.write_quorum;
            break;
    }
    
    if (static_cast<int>(target_shards.size()) < required_acks) {
        WriteResult result;
        result.success = false;
        result.document_id = document_id;
        result.error_message = "Insufficient replica targets to satisfy write concern";
        return result;
    }
    
    // Write chunks to different shards with timeout
    std::vector<std::future<bool>> futures;
    std::vector<std::string> written_shards;
    std::vector<std::string> failed_shards = {};

    futures.reserve(target_shards.size());
    written_shards.reserve(target_shards.size());
    failed_shards.reserve(target_shards.size());
    
    for (size_t i = 0; i <static_cast<int>(chunks.size())  && static_cast<size_t>(i) <static_cast<int>(target_shards.size()); ++i) {
        const auto& chunk = chunks[i];
        const auto& shard_id = target_shards[i];
        
        futures.push_back(std::async(std::launch::async, [&, shard_id, chunk]() {
            return handler(shard_id, document_id + ":chunk:" + std::to_string(i), chunk);
        }));
    }
    
    // Wait for writes based on write concern and enforce deadline
    uint32_t successful = 0;
    const auto wait_timeout = config_.replication_timeout;
    for (size_t i = 0; i <static_cast<int>(futures.size()); ++i) {
        try {
            auto status = futures[i].wait_for(wait_timeout);
            if (status != std::future_status::ready) {
                spdlog::warn("Stripe write to shard {} timed out", target_shards[i]);
                failed_shards.push_back(target_shards[i]);
                continue;
            }
            if (futures[i].get()) {
                written_shards.push_back(target_shards[i]);
                successful++;
            } else {
                failed_shards.push_back(target_shards[i]);
            }
        } catch (const std::exception& e) {
            spdlog::warn("Stripe write to shard {} failed: {}", target_shards[i], e.what());
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
        [[fallthrough]];\n        case WriteConcern::ALL:
        [[fallthrough]];\n        case WriteConcern::QUORUM:
            success = successful >= required_acks;
            break;
    }
    
    if (success) {
        return WriteResult::successful(document_id, written_shards, std::chrono::milliseconds(0));
    } else {
        WriteResult result;
        result.success = false;
        result.document_id = document_id;
        result.failed_shards = failed_shards;
        result.acknowledgements = successful;
        result.error_message = "Write concern not met";
        return result;
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
    
    for (size_t i = 0; i <static_cast<int>(chunks.size()); ++i) {
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
    ShardTopology& topology [[maybe_unused]],
    WriteHandler handler
) {
    // W2-S06: Erasure coding consensus — snapshot config under lock to prevent races
    // Snapshot erasure coder config and encode under the shared lock to guard
    // against a concurrent configure() resetting erasure_coder_ (data race fix).
    std::vector<std::vector<uint8_t>> chunks;
    uint32_t data_shards = {};
    uint32_t parity_shards = {};
    {
        std::shared_lock<std::shared_mutex> ec_lock(mutex_);
        if (!erasure_coder_) {
            return WriteResult::failed(document_id, "Erasure coder not initialized");
        }
        data_shards = config_.erasure_coding.data_shards;
        parity_shards = config_.erasure_coding.parity_shards;
        chunks = erasure_coder_->encode(data, data_shards, parity_shards);
    }

    // Get target shards
    auto primary_shard = ring.getNode(document_id);
    if (!primary_shard) {
        return WriteResult::failed(document_id, "No primary shard available");
    }
    
    std::vector<std::string> target_shards;
    target_shards.push_back(*primary_shard);
    
    auto replicas = ring.getReplicaNodes(document_id, static_cast<int>(chunks.size()) - 1);
    target_shards.insert(target_shards.end(), replicas.begin(), replicas.end());
    
    // W2-S06: RAID/Erasure consensus — write all chunks (data + parity) with quorum
    // For erasure coding, we need at least k (data_shards) successful writes to guarantee recovery
    std::vector<std::future<bool>> futures;
    std::vector<std::string> written_shards;
    
    for (size_t i = 0; i <static_cast<int>(chunks.size())  && static_cast<size_t>(i) <static_cast<int>(target_shards.size()); ++i) {
        const auto& chunk = chunks[i];
        const auto& shard_id = target_shards[i];
        bool is_parity = i >= data_shards;
        
        std::string chunk_id = document_id + (is_parity ? ":parity:" : ":data:") + std::to_string(i);
        
        futures.push_back(std::async(std::launch::async, [&, shard_id, chunk, chunk_id]() {
            return handler(shard_id, chunk_id, chunk);
        }));
    }
    
    // W2-S06: Consensus validation — wait for sufficient acknowledgments
    // Erasure coding consensus: need at least data_shards successful writes
    uint32_t successful = 0;
    for (size_t i = 0; i <static_cast<int>(futures.size()); ++i) {
        try {
            if (futures[i].get()) {
                written_shards.push_back(target_shards[i]);
                successful++;
            }
        } catch (const std::exception& e) {
            spdlog::warn("Parity write to shard {} failed: {}", target_shards[i], e.what());
        }
    }
    
    // W2-S06: Consensus check — ensure we have enough chunks for recovery
    if (successful >= data_shards) {
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
    bool enable_geo_failover = false;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        enable_geo_failover = config_.geo_replication.enable_geo_failover;
    }

    if (enable_geo_failover) {
        evaluateGeoFailover(topology);
    }

    GeoReplicationConfig geo;
    uint32_t replication_factor = 0;
    WriteConcern write_concern;
    uint32_t write_quorum = 0;
    std::chrono::milliseconds replication_timeout;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        geo = config_.geo_replication;
        replication_factor = config_.replication_factor;
        write_concern = config_.write_concern;
        write_quorum = config_.write_quorum;
        replication_timeout = config_.replication_timeout;
    }

    // Collect candidate shards (primary + replicas)
    auto primary_opt = ring.getNode(document_id);
    if (!primary_opt) {
        return WriteResult::failed(document_id, "No primary shard available");
    }

    std::vector<std::string> candidates;
    candidates.push_back(*primary_opt);
    auto replicas = ring.getReplicaNodes(document_id, replication_factor - 1);
    candidates.insert(candidates.end(), replicas.begin(), replicas.end());
    const std::unordered_set<std::string> write_failed_set(
        geo.failed_regions.begin(), geo.failed_regions.end());

    // If region_shards placement is configured, build the ordered write targets
    // by consulting region_write_quorums; fall through to mirror logic if not set.
    std::vector<std::string> target_shards;

    if (!geo.region_shards.empty() || !geo.region_write_quorums.empty()) {
        // Build region->candidates mapping from the ring candidates
        std::map<std::string, std::vector<std::string>> region_candidates;
        for (const auto& shard_id : candidates) {
            auto shard_info = topology.getShard(shard_id);
            std::string region = shard_info ? shard_info->region : "";
            region_candidates[region].push_back(shard_id);
        }

        // Prioritise local-region shards first, then remote
        if (!geo.local_region.empty() && !write_failed_set.count(geo.local_region)) {
            auto it = region_candidates.find(geo.local_region);
            if (it != region_candidates.end()) {
                for (const auto& s : it->second) {
                  target_shards.push_back(s);
                }
            }
        }
        for (const auto& [region, shards] : region_candidates) {
            if (region == geo.local_region) {
              continue;
            }
            if (!write_failed_set.count(region)) {
                for (const auto& s : shards) {
                  target_shards.push_back(s);
                }
            }
        }
    } else {
        target_shards = candidates;
    }

    if (target_shards.empty()) {
        return WriteResult::failed(document_id, "No healthy shards available after geo-failover");
    }

    const uint32_t configured_targets = std::max<uint32_t>(1, replication_factor);
    uint32_t required_acks = 1;
    switch (write_concern) {
        case WriteConcern::ONE:
            required_acks = 1;
            break;
        case WriteConcern::MAJORITY:
            required_acks = (configured_targets / 2) + 1;
            break;
        case WriteConcern::ALL:
            required_acks = configured_targets;
            break;
        case WriteConcern::QUORUM:
            required_acks = write_quorum;
            break;
    }

    if (static_cast<int>(target_shards.size()) < required_acks) {
        WriteResult r;
        r.success = false;
        r.document_id = document_id;
        r.error_message = "Insufficient replica targets to satisfy write concern";
        return r;
    }

    if (!geo.region_write_quorums.empty()) {
        std::map<std::string, uint32_t> region_targets = {};

        for (const auto& shard_id : target_shards) {
            auto info = topology.getShard(shard_id);
            const std::string region = info ? info->region : "";
            region_targets[region]++;
        }
        for (const auto& [region, required] : geo.region_write_quorums) {
            if (write_failed_set.count(region)) {
              continue;
            }
            if (region_targets[region] < required) {
                WriteResult r;
                r.success = false;
                r.document_id = document_id;
                r.error_message = "Insufficient replica targets to satisfy geo write quorum";
                return r;
            }
        }
    }

    // Perform writes in parallel
    std::vector<std::future<bool>> futures;
    futures.reserve(target_shards.size());
    for (const auto& shard_id : target_shards) {
        // Capture shard_id by value to avoid dangling reference to the loop variable
        futures.push_back(std::async(std::launch::async,
            [&handler, shard_id, &document_id, &data]() {
                return handler(shard_id, document_id, data);
            }));
    }

    std::vector<std::string> written_shards, failed_shards;
    uint32_t successful = 0;
    for (size_t i = 0; i <static_cast<int>(futures.size()); ++i) {
        try {
            auto status = futures[i].wait_for(replication_timeout);
            if (status != std::future_status::ready) {
                spdlog::warn("GEO_MIRROR: write to shard {} timed out", target_shards[i]);
                failed_shards.push_back(target_shards[i]);
                continue;
            }
            if (futures[i].get()) {
                written_shards.push_back(target_shards[i]);
                successful++;
            } else {
                failed_shards.push_back(target_shards[i]);
            }
        } catch (const std::exception& e) {
            spdlog::warn("GEO_MIRROR: write to shard {} failed: {}", target_shards[i], e.what());
            failed_shards.push_back(target_shards[i]);
        }
    }

    // Check per-region quorums if configured
    if (!geo.region_write_quorums.empty()) {
        // Build O(1) failed-region lookup set once
        std::unordered_set<std::string> failed_set(geo.failed_regions.begin(),
                                                   geo.failed_regions.end());
        for (const auto& [region, required] : geo.region_write_quorums) {
            if (failed_set.count(region)) continue;  // region is failed-out

            uint32_t region_acks = 0;
            for (const auto& shard_id : written_shards) {
                auto info = topology.getShard(shard_id);
                if (info && info->region == region) {
                  ++region_acks;
                }
            }
            if (region_acks < required) {
                WriteResult r;
                r.success = false;
                r.document_id = document_id;
                r.written_shards = written_shards;
                r.failed_shards = failed_shards;
                r.acknowledgements = successful;
                r.error_message = "Geo-quorum not met for region: " + region;
                return r;
            }
        }
    }

    // Fall back to global write-concern check
    bool success = false;
    switch (write_concern) {
        case WriteConcern::ONE:
            success = successful >= 1;
            break;
        case WriteConcern::MAJORITY:
            success = successful >= required_acks;
            break;
        case WriteConcern::ALL:
            success = successful >= required_acks;
            break;
        case WriteConcern::QUORUM:
            success = successful >= required_acks;
            break;
    }

    if (success) {
        return WriteResult::successful(document_id, written_shards, std::chrono::milliseconds(0));
    }
    WriteResult r;
    r.success = false;
    r.document_id = document_id;
    r.written_shards = written_shards;
    r.failed_shards = failed_shards;
    r.acknowledgements = successful;
    r.error_message = "Write concern not met";
    return r;
}

// ═══════════════════════════════════════════════════════════
// Internal Read Methods
// ═══════════════════════════════════════════════════════════

ReadResult RedundancyStrategy::readGeoMirror(
    const std::string& document_id,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler handler
) {
    bool enable_geo_failover = false;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        enable_geo_failover = config_.geo_replication.enable_geo_failover;
    }

    if (enable_geo_failover) {
        evaluateGeoFailover(topology);
    }

    GeoReplicationConfig geo;
    uint32_t replication_factor = 0;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        geo = config_.geo_replication;
        replication_factor = config_.replication_factor;
    }

    // Collect all replica candidates
    auto primary_opt = ring.getNode(document_id);
    if (!primary_opt) {
        ReadResult r;
        r.success = false;
        r.error_message = "No shard available";
        return r;
    }

    std::vector<std::string> candidates;
    candidates.push_back(*primary_opt);
    auto replicas = ring.getReplicaNodes(document_id, replication_factor - 1);
    candidates.insert(candidates.end(), replicas.begin(), replicas.end());

    // Remove candidates that belong to failed-out regions
    if (!geo.failed_regions.empty()) {
        // Build O(1) lookup set once, then filter candidates in a single pass
        const std::unordered_set<std::string> read_failed_set(
            geo.failed_regions.begin(), geo.failed_regions.end());
        candidates.erase(
            std::remove_if(candidates.begin(), candidates.end(),
                [&]([[maybe_unused]] const std::string& sid) {
                    auto info = topology.getShard(sid);
                    return info && read_failed_set.count(info->region);
                }),
            candidates.end());
    }

    if (candidates.empty()) {
        ReadResult r;
        r.success = false;
        r.error_message = "No healthy shards available after geo-failover";
        return r;
    }

    // -------------------------------------------------------------------
    // Per-region read quorum path: contact enough replicas to satisfy the
    // per-region quorum requirement before returning.
    // -------------------------------------------------------------------
    if (!geo.region_read_quorums.empty()) {
        // Build O(1) lookup set for failed regions (avoids repeated linear scan)
        std::unordered_set<std::string> failed_set = {};

        failed_set.reserve(geo.failed_regions.size());
        failed_set.insert(geo.failed_regions.begin(), geo.failed_regions.end());

        // Ensure quorum requirements are satisfiable with available candidates.
        std::map<std::string, uint32_t> region_candidates = {};

        for (const auto& shard_id : candidates) {
            auto info = topology.getShard(shard_id);
            const std::string region = info ? info->region : "";
            region_candidates[region]++;
        }
        for (const auto& [region, required] : geo.region_read_quorums) {
            if (failed_set.count(region)) {
              continue;
            }
            if (region_candidates[region] < required) {
                ReadResult result;
                result.success = false;
                result.document_id = document_id;
                result.error_message = "Insufficient replica targets to satisfy read quorum";
                return result;
            }
        }

        // Track per-region successes
        std::map<std::string, uint32_t> region_reads;
        ReadResult result;
        result.document_id = document_id;
        // Use TrueTime for globally consistent version tokens
        if (truetime_) {
            auto tt_now = truetime_->now();
            result.version_token = tt_now.midpoint().count();
        } else {
            result.version_token = makeVersionToken();
        }
        result.chunks_read = 1;
        bool all_region_quorums_met = false;

        // Prefer local-region shard first so we can return data quickly
        std::vector<std::string> ordered = candidates;
        if (!geo.local_region.empty()) {
            std::stable_partition(ordered.begin(), ordered.end(),
                [&]([[maybe_unused]] const std::string& sid) {
                    auto info = topology.getShard(sid);
                    return info && info->region == geo.local_region;
                });
        }

        for (const auto& shard_id : ordered) {
            auto data_opt = handler(shard_id, document_id);
            if (data_opt) {
                auto info = topology.getShard(shard_id);
                std::string region = info ? info->region : "";
                region_reads[region]++;

                // Capture the first successful read as the result
                if (!result.success) {
                    result.success = true;
                    result.data = std::string(data_opt->begin(), data_opt->end());
                    result.source_shard = shard_id;
                    result.from_replica = (shard_id != *primary_opt);
                }
            }

            // Check if all region quorums are satisfied
            bool all_met = true;
            for (const auto& [region, required] : geo.region_read_quorums) {
                // Skip failed-out regions (O(1) lookup)
                if (failed_set.count(region)) {
                  continue;
                }

                auto it = region_reads.find(region);
                if (it == region_reads.end() || it->second < required) {
                    all_met = false;
                    break;
                }
            }
            if (all_met && result.success) {
                all_region_quorums_met = true;
                break;
            }
        }

        if (!result.success) {
            result.error_message = "Failed to read from any geo-replica";
            return result;
        }

        if (!all_region_quorums_met) {
            result.success = false;
            result.data.clear();
            result.error_message = "Read quorum not met";
        }
        return result;
    }

    // -------------------------------------------------------------------
    // Standard single-shard read path (no per-region read quorums)
    // -------------------------------------------------------------------

    // Select shard based on read preference
    std::string selected_shard = {};
    const auto pref = geo.read_preference;
    if (pref == ReadPreference::LOCAL_REGION || pref == ReadPreference::FOLLOWER) {
        selected_shard = selectGeoReadShard(candidates, topology, geo.local_region);
    } else {
        selected_shard = selectReadShard(candidates, topology);
    }

    auto data_opt = handler(selected_shard, document_id);

    ReadResult result;
    result.document_id = document_id;
    // Use TrueTime for globally consistent version tokens
    if (truetime_) {
        auto tt_now = truetime_->now();
        result.version_token = tt_now.midpoint().count();
    } else {
        result.version_token = makeVersionToken();
    }
    result.source_shard = selected_shard;
    result.from_replica = (selected_shard != *primary_opt);
    result.chunks_read = 1;

    if (data_opt) {
        result.success = true;
        result.data = std::string(data_opt->begin(), data_opt->end());
    } else {
        // Bounded-staleness / follower-read fallback: try remaining candidates
        result.success = false;
        for (const auto& shard_id : candidates) {
            if (shard_id == selected_shard) {
              continue;
            }
            data_opt = handler(shard_id, document_id);
            if (data_opt) {
                result.success = true;
                result.data = std::string(data_opt->begin(), data_opt->end());
                result.source_shard = shard_id;
                result.from_replica = (shard_id != *primary_opt);
                break;
            }
        }
        if (!result.success) {
            result.error_message = "Failed to read from any geo-replica";
        }
    }

    return result;
}

ReadResult RedundancyStrategy::readMirror(
    const std::string& document_id,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler handler
) {
    uint32_t replication_factor = 0;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        replication_factor = config_.replication_factor;
    }

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
    
    auto replicas = ring.getReplicaNodes(document_id, replication_factor - 1);
    available_shards.insert(available_shards.end(), replicas.begin(), replicas.end());
    
    // Select shard based on read preference
    std::string selected_shard = selectReadShard(available_shards, topology);
    
    // Read from selected shard
    auto data_opt = handler(selected_shard, document_id);
    
    ReadResult result;
    result.document_id = document_id;
    // Use TrueTime for globally consistent version tokens
    if (truetime_) {
        auto tt_now = truetime_->now();
        result.version_token = tt_now.midpoint().count();
    } else {
        result.version_token = makeVersionToken();
    }
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
    ShardTopology& topology [[maybe_unused]],
    ReadHandler handler
) {
    ReadResult result;
    result.document_id = document_id;
    // Use TrueTime for globally consistent version tokens
    if (truetime_) {
        auto tt_now = truetime_->now();
        result.version_token = tt_now.midpoint().count();
    } else {
        result.version_token = makeVersionToken();
    }
    result.success = false;
    
    // Read all chunks
    // For now, assume chunks are numbered sequentially
    std::vector<std::vector<uint8_t>> chunks;
    
    // Try to read chunks until we can't find any more
    for (uint32_t i = 0; ; ++i) {
        std::string chunk_doc_id = document_id + ":chunk:" + std::to_string(i);
        auto shard_opt = ring.getNode(chunk_doc_id);
        
        if (!shard_opt) {
          break;
        }
        
        auto data_opt = handler(*shard_opt, chunk_doc_id);
        if (!data_opt) {
          break;
        }
        
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
    result.chunks_read = static_cast<uint32_t>(chunks.size());
    
    return result;
}

ReadResult RedundancyStrategy::readParity(
    const std::string& document_id,
    ConsistentHashRing& ring,
    [[maybe_unused]] ShardTopology& topology,
    ReadHandler handler
) {
    // W2-S06: Read consensus for erasure coding — snapshot config under lock to prevent races
    // Snapshot erasure-coding config under the shared lock to guard against a
    // concurrent configure() resetting erasure_coder_ (data race fix).
    uint32_t data_shards_snap = {};
    uint32_t parity_shards_snap = {};
    uint32_t total_shards = {};
    {
        std::shared_lock<std::shared_mutex> ec_lock(mutex_);
        if (!erasure_coder_) {
            ReadResult result;
            result.success = false;
            result.error_message = "Erasure coder not initialized";
            return result;
        }
        data_shards_snap  = config_.erasure_coding.data_shards;
        parity_shards_snap = config_.erasure_coding.parity_shards;
        total_shards = config_.erasure_coding.totalShards();
    }

    ReadResult result;
    result.document_id = document_id;
    // Use TrueTime for globally consistent version tokens
    if (truetime_) {
        auto tt_now = truetime_->now();
        result.version_token = tt_now.midpoint().count();
    } else {
        result.version_token = makeVersionToken();
    }
    result.success = false;
    
    // W2-S06: Read consensus — try to read all chunks (data + parity) to enable recovery
    // Try to read all chunks (data + parity)
    std::map<uint32_t, std::vector<uint8_t>> available_chunks;
    std::vector<uint32_t> missing_indices;
    
    for (uint32_t i = 0; i < total_shards; ++i) {
        bool is_parity = i >= data_shards_snap;
        std::string chunk_id = document_id + (is_parity ? ":parity:" : ":data:") + std::to_string(i);
        
        auto shard_opt = ring.getNode(chunk_id);
        if (!shard_opt) {
          continue;
        }
        
        auto data_opt = handler(*shard_opt, chunk_id);
        if (data_opt) {
            available_chunks[i] = *data_opt;
        } else {
            missing_indices.push_back(i);
        }
    }
    
    // W2-S06: Consensus validation — check if we have enough chunks for recovery
    // Check if we can recover (need at least k data shards)
    if (static_cast<int>(available_chunks.size()) < data_shards_snap) {
        result.error_message = "Not enough chunks available for recovery";
        return result;
    }
    
    try {
        // W2-S06: Erasure consensus — re-acquire shared lock around erasure_coder_ use
        // Decode/recover data — re-acquire shared lock around erasure_coder_ use.
        std::shared_lock<std::shared_mutex> ec_lock(mutex_);
        if (!erasure_coder_) {
            result.error_message = "Erasure coder was reconfigured during read";
            return result;
        }
        auto recovered = erasure_coder_->decode(
            available_chunks,
            missing_indices,
            data_shards_snap,
            parity_shards_snap
        );
        ec_lock.unlock();
        
        result.success = true;
        result.data = std::string(recovered.begin(), recovered.end());
        result.chunks_read = static_cast<uint32_t>(available_chunks.size());
        
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
    
    for (size_t offset = 0; offset <static_cast<int>(data.size()); offset += chunk_size) {
        size_t size = std::min(chunk_size, static_cast<int>(data.size()) - offset);
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

// ============================================================================
// Version-aware chunk merging with conflict resolution
// Resolves GAP: undefined_conflict_resolution, unspecified_consistency
// ============================================================================

/**
 * @brief Merge chunks with version consistency checking and conflict resolution
 * 
 * This function addresses GAP categories:
 * - undefined_conflict_resolution: Provides explicit conflict resolution strategy
 * - unspecified_consistency: Uses version tokens for consistency verification
 * - missing_version_tracking: Tracks and validates version tokens
 * 
 * @param versioned_chunks Chunks with version information
 * @param conflict_resolution How to resolve version conflicts
 * @param result_version Output parameter for merged version token
 * @return Merged data
 */
std::vector<uint8_t> RedundancyStrategy::mergeChunksWithConsistency(
    const std::vector<VersionedChunk>& versioned_chunks,
    ConflictResolution conflict_resolution,
    uint64_t& result_version
) {
    if (versioned_chunks.empty()) {
        result_version = 0;
        return {};
    }
    
    // Check if all chunks have the same version (consistent)
    uint64_t first_version = versioned_chunks[0].version_token;
    bool all_consistent = true;
    for (const auto& vc : versioned_chunks) {
        if (vc.version_token != first_version) {
            all_consistent = false;
            break;
        }
    }
    
    if (all_consistent) {
        // All chunks are consistent - simple merge
        result_version = first_version;
        std::vector<uint8_t> merged = {};

        for (const auto& vc : versioned_chunks) {
            merged.insert(merged.end(), vc.data.begin(), vc.data.end());
        }
        return merged;
    }
    
    // ========================================================================
    // CONFLICT DETECTED - Apply resolution strategy
    // ========================================================================
    
    spdlog::warn("RedundancyStrategy: Version conflict detected in mergeChunks. "
                 "Applying conflict resolution strategy: {}", 
                 static_cast<int>(conflict_resolution));
    
    switch (conflict_resolution) {
        case ConflictResolution::LAST_WRITE_WINS: {
            // Find chunk with highest version token
            const VersionedChunk* latest = &versioned_chunks[0];
            for (const auto& vc : versioned_chunks) {
                if (vc.version_token > latest->version_token) {
                    latest = &vc;
                }
            }
            result_version = latest->version_token;
            spdlog::debug("RedundancyStrategy: LAST_WRITE_WINS selected version {} from shard {}",
                         latest->version_token, latest->shard_id);
            return latest->data;
        }
        
        case ConflictResolution::FIRST_WRITE_WINS: {
            // Find chunk with lowest version token (oldest)
            const VersionedChunk* oldest = &versioned_chunks[0];
            for (const auto& vc : versioned_chunks) {
                if (vc.version_token < oldest->version_token) {
                    oldest = &vc;
                }
            }
            result_version = oldest->version_token;
            spdlog::debug("RedundancyStrategy: FIRST_WRITE_WINS selected version {} from shard {}",
                         oldest->version_token, oldest->shard_id);
            return oldest->data;
        }
        
        case ConflictResolution::HIGHEST_NODE_ID: {
            // Find chunk from highest node ID (deterministic)
            const VersionedChunk* selected = &versioned_chunks[0];
            for (const auto& vc : versioned_chunks) {
                if (vc.shard_id > selected->shard_id) {
                    selected = &vc;
                }
            }
            result_version = selected->version_token;
            spdlog::debug("RedundancyStrategy: HIGHEST_NODE_ID selected shard {}",
                         selected->shard_id);
            return selected->data;
        }
        
        case ConflictResolution::CUSTOM: {
            // For CUSTOM, use LAST_WRITE_WINS as safe default
            spdlog::warn("RedundancyStrategy: CUSTOM conflict resolution not implemented, "
                         "falling back to LAST_WRITE_WINS");
            const VersionedChunk* latest = &versioned_chunks[0];
            for (const auto& vc : versioned_chunks) {
                if (vc.version_token > latest->version_token) {
                    latest = &vc;
                }
            }
            result_version = latest->version_token;
            return latest->data;
        }
    }
    
    // Should not reach here
    result_version = 0;
    return {};
}

// ============================================================================
// Version-aware read with consistency checking
// ============================================================================

/**
 * @brief Read with version-aware consistency checking
 * 
 * Resolves GAP: unspecified_consistency, missing_version_tracking
 * 
 * @param document_id Document identifier
 * @param collection Collection name
 * @param ring Consistent hash ring
 * @param topology Shard topology
 * @param handler Version-aware read handler
 * @return ReadResult with merged data and consistency metadata
 */
ReadResult RedundancyStrategy::readMirrorWithVersion(
    const std::string& document_id,
    const std::string& collection,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandlerWithVersion handler
) {
    (void)collection;
    (void)topology;

    ReadResult result;
    result.document_id = document_id;
    
    uint32_t replication_factor = 0;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        replication_factor = config_.replication_factor;
    }
    
    // Get available shards
    auto primary_shard = ring.getNode(document_id);
    if (!primary_shard) {
        result.success = false;
        result.error_message = "No shard available";
        return result;
    }
    
    std::vector<std::string> available_shards;
    available_shards.push_back(*primary_shard);
    
    auto replicas = ring.getReplicaNodes(document_id, replication_factor - 1);
    available_shards.insert(available_shards.end(), replicas.begin(), replicas.end());
    
    // Read from all shards with version tracking
    std::vector<VersionedChunk> versioned_chunks;
    std::optional<std::string> primary_opt = primary_shard;
    
    for (const auto& shard_id : available_shards) {
        auto versioned_result = handler(shard_id, document_id);
        if (versioned_result.data) {
            versioned_chunks.push_back({
                *versioned_result.data,
                versioned_result.version_token,
                versioned_result.shard_id
            });
        }
    }
    
    if (versioned_chunks.empty()) {
        result.success = false;
        result.error_message = "Failed to read from any shard";
        return result;
    }
    
    // Merge with consistency checking
    uint64_t merged_version = 0;
    auto merged_data = mergeChunksWithConsistency(
        versioned_chunks, 
        config_.geo_replication.conflict_resolution,
        merged_version
    );
    
    result.success = true;
    result.data = std::string(merged_data.begin(), merged_data.end());
    result.version_token = merged_version;
    result.chunks_read = static_cast<uint32_t>(versioned_chunks.size());
    result.source_shard = versioned_chunks[0].shard_id;
    result.from_replica = (result.source_shard != *primary_opt);
    
    return result;
}

std::string RedundancyStrategy::selectReadShard(
    const std::vector<std::string>& available_shards,
    ShardTopology& /*topology*/
) {
    if (available_shards.empty()) {
        throw std::runtime_error("No available shards");
    }
    
    switch (config_.read_preference) {
        case ReadPreference::PRIMARY:
            return available_shards[0];
            
        case ReadPreference::NEAREST: {
            // Select the available shard with the lowest recorded EWMA latency.
            // Falls back to the first shard when no latency data is available yet.
            std::lock_guard<std::mutex> lat_lock(latency_mutex_);
            const std::string* best = &available_shards[0];
            double best_latency = std::numeric_limits<double>::max();
            for (const auto& shard_id : available_shards) {
                auto it = shard_latency_ewma_ms_.find(shard_id);
                if (it != shard_latency_ewma_ms_.end() && it->second < best_latency) {
                    best_latency = it->second;
                    best = &shard_id;
                }
            }
            return *best;
        }
            
        case ReadPreference::ROUND_ROBIN: {
            static std::atomic<uint32_t> counter{0};
            return available_shards[counter.fetch_add(1) % available_shards.size()];
        }
            
        case ReadPreference::RANDOM: {
            size_t idx = std::rand() % available_shards.size();
            return available_shards[idx];
        }
            
        case ReadPreference::SECONDARY_ONLY:
            if (static_cast<int>(available_shards.size()) > 1) {
                return available_shards[1];
            }
            return available_shards[0];

        case ReadPreference::FOLLOWER:
            // Any follower (non-primary); fall through to second shard if available
            if (static_cast<int>(available_shards.size()) > 1) {
                return available_shards[1];
            }
            return available_shards[0];

        case ReadPreference::LOCAL_REGION:
            // Prefer local region; handled by selectGeoReadShard - fall back to first
            return available_shards[0];

        default:
            return available_shards[0];
    }
}

std::string RedundancyStrategy::selectGeoReadShard(
    const std::vector<std::string>& candidates,
    ShardTopology& topology,
    const std::string& local_region
) {
    if (candidates.empty()) {
        throw std::runtime_error("No available shards");
    }

    if (!local_region.empty()) {
        // Prefer healthy shards in local_region
        for (const auto& shard_id : candidates) {
            auto info = topology.getShard(shard_id);
            if (info && info->region == local_region && info->is_healthy) {
                return shard_id;
            }
        }
        // Fall back: any shard in local_region (even if unhealthy marker not yet updated)
        for (const auto& shard_id : candidates) {
            auto info = topology.getShard(shard_id);
            if (info && info->region == local_region) {
                return shard_id;
            }
        }
    }

    // No local-region shard found – return nearest healthy candidate
    for (const auto& shard_id : candidates) {
        auto info = topology.getShard(shard_id);
        if (info && info->is_healthy) {
            return shard_id;
        }
    }

    return candidates[0];
}

void RedundancyStrategy::evaluateGeoFailover(ShardTopology& topology) const {
    double region_failure_threshold = 0.0;
    std::vector<std::string> failed_regions;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        region_failure_threshold = config_.geo_replication.region_failure_threshold;
        failed_regions = config_.geo_replication.failed_regions;
    }

    const auto regions = topology.getRegions();
    std::unordered_set<std::string> failed_set(failed_regions.begin(), failed_regions.end());

    for (const auto& region : regions) {
        const auto all_shards = topology.getShardsInRegion(region);
        if (all_shards.empty()) {
          continue;
        }

        const auto healthy = topology.getHealthyShardsInRegion(region);
        double healthy_fraction = static_cast<double>(healthy.size()) /
                                  static_cast<double>(all_shards.size());

        const bool already_failed = failed_set.count(region) > 0;

        if (healthy_fraction < region_failure_threshold) {
            if (!already_failed) {
                spdlog::warn("GEO_MIRROR: region '{}' is below failure threshold "
                             "({:.0f}% healthy), marking as failed-out", region,
                             healthy_fraction * 100.0);
                failed_set.insert(region);
            }
        } else if (already_failed) {
            spdlog::info("GEO_MIRROR: region '{}' has recovered ({:.0f}% healthy), "
                         "removing from failed list", region, healthy_fraction * 100.0);
            failed_set.erase(region);
        }
    }

    std::vector<std::string> updated_failed_regions(failed_set.begin(), failed_set.end());
    std::sort(updated_failed_regions.begin(), updated_failed_regions.end());

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        config_.geo_replication.failed_regions = std::move(updated_failed_regions);
    }
}

/**
 * @brief Remove document keys from all currently targeted replicas/chunks.
 * @return true if at least one target deletion succeeded; false otherwise.
 */
bool RedundancyStrategy::remove(
    const std::string& document_id,
    const std::string& collection [[maybe_unused]],
    ConsistentHashRing& ring,
    ShardTopology& topology,
    WriteHandler handler
) {

    // Determine the set of shards that hold this document
    auto primary_opt = ring.getNode(document_id);
    if (!primary_opt) {
        spdlog::warn("remove: no primary shard for document {}", document_id);
        return false;
    }

    RedundancyMode mode;
    StripeConfig stripe_config;
    ErasureCodingConfig erasure_config;
    GeoReplicationConfig geo;
    uint32_t replication_factor = 0;
    bool enable_geo_failover = false;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        mode = config_.mode;
        stripe_config = config_.stripe;
        erasure_config = config_.erasure_coding;
        geo = config_.geo_replication;
        replication_factor = config_.replication_factor;
        enable_geo_failover = config_.geo_replication.enable_geo_failover;
    }

    if (mode == RedundancyMode::GEO_MIRROR && enable_geo_failover) {
        evaluateGeoFailover(topology);
        std::shared_lock<std::shared_mutex> lock(mutex_);
        geo = config_.geo_replication;
    }

    // Build the list of shard IDs and the doc-keys to delete
    // ── MIRROR / GEO_MIRROR: all replicas hold the full document ──
    // ── STRIPE / STRIPE_MIRROR: each replica holds a chunk key ──
    // ── PARITY / RAID6: each replica holds a data: or parity: chunk key ──

    std::vector<std::pair<std::string /*shard*/, std::string /*doc_key*/>> targets;

    if (mode == RedundancyMode::STRIPE ||
        mode == RedundancyMode::STRIPE_MIRROR) {

        auto primary_shard = *primary_opt;
        auto replicas = ring.getReplicaNodes(document_id,
            stripe_config.min_stripe_shards > 0
                ? stripe_config.min_stripe_shards - 1
                : replication_factor - 1);
        std::vector<std::string> shards{primary_shard};
        shards.insert(shards.end(), replicas.begin(), replicas.end());

        for (size_t i = 0; i <static_cast<int>(shards.size()); ++i) {
            targets.emplace_back(shards[i],
                                 document_id + ":chunk:" + std::to_string(i));
        }

    } else if (mode == RedundancyMode::PARITY ||
               mode == RedundancyMode::RAID6) {

        const uint32_t total = erasure_config.data_shards +
                               erasure_config.parity_shards;
        auto primary_shard = *primary_opt;
        auto replicas = ring.getReplicaNodes(document_id, total - 1);
        std::vector<std::string> shards{primary_shard};
        shards.insert(shards.end(), replicas.begin(), replicas.end());

        for (size_t i = 0; i <static_cast<int>(shards.size()) && i < total; ++i) {
            bool is_parity = (i >= erasure_config.data_shards);
            std::string key = document_id +
                              (is_parity ? ":parity:" : ":data:") +
                              std::to_string(i);
            targets.emplace_back(shards[i], key);
        }

    } else {
        // NONE / MIRROR / GEO_MIRROR — full document on every replica
        std::vector<std::string> shards;
        shards.push_back(*primary_opt);
        auto replicas = ring.getReplicaNodes(document_id, replication_factor - 1);
        shards.insert(shards.end(), replicas.begin(), replicas.end());

        // For GEO_MIRROR, skip shards that belong to failed-out regions
        const auto& failed_regions = geo.failed_regions;
        const std::unordered_set<std::string> failed_set(failed_regions.begin(),
                                                         failed_regions.end());

        for (const auto& sid : shards) {
            if (!failed_set.empty()) {
                auto info = topology.getShard(sid);
                if (info && failed_set.count(info->region)) {
                  continue;
                }
            }
            targets.emplace_back(sid, document_id);
        }
    }

    if (targets.empty()) {
        spdlog::warn("remove: no target shards for document {}", document_id);
        return false;
    }

    // Delete from all targets in parallel (send a "write" with empty payload
    // — the WriteHandler interprets an empty payload as a delete command)
    const std::vector<uint8_t> empty_payload;
    std::vector<std::future<bool>> futures;
    futures.reserve(targets.size());
    for (const auto& [shard_id, doc_key] : targets) {
        futures.push_back(std::async(std::launch::async,
            [&handler, shard_id = shard_id, doc_key = doc_key, &empty_payload]() {
                return handler(shard_id, doc_key, empty_payload);
            }));
    }

    uint32_t successes = 0;
    for (size_t i = 0; i <static_cast<int>(futures.size()); ++i) {
        try {
            if (futures[i].get()) {
                ++successes;
            } else {
                spdlog::warn("remove: delete from shard {} failed for doc {}",
                             targets[i].first, document_id);
            }
        } catch (const std::exception& e) {
            spdlog::warn("remove: delete from shard {} threw: {}",
                         targets[i].first, e.what());
        }
    }

    // Succeed if at least one replica was deleted (soft-delete semantics)
    return successes > 0;
}

/**
 * @brief Recover missing replicas/chunks from surviving copies or parity.
 * @return true when at least one missing replica/chunk was restored.
 */
bool RedundancyStrategy::recoverDocument(
    const std::string& document_id,
    const std::string& collection [[maybe_unused]],
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler read_handler,
    WriteHandler write_handler
) {
    stats_recoveries_++;

    // Recovery is only meaningful for modes that have redundant copies or parity
    if (config_.mode == RedundancyMode::NONE ||
        config_.mode == RedundancyMode::STRIPE) {
        // No redundancy — cannot recover
        return false;
    }

    auto primary_opt = ring.getNode(document_id);
    if (!primary_opt) {
      return false;
    }

    std::vector<std::string> all_shards{*primary_opt};
    auto replicas = ring.getReplicaNodes(document_id, config_.replication_factor - 1);
    all_shards.insert(all_shards.end(), replicas.begin(), replicas.end());

    if (config_.mode == RedundancyMode::MIRROR ||
        config_.mode == RedundancyMode::GEO_MIRROR ||
        config_.mode == RedundancyMode::STRIPE_MIRROR) {

        // Find a healthy replica that has the document
        std::optional<std::vector<uint8_t>> source_data;
        for (const auto& shard_id : all_shards) {
            auto info = topology.getShard(shard_id);
            if (info && !info->is_healthy) {
              continue;
            }

            auto data = read_handler(shard_id, document_id);
            if (data) {
                source_data = data;
                break;
            }
        }

        if (!source_data) {
            spdlog::error("recoverDocument: no healthy replica with data for {}",
                          document_id);
            return false;
        }

        // Re-write to any shard that is missing the document
        uint32_t recovered = 0;
        for (const auto& shard_id : all_shards) {
            auto existing = read_handler(shard_id, document_id);
            if (existing) continue;  // already has the data

            bool ok = false;
            try {
                ok = write_handler(shard_id, document_id, *source_data);
            } catch (const std::exception& e) {
                spdlog::warn("recoverDocument: write to {} failed: {}",
                             shard_id, e.what());
            }
            if (ok) {
                ++recovered;
                spdlog::info("recoverDocument: restored doc {} to shard {}",
                             document_id, shard_id);
            }
        }
        return recovered > 0;
    }

    if (config_.mode == RedundancyMode::PARITY ||
        config_.mode == RedundancyMode::RAID6) {

        // Snapshot erasure-coding parameters under the shared lock to guard
        // against a concurrent configure() resetting erasure_coder_.
        uint32_t k, m;
        {
            std::shared_lock<std::shared_mutex> ec_lock(mutex_);
            if (!erasure_coder_) {
              return false;
            }
            k = config_.erasure_coding.data_shards;
            m = config_.erasure_coding.parity_shards;
        }
        const uint32_t total = k + m;

        // Read all available chunks
        std::vector<std::optional<std::vector<uint8_t>>> chunk_opts(total);
        std::vector<std::string> chunk_shards(total);

        auto replicas2 = ring.getReplicaNodes(document_id, total - 1);
        std::vector<std::string> shards{*primary_opt};
        shards.insert(shards.end(), replicas2.begin(), replicas2.end());

        for (size_t i = 0; i <static_cast<int>(shards.size()) && i < total; ++i) {
            bool is_parity = (i >= k);
            std::string key = document_id +
                              (is_parity ? ":parity:" : ":data:") +
                              std::to_string(i);
            chunk_opts[i] = read_handler(shards[i], key);
            chunk_shards[i] = shards[i];
        }

        // Count available chunks
        uint32_t available = 0;
        for (const auto& c : chunk_opts) {
          if (c) ++available;
        }

        if (available < k) {
            spdlog::error("recoverDocument: only {}/{} chunks available for {} (need {})",
                          available, total, document_id, k);
            return false;
        }

        // Build the map and missing-indices vector required by decode()
        std::map<uint32_t, std::vector<uint8_t>> available_map;
        std::vector<uint32_t> missing_idx_vec = {};

        for (uint32_t i = 0; i < total; ++i) {
            if (chunk_opts[i]) {
                available_map[i] = *chunk_opts[i];
            } else {
                missing_idx_vec.push_back(i);
            }
        }

        // Decode and re-encode under the shared lock to guard erasure_coder_.
        std::vector<std::vector<uint8_t>> all_chunks;
        {
            std::shared_lock<std::shared_mutex> ec_lock(mutex_);
            if (!erasure_coder_) {
                spdlog::error("recoverDocument: erasure coder was reconfigured during recovery");
                return false;
            }
            auto recovered_data = erasure_coder_->decode(available_map, missing_idx_vec, k, m);
            if (recovered_data.empty()) {
                spdlog::error("recoverDocument: erasure decode failed for {}", document_id);
                return false;
            }
            all_chunks = erasure_coder_->encode(recovered_data, k, m);
        }

        uint32_t restored = 0;
        for (size_t i = 0; i <static_cast<int>(shards.size())  && static_cast<size_t>(i) <static_cast<int>(all_chunks.size()); ++i) {
            if (chunk_opts[i]) continue;  // chunk was already present

            bool is_parity = (i >= k);
            std::string key = document_id +
                              (is_parity ? ":parity:" : ":data:") +
                              std::to_string(i);
            try {
                if (write_handler(shards[i], key, all_chunks[i])) {
                    ++restored;
                    spdlog::info("recoverDocument: restored chunk {} of {} to shard {}",
                                 i, document_id, shards[i]);
                }
            } catch (const std::exception& e) {
                spdlog::warn("recoverDocument: write chunk {} to {} failed: {}",
                             i, shards[i], e.what());
            }
        }
        return restored > 0;
    }

    return false;
}

/**
 * @brief Evaluate document redundancy health and recoverability.
 * @return DocumentHealth snapshot for the current mode/topology state.
 */
RedundancyStrategy::DocumentHealth RedundancyStrategy::checkDocumentHealth(
    const std::string& document_id,
    const std::string& collection [[maybe_unused]],
    ConsistentHashRing& ring,
    ShardTopology& topology,
    ReadHandler handler
) {
    DocumentHealth health;
    health.required_replicas = config_.replication_factor;
    health.is_healthy = false;
    health.available_replicas = 0;
    health.can_recover = false;

    auto primary_opt = ring.getNode(document_id);
    if (!primary_opt) {
        return health;  // no shard at all
    }

    std::vector<std::string> all_shards{*primary_opt};
    {
        auto replicas = ring.getReplicaNodes(document_id,
                                             config_.replication_factor - 1);
        all_shards.insert(all_shards.end(), replicas.begin(), replicas.end());
    }

    if (config_.mode == RedundancyMode::STRIPE) {
        // Each shard holds a unique chunk — read each chunk key
        for (size_t i = 0; i <static_cast<int>(all_shards.size()); ++i) {
            const std::string key = document_id + ":chunk:" + std::to_string(i);
            auto shard_info = topology.getShard(all_shards[i]);
            bool shard_healthy = (!shard_info || shard_info->is_healthy);
            auto chunk = handler(all_shards[i], key);
            if (chunk) {
                ++health.available_replicas;
            } else {
                if (shard_healthy) {
                    health.missing_shards.push_back(all_shards[i]);
                }
            }
        }
        health.is_healthy = (health.available_replicas == all_shards.size());
        health.can_recover = false;  // STRIPE: no recovery without all chunks
        return health;
    }

    if (config_.mode == RedundancyMode::PARITY ||
        config_.mode == RedundancyMode::RAID6) {

        const uint32_t k = config_.erasure_coding.data_shards;
        const uint32_t m = config_.erasure_coding.parity_shards;
        const uint32_t total = k + m;
        auto replicas = ring.getReplicaNodes(document_id, total - 1);
        std::vector<std::string> shards{*primary_opt};
        shards.insert(shards.end(), replicas.begin(), replicas.end());

        for (size_t i = 0; i <static_cast<int>(shards.size()) && i < total; ++i) {
            bool is_parity = (i >= k);
            std::string key = document_id +
                              (is_parity ? ":parity:" : ":data:") +
                              std::to_string(i);
            auto chunk = handler(shards[i], key);
            if (chunk) {
                ++health.available_replicas;
            } else {
                health.missing_shards.push_back(shards[i]);
            }
        }
        health.is_healthy = (health.missing_shards.empty());
        health.can_recover = (health.available_replicas >= k);
        return health;
    }

    // NONE / MIRROR / GEO_MIRROR / STRIPE_MIRROR — full document on each replica
    for (const auto& shard_id : all_shards) {
        auto data = handler(shard_id, document_id);
        if (data) {
            ++health.available_replicas;
        } else {
            auto info = topology.getShard(shard_id);
            bool shard_healthy = (!info || info->is_healthy);
            if (shard_healthy) {
                // Data missing from a healthy shard → needs recovery
                health.missing_shards.push_back(shard_id);
            }
        }
    }

    // MIRROR / GEO_MIRROR: can recover if at least 1 replica is available and some are missing
    constexpr uint32_t min_required = 1;

    health.is_healthy   = health.missing_shards.empty();
    health.can_recover  = (health.available_replicas >= min_required) &&
                          !health.missing_shards.empty();
    return health;
}

/**
 * @brief Replace active configuration after validating invariants.
 * @throws std::invalid_argument if the supplied configuration is invalid.
 */
void RedundancyStrategy::updateConfig(const RedundancyConfig& config) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (!config.validate()) {
        throw std::invalid_argument("Invalid redundancy configuration");
    }
    
    config_ = config;
    
    spdlog::info("RedundancyStrategy configuration updated");
}

/** @brief Return point-in-time counters for high-level redundancy activity. */
RedundancyStats RedundancyStrategy::getStats() const {
    RedundancyStats stats;
    stats.total_documents = 0;
    stats.total_replicas = 0;
    stats.reads_from_primary = stats_reads_.load();
    return stats;
}

/** @brief Export redundancy counters in Prometheus text exposition format. */
std::string RedundancyStrategy::exportPrometheusMetrics() const {
    std::stringstream ss;
    
    // Convert mode to string
    std::string mode_str = {};
    switch (config_.mode) {
        case RedundancyMode::NONE: mode_str = "none"; break;
        case RedundancyMode::MIRROR: mode_str = "mirror"; break;
        case RedundancyMode::STRIPE: mode_str = "stripe"; break;
        case RedundancyMode::STRIPE_MIRROR: mode_str = "stripe_mirror"; break;
        case RedundancyMode::PARITY: mode_str = "parity"; break;
        case RedundancyMode::RAID6: mode_str = "raid6"; break;
        case RedundancyMode::GEO_MIRROR: mode_str = "geo_mirror"; break;
        default: mode_str = "unknown"; break;
    }
    
    ss << "# HELP themis_redundancy_writes_total Total number of write operations\n";
    ss << "# TYPE themis_redundancy_writes_total counter\n";
    ss << "themis_redundancy_writes_total{mode=\"" << mode_str << "\"} " 
       << stats_writes_.load() << "\n";
    
    ss << "# HELP themis_redundancy_reads_total Total number of read operations\n";
    ss << "# TYPE themis_redundancy_reads_total counter\n";
    ss << "themis_redundancy_reads_total{mode=\"" << mode_str << "\"} " 
       << stats_reads_.load() << "\n";
    
    ss << "# HELP themis_redundancy_bytes_written_total Total bytes written\n";
    ss << "# TYPE themis_redundancy_bytes_written_total counter\n";
    ss << "themis_redundancy_bytes_written_total{mode=\"" << mode_str << "\"} " 
       << stats_bytes_written_.load() << "\n";
    
    ss << "# HELP themis_redundancy_bytes_read_total Total bytes read\n";
    ss << "# TYPE themis_redundancy_bytes_read_total counter\n";
    ss << "themis_redundancy_bytes_read_total{mode=\"" << mode_str << "\"} " 
       << stats_bytes_read_.load() << "\n";
    
    ss << "# HELP themis_redundancy_recoveries_total Total recovery operations\n";
    ss << "# TYPE themis_redundancy_recoveries_total counter\n";
    ss << "themis_redundancy_recoveries_total{mode=\"" << mode_str << "\"} " 
       << stats_recoveries_.load() << "\n";
    
    // Configuration info
    ss << "# HELP themis_redundancy_storage_efficiency Storage efficiency ratio (0.0-1.0)\n";
    ss << "# TYPE themis_redundancy_storage_efficiency gauge\n";
    ss << "themis_redundancy_storage_efficiency{mode=\"" << mode_str << "\"} " 
       << config_.getStorageEfficiency() << "\n";
    
    ss << "# HELP themis_redundancy_fault_tolerance Number of failures tolerated\n";
    ss << "# TYPE themis_redundancy_fault_tolerance gauge\n";
    ss << "themis_redundancy_fault_tolerance{mode=\"" << mode_str << "\"} " 
       << config_.getFaultTolerance() << "\n";
    
    // RAID 6 specific metrics
    if (config_.mode == RedundancyMode::RAID6 || config_.mode == RedundancyMode::PARITY) {
        ss << "# HELP themis_redundancy_data_shards Number of data shards\n";
        ss << "# TYPE themis_redundancy_data_shards gauge\n";
        ss << "themis_redundancy_data_shards{mode=\"" << mode_str << "\"} " 
           << config_.erasure_coding.data_shards << "\n";
        
        ss << "# HELP themis_redundancy_parity_shards Number of parity shards\n";
        ss << "# TYPE themis_redundancy_parity_shards gauge\n";
        ss << "themis_redundancy_parity_shards{mode=\"" << mode_str << "\"} " 
           << config_.erasure_coding.parity_shards << "\n";
        
        std::string algo_str = {};
        switch (config_.erasure_coding.algorithm) {
            case ErasureCodingAlgorithm::REED_SOLOMON: algo_str = "reed_solomon"; break;
            case ErasureCodingAlgorithm::CAUCHY: algo_str = "cauchy"; break;
            case ErasureCodingAlgorithm::LRC: algo_str = "lrc"; break;
            default: algo_str = "unknown"; break;
        }
        
        ss << "# HELP themis_redundancy_erasure_algorithm Erasure coding algorithm (info metric)\n";
        ss << "# TYPE themis_redundancy_erasure_algorithm gauge\n";
        ss << "themis_redundancy_erasure_algorithm{mode=\"" << mode_str 
           << "\",algorithm=\"" << algo_str << "\"} 1\n";
    }

    // GEO_MIRROR specific metrics
    if (config_.mode == RedundancyMode::GEO_MIRROR) {
        const auto& geo = config_.geo_replication;

        // Replication mode
        std::string repl_mode_str = {};
        switch (geo.replication_mode) {
            case GeoReplicationConfig::ReplicationMode::SYNC:      repl_mode_str = "sync";      break;
            case GeoReplicationConfig::ReplicationMode::SEMI_SYNC: repl_mode_str = "semi_sync"; break;
            case GeoReplicationConfig::ReplicationMode::ASYNC:     repl_mode_str = "async";     break;
            default:                                               repl_mode_str = "unknown";   break;
        }
        ss << "# HELP themis_geo_replication_mode Geo replication mode (info metric)\n";
        ss << "# TYPE themis_geo_replication_mode gauge\n";
        ss << "themis_geo_replication_mode{replication_mode=\"" << repl_mode_str << "\"} 1\n";

        // Number of configured region write quorums
        ss << "# HELP themis_geo_region_write_quorums_total Number of regions with write quorum configured\n";
        ss << "# TYPE themis_geo_region_write_quorums_total gauge\n";
        ss << "themis_geo_region_write_quorums_total{mode=\"geo_mirror\"} "
           <<static_cast<int>(geo.region_write_quorums.size()) << "\n";

        // Per-region write quorum values
        ss << "# HELP themis_geo_region_write_quorum Required write quorum per region\n";
        ss << "# TYPE themis_geo_region_write_quorum gauge\n";
        for (const auto& [region, quorum] : geo.region_write_quorums) {
            ss << "themis_geo_region_write_quorum{region=\"" << region << "\"} " << quorum << "\n";
        }

        // Per-region read quorum values
        ss << "# HELP themis_geo_region_read_quorum Required read quorum per region\n";
        ss << "# TYPE themis_geo_region_read_quorum gauge\n";
        for (const auto& [region, quorum] : geo.region_read_quorums) {
            ss << "themis_geo_region_read_quorum{region=\"" << region << "\"} " << quorum << "\n";
        }

        // Failed regions count
        ss << "# HELP themis_geo_failed_regions_total Number of regions currently failed-out\n";
        ss << "# TYPE themis_geo_failed_regions_total gauge\n";
        ss << "themis_geo_failed_regions_total{mode=\"geo_mirror\"} "
           <<static_cast<int>(geo.failed_regions.size()) << "\n";

        // Per-failed-region marker
        ss << "# HELP themis_geo_region_failed Whether a region is currently failed-out (1=failed)\n";
        ss << "# TYPE themis_geo_region_failed gauge\n";
        for (const auto& region : geo.failed_regions) {
            ss << "themis_geo_region_failed{region=\"" << region << "\"} 1\n";
        }

        // Geo-failover enabled flag
        ss << "# HELP themis_geo_failover_enabled Whether geo-failover is enabled\n";
        ss << "# TYPE themis_geo_failover_enabled gauge\n";
        ss << "themis_geo_failover_enabled{mode=\"geo_mirror\"} "
           << (geo.enable_geo_failover ? 1 : 0) << "\n";

        // Bounded-staleness limit
        ss << "# HELP themis_geo_max_staleness_ms Maximum accepted replication staleness in ms\n";
        ss << "# TYPE themis_geo_max_staleness_ms gauge\n";
        ss << "themis_geo_max_staleness_ms{mode=\"geo_mirror\"} " << geo.max_staleness_ms << "\n";
    }

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
    RedundancyConfig config = default_config_;
    auto cfg_it = collection_configs_.find(collection);
    if (cfg_it != collection_configs_.end()) {
        config = cfg_it->second;
    }
    auto strategy = std::make_shared<RedundancyStrategy>(config);
    strategies_[collection] = strategy;
    
    return strategy;
}

std::vector<std::string> CollectionRedundancyManager::listCollections() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<std::string> collections = {};

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
} // namespace themis

// Backward compatibility shim: expose under themisdb::sharding
namespace themisdb {
namespace sharding {
using namespace themis::sharding;
}
}
