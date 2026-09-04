/**
 * @file cpu_backend_mt.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Multi-Threaded CPU Backend with OpenMP and SIMD optimizations
// Provides high-performance CPU-based acceleration for vector, graph, and geo operations
// Copyright (c) 2024 ThemisDB

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <queue>
#include <thread>

#include "acceleration/batch_validator.h"
#include "acceleration/cpu_backend.h"
#include "utils/simd_distance.h"

// OpenMP support (if available)
#ifdef _OPENMP
#include <omp.h>
#define THEMIS_HAS_OPENMP 1
#else
#define THEMIS_HAS_OPENMP 0
#endif

// SIMD support detection
#if defined(__AVX2__) || defined(__AVX512F__)
#define THEMIS_HAS_SIMD_X86 1
#include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__aarch64__)
#define THEMIS_HAS_SIMD_ARM 1
#include <arm_neon.h>
#else
#define THEMIS_HAS_SIMD 0
#endif

namespace themis {
namespace acceleration {

// ============================================================================
// Multi-Threaded CPUVectorBackend Implementation
// ============================================================================

/** @brief Multi-Threaded CPUVectorBackend Implementation. */
class CPUVectorBackendMT : public CPUVectorBackend {
  private:
    int numThreads_;
    bool enableSIMD_;

  public:
    CPUVectorBackendMT() {
#if THEMIS_HAS_OPENMP
        numThreads_ = omp_get_max_threads();
#else
        numThreads_ = std::thread::hardware_concurrency();
        if (numThreads_ == 0) {
            numThreads_ = 1;
        }
#endif
        enableSIMD_ = true;

        std::cout << "Multi-threaded CPU backend initialized\n";
        std::cout << "  Threads: " << numThreads_ << "\n";
        std::cout << "  OpenMP: " << (THEMIS_HAS_OPENMP ? "Yes" : "No") << "\n";
#if THEMIS_HAS_SIMD_X86
        std::cout << "  SIMD: AVX2/AVX-512\n";
#elif THEMIS_HAS_SIMD_ARM
        std::cout << "  SIMD: NEON\n";
#else
        std::cout << "  SIMD: No\n";
#endif
    }

    void setThreadCount([[maybe_unused]] int threads) {
        numThreads_ = threads;
#if THEMIS_HAS_OPENMP
        omp_set_num_threads(threads);
#endif
    }

    void enableSIMD([[maybe_unused]] bool enable) {
        enableSIMD_ = enable;
    }

    const char *name() const noexcept override {
        return "CPU Multi-Threaded (OpenMP + SIMD)";
    }

    // L2 distance — returns SQUARED distance (no sqrt), matching the contract of
    // the base-class CPUVectorBackend::computeL2Distance().  This is intentional:
    // callers that only need relative ordering (e.g. kNN ranking) skip the sqrt
    // for performance.  The prior SIMD path called simd::l2_distance() (with sqrt)
    // which was inconsistent with the non-SIMD fallback; this is now harmonised.
    float computeL2Distance(const float *a, const float *b, size_t dim) const {
        if (enableSIMD_) {
            return themis::simd::l2_distance_sq(a, b, dim);
        }
        return CPUVectorBackend::computeL2Distance(a, b, dim);
    }

    // Optimized cosine distance — delegates to the centralized SIMD kernel
    // (utils/simd_distance.h) which already covers AVX-512, AVX2, NEON and
    // scalar paths.  The ~80-line custom AVX2/NEON implementation that was
    // previously inlined here has been removed; simd::cosine_distance() is
    // equivalent and carries prefetch hints for 1536-D embedding vectors.
    float computeCosineDistance(const float *a, const float *b, size_t dim) const {
        if (enableSIMD_) {
            return themis::simd::cosine_distance(a, b, dim);
        }
        return CPUVectorBackend::computeCosineDistance(a, b, dim);
    }

    // Multi-threaded batch distance computation with improved cache utilization
    std::vector<float> computeDistances(const float *queries, size_t numQueries, size_t dim, const float *vectors,
                                        size_t numVectors, bool useL2) override {
        auto sink = [this](ErrorContext e) { setError(std::move(e)); };
        if (!BatchValidator::validateVectorBatch(name(), queries, numQueries, dim, vectors, numVectors, sink)) {
            return {};
        }

        std::vector<float> distances(numQueries * numVectors);

#if THEMIS_HAS_OPENMP
// Parallel processing with OpenMP - improved cache locality
#pragma omp parallel for schedule(dynamic, 16)
        for (size_t q = 0; q < numQueries; ++q) {
            const float *query = queries + q * dim;
            float *result      = &distances[q * numVectors];

            if (useL2 && enableSIMD_) {
                // Compute squared distances with batch optimization
                themis::simd::batch_l2_distance_sq(query, vectors, numVectors, dim, result);
                // Convert to actual distances in same loop - better cache locality
                for (size_t v = 0; v < numVectors; ++v) {
                    result[v] = std::sqrt(result[v]);
                }
            } else {
                // Standard processing for cosine or when SIMD disabled
                for (size_t v = 0; v < numVectors; ++v) {
                    const float *vector = vectors + v * dim;
                    result[v]
                        = useL2 ? computeL2Distance(query, vector, dim) : computeCosineDistance(query, vector, dim);
                }
            }
        }
#else
        // Fallback to single-threaded
        return CPUVectorBackend::computeDistances(queries, numQueries, dim, vectors, numVectors, useL2);
#endif

        return distances;
    }

    // Multi-threaded KNN search
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(const float *queries, size_t numQueries,
                                                                        size_t dim, const float *vectors,
                                                                        size_t numVectors, size_t k,
                                                                        bool useL2) override {
        auto sink = [this](ErrorContext e) { setError(std::move(e)); };
        if (!BatchValidator::validateVectorBatch(name(), queries, numQueries, dim, vectors, numVectors, sink)) {
            return {};
        }
        if (!BatchValidator::validateK(name(), k, sink)) {
            return {};
        }

        std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);

#if THEMIS_HAS_OPENMP
// Parallel KNN search
#pragma omp parallel for schedule(dynamic)
        for (size_t q = 0; q < numQueries; ++q) {
            const float *query = queries + q * dim;

            std::vector<std::pair<uint32_t, float>> distances;
            distances.reserve(numVectors);

            // Compute distances for this query
            for (size_t v = 0; v < numVectors; ++v) {
                const float *vector = vectors + v * dim;
                float dist = useL2 ? computeL2Distance(query, vector, dim) : computeCosineDistance(query, vector, dim);
                distances.emplace_back(static_cast<uint32_t>(v), dist);
            }

            // Partial sort to get k nearest
            size_t actualK = std::min(k, distances.size());
            std::partial_sort(distances.begin(), distances.begin() + actualK, distances.end(),
                              [](const auto &a, const auto &b) { return a.second < b.second; });

            results[q].assign(distances.begin(), distances.begin() + actualK);
        }
#else
        return CPUVectorBackend::batchKnnSearch(queries, numQueries, dim, vectors, numVectors, k, useL2);
#endif

        return results;
    }
};

// ============================================================================
// Multi-Threaded CPUGeoBackend Implementation
// ============================================================================

/** @brief Multi-Threaded CPUGeoBackend Implementation. */
class CPUGeoBackendMT : public CPUGeoBackend {
  private:
    int numThreads_;

  public:
    CPUGeoBackendMT() {
#if THEMIS_HAS_OPENMP
        numThreads_ = omp_get_max_threads();
#else
        numThreads_ = std::thread::hardware_concurrency();
#endif
    }

    const char *name() const noexcept override {
        return "CPU Geo Multi-Threaded (OpenMP)";
    }

    std::vector<float> batchDistances(const double *latitudes1, const double *longitudes1, const double *latitudes2,
                                      const double *longitudes2, size_t count, bool useHaversine) override {
        std::vector<float> distances(count);

#if THEMIS_HAS_OPENMP
#pragma omp parallel for schedule(static)
        for (size_t i = 0; i < count; ++i) {
            double dist = useHaversine ? haversineDistance(latitudes1[i], longitudes1[i], latitudes2[i], longitudes2[i])
                                       : vincentyDistance(latitudes1[i], longitudes1[i], latitudes2[i], longitudes2[i]);
            distances[i] = static_cast<float>(dist);
        }
#else
        return CPUGeoBackend::batchDistances(latitudes1, longitudes1, latitudes2, longitudes2, count, useHaversine);
#endif

        return distances;
    }
};

// Factory functions
std::unique_ptr<CPUVectorBackend> createMultiThreadedCPUVectorBackend() {
    return std::make_unique<CPUVectorBackendMT>();
}

std::unique_ptr<CPUGeoBackend> createMultiThreadedCPUGeoBackend() {
    return std::make_unique<CPUGeoBackendMT>();
}

} // namespace acceleration
} // namespace themis
