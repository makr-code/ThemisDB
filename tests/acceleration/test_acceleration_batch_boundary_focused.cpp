#include <gtest/gtest.h>

#include "acceleration/batch_validator.h"

#include <limits>
#include <vector>

namespace {

using namespace themis::acceleration;

TEST(AccelerationBatchBoundaryTest, EmptyDistanceOutputNeedsNoBufferOrError) {
    std::vector<ErrorContext> errors;
    EXPECT_TRUE(BatchValidator::validateGeoDistanceResults(
        "cpu", nullptr, 0, [&](ErrorContext error) { errors.push_back(error); }));
    EXPECT_TRUE(errors.empty());
}

TEST(AccelerationBatchBoundaryTest, DistanceEndpointsAreInclusive) {
    const float values[]{0.0f, BatchValidator::kMaxGeodesicDistanceKm};
    std::vector<ErrorContext> errors;
    EXPECT_TRUE(BatchValidator::validateGeoDistanceResults(
        "cpu", values, 2, [&](ErrorContext error) { errors.push_back(error); }));
    EXPECT_TRUE(errors.empty());
}

TEST(AccelerationBatchBoundaryTest, RejectsNonFiniteAndNegativeDistancesAfterValidPrefix) {
    for (float invalid : {std::numeric_limits<float>::quiet_NaN(),
                          std::numeric_limits<float>::infinity(),
                          -std::numeric_limits<float>::infinity(), -1.0f}) {
        const float values[]{1.0f, invalid, 2.0f};
        std::vector<ErrorContext> errors;
        EXPECT_FALSE(BatchValidator::validateGeoDistanceResults(
            "cpu", values, 3, [&](ErrorContext error) { errors.push_back(error); }));
        ASSERT_EQ(errors.size(), 1u);
        EXPECT_EQ(errors.front().code, AccelerationErrorCode::InputRangeViolation);
        EXPECT_NE(errors.front().message.find("index 1"), std::string::npos);
    }
}

TEST(AccelerationBatchBoundaryTest, ShortestPathIgnoresInvalidWeightsForAbsentEdges) {
    const uint32_t adjacency[]{0, 1, 0, 0};
    const float weights[]{std::numeric_limits<float>::quiet_NaN(), 1.0f,
                          -1.0f, std::numeric_limits<float>::infinity()};
    EXPECT_FALSE(BatchValidator::shouldUseCpuFallbackForShortestPath(
        adjacency, weights, 2));
    const uint32_t with_invalid_edge[]{0, 1, 1, 0};
    EXPECT_TRUE(BatchValidator::shouldUseCpuFallbackForShortestPath(
        with_invalid_edge, weights, 2));
}

} // namespace
