#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <vector>

#include "acceleration/cpu_backend.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif

using namespace themis::acceleration;

namespace {

std::vector<uint32_t> sortedCopy(std::vector<uint32_t> values) {
    std::sort(values.begin(), values.end());
    return values;
}

} // namespace

TEST(CategoryBGraphParityBFS, CudaVsCpuParity_MultiStartDepthThree) {
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

    constexpr size_t kNumVertices = 6;
    std::vector<uint32_t> adjacency(kNumVertices * kNumVertices, 0u);
    adjacency[0 * kNumVertices + 1] = 1u;
    adjacency[0 * kNumVertices + 2] = 1u;
    adjacency[1 * kNumVertices + 3] = 1u;
    adjacency[2 * kNumVertices + 4] = 1u;
    adjacency[4 * kNumVertices + 5] = 1u;

    const std::array<uint32_t, 2> starts = {0u, 2u};
    constexpr uint32_t kMaxDepth = 3u;

    const auto cpu_result =
        cpu_backend.batchBFS(adjacency.data(), kNumVertices, starts.data(), starts.size(), kMaxDepth);
    const auto gpu_result =
        gpu_backend.batchBFS(adjacency.data(), kNumVertices, starts.data(), starts.size(), kMaxDepth);

    ASSERT_EQ(cpu_result.size(), starts.size());
    ASSERT_EQ(gpu_result.size(), starts.size());

    for (std::size_t i = 0; i < starts.size(); ++i) {
        EXPECT_EQ(sortedCopy(gpu_result[i]), sortedCopy(cpu_result[i]))
            << "BFS parity mismatch for start vertex " << starts[i];
    }

    gpu_backend.shutdown();
    cpu_backend.shutdown();
#endif
}

TEST(CategoryBGraphParityBFS, CudaVsCpuParity_MaxDepthOneFrontierCutoff) {
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
    adjacency[0 * kNumVertices + 1] = 1u;
    adjacency[0 * kNumVertices + 2] = 1u;
    adjacency[1 * kNumVertices + 3] = 1u;
    adjacency[2 * kNumVertices + 4] = 1u;

    const std::array<uint32_t, 1> starts = {0u};
    constexpr uint32_t kMaxDepth = 1u;

    const auto cpu_result =
        cpu_backend.batchBFS(adjacency.data(), kNumVertices, starts.data(), starts.size(), kMaxDepth);
    const auto gpu_result =
        gpu_backend.batchBFS(adjacency.data(), kNumVertices, starts.data(), starts.size(), kMaxDepth);

    ASSERT_EQ(cpu_result.size(), 1u);
    ASSERT_EQ(gpu_result.size(), 1u);
    const std::vector<uint32_t> expected = {0u, 1u, 2u};

    EXPECT_EQ(sortedCopy(cpu_result[0]), expected);
    EXPECT_EQ(sortedCopy(gpu_result[0]), expected);

    gpu_backend.shutdown();
    cpu_backend.shutdown();
#endif
}
