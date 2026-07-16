#pragma once

#include <cub/cub.cuh>

namespace cuda_cosine {

constexpr int kTileSize = 32;
constexpr int kVecsPerBlock = 4;
constexpr int kQueriesPerBlock = 2;
constexpr int kNumReductions = 3; // dot, normQ, normV

template <typename ReductionType>
constexpr size_t CosineSharedBytes() {
    return sizeof(float) * (kTileSize * (kVecsPerBlock + kQueriesPerBlock)) +
           sizeof(typename cub::WarpReduce<ReductionType>::TempStorage) * kVecsPerBlock * kQueriesPerBlock * kNumReductions;
}

} // namespace cuda_cosine
