#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>

namespace themis {
namespace test {

/**
 * @brief RAID mode enumeration
 */
enum class RAIDMode {
    STRIPE = 0,   // RAID 0 - data striping
    MIRROR = 1,   // RAID 1 - data mirroring
    PARITY = 5,   // RAID 5 - striping with distributed parity
    HYBRID = 10   // RAID 10 - stripe + mirror
};

/**
 * @brief RAID mode simulator for testing distributed LoRA storage
 * 
 * Simulates RAID operations including:
 * - Data striping (RAID 0)
 * - Data mirroring (RAID 1)
 * - Parity-based recovery (RAID 5)
 * - Hybrid stripe+mirror (RAID 10)
 */
class RAIDSimulator {
public:
    /**
     * @brief Construct RAID simulator
     * @param mode RAID mode
     * @param num_shards Number of shards in the array
     */
    RAIDSimulator(RAIDMode mode, int num_shards);

    /**
     * @brief Destructor
     */
    ~RAIDSimulator();

    // ═══════════════════════════════════════════════════════════
    // RAID Operations
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Stripe data across shards (RAID 0)
     * @param data Input data
     * @return Vector of data chunks (one per shard)
     */
    std::vector<std::vector<uint8_t>> stripe(const std::vector<uint8_t>& data);

    /**
     * @brief Mirror data to multiple shards (RAID 1)
     * @param data Input data
     * @param replication_factor Number of copies
     * @return Vector of replicated data
     */
    std::vector<std::vector<uint8_t>> mirror(const std::vector<uint8_t>& data,
                                              int replication_factor);

    /**
     * @brief Encode data with parity (RAID 5)
     * @param data Input data
     * @return Vector of data+parity chunks
     */
    std::vector<std::vector<uint8_t>> encodeWithParity(const std::vector<uint8_t>& data);

    /**
     * @brief Reconstruct data from chunks (may have missing/failed chunks)
     * @param chunks Data chunks (nullopt for missing chunks)
     * @return Reconstructed data
     */
    std::optional<std::vector<uint8_t>> reconstruct(
        const std::vector<std::optional<std::vector<uint8_t>>>& chunks);

    // ═══════════════════════════════════════════════════════════
    // Failure Simulation
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Mark shards as failed
     * @param shard_ids Shard IDs to fail
     */
    void failShards(const std::vector<int>& shard_ids);

    /**
     * @brief Check if data can be recovered with failed shards
     * @param failed_shards List of failed shard IDs
     * @return true if recovery is possible
     */
    bool canRecover(const std::vector<int>& failed_shards) const;

    /**
     * @brief Get maximum tolerable shard failures
     * @return Number of shards that can fail while still recovering data
     */
    int getMaxTolerableFailures() const;

    /**
     * @brief Recover failed shards
     * @param shard_ids Shard IDs to recover
     */
    void recoverShards(const std::vector<int>& shard_ids);

    /**
     * @brief Get list of failed shards
     * @return Vector of failed shard IDs
     */
    std::vector<int> getFailedShards() const;

    // ═══════════════════════════════════════════════════════════
    // Configuration
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Get RAID mode
     * @return Current RAID mode
     */
    RAIDMode getMode() const { return mode_; }

    /**
     * @brief Get number of shards
     * @return Shard count
     */
    int getShardCount() const { return num_shards_; }

    /**
     * @brief Set chunk size for striping
     * @param size Chunk size in bytes
     */
    void setChunkSize(size_t size) { chunk_size_ = size; }

    /**
     * @brief Get chunk size
     * @return Chunk size in bytes
     */
    size_t getChunkSize() const { return chunk_size_; }

private:
    RAIDMode mode_;
    int num_shards_;
    size_t chunk_size_;
    std::vector<int> failed_shards_;

    /**
     * @brief Calculate XOR parity for RAID 5
     * @param chunks Data chunks
     * @return Parity chunk
     */
    std::vector<uint8_t> calculateParity(
        const std::vector<std::vector<uint8_t>>& chunks) const;

    /**
     * @brief Reconstruct from RAID 0 chunks
     * @param chunks Data chunks
     * @return Reconstructed data or nullopt if any chunk is missing
     */
    std::optional<std::vector<uint8_t>> reconstructRAID0(
        const std::vector<std::optional<std::vector<uint8_t>>>& chunks) const;

    /**
     * @brief Reconstruct from RAID 1 chunks
     * @param chunks Mirrored data chunks
     * @return Reconstructed data from first available replica
     */
    std::optional<std::vector<uint8_t>> reconstructRAID1(
        const std::vector<std::optional<std::vector<uint8_t>>>& chunks) const;

    /**
     * @brief Reconstruct from RAID 5 chunks with parity
     * @param chunks Data+parity chunks
     * @return Reconstructed data
     */
    std::optional<std::vector<uint8_t>> reconstructRAID5(
        const std::vector<std::optional<std::vector<uint8_t>>>& chunks) const;

    /**
     * @brief Reconstruct from RAID 10 chunks
     * @param chunks Striped+mirrored chunks
     * @return Reconstructed data
     */
    std::optional<std::vector<uint8_t>> reconstructRAID10(
        const std::vector<std::optional<std::vector<uint8_t>>>& chunks) const;
};

} // namespace test
} // namespace themis
