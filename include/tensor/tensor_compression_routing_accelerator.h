#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace themis::tensor {

struct TensorAccelerationStats {
    std::size_t gpu_compression_calls = 0;
    std::size_t gpu_routing_calls = 0;
    std::size_t cpu_fallback_calls = 0;
    std::size_t gpu_error_calls = 0;
};

/**
 * @brief Compression + routing score accelerator with CUDA dispatch and CPU fallback.
 */

/**
 * @file tensor_compression_routing_accelerator.h
 * @brief Hardware-accelerated routing for tensor compression operations.
 *
 * Declares TensorCompressionRoutingAccelerator, which selects the
 * optimal compression backend (CPU, CUDA, AVX-512) at runtime.
 */
class TensorCompressionRoutingAccelerator {
public:
    TensorCompressionRoutingAccelerator() = default;

    [[nodiscard]] std::vector<int8_t> compressToInt8(const std::vector<float>& input,
                                                     float scale,
                                                     bool force_cpu = false);

    [[nodiscard]] std::vector<float> computeRoutingScores(const std::vector<float>& tensor,
                                                          const std::vector<std::vector<float>>& route_weights,
                                                          bool force_cpu = false);

    [[nodiscard]] TensorAccelerationStats stats() const noexcept { return stats_; }

private:
    [[nodiscard]] std::vector<int8_t> compressToInt8Cpu(const std::vector<float>& input, float scale) const;
    [[nodiscard]] std::vector<float> computeRoutingScoresCpu(const std::vector<float>& tensor,
                                                             const std::vector<std::vector<float>>& route_weights) const;

    TensorAccelerationStats stats_{};
};

}  // namespace themis::tensor
