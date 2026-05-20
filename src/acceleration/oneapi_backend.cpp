/*
 * ThemisDB | File: oneapi_backend.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 88/100
 * Gap Summary: total=14; TODO=1, Stub=10, Unimpl=0, Mock=1, Sim=2, Debt=0, C=11, H=34, M=22, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// OneAPI Backend Implementation for Intel GPUs (Arc, Xe, XPU)
// Uses SYCL/DPC++ for unified CPU/GPU/FPGA programming
// Copyright (c) 2024 ThemisDB

#include "acceleration/compute_backend.h"
#include <vector>
#include <cmath>
#include <iostream>
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
    sycl::queue* queue_ = nullptr;
    bool initialized_ = false;

public:
    OneAPIVectorBackend() = default;
    
    ~OneAPIVectorBackend() {
        if (queue_) {
            delete queue_;
            queue_ = nullptr;
        }
    }

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
            std::cerr << "OneAPI: Error checking availability: " << e.what() << "\n";
        }
        return false;
    }
    
    bool initialize() override {
        try {
            // Try to create GPU queue
            try {
                queue_ = new sycl::queue(sycl::gpu_selector_v);
            } catch (...) {
                // Fallback to default device
                std::cerr << "OneAPI: GPU selector failed, trying default device\n";
                queue_ = new sycl::queue(sycl::default_selector_v);
            }
            
            auto device = queue_->get_device();
            auto platform = device.get_platform();
            
            std::cout << "OneAPI backend initialized successfully\n";
            std::cout << "  Platform: " << platform.get_info<sycl::info::platform::name>() << "\n";
            std::cout << "  Device: " << device.get_info<sycl::info::device::name>() << "\n";
            std::cout << "  Max Compute Units: " << device.get_info<sycl::info::device::max_compute_units>() << "\n";
            std::cout << "  Max Work Group Size: " << device.get_info<sycl::info::device::max_work_group_size>() << "\n";
            
            initialized_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "OneAPI: Initialization failed: " << e.what() << "\n";
            return false;
        }
    }
    
    void shutdown() override {
        if (queue_) {
            delete queue_;
            queue_ = nullptr;
        }
        initialized_ = false;
    }
    
    std::vector<float> computeDistances(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors,
        size_t dimension, bool useL2) override 
    {
        if (!initialized_ || !queue_) {
            std::cerr << "OneAPI: Backend not initialized\n";
            return {};
        }
        
        try {
            size_t resultSize = numQueries * numVectors;
            std::vector<float> distances(resultSize);
            
            // Allocate device memory
            float* d_queries = sycl::malloc_device<float>(numQueries * dimension, *queue_);
            float* d_vectors = sycl::malloc_device<float>(numVectors * dimension, *queue_);
            float* d_distances = sycl::malloc_device<float>(resultSize, *queue_);
            
            // Copy data to device
            queue_->memcpy(d_queries, queries, numQueries * dimension * sizeof(float)).wait();
            queue_->memcpy(d_vectors, vectors, numVectors * dimension * sizeof(float)).wait();
            
            // Launch kernel
            if (useL2) {
                // L2 Distance Kernel
                queue_->parallel_for(sycl::range<2>(numQueries, numVectors),
                    [=](sycl::id<2> idx) {
                        size_t q = idx[0];
                        size_t v = idx[1];
                        
                        float sum = 0.0f;
                        for (size_t d = 0; d < dimension; d++) {
                            float diff = d_queries[q * dimension + d] - d_vectors[v * dimension + d];
                            sum += diff * diff;
                        }
                        
                        d_distances[q * numVectors + v] = sycl::sqrt(sum);
                    }).wait();
            } else {
                // Cosine Distance Kernel
                queue_->parallel_for(sycl::range<2>(numQueries, numVectors),
                    [=](sycl::id<2> idx) {
                        size_t q = idx[0];
                        size_t v = idx[1];
                        
                        float dotProduct = 0.0f;
                        float normQ = 0.0f;
                        float normV = 0.0f;
                        
                        for (size_t d = 0; d < dimension; d++) {
                            float qVal = d_queries[q * dimension + d];
                            float vVal = d_vectors[v * dimension + d];
                            dotProduct += qVal * vVal;
                            normQ += qVal * qVal;
                            normV += vVal * vVal;
                        }
                        
                        float cosineSim = dotProduct / (sycl::sqrt(normQ) * sycl::sqrt(normV) + 1e-8f);
                        d_distances[q * numVectors + v] = 1.0f - cosineSim;
                    }).wait();
            }
            
            // Copy results back
            queue_->memcpy(distances.data(), d_distances, resultSize * sizeof(float)).wait();
            
            // Cleanup
            sycl::free(d_queries, *queue_);
            sycl::free(d_vectors, *queue_);
            sycl::free(d_distances, *queue_);
            
            return distances;
        } catch (const std::exception& e) {
            std::cerr << "OneAPI: Compute failed: " << e.what() << "\n";
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
        } catch (...) {
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
