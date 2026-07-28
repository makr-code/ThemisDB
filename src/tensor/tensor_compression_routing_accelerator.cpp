/**
 * @file tensor_compression_routing_accelerator.cpp
 * @brief Tensor compression routing accelerator implementation.
 *
 * Implements backend selection logic, dispatch tables, and
 * registration of platform-specific compression kernels.
 */

#include "tensor/tensor_compression_routing_accelerator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#if defined(THEMIS_ENABLE_CUDA) && THEMIS_ENABLE_CUDA
extern "C" int themisCudaCompressToInt8Host(const float* input, int8_t* output, int n, float scale);
extern "C" int themisCudaComputeRoutingScoresHost(const float* tensor,
                                                   const float* route_weights,
                                                   float* scores,
                                                   int num_routes,
                                                   int dim);
#endif

namespace themis::tensor {

std::vector<int8_t>
TensorCompressionRoutingAccelerator::compressToInt8(const std::vector<float>& input,
                                                    float scale,
                                                    bool force_cpu) {
    if (input.empty() || scale <= 0.0f) {
        return {};
    }

#if defined(THEMIS_ENABLE_CUDA) && THEMIS_ENABLE_CUDA
    if (!force_cpu) {
        std::vector<int8_t> out(input.size(), 0);
        const int rc = themisCudaCompressToInt8Host(input.data(), out.data(), static_cast<int>(input.size()), scale);
        if (rc == 0) {
            ++stats_.gpu_compression_calls;
            return out;
        }
        ++stats_.gpu_error_calls;
    }
#else
    (void)force_cpu;
#endif

    ++stats_.cpu_fallback_calls;
    return compressToInt8Cpu(input, scale);
}

std::vector<float>
TensorCompressionRoutingAccelerator::computeRoutingScores(
    const std::vector<float>& tensor,
    const std::vector<std::vector<float>>& route_weights,
    bool force_cpu) {
    if (tensor.empty() || route_weights.empty()) {
        return {};
    }

    const std::size_t dim = tensor.size();
    for (const auto& route : route_weights) {
        if (route.size() != dim) {
            return {};
        }
    }

#if defined(THEMIS_ENABLE_CUDA) && THEMIS_ENABLE_CUDA
    if (!force_cpu) {
        std::vector<float> packed;
        packed.reserve(route_weights.size() * dim);
        for (const auto& route : route_weights) {
            packed.insert(packed.end(), route.begin(), route.end());
        }

        std::vector<float> out(route_weights.size(), 0.0f);
        const int rc = themisCudaComputeRoutingScoresHost(tensor.data(),
                                                          packed.data(),
                                                          out.data(),
                                                          static_cast<int>(route_weights.size()),
                                                          static_cast<int>(dim));
        if (rc == 0) {
            ++stats_.gpu_routing_calls;
            return out;
        }
        ++stats_.gpu_error_calls;
    }
#else
    (void)force_cpu;
#endif

    ++stats_.cpu_fallback_calls;
    return computeRoutingScoresCpu(tensor, route_weights);
}

std::vector<int8_t>
TensorCompressionRoutingAccelerator::compressToInt8Cpu(const std::vector<float>& input,
                                                       float scale) const {
    std::vector<int8_t> out;
    out.reserve(input.size());

    for (const float v : input) {
        const float q = std::round(v / scale);
        const float clamped = std::max<float>(-127.0f, std::min<float>(127.0f, q));
        out.push_back(static_cast<int8_t>(clamped));
    }

    return out;
}

std::vector<float>
TensorCompressionRoutingAccelerator::computeRoutingScoresCpu(
    const std::vector<float>& tensor,
    const std::vector<std::vector<float>>& route_weights) const {
    std::vector<float> scores(route_weights.size(), 0.0f);
    for (std::size_t r = 0; r < route_weights.size(); ++r) {
        float dot = 0.0f;
        for (std::size_t i = 0; i < tensor.size(); ++i) {
            dot += tensor[i] * route_weights[r][i];
        }
        scores[r] = dot;
    }
    return scores;
}

}  // namespace themis::tensor
