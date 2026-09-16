#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

#include "index/advanced_vector_index.h"

namespace {

std::vector<float> buildVectors(std::size_t count, std::size_t dim) {
    std::vector<float> data(count * dim);
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t j = 0; j < dim; ++j) {
            data[i * dim + j] = static_cast<float>((i * 17 + j * 13) % 997) / 997.0F;
        }
    }
    return data;
}

} // namespace

TEST(AdvancedVectorIndexCudaKnn, CudaAndCpuResultsStayWithinParityTolerance) {
#if !defined(THEMIS_ENABLE_CUDA) || !defined(THEMIS_HAS_FAISS) || !defined(THEMIS_ENABLE_CUVS)
    GTEST_SKIP() << "Requires THEMIS_ENABLE_CUDA, THEMIS_HAS_FAISS, and THEMIS_ENABLE_CUVS";
#else
    constexpr std::size_t dimension = 64;
    constexpr std::size_t train_count = 768;
    constexpr std::size_t search_k = 16;
    constexpr float distance_tolerance = 1e-5F;

    themis::AdvancedVectorIndex::Config cpu_cfg;
    cpu_cfg.use_gpu = false;
    cpu_cfg.use_pq = false;
    cpu_cfg.nlist = 32;
    cpu_cfg.nprobe = 32;
    cpu_cfg.train_size = train_count;
    cpu_cfg.index_type = themis::AdvancedVectorIndex::Config::Type::IVF_FLAT;

    themis::AdvancedVectorIndex::Config gpu_cfg = cpu_cfg;
    gpu_cfg.use_gpu = true;
    gpu_cfg.gpu_device = 0;

    auto training = buildVectors(train_count, dimension);
    std::vector<int64_t> ids(train_count);
    std::iota(ids.begin(), ids.end(), int64_t{0});

    themis::AdvancedVectorIndex cpu_index(dimension, cpu_cfg);
    ASSERT_TRUE(cpu_index.train(training.data(), train_count));
    ASSERT_TRUE(cpu_index.addWithIds(training.data(), ids.data(), train_count));

    themis::AdvancedVectorIndex gpu_index(dimension, gpu_cfg);
    ASSERT_TRUE(gpu_index.train(training.data(), train_count));
    ASSERT_TRUE(gpu_index.addWithIds(training.data(), ids.data(), train_count));

    std::vector<float> query(dimension);
    for (std::size_t j = 0; j < dimension; ++j) {
        query[j] = training[(123 * dimension) + j] + 0.0005F;
    }

    const auto cpu_result = cpu_index.search(query.data(), search_k);
    const auto gpu_result = gpu_index.search(query.data(), search_k);

    ASSERT_EQ(cpu_result.ids.size(), search_k);
    ASSERT_EQ(gpu_result.ids.size(), search_k);
    ASSERT_EQ(cpu_result.distances.size(), search_k);
    ASSERT_EQ(gpu_result.distances.size(), search_k);

    std::vector<bool> matched_cpu(cpu_result.distances.size(), false);
    for (const auto gpu_distance : gpu_result.distances) {
        bool found_match = false;
        for (std::size_t i = 0; i < cpu_result.distances.size(); ++i) {
            if (matched_cpu[i]) {
                continue;
            }
            if (std::abs(gpu_distance - cpu_result.distances[i]) <= distance_tolerance) {
                matched_cpu[i] = true;
                found_match = true;
                break;
            }
        }
        EXPECT_TRUE(found_match) << "No CPU distance match found for GPU distance " << gpu_distance;
    }
    for (std::size_t i = 0; i < matched_cpu.size(); ++i) {
        EXPECT_TRUE(matched_cpu[i]) << "Unmatched CPU result index " << i
                                    << " distance=" << cpu_result.distances[i];
    }
#endif
}
