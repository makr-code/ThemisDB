#include "raid_simulator.h"
#include <algorithm>
#include <cstring>

namespace themis {
namespace test {

RAIDSimulator::RAIDSimulator(RAIDMode mode, int num_shards)
    : mode_(mode), num_shards_(num_shards), chunk_size_(4096) {}

RAIDSimulator::~RAIDSimulator() = default;

std::vector<std::vector<uint8_t>> RAIDSimulator::stripe(const std::vector<uint8_t>& data) {
    std::vector<std::vector<uint8_t>> chunks;
    chunks.resize(num_shards_);

    size_t chunk_idx = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        chunks[chunk_idx].push_back(data[i]);
        chunk_idx = (chunk_idx + 1) % num_shards_;
    }

    return chunks;
}

std::vector<std::vector<uint8_t>> RAIDSimulator::mirror(const std::vector<uint8_t>& data,
                                                         int replication_factor) {
    std::vector<std::vector<uint8_t>> replicas;
    replicas.reserve(replication_factor);

    for (int i = 0; i < replication_factor; ++i) {
        replicas.push_back(data);
    }

    return replicas;
}

std::vector<std::vector<uint8_t>> RAIDSimulator::encodeWithParity(
    const std::vector<uint8_t>& data) {
    constexpr int MIN_RAID5_SHARDS = 3;  // Minimum shards for RAID 5 (2 data + 1 parity)
    if (num_shards_ < MIN_RAID5_SHARDS) {
        return {};  // Need at least 3 shards for RAID 5
    }

    // Stripe data across n-1 shards
    int data_shards = num_shards_ - 1;
    std::vector<std::vector<uint8_t>> chunks;
    chunks.resize(data_shards);

    size_t chunk_idx = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        chunks[chunk_idx].push_back(data[i]);
        chunk_idx = (chunk_idx + 1) % data_shards;
    }

    // Pad chunks to same size
    size_t max_size = 0;
    for (const auto& chunk : chunks) {
        max_size = std::max(max_size, chunk.size());
    }
    for (auto& chunk : chunks) {
        chunk.resize(max_size, 0);
    }

    // Calculate parity
    auto parity = calculateParity(chunks);
    chunks.push_back(parity);

    return chunks;
}

std::vector<uint8_t> RAIDSimulator::calculateParity(
    const std::vector<std::vector<uint8_t>>& chunks) const {
    if (chunks.empty()) {
        return {};
    }

    size_t parity_size = chunks[0].size();
    std::vector<uint8_t> parity(parity_size, 0);

    for (const auto& chunk : chunks) {
        for (size_t i = 0; i < std::min(parity_size, chunk.size()); ++i) {
            parity[i] ^= chunk[i];
        }
    }

    return parity;
}

std::optional<std::vector<uint8_t>> RAIDSimulator::reconstruct(
    const std::vector<std::optional<std::vector<uint8_t>>>& chunks) {
    switch (mode_) {
        case RAIDMode::STRIPE:
            return reconstructRAID0(chunks);
        case RAIDMode::MIRROR:
            return reconstructRAID1(chunks);
        case RAIDMode::PARITY:
            return reconstructRAID5(chunks);
        case RAIDMode::HYBRID:
            return reconstructRAID10(chunks);
        default:
            return std::nullopt;
    }
}

std::optional<std::vector<uint8_t>> RAIDSimulator::reconstructRAID0(
    const std::vector<std::optional<std::vector<uint8_t>>>& chunks) const {
    // RAID 0 requires ALL chunks to be present
    std::vector<uint8_t> result;

    for (const auto& chunk_opt : chunks) {
        if (!chunk_opt) {
            return std::nullopt;  // Missing chunk - cannot reconstruct
        }
        result.insert(result.end(), chunk_opt->begin(), chunk_opt->end());
    }

    return result;
}

std::optional<std::vector<uint8_t>> RAIDSimulator::reconstructRAID1(
    const std::vector<std::optional<std::vector<uint8_t>>>& chunks) const {
    // RAID 1 needs at least one replica
    for (const auto& chunk_opt : chunks) {
        if (chunk_opt) {
            return *chunk_opt;
        }
    }

    return std::nullopt;  // All replicas missing
}

std::optional<std::vector<uint8_t>> RAIDSimulator::reconstructRAID5(
    const std::vector<std::optional<std::vector<uint8_t>>>& chunks) const {
    if (chunks.size() < 3) {
        return std::nullopt;
    }

    // Count missing chunks
    int missing_count = 0;
    int missing_idx = -1;
    for (size_t i = 0; i < chunks.size(); ++i) {
        if (!chunks[i]) {
            missing_count++;
            missing_idx = static_cast<int>(i);
        }
    }

    // RAID 5 can recover from at most 1 missing chunk
    if (missing_count > 1) {
        return std::nullopt;
    }

    // If no missing chunks, just reconstruct
    if (missing_count == 0) {
        std::vector<uint8_t> result = {};

        for (size_t i = 0; i < chunks.size() - 1; ++i) {  // Exclude parity
            if (chunks[i]) {
                result.insert(result.end(), chunks[i]->begin(), chunks[i]->end());
            }
        }
        // Remove padding zeros
        while (!result.empty() && result.back() == 0) {
            result.pop_back();
        }
        return result;
    }

    // Reconstruct missing chunk using XOR of all others
    size_t chunk_size = 0;
    for (const auto& chunk : chunks) {
        if (chunk) {
            chunk_size = chunk->size();
            break;
        }
    }

    std::vector<uint8_t> reconstructed(chunk_size, 0);
    for (size_t i = 0; i < chunks.size(); ++i) {
        if (i != static_cast<size_t>(missing_idx) && chunks[i]) {
            for (size_t j = 0; j < chunk_size; ++j) {
                reconstructed[j] ^= (*chunks[i])[j];
            }
        }
    }

    // Build complete data
    std::vector<uint8_t> result = {};

    for (size_t i = 0; i < chunks.size() - 1; ++i) {  // Exclude parity
        if (i == static_cast<size_t>(missing_idx)) {
            result.insert(result.end(), reconstructed.begin(), reconstructed.end());
        } else if (chunks[i]) {
            result.insert(result.end(), chunks[i]->begin(), chunks[i]->end());
        }
    }

    // Remove padding zeros
    while (!result.empty() && result.back() == 0) {
        result.pop_back();
    }

    return result;
}

std::optional<std::vector<uint8_t>> RAIDSimulator::reconstructRAID10(
    const std::vector<std::optional<std::vector<uint8_t>>>& chunks) const {
    // RAID 10: Assume even number of chunks (pairs)
    if (chunks.size() % 2 != 0) {
        return std::nullopt;
    }

    std::vector<std::optional<std::vector<uint8_t>>> stripe_chunks;
    stripe_chunks.reserve(chunks.size() / 2);

    // Each pair is mirrored - use first available
    for (size_t i = 0; i < chunks.size(); i += 2) {
        if (chunks[i]) {
            stripe_chunks.push_back(chunks[i]);
        } else if (i + 1 < chunks.size() && chunks[i + 1]) {
            stripe_chunks.push_back(chunks[i + 1]);
        } else {
            stripe_chunks.push_back(std::nullopt);
        }
    }

    // Now reconstruct as RAID 0
    return reconstructRAID0(stripe_chunks);
}

void RAIDSimulator::failShards(const std::vector<int>& shard_ids) {
    for (int id : shard_ids) {
        if (std::find(failed_shards_.begin(), failed_shards_.end(), id) ==
            failed_shards_.end()) {
            failed_shards_.push_back(id);
        }
    }
}

bool RAIDSimulator::canRecover(const std::vector<int>& failed_shards) const {
    int max_failures = getMaxTolerableFailures();
    return static_cast<int>(failed_shards.size()) <= max_failures;
}

int RAIDSimulator::getMaxTolerableFailures() const {
    switch (mode_) {
        case RAIDMode::STRIPE:
            return 0;  // RAID 0 cannot tolerate any failures
        case RAIDMode::MIRROR:
            return num_shards_ - 1;  // RAID 1 can lose all but one
        case RAIDMode::PARITY:
            return 1;  // RAID 5 can lose one disk
        case RAIDMode::HYBRID:
            return num_shards_ / 2;  // RAID 10 can lose one from each mirror pair
        default:
            return 0;
    }
}

void RAIDSimulator::recoverShards(const std::vector<int>& shard_ids) {
    for (int id : shard_ids) {
        auto it = std::find(failed_shards_.begin(), failed_shards_.end(), id);
        if (it != failed_shards_.end()) {
            failed_shards_.erase(it);
        }
    }
}

std::vector<int> RAIDSimulator::getFailedShards() const {
    return failed_shards_;
}

} // namespace test
} // namespace themis
