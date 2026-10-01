/**
 * @file batch_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <functional>
#include <limits>
#include <string>

namespace themis {
namespace acceleration {

struct BatchValidator {
    using ErrorSink = std::function<void(ErrorContext)>;
    static constexpr double kMinLatitude  = -90.0;
    static constexpr double kMaxLatitude  = 90.0;
    static constexpr double kMinLongitude = -180.0;
    static constexpr double kMaxLongitude = 180.0;
    static constexpr float kMaxGeodesicDistanceKm = 20039.5f;
    static constexpr uint32_t kBfsMaxDepth = 3;
    static constexpr size_t kBfsMaxNodesPerHop = 10'000;


    /**
     * @brief Validate Vector Batch.
     * @param[in] backendName Input parameter.
     * @param[in] queries Input parameter.
     * @param[in] numQueries Input parameter.
     * @param[in] dim Input parameter.
     * @param[in] vectors Input parameter.
     * @param[in] numVectors Input parameter.
     * @param[in] onError Input parameter.
     * @return True when the operation succeeds.
     */
    static bool validateVectorBatch(
        const char*   backendName,
        const float*  queries,
        size_t        numQueries,
        size_t        dim,
        const float*  vectors,
        size_t        numVectors,
        const ErrorSink& onError)
    {
        if (queries == nullptr || vectors == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "queries and vectors pointers must be non-null"));
            return false;
        }
        if (numQueries == 0 || numVectors == 0 || dim == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "numQueries, numVectors, and dim must all be > 0"));
            return false;
        }
        return true;
    }

    /**
     * @brief Validate K.
     * @param[in] backendName Input parameter.
     * @param[in] k Input parameter.
     * @param[in] onError Input parameter.
     * @return True when the operation succeeds.
     */
    static bool validateK(
        const char*      backendName,
        size_t           k,
        const ErrorSink& onError)
    {
        if (k == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "k must be > 0"));
            return false;
        }
        return true;
    }


    /**
     * @brief Validate Geo Batch.
     * @param[in] backendName Input parameter.
     * @param[in] lats1 Input parameter.
     * @param[in] lons1 Input parameter.
     * @param[in] lats2 Input parameter.
     * @param[in] lons2 Input parameter.
     * @param[in] count Input parameter.
     * @param[in] onError Input parameter.
     * @return True when the operation succeeds.
     */
    static bool validateGeoBatch(
        const char*    backendName,
        const double*  lats1,
        const double*  lons1,
        const double*  lats2,
        const double*  lons2,
        size_t         count,
        const ErrorSink& onError)
    {
        if (lats1 == nullptr || lons1 == nullptr ||
            lats2 == nullptr || lons2 == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "latitude/longitude pointers must be non-null"));
            return false;
        }
        if (count == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "count must be > 0"));
            return false;
        }
        for (size_t i = 0; i < count; ++i) {
            const bool lat1Ok = std::isfinite(lats1[i]) && lats1[i] >= kMinLatitude && lats1[i] <= kMaxLatitude;
            const bool lon1Ok = std::isfinite(lons1[i]) && lons1[i] >= kMinLongitude && lons1[i] <= kMaxLongitude;
            const bool lat2Ok = std::isfinite(lats2[i]) && lats2[i] >= kMinLatitude && lats2[i] <= kMaxLatitude;
            const bool lon2Ok = std::isfinite(lons2[i]) && lons2[i] >= kMinLongitude && lons2[i] <= kMaxLongitude;
            if (!(lat1Ok && lon1Ok && lat2Ok && lon2Ok)) {
                onError(ErrorContextHelpers::createValidationError(
                    backendName,
                    AccelerationErrorCode::InputRangeViolation,
                    "latitude/longitude must be finite and within WGS84 bounds at index "
                        + std::to_string(i)));
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Validate Point In Polygon Batch.
     * @param[in] backendName Input parameter.
     * @param[in] pointLats Input parameter.
     * @param[in] pointLons Input parameter.
     * @param[in] numPoints Input parameter.
     * @param[in] polygonCoords Input parameter.
     * @param[in] numPolygonVertices Input parameter.
     * @param[in] onError Input parameter.
     * @return True when the operation succeeds.
     */
    static bool validatePointInPolygonBatch(
        const char*    backendName,
        const double*  pointLats,
        const double*  pointLons,
        size_t         numPoints,
        const double*  polygonCoords,
        size_t         numPolygonVertices,
        const ErrorSink& onError)
    {
        if (pointLats == nullptr || pointLons == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "point latitude/longitude pointers must be non-null"));
            return false;
        }
        if (polygonCoords == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "polygonCoords pointer must be non-null"));
            return false;
        }
        if (numPoints == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "numPoints must be > 0"));
            return false;
        }
        if (numPolygonVertices < 3) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "polygon must contain at least 3 vertices"));
            return false;
        }
        for (size_t i = 0; i < numPoints; ++i) {
            const bool latOk = std::isfinite(pointLats[i]) && pointLats[i] >= kMinLatitude && pointLats[i] <= kMaxLatitude;
            const bool lonOk = std::isfinite(pointLons[i]) && pointLons[i] >= kMinLongitude && pointLons[i] <= kMaxLongitude;
            if (!(latOk && lonOk)) {
                onError(ErrorContextHelpers::createValidationError(
                    backendName,
                    AccelerationErrorCode::InputRangeViolation,
                    "point latitude/longitude must be finite and within WGS84 bounds at index "
                        + std::to_string(i)));
                return false;
            }
        }
        for (size_t i = 0; i < numPolygonVertices; ++i) {
            const double lat = polygonCoords[i * 2];
            const double lon = polygonCoords[i * 2 + 1];
            const bool latOk = std::isfinite(lat) && lat >= kMinLatitude && lat <= kMaxLatitude;
            const bool lonOk = std::isfinite(lon) && lon >= kMinLongitude && lon <= kMaxLongitude;
            if (!(latOk && lonOk)) {
                onError(ErrorContextHelpers::createValidationError(
                    backendName,
                    AccelerationErrorCode::InputRangeViolation,
                    "polygon latitude/longitude must be finite and within WGS84 bounds at vertex "
                        + std::to_string(i)));
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Validate Geo Distance Results.
     * @param[in] backendName Input parameter.
     * @param[in] distancesKm Input parameter.
     * @param[in] count Input parameter.
     * @param[in] onError Input parameter.
     * @return True when the operation succeeds.
     */
    static bool validateGeoDistanceResults(
        const char* backendName,
        const float* distancesKm,
        size_t count,
        const ErrorSink& onError)
    {
        if (distancesKm == nullptr && count > 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "distances pointer must be non-null when count > 0"));
            return false;
        }
        for (size_t i = 0; i < count; ++i) {
            const float v = distancesKm[i];
            if (!std::isfinite(v) || v < 0.0f || v > kMaxGeodesicDistanceKm) {
                onError(ErrorContextHelpers::createValidationError(
                    backendName,
                    AccelerationErrorCode::InputRangeViolation,
                    "distance output must be finite and within [0, 20039.5] km at index "
                        + std::to_string(i)));
                return false;
            }
        }
        return true;
    }


    /**
     * @brief Validate Graph BFSBatch.
     * @param[in] backendName Input parameter.
     * @param[in] adjacency Input parameter.
     * @param[in] numVertices Input parameter.
     * @param[in] startVertices Input parameter.
     * @param[in] numStarts Input parameter.
     * @param[in] onError Input parameter.
     * @return True when the operation succeeds.
     */
    static bool validateGraphBFSBatch(
        const char*      backendName,
        const uint32_t*  adjacency,
        size_t           numVertices,
        const uint32_t*  startVertices,
        size_t           numStarts,
        const ErrorSink& onError)
    {
        if (adjacency == nullptr || startVertices == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "adjacency and startVertices pointers must be non-null"));
            return false;
        }
        if (numVertices == 0 || numStarts == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "numVertices and numStarts must both be > 0"));
            return false;
        }
        return true;
    }

    /**
     * @brief Validate Graph BFSHop Limit.
     * @param[in] backendName Input parameter.
     * @param[in] maxDepth Input parameter.
     * @param[in] onError Input parameter.
     * @return True when the operation succeeds.
     */
    static bool validateGraphBFSHopLimit(
        const char* backendName,
        uint32_t maxDepth,
        const ErrorSink& onError)
    {
        if (maxDepth > kBfsMaxDepth) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::BatchSizeExceeded,
                "maxDepth exceeds BFS hop limit (max 3 hops)"));
            return false;
        }
        return true;
    }

    static bool shouldUseCpuFallbackForGraphBFS(size_t numVertices, uint32_t maxDepth) noexcept {
        return numVertices > kBfsMaxNodesPerHop || maxDepth > kBfsMaxDepth;
    }

    /**
     * @brief Validate Shortest Path Batch.
     * @param[in] backendName Input parameter.
     * @param[in] adjacency Input parameter.
     * @param[in] weights Input parameter.
     * @param[in] numVertices Input parameter.
     * @param[in] startVertices Input parameter.
     * @param[in] endVertices Input parameter.
     * @param[in] numPairs Input parameter.
     * @param[in] onError Input parameter.
     * @return True when the operation succeeds.
     */
    static bool validateShortestPathBatch(
        const char*      backendName,
        const uint32_t*  adjacency,
        const float*     weights,
        size_t           numVertices,
        const uint32_t*  startVertices,
        const uint32_t*  endVertices,
        size_t           numPairs,
        const ErrorSink& onError)
    {
        if (adjacency == nullptr || weights == nullptr ||
            startVertices == nullptr || endVertices == nullptr) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "adjacency, weights, startVertices, and endVertices pointers "
                "must be non-null"));
            return false;
        }
        if (numVertices == 0 || numPairs == 0) {
            onError(ErrorContextHelpers::createValidationError(
                backendName,
                AccelerationErrorCode::InvalidInputShape,
                "numVertices and numPairs must both be > 0"));
            return false;
        }
        return true;
    }

    static bool shouldUseCpuFallbackForShortestPath(
        const uint32_t* adjacency,
        const float* weights,
        size_t numVertices) noexcept
    {
        if (adjacency == nullptr || weights == nullptr || numVertices == 0) {
            return true;
        }
        constexpr float kFiniteCap = 1e37f;
        const double maxPathWeight = (numVertices > 1)
            ? static_cast<double>(kFiniteCap) / static_cast<double>(numVertices - 1)
            : static_cast<double>(kFiniteCap);
        for (size_t u = 0; u < numVertices; ++u) {
            for (size_t v = 0; v < numVertices; ++v) {
                const size_t idx = u * numVertices + v;
                if (adjacency[idx] == 0) {
                    continue;
                }
                const float w = weights[idx];
                if (!std::isfinite(w) || w < 0.0f || static_cast<double>(w) > maxPathWeight) {
                    return true;
                }
            }
        }
        return false;
    }
};

} // namespace acceleration
} // namespace themis
