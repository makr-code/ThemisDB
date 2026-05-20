/*
 * ThemisDB | File: hamming_coder.cpp | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 194
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=60 | delta=57 | status=divergent
 * External Severity (v3): C=2, H=48, M=10
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * ThemisDB - Hamming erasure coder implementation for storage module.
 *
 * This translation unit provides HammingCoder::encode/decode so storage-local
 * factories can resolve symbols without depending on the sharding module link
 * order. The algorithm matches the shard-level XOR behavior used in sharding.
 */

#include "sharding/redundancy_strategy.h"

#include <algorithm>
#include <stdexcept>

namespace themis::sharding {
namespace {

// Parity shard p (0-indexed) covers data shard j (0-indexed) when bit p is
// set in the 1-based position (j + 1).
inline bool hammingCovers(const uint32_t j, const uint32_t p) noexcept {
    return (((j + 1u) >> p) & 1u) != 0u;
}

} // namespace

std::vector<std::vector<uint8_t>> HammingCoder::encode(
    const std::vector<uint8_t>& data,
    const uint32_t data_shards,
    const uint32_t parity_shards) {
    if (data_shards == 0 || parity_shards == 0) {
        throw std::invalid_argument("HammingCoder::encode: shard counts must be > 0");
    }
    if (data.empty()) {
        throw std::invalid_argument("HammingCoder::encode: data must not be empty");
    }

    const uint32_t shard_size = static_cast<uint32_t>((data.size() + data_shards - 1) / data_shards);
    const uint32_t total_shards = data_shards + parity_shards;

    std::vector<std::vector<uint8_t>> shards(total_shards, std::vector<uint8_t>(shard_size, 0));

    for (uint32_t s = 0; s < data_shards; ++s) {
        const uint32_t start = s * shard_size;
        const uint32_t end = std::min(start + shard_size, static_cast<uint32_t>(data.size()));
        if (start < static_cast<uint32_t>(data.size())) {
            std::copy(data.begin() + start, data.begin() + end, shards[s].begin());
        }
    }

    for (uint32_t p = 0; p < parity_shards; ++p) {
        auto& parity = shards[data_shards + p];
        for (uint32_t j = 0; j < data_shards; ++j) {
            if (!hammingCovers(j, p)) {
                continue;
            }
            for (uint32_t b = 0; b < shard_size; ++b) {
                parity[b] ^= shards[j][b];
            }
        }
    }

    return shards;
}

std::vector<uint8_t> HammingCoder::decode(
    const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
    const std::vector<uint32_t>& missing_indices,
    const uint32_t data_shards,
    const uint32_t parity_shards) {
    if (available_chunks.empty()) {
        throw std::runtime_error("HammingCoder::decode: no chunks available");
    }

    const uint32_t total_shards = data_shards + parity_shards;
    const uint32_t shard_size = static_cast<uint32_t>(available_chunks.begin()->second.size());

    if (missing_indices.empty()) {
        std::vector<uint8_t> result;
        result.reserve(static_cast<size_t>(data_shards) * shard_size);
        for (uint32_t s = 0; s < data_shards; ++s) {
            const auto it = available_chunks.find(s);
            if (it == available_chunks.end()) {
                throw std::runtime_error(
                    "HammingCoder::decode: data shard " + std::to_string(s) +
                    " missing but not listed in missing_indices");
            }
            result.insert(result.end(), it->second.begin(), it->second.end());
        }
        return result;
    }

    std::vector<std::vector<uint8_t>> shards(total_shards, std::vector<uint8_t>(shard_size, 0));
    std::vector<bool> present(total_shards, false);
    for (const auto& [idx, chunk] : available_chunks) {
        if (idx < total_shards) {
            shards[idx] = chunk;
            present[idx] = true;
        }
    }

    bool progress = true;
    while (progress) {
        progress = false;

        for (uint32_t target = 0; target < total_shards; ++target) {
            if (present[target]) {
                continue;
            }

            if (target >= data_shards) {
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
                        if (!hammingCovers(j, p)) {
                            continue;
                        }
                        for (uint32_t b = 0; b < shard_size; ++b) {
                            shards[target][b] ^= shards[j][b];
                        }
                    }
                    present[target] = true;
                    progress = true;
                }
                continue;
            }

            for (uint32_t p = 0; p < parity_shards; ++p) {
                if (!present[data_shards + p] || !hammingCovers(target, p)) {
                    continue;
                }

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

                shards[target] = shards[data_shards + p];
                for (uint32_t j = 0; j < data_shards; ++j) {
                    if (j == target || !hammingCovers(j, p)) {
                        continue;
                    }
                    for (uint32_t b = 0; b < shard_size; ++b) {
                        shards[target][b] ^= shards[j][b];
                    }
                }
                present[target] = true;
                progress = true;
                break;
            }
        }
    }

    for (uint32_t s = 0; s < data_shards; ++s) {
        if (!present[s]) {
            throw std::runtime_error(
                "HammingCoder::decode: cannot recover data shard " +
                std::to_string(s) + " - too many simultaneous failures");
        }
    }

    std::vector<uint8_t> result;
    result.reserve(static_cast<size_t>(data_shards) * shard_size);
    for (uint32_t s = 0; s < data_shards; ++s) {
        result.insert(result.end(), shards[s].begin(), shards[s].end());
    }

    return result;
}

} // namespace themis::sharding
