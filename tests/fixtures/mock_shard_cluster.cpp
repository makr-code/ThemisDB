#include "mock_shard_cluster.h"
#include <algorithm>
#include <thread>

namespace themis {
namespace test {

MockShardCluster::MockShardCluster(int num_shards)
    : num_shards_(num_shards), rng_(std::random_device{}()) {
    shards_.reserve(num_shards);
    for (int i = 0; i < num_shards; ++i) {
        shards_.push_back(std::make_shared<ShardState>());
    }
}

MockShardCluster::~MockShardCluster() = default;

bool MockShardCluster::saveToShard(int shard_id, const std::string& key,
                                    const std::vector<uint8_t>& data) {
    if (shard_id < 0 || shard_id >= num_shards_) {
        return false;
    }

    // Check network partition
    if (isShardIsolated(shard_id)) {
        return false;
    }

    // Simulate packet loss
    if (shouldDropPacket()) {
        return false;
    }

    // Simulate network delay
    simulateNetworkDelay();

    auto& shard = shards_[shard_id];
    if (!shard->is_healthy) {
        return false;
    }

    std::lock_guard<std::mutex> lock(shard->mutex);
    shard->data[key] = data;
    shard->write_count++;
    shard->bytes_written += data.size();
    shard->last_access = std::chrono::steady_clock::now();
    return true;
}

std::optional<std::vector<uint8_t>> MockShardCluster::loadFromShard(
    int shard_id, const std::string& key) {
    if (shard_id < 0 || shard_id >= num_shards_) {
        return std::nullopt;
    }

    // Check network partition
    if (isShardIsolated(shard_id)) {
        return std::nullopt;
    }

    // Simulate packet loss
    if (shouldDropPacket()) {
        return std::nullopt;
    }

    // Simulate network delay
    simulateNetworkDelay();

    auto& shard = shards_[shard_id];
    if (!shard->is_healthy) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(shard->mutex);
    shard->read_count++;
    shard->last_access = std::chrono::steady_clock::now();

    auto it = shard->data.find(key);
    if (it != shard->data.end()) {
        shard->bytes_read += it->second.size();
        return it->second;
    }
    return std::nullopt;
}

bool MockShardCluster::existsInShard(int shard_id, const std::string& key) {
    if (shard_id < 0 || shard_id >= num_shards_) {
        return false;
    }

    auto& shard = shards_[shard_id];
    if (!shard->is_healthy || isShardIsolated(shard_id)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(shard->mutex);
    return shard->data.count(key) > 0;
}

bool MockShardCluster::deleteFromShard(int shard_id, const std::string& key) {
    if (shard_id < 0 || shard_id >= num_shards_) {
        return false;
    }

    if (isShardIsolated(shard_id)) {
        return false;
    }

    auto& shard = shards_[shard_id];
    if (!shard->is_healthy) {
        return false;
    }

    std::lock_guard<std::mutex> lock(shard->mutex);
    return shard->data.erase(key) > 0;
}

bool MockShardCluster::failShard(int shard_id) {
    if (shard_id < 0 || shard_id >= num_shards_) {
        return false;
    }

    shards_[shard_id]->is_healthy = false;
    return true;
}

bool MockShardCluster::recoverShard(int shard_id) {
    if (shard_id < 0 || shard_id >= num_shards_) {
        return false;
    }

    shards_[shard_id]->is_healthy = true;
    return true;
}

bool MockShardCluster::isShardHealthy(int shard_id) const {
    if (shard_id < 0 || shard_id >= num_shards_) {
        return false;
    }

    return shards_[shard_id]->is_healthy;
}

std::vector<int> MockShardCluster::getHealthyShards() const {
    std::vector<int> healthy = {};

    for (int i = 0; i < num_shards_; ++i) {
        if (shards_[i]->is_healthy) {
            healthy.push_back(i);
        }
    }
    return healthy;
}

std::vector<int> MockShardCluster::getFailedShards() const {
    std::vector<int> failed = {};

    for (int i = 0; i < num_shards_; ++i) {
        if (!shards_[i]->is_healthy) {
            failed.push_back(i);
        }
    }
    return failed;
}

void MockShardCluster::injectPacketLoss(float loss_rate) {
    std::lock_guard<std::mutex> lock(network_mutex_);
    network_config_.packet_loss_rate = std::clamp(loss_rate, 0.0f, 1.0f);
}

void MockShardCluster::setLatency(int min_ms, int max_ms) {
    std::lock_guard<std::mutex> lock(network_mutex_);
    network_config_.min_latency_ms = std::max(0, min_ms);
    network_config_.max_latency_ms = std::max(network_config_.min_latency_ms, max_ms);
}

void MockShardCluster::simulateNetworkPartition(const std::vector<int>& isolated_shards) {
    std::lock_guard<std::mutex> lock(network_mutex_);
    isolated_shards_ = isolated_shards;
}

void MockShardCluster::healNetworkPartition() {
    std::lock_guard<std::mutex> lock(network_mutex_);
    isolated_shards_.clear();
}

bool MockShardCluster::isNetworkPartitioned() const {
    std::lock_guard<std::mutex> lock(network_mutex_);
    return !isolated_shards_.empty();
}

bool MockShardCluster::isShardIsolated(int shard_id) const {
    std::lock_guard<std::mutex> lock(network_mutex_);
    return std::find(isolated_shards_.begin(), isolated_shards_.end(), shard_id) !=
           isolated_shards_.end();
}

uint64_t MockShardCluster::getTotalWrites() const {
    uint64_t total = 0;
    for (const auto& shard : shards_) {
        total += shard->write_count.load();
    }
    return total;
}

uint64_t MockShardCluster::getTotalReads() const {
    uint64_t total = 0;
    for (const auto& shard : shards_) {
        total += shard->read_count.load();
    }
    return total;
}

uint64_t MockShardCluster::getTotalBytesWritten() const {
    uint64_t total = 0;
    for (const auto& shard : shards_) {
        total += shard->bytes_written.load();
    }
    return total;
}

uint64_t MockShardCluster::getTotalBytesRead() const {
    uint64_t total = 0;
    for (const auto& shard : shards_) {
        total += shard->bytes_read.load();
    }
    return total;
}

std::shared_ptr<const MockShardCluster::ShardState> MockShardCluster::getShardStats(
    int shard_id) const {
    if (shard_id < 0 || shard_id >= num_shards_) {
        return nullptr;
    }
    return shards_[shard_id];
}

void MockShardCluster::resetStats() {
    for (auto& shard : shards_) {
        shard->write_count = 0;
        shard->read_count = 0;
        shard->bytes_written = 0;
        shard->bytes_read = 0;
    }
}

void MockShardCluster::clearAllData() {
    for (auto& shard : shards_) {
        std::lock_guard<std::mutex> lock(shard->mutex);
        shard->data.clear();
    }
}

void MockShardCluster::simulateNetworkDelay() {
    std::lock_guard<std::mutex> lock(network_mutex_);
    if (!network_config_.enable_latency_simulation) {
        return;
    }

    std::uniform_int_distribution<int> dist(network_config_.min_latency_ms,
                                            network_config_.max_latency_ms);
    int delay_ms = dist(rng_);
    if (delay_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}

bool MockShardCluster::shouldDropPacket() {
    std::lock_guard<std::mutex> lock(network_mutex_);
    if (network_config_.packet_loss_rate <= 0.0f) {
        return false;
    }

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng_) < network_config_.packet_loss_rate;
}

} // namespace test
} // namespace themis
