#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "acceleration/cpu_backend.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif

using namespace themis::acceleration;

TEST(CategoryBGraphParityDijkstra, CudaVsCpuParity_ShortestPathsMatch) {
#ifndef THEMIS_ENABLE_CUDA
    GTEST_SKIP() << "THEMIS_ENABLE_CUDA is disabled";
#else
    CUDAGraphBackend gpu_backend;
    if (!gpu_backend.isAvailable()) {
        GTEST_SKIP() << "No CUDA-capable GPU available";
    }
    ASSERT_TRUE(gpu_backend.initialize());

    CPUGraphBackend cpu_backend;
    ASSERT_TRUE(cpu_backend.initialize());

    constexpr size_t kNumVertices = 5;
    std::vector<uint32_t> adjacency(kNumVertices * kNumVertices, 0u);
    std::vector<float> weights(kNumVertices * kNumVertices, 0.0f);

    // 0 -> 1 -> 4 (best path to 4), alternative 0 -> 2 -> 3 -> 4 is longer.
    adjacency[0 * kNumVertices + 1] = 1u; weights[0 * kNumVertices + 1] = 1.0f;
    adjacency[1 * kNumVertices + 4] = 1u; weights[1 * kNumVertices + 4] = 2.0f;
    adjacency[0 * kNumVertices + 2] = 1u; weights[0 * kNumVertices + 2] = 2.0f;
    adjacency[2 * kNumVertices + 3] = 1u; weights[2 * kNumVertices + 3] = 2.0f;
    adjacency[3 * kNumVertices + 4] = 1u; weights[3 * kNumVertices + 4] = 2.0f;
    adjacency[1 * kNumVertices + 3] = 1u; weights[1 * kNumVertices + 3] = 1.0f;

    const std::array<uint32_t, 3> starts = {0u, 0u, 1u};
    const std::array<uint32_t, 3> ends = {4u, 3u, 4u};

    const auto cpu_paths = cpu_backend.batchShortestPath(
        adjacency.data(), weights.data(), kNumVertices, starts.data(), ends.data(), starts.size());
    const auto gpu_paths = gpu_backend.batchShortestPath(
        adjacency.data(), weights.data(), kNumVertices, starts.data(), ends.data(), starts.size());

    ASSERT_EQ(cpu_paths.size(), starts.size());
    ASSERT_EQ(gpu_paths.size(), starts.size());

    for (std::size_t i = 0; i < starts.size(); ++i) {
        EXPECT_EQ(gpu_paths[i], cpu_paths[i])
            << "Shortest-path parity mismatch for pair index " << i
            << " (" << starts[i] << " -> " << ends[i] << ")";
    }

    gpu_backend.shutdown();
    cpu_backend.shutdown();
#endif
}
