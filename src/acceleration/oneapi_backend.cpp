/**
 * @file oneapi_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=14; TODO=1, Stub=10, Unimpl=0, Mock=1, Sim=2, Debt=0, C=5, H=6, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// OneAPI Backend Implementation for Intel GPUs (Arc, Xe, XPU)
// Uses SYCL/DPC++ for unified CPU/GPU/FPGA programming
// Copyright (c) 2024 ThemisDB

#include "acceleration/compute_backend.h"
#include "utils/logger.h"
#include <stdexcept>
#include <vector>
#include <cmath>
#include <optional>
#include <algorithm>
#include <functional>
#include <mutex>

// OneAPI/SYCL headers (conditionally included if available)
#ifdef THEMIS_ENABLE_ONEAPI
#include <sycl/sycl.hpp>
#endif

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_ONEAPI

class OneAPIVectorBackend : public IVectorBackend {
private:
    /// @brief SYCL execution queue; held in optional to avoid raw pointer.
    std::optional<sycl::queue> queue_{};
    /// @brief Guards initialize() / shutdown() for thread-safe lifecycle management.
    mutable std::mutex lifecycle_mutex_;
    bool initialized_ = false;

public:
    OneAPIVectorBackend() = default;

    // Non-copyable, non-movable: sycl::queue wraps a device context.
    OneAPIVectorBackend(const OneAPIVectorBackend&) = delete;
    OneAPIVectorBackend& operator=(const OneAPIVectorBackend&) = delete;

    BackendType type() const override { return BackendType::ONEAPI; }
    
    std::string name() const override { return "OneAPI (Intel XPU)"; }
    
    bool isAvailable() const override {
        try {
            // Check for GPU devices
            auto platforms = sycl::platform::get_platforms();
            for (const auto& platform : platforms) {
                auto devices = platform.get_devices(sycl::info::device_type::gpu);
                if (!devices.empty()) {
                    return true;
                }
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("OneAPI: Error checking availability: {}", e.what());
        }
        return false;
    }
    
    bool initialize() override {
        std::lock_guard<std::mutex> lk(lifecycle_mutex_);
        if (initialized_) {
            return true;
        }
        try {
            // Try to create GPU queue; fall back to default device on failure.
            try {
                queue_.emplace(sycl::gpu_selector_v);
            } catch (const std::exception &e) {
                THEMIS_WARN("OneAPI: GPU selector failed, trying default device: {}", e.what());
                queue_.emplace(sycl::default_selector_v);
            } catch (const std::string &e) {
                THEMIS_WARN("OneAPI: GPU selector failed, trying default device: {}", e);
                queue_.emplace(sycl::default_selector_v);
            } catch (const char *e) {
                THEMIS_WARN("OneAPI: GPU selector failed, trying default device: {}",
                            (e ? e : "<null>"));
                queue_.emplace(sycl::default_selector_v);
            }

            const auto device   = queue_->get_device();
            const auto platform = device.get_platform();

            THEMIS_INFO("OneAPI backend initialized successfully");
            THEMIS_INFO("  Platform: {}", platform.get_info<sycl::info::platform::name>());
            THEMIS_INFO("  Device: {}", device.get_info<sycl::info::device::name>());
            THEMIS_INFO("  Max Compute Units: {}",
                        device.get_info<sycl::info::device::max_compute_units>());
            THEMIS_INFO("  Max Work Group Size: {}",
                        device.get_info<sycl::info::device::max_work_group_size>());

            initialized_ = true;
            return true;
        } catch (const std::exception& e) {
            THEMIS_ERROR("OneAPI: Initialization failed: {}", e.what());
            queue_.reset();
            return false;
        }
    }
    
    void shutdown() override {
        std::lock_guard<std::mutex> lk(lifecycle_mutex_);
        queue_.reset();
        initialized_ = false;
    }
    
    std::vector<float> computeDistances(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors,
        size_t dimension, bool useL2) override 
    {
        if (!initialized_ || !queue_.has_value()) {
            THEMIS_ERROR("OneAPI: Backend not initialized");
            return {};
        }
        
        try {
            const size_t resultSize = numQueries * numVectors;
            std::vector<float> distances(resultSize);

            sycl::queue& q = *queue_;

            // Allocate USM device buffers; freed via RAII lambdas on scope exit.
            float* d_queries   = sycl::malloc_device<float>(numQueries * dimension, q);
            float* d_vectors   = sycl::malloc_device<float>(numVectors * dimension, q);
            float* d_distances = sycl::malloc_device<float>(resultSize, q);

            // Defer USM frees so they always run even on throw.
            auto freeUSM = [&]() noexcept {
                sycl::free(d_queries,   q);
                sycl::free(d_vectors,   q);
                sycl::free(d_distances, q);
            };

            try {
                // Copy data to device; wait_and_throw() propagates SYCL async errors.
                q.memcpy(d_queries, queries, numQueries * dimension * sizeof(float))
                 .wait_and_throw();
                q.memcpy(d_vectors, vectors, numVectors * dimension * sizeof(float))
                 .wait_and_throw();

                // Launch kernel
                if (useL2) {
                    q.parallel_for(sycl::range<2>(numQueries, numVectors),
                        [=](sycl::id<2> idx) {
                            const size_t qi = idx[0];
                            const size_t vi = idx[1];
                            float sum = 0.0f;
                            for (size_t d = 0; d < dimension; d++) {
                                const float diff = d_queries[qi * dimension + d]
                                                 - d_vectors[vi * dimension + d];
                                sum += diff * diff;
                            }
                            d_distances[qi * numVectors + vi] = sycl::sqrt(sum);
                        }).wait_and_throw();
                } else {
                    q.parallel_for(sycl::range<2>(numQueries, numVectors),
                        [=](sycl::id<2> idx) {
                            const size_t qi = idx[0];
                            const size_t vi = idx[1];
                            float dotProduct = 0.0f;
                            float normQ      = 0.0f;
                            float normV      = 0.0f;
                            for (size_t d = 0; d < dimension; d++) {
                                const float qVal = d_queries[qi * dimension + d];
                                const float vVal = d_vectors[vi * dimension + d];
                                dotProduct += qVal * vVal;
                                normQ      += qVal * qVal;
                                normV      += vVal * vVal;
                            }
                            const float cosineSim = dotProduct
                                / (sycl::sqrt(normQ) * sycl::sqrt(normV) + 1e-8f);
                            d_distances[qi * numVectors + vi] = 1.0f - cosineSim;
                        }).wait_and_throw();
                }

                // Copy results back
                q.memcpy(distances.data(), d_distances, resultSize * sizeof(float))
                 .wait_and_throw();
            } catch (...) {
                freeUSM();
                throw;
            }

            freeUSM();
            return distances;
        } catch (const std::exception& e) {
            THEMIS_ERROR("OneAPI: Compute failed: {}", e.what());
            return {};
        }
    }
    
    std::vector<VectorSearchResult> batchKnnSearch(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors,
        size_t dimension, size_t k, bool useL2) override 
    {
        auto distances = computeDistances(queries, numQueries, vectors, numVectors, dimension, useL2);
        
        std::vector<VectorSearchResult> results(numQueries * k);
        
        for (size_t q = 0; q < numQueries; q++) {
            std::vector<std::pair<float, size_t>> pairs;
            pairs.reserve(numVectors);
            
            for (size_t v = 0; v < numVectors; v++) {
                pairs.push_back({distances[q * numVectors + v], v});
            }
            
            std::partial_sort(pairs.begin(), pairs.begin() + k, pairs.end());
            
            for (size_t i = 0; i < k; i++) {
                results[q * k + i].vectorId = pairs[i].second;
                results[q * k + i].distance = pairs[i].first;
            }
        }
        
        return results;
    }
};

#else

// STUB/SIMULATION NOTE:
// Purpose: Satisfy the linker and allow ThemisDB to be built without the Intel
//   oneAPI DPC++/SYCL toolkit.  The stub OneAPIVectorBackend class is compiled
//   in place of the real SYCL-backed class; all methods return false/empty so
//   that the BackendRegistry can probe and skip this backend gracefully.
// Activation: `THEMIS_ENABLE_ONEAPI` is not defined at compile time (default
//   for non-Intel-GPU builds and CPU-only builds).
// Production Delta: Intel Arc / Xe / XPU acceleration is unavailable.
//   `computeDistances()` and `batchKnnSearch()` return empty vectors;
//   `isAvailable()` returns false.  Any workload routed to this backend falls
//   through to the next registered backend (typically CPU).
// Removal Plan: Install Intel oneAPI Base Toolkit (including DPC++ compiler and
//   OpenCL runtime) and set `-DTHEMIS_ENABLE_ONEAPI=1` in CMake.
// Roadmap ref: src/acceleration/FUTURE_ENHANCEMENTS.md §"OneAPI Backend Activation"

// STUB/SIMULATION NOTE (computeDistances bridge):
// Purpose:    Allow injection of a real computeDistances implementation for the
//             non-OneAPI stub path (tests / integration without Intel XPU SDK).
// Activation: Runtime — when setComputeDistancesFn() is called with a non-empty fn.
// Production Delta: With no fn, computeDistances() returns {}; with fn the
//             provided implementation is called instead.
// Removal Plan: Remove bridge once THEMIS_ENABLE_ONEAPI is standard in all envs.

// Stub implementation when OneAPI is not available
/** @brief Stub implementation when OneAPI is not available. */
class OneAPIVectorBackend : public IVectorBackend {
public:
    // Injectable bridge type for the non-OneAPI stub path.
    using ComputeDistancesFn = std::function<std::vector<float>(
        const float* query, size_t query_count, size_t dim,
        const float* db, size_t db_count, bool use_l2)>;

    /// Inject a computeDistances implementation for the non-OneAPI stub path.
    /// Pass empty fn to restore fail-closed stub default (returns {}).
    static void setComputeDistancesFn(ComputeDistancesFn fn);

    BackendType type() const noexcept override { return BackendType::ONEAPI; }
    const char* name() const noexcept override { return "OneAPI (Not Available)"; }
    bool isAvailable() const noexcept override { return false; }
    BackendCapabilities getCapabilities() const override { return {}; }
    bool initialize() override { return false; }
    void shutdown() override {}

    std::vector<float> computeDistances(
        const float* queries, size_t numQueries, size_t dimension,
        const float* vectors, size_t numVectors, bool useL2) override;

    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float*, size_t, size_t, const float*, size_t, size_t, bool) override {
        return {};
    }
};

static std::mutex s_oneapi_compute_fn_mutex_;
static OneAPIVectorBackend::ComputeDistancesFn s_oneapi_compute_fn_;

void OneAPIVectorBackend::setComputeDistancesFn(
    OneAPIVectorBackend::ComputeDistancesFn fn) {
    std::lock_guard<std::mutex> lk(s_oneapi_compute_fn_mutex_);
    s_oneapi_compute_fn_ = std::move(fn);
}

std::vector<float> OneAPIVectorBackend::computeDistances(
    const float* queries, size_t numQueries, size_t dimension,
    const float* vectors, size_t numVectors, bool useL2) {
    OneAPIVectorBackend::ComputeDistancesFn fn;
    {
        std::lock_guard<std::mutex> lk(s_oneapi_compute_fn_mutex_);
        fn = s_oneapi_compute_fn_;
    }
    if (fn) [[unlikely]] {
        try {
            return fn(queries, numQueries, dimension, vectors, numVectors, useL2);
        } catch (const std::exception &) {
            return {};
        } catch (const std::string &) {
            return {};
        } catch (const char *) {
            return {};
        }
    }
    return {};
}

/// Free-function wrapper — allows test code to inject a computeDistances
/// implementation for the non-OneAPI stub path without requiring access to
/// the local `OneAPIVectorBackend` class definition.
void setOneAPIComputeDistancesFn(
    std::function<std::vector<float>(
        const float*, size_t, size_t, const float*, size_t, bool)> fn) {
    OneAPIVectorBackend::setComputeDistancesFn(std::move(fn));
}

#endif

// Factory function
std::unique_ptr<IVectorBackend> createOneAPIBackend() {
    return std::make_unique<OneAPIVectorBackend>();
}

} // namespace acceleration
} // namespace themis
