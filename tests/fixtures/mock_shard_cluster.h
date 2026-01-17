#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <random>

namespace themis {
namespace test {

/**
 * @brief Mock shard cluster for testing distributed LoRA operations
 * 
 * Simulates a multi-shard cluster with:
 * - Network latency simulation
 * - Packet loss injection
 * - Shard failure simulation
 * - Network partition simulation
 */
class MockShardCluster {
public:
    /**
     * @brief Shard state tracking
     */
    struct ShardState {
        std::map<std::string, std::vector<uint8_t>> data;
        std::atomic<bool> is_healthy{true};
        std::atomic<uint64_t> write_count{0};
        std::atomic<uint64_t> read_count{0};
        std::atomic<uint64_t> bytes_written{0};
        std::atomic<uint64_t> bytes_read{0};
        std::chrono::steady_clock::time_point last_access;
        std::mutex mutex;
    };

    /**
     * @brief Network configuration
     */
    struct NetworkConfig {
        int min_latency_ms = 1;
        int max_latency_ms = 5;
        float packet_loss_rate = 0.0f;
        bool enable_latency_simulation = true;
    };

    /**
     * @brief Construct mock shard cluster
     * @param num_shards Number of shards to create
     */
    explicit MockShardCluster(int num_shards);

    /**
     * @brief Destructor
     */
    ~MockShardCluster();

    // ═══════════════════════════════════════════════════════════
    // Shard Operations
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Save data to a specific shard
     * @param shard_id Shard identifier (0-based index)
     * @param key Data key
     * @param data Data to save
     * @return true if saved successfully
     */
    bool saveToShard(int shard_id, const std::string& key, const std::vector<uint8_t>& data);

    /**
     * @brief Load data from a specific shard
     * @param shard_id Shard identifier
     * @param key Data key
     * @return Optional data if found and shard is healthy
     */
    std::optional<std::vector<uint8_t>> loadFromShard(int shard_id, const std::string& key);

    /**
     * @brief Check if key exists in shard
     * @param shard_id Shard identifier
     * @param key Data key
     * @return true if exists
     */
    bool existsInShard(int shard_id, const std::string& key);

    /**
     * @brief Delete data from shard
     * @param shard_id Shard identifier
     * @param key Data key
     * @return true if deleted
     */
    bool deleteFromShard(int shard_id, const std::string& key);

    // ═══════════════════════════════════════════════════════════
    // Shard Health Management
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Simulate shard failure
     * @param shard_id Shard to fail
     * @return true if shard existed and was failed
     */
    bool failShard(int shard_id);

    /**
     * @brief Recover a failed shard
     * @param shard_id Shard to recover
     * @return true if shard existed and was recovered
     */
    bool recoverShard(int shard_id);

    /**
     * @brief Check if shard is healthy
     * @param shard_id Shard to check
     * @return true if healthy
     */
    bool isShardHealthy(int shard_id) const;

    /**
     * @brief Get list of healthy shards
     * @return Vector of healthy shard IDs
     */
    std::vector<int> getHealthyShards() const;

    /**
     * @brief Get list of failed shards
     * @return Vector of failed shard IDs
     */
    std::vector<int> getFailedShards() const;

    // ═══════════════════════════════════════════════════════════
    // Network Simulation
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Inject packet loss
     * @param loss_rate Packet loss rate (0.0 - 1.0)
     */
    void injectPacketLoss(float loss_rate);

    /**
     * @brief Set network latency range
     * @param min_ms Minimum latency in milliseconds
     * @param max_ms Maximum latency in milliseconds
     */
    void setLatency(int min_ms, int max_ms);

    /**
     * @brief Simulate network partition (isolate specific shards)
     * @param isolated_shards Shards to isolate
     */
    void simulateNetworkPartition(const std::vector<int>& isolated_shards);

    /**
     * @brief Heal network partition
     */
    void healNetworkPartition();

    /**
     * @brief Check if network is partitioned
     * @return true if partition exists
     */
    bool isNetworkPartitioned() const;

    /**
     * @brief Check if shard is isolated by network partition
     * @param shard_id Shard to check
     * @return true if isolated
     */
    bool isShardIsolated(int shard_id) const;

    // ═══════════════════════════════════════════════════════════
    // Statistics
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Get total writes across all shards
     * @return Total write count
     */
    uint64_t getTotalWrites() const;

    /**
     * @brief Get total reads across all shards
     * @return Total read count
     */
    uint64_t getTotalReads() const;

    /**
     * @brief Get total bytes written
     * @return Total bytes
     */
    uint64_t getTotalBytesWritten() const;

    /**
     * @brief Get total bytes read
     * @return Total bytes
     */
    uint64_t getTotalBytesRead() const;

    /**
     * @brief Get shard statistics
     * @param shard_id Shard identifier
     * @return Shard state pointer
     */
    std::shared_ptr<const ShardState> getShardStats(int shard_id) const;

    /**
     * @brief Get number of shards
     * @return Shard count
     */
    int getShardCount() const { return num_shards_; }

    /**
     * @brief Reset all statistics
     */
    void resetStats();

    /**
     * @brief Clear all data from all shards
     */
    void clearAllData();

private:
    int num_shards_;
    std::vector<std::shared_ptr<ShardState>> shards_;
    NetworkConfig network_config_;
    std::vector<int> isolated_shards_;
    mutable std::mutex network_mutex_;
    std::mt19937 rng_;

    /**
     * @brief Simulate network delay
     */
    void simulateNetworkDelay();

    /**
     * @brief Check if packet should be dropped
     * @return true if packet is dropped
     */
    bool shouldDropPacket();
};

} // namespace test
} // namespace themis
