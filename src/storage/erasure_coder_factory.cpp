#include "sharding/redundancy_strategy.h"

#include <algorithm>
#include <stdexcept>

namespace themis {
namespace sharding {

namespace {

class LocalXorErasureCoder final : public ErasureCoder {
public:
    std::vector<std::vector<uint8_t>> encode(const std::vector<uint8_t>& data,
                                             uint32_t data_shards,
                                             uint32_t parity_shards) override {
        if (data_shards == 0 || parity_shards == 0) {
            throw std::invalid_argument("ErasureCoder: data_shards/parity_shards must be > 0");
        }

        const size_t shard_size = (data.size() + data_shards - 1) / data_shards;
        std::vector<std::vector<uint8_t>> chunks(data_shards + parity_shards,
                                                 std::vector<uint8_t>(shard_size, 0));

        for (uint32_t i = 0; i < data_shards; ++i) {
            const size_t begin = static_cast<size_t>(i) * shard_size;
            if (begin >= data.size()) break;
            const size_t end = std::min(begin + shard_size, data.size());
            std::copy(data.begin() + static_cast<std::ptrdiff_t>(begin),
                      data.begin() + static_cast<std::ptrdiff_t>(end),
                      chunks[i].begin());
        }

        std::vector<uint8_t> parity(shard_size, 0);
        for (uint32_t i = 0; i < data_shards; ++i) {
            for (size_t j = 0; j < shard_size; ++j) {
                parity[j] ^= chunks[i][j];
            }
        }
        for (uint32_t p = 0; p < parity_shards; ++p) {
            chunks[data_shards + p] = parity;
        }

        return chunks;
    }

    std::vector<uint8_t> decode(const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
                                const std::vector<uint32_t>&,
                                uint32_t data_shards,
                                uint32_t parity_shards) override {
        if (available_chunks.empty()) {
            throw std::runtime_error("ErasureCoder: no chunks available");
        }

        const size_t shard_size = available_chunks.begin()->second.size();
        std::vector<std::vector<uint8_t>> data_chunks(data_shards,
                                                      std::vector<uint8_t>(shard_size, 0));
        std::vector<uint8_t> parity(shard_size, 0);
        bool has_parity = false;

        uint32_t missing_data = 0;
        uint32_t missing_idx = 0;

        for (uint32_t i = 0; i < data_shards; ++i) {
            auto it = available_chunks.find(i);
            if (it == available_chunks.end()) {
                ++missing_data;
                missing_idx = i;
                continue;
            }
            data_chunks[i] = it->second;
        }

        for (uint32_t p = 0; p < parity_shards; ++p) {
            auto it = available_chunks.find(data_shards + p);
            if (it != available_chunks.end()) {
                parity = it->second;
                has_parity = true;
                break;
            }
        }

        if (missing_data > 1) {
            throw std::runtime_error("ErasureCoder: local fallback supports recovery of only one missing data shard");
        }
        if (missing_data == 1) {
            if (!has_parity) {
                throw std::runtime_error("ErasureCoder: missing parity shard for recovery");
            }
            std::vector<uint8_t> recovered = parity;
            for (uint32_t i = 0; i < data_shards; ++i) {
                if (i == missing_idx) continue;
                for (size_t j = 0; j < shard_size; ++j) {
                    recovered[j] ^= data_chunks[i][j];
                }
            }
            data_chunks[missing_idx] = std::move(recovered);
        }

        std::vector<uint8_t> out;
        out.reserve(static_cast<size_t>(data_shards) * shard_size);
        for (uint32_t i = 0; i < data_shards; ++i) {
            out.insert(out.end(), data_chunks[i].begin(), data_chunks[i].end());
        }
        return out;
    }
};

} // namespace

std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm) {
    return std::make_unique<LocalXorErasureCoder>();
}

} // namespace sharding
} // namespace themis
