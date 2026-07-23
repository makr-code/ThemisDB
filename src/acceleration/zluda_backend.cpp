/**
 * @file zluda_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=6, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: zluda_backend.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:49:01
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 88/100 | Lines: 370
 * Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=8, M=1, L=0
 * PR History (last 5): #3609 feat(acceleration): wire mi... (2026-03-12) | #3551 docs(chimera + acceleration... (2026-03-12) | #2712 [acceleration] Publish back... (2026-03-12) | #30 Add comprehensive GPU accel... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// ZLUDA Backend Implementation - MODIFIED
// ZLUDA: CUDA compatibility layer for AMD GPUs
// Allows running CUDA code on AMD hardware without modification

#include "acceleration/zluda_backend.h"
#include "acceleration/compute_backend.h"
#include "utils/logger.h"
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <functional>
#include <mutex>
#include <utility>
#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
// Minimal dl* compatibility layer for Windows builds (maps to LoadLibrary/GetProcAddress)
static inline void* dlopen(const char* name, int /*flags*/) {
    return reinterpret_cast<void*>(::LoadLibraryA(name));
}
static inline void* dlsym(void* handle, const char* name) {
    if (!handle) return nullptr;
    return reinterpret_cast<void*>(::GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
}
static inline int dlclose(void* handle) {
    if (!handle) return 0;
    return ::FreeLibrary(reinterpret_cast<HMODULE>(handle)) ? 0 : -1;
}
#define RTLD_NOW 0
#define RTLD_LOCAL 0
#endif

// Provide a minimal `dim3` type on Windows when CUDA headers are not available.
// This avoids build errors on MSVC for code that references `dim3` in ZLUDA
// compatibility code. If CUDA headers are present, they define `dim3` and
// this shim will be skipped.
#if defined(_WIN32) && !defined(__CUDACC__) && !defined(__CUDA_RUNTIME_H__)
struct dim3 {
    unsigned int x;
    unsigned int y;
    unsigned int z;
    constexpr dim3(unsigned int x_ = 1u, unsigned int y_ = 1u, unsigned int z_ = 1u) noexcept
        : x(x_), y(y_), z(z_) {}
};
#endif

#ifdef THEMIS_ENABLE_ZLUDA

namespace themis {
namespace acceleration {

// ============================================================================
// ZLUDA uses CUDA API, but runs on AMD GPUs
// ============================================================================

// ZLUDA runtime types (same as CUDA)
typedef void* ZludaDevicePtr;
typedef void* ZludaStream;
typedef int ZludaError;

#define ZLUDA_SUCCESS 0

// Function pointer types
typedef ZludaError (*PFN_zludaGetDeviceCount)(int*);
typedef ZludaError (*PFN_zludaSetDevice)(int);
typedef ZludaError (*PFN_zludaMalloc)(void**, size_t);
typedef ZludaError (*PFN_zludaFree)(void*);
typedef ZludaError (*PFN_zludaMemcpy)(void*, const void*, size_t, int);
typedef ZludaError (*PFN_zludaStreamCreate)(ZludaStream*);
typedef ZludaError (*PFN_zludaStreamDestroy)(ZludaStream);
typedef ZludaError (*PFN_zludaStreamSynchronize)(ZludaStream);
typedef ZludaError (*PFN_zludaLaunchKernel)(const void*, dim3, dim3, void**, size_t, ZludaStream);
typedef ZludaError (*PFN_zludaDeviceTotalMem)(size_t*, int);

// ============================================================================
// ZLUDAVectorBackend Implementation
// ============================================================================

class ZLUDAVectorBackend : public IVectorBackend {
public:
    using ComputeDistancesFn = std::function<std::vector<float>(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2)>;
    using BatchKnnSearchFn = std::function<std::vector<std::vector<std::pair<uint32_t, float>>>(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2)>;

    /// Inject a computeDistances callback for non-PTX/test/integration paths.
    /// Thread-safe: callback storage is guarded by a static mutex.
    static void setComputeDistancesFn(ComputeDistancesFn fn);
    /// Inject a batchKnnSearch callback for non-PTX/test/integration paths.
    /// Thread-safe: callback storage is guarded by a static mutex.
    static void setBatchKnnSearchFn(BatchKnnSearchFn fn);
    /**
     * @brief Inject the generic ZludaKernelFn bridge.
     *
     * @param fn  Callable satisfying ZludaKernelFn; clears when null.
     * @note Thread-safe -- guarded by s_callback_fn_mutex_.
     */
    static void setZludaKernelFnImpl(ZludaKernelFn fn);

    ZLUDAVectorBackend() = default;
    ~ZLUDAVectorBackend() override { shutdown(); }
    
    const char* name() const noexcept override { return "ZLUDA"; }
    BackendType type() const noexcept override { return BackendType::ZLUDA; }
    
    bool isAvailable() const noexcept override {
        // Try to load ZLUDA library
        void* handle = dlopen("libcuda.so.zluda", RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            handle = dlopen("libcuda.so", RTLD_NOW | RTLD_LOCAL);
            if (!handle) return false;
            
            // Check if this is actually ZLUDA and not NVIDIA CUDA
            // ZLUDA sets a special environment variable or has specific version strings
            const char* zludaEnv = getenv("ZLUDA_ENABLE");
            if (!zludaEnv) {
                dlclose(handle);
                return false;
            }
        }
        
        dlclose(handle);
        return true;
    }
    
    BackendCapabilities getCapabilities() const override {
        BackendCapabilities caps;
        caps.supportsVectorOps = true;
        caps.supportsGraphOps = false;
        caps.supportsGeoOps = false;
        caps.supportsBatchProcessing = true;
        caps.supportsAsync = true;
        caps.supportedPrecisions = PrecisionMode::FP32;
        caps.supportedMetrics = metricBit(DistanceMetric::L2)
                              | metricBit(DistanceMetric::COSINE)
                              | metricBit(DistanceMetric::INNER_PRODUCT);
        caps.deviceName = "AMD GPU via ZLUDA (CUDA compatibility)";
        
        if (initialized_) {
            // Query device properties through ZLUDA
            caps.deviceName = "AMD Radeon (ZLUDA)";
            // Use the real VRAM size detected during initialize(); fall back to
            // 8 GiB sentinel when cuDeviceTotalMem was unavailable at init time.
            caps.maxMemoryBytes = detected_memory_bytes_;
        }
        
        return caps;
    }
    
    bool initialize() override {
        if (initialized_) return true;
        
        THEMIS_INFO("ZLUDA Backend: Initializing...");
        THEMIS_INFO("ZLUDA: CUDA compatibility layer for AMD GPUs");
        
        // Load ZLUDA library
        zludaLib_ = dlopen("libcuda.so.zluda", RTLD_NOW);
        if (!zludaLib_) {
            zludaLib_ = dlopen("libcuda.so", RTLD_NOW);
            if (!zludaLib_) {
                THEMIS_WARN("ZLUDA: failed to load ZLUDA library -- backend unavailable");
                return false;
            }
        }
        
        // Load function pointers
        loadFunctions();

        // Query total VRAM for this device via cuDeviceTotalMem.
        // Store in detected_memory_bytes_; used by getCapabilities().
        if (fnDeviceTotalMem_) {
            size_t total_mem = 0;
            if (fnDeviceTotalMem_(&total_mem, deviceId_) == ZLUDA_SUCCESS && total_mem > 0) {
                detected_memory_bytes_ = total_mem;
            }
        }
        
        // Check device count
        int deviceCount = 0;
        if (fnGetDeviceCount_(&deviceCount) != ZLUDA_SUCCESS || deviceCount == 0) {
            THEMIS_WARN("ZLUDA: no ZLUDA-compatible devices found");
            return false;
        }
        
        THEMIS_INFO("ZLUDA: found {} AMD GPU(s)", deviceCount);
        
        // Set device
        deviceId_ = 0;
        if (fnSetDevice_(deviceId_) != ZLUDA_SUCCESS) {
            THEMIS_WARN("ZLUDA: failed to set ZLUDA device {}", deviceId_);
            return false;
        }
        
        // Create stream
        if (fnStreamCreate_(&stream_) != ZLUDA_SUCCESS) {
            THEMIS_WARN("ZLUDA: failed to create ZLUDA stream");
            return false;
        }
        
        initialized_ = true;
        THEMIS_INFO("ZLUDA Backend: successfully initialized");
        THEMIS_INFO("ZLUDA: running CUDA kernels on AMD GPUs via ZLUDA compatibility layer");
        
        return true;
    }
    
    void shutdown() override {
        if (initialized_) {
            if (stream_) fnStreamDestroy_(stream_);
            if (zludaLib_) dlclose(zludaLib_);
            initialized_ = false;
        }
    }
    
    std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) override {
        ComputeDistancesFn fn;
        {
            std::lock_guard<std::mutex> lk(s_callback_fn_mutex_);
            fn = s_compute_distances_fn_;
        }
        if (fn) {
            try {
                return fn(queries, numQueries, dim, vectors, numVectors, useL2);
            } catch (const std::exception& e) {
                THEMIS_WARN("ZLUDA: computeDistances callback failed: {} "
                            "(fail-closed -> returning empty result)", e.what());
                return {};
            } catch (const std::string& e) {
                THEMIS_WARN("ZLUDA: computeDistances callback failed: {} "
                            "(fail-closed -> returning empty result)", e);
                return {};
            } catch (const char* e) {
                THEMIS_WARN("ZLUDA: computeDistances callback failed: {} "
                            "(fail-closed -> returning empty result)",
                            (e ? e : "<null>"));
                return {};
            }
        }

        // -- Bridge 2: ZludaKernelFn (checked before initialized-guard)
        {
            ZludaKernelFn kfn;
            {
                std::lock_guard<std::mutex> lk(s_callback_fn_mutex_);
                kfn = s_zluda_kernel_fn_;
            }
            if (kfn) {
                std::vector<float> input;
                input.reserve(numQueries * dim + numVectors * dim);
                input.insert(input.end(), queries, queries + numQueries * dim);
                input.insert(input.end(), vectors, vectors + numVectors * dim);
                try {
                    auto result = kfn(input);
                    if (!result.empty()) return result;
                } catch (const std::exception& e) {
                    THEMIS_WARN("ZLUDA: ZludaKernelFn threw in computeDistances: {} "
                                "-- falling back to CPU", e.what());
                } catch (...) {
                    THEMIS_WARN("ZLUDA: ZludaKernelFn threw unknown exception in "
                                "computeDistances -- falling back to CPU");
                }
            }
        }

        if (!initialized_) {
            THEMIS_WARN("ZLUDA: computeDistances -- backend not initialized, falling back to CPU");
            return {};
        }

    // STUB/SIMULATION NOTE:
    // Activation: When no ZludaKernelFn or typed bridge is injected and
    //             no PTX is loaded via cuModuleLoadData().
    // Removal Plan: Load PTX via cuModuleLoadData; launch via cuLaunchKernel.
    THEMIS_WARN("ZLUDA: computeDistances requires CUDA PTX kernel -- "
                "no PTX loaded, falling back to CPU");
    return {};
    }
    
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) override {
        BatchKnnSearchFn fn;
        {
            std::lock_guard<std::mutex> lk(s_callback_fn_mutex_);
            fn = s_batch_knn_fn_;
        }
        if (fn) {
            try {
                return fn(queries, numQueries, dim, vectors, numVectors, k, useL2);
            } catch (const std::exception& e) {
                THEMIS_WARN("ZLUDA: batchKnnSearch callback failed: {} "
                            "(fail-closed -> returning empty result)", e.what());
                return {};
            } catch (const std::string& e) {
                THEMIS_WARN("ZLUDA: batchKnnSearch callback failed: {} "
                            "(fail-closed -> returning empty result)", e);
                return {};
            } catch (const char* e) {
                THEMIS_WARN("ZLUDA: batchKnnSearch callback failed: {} "
                            "(fail-closed -> returning empty result)",
                            (e ? e : "<null>"));
                return {};
            }
        }

        if (!initialized_) {
            THEMIS_WARN("ZLUDA: batchKnnSearch -- backend not initialized, falling back to CPU");
            return {};
        }

        // -- Bridge 2: ZludaKernelFn (checked before initialized-guard)
        {
            ZludaKernelFn kfn;
            {
                std::lock_guard<std::mutex> lk(s_callback_fn_mutex_);
                kfn = s_zluda_kernel_fn_;
            }
            if (kfn) {
                std::vector<float> input;
                input.reserve(numQueries * dim + numVectors * dim);
                input.insert(input.end(), queries, queries + numQueries * dim);
                input.insert(input.end(), vectors, vectors + numVectors * dim);
                try {
                    auto raw_out = kfn(input);
                    if (raw_out.size() == numQueries * numVectors) {
                        std::vector<std::vector<std::pair<uint32_t, float>>> result(numQueries);
                        const size_t kk = std::min(k, numVectors);
                        for (size_t q = 0; q < numQueries; ++q) {
                            const float* row = raw_out.data() + q * numVectors;
                            std::vector<std::pair<uint32_t, float>> dists(numVectors);
                            for (size_t v = 0; v < numVectors; ++v)
                                dists[v] = {static_cast<uint32_t>(v), row[v]};
                            std::partial_sort(dists.begin(),
                                              dists.begin() + static_cast<std::ptrdiff_t>(kk),
                                              dists.end(),
                                              [](const auto& a, const auto& b) {
                                                  return a.second < b.second;
                                              });
                            dists.resize(kk);
                            result[q] = std::move(dists);
                        }
                        return result;
                    }
                    THEMIS_WARN("ZLUDA: ZludaKernelFn size mismatch in batchKnnSearch"
                                " -- falling back to CPU");
                } catch (const std::exception& e) {
                    THEMIS_WARN("ZLUDA: ZludaKernelFn threw in batchKnnSearch: {} "
                                "-- falling back to CPU", e.what());
                } catch (...) {
                    THEMIS_WARN("ZLUDA: ZludaKernelFn threw unknown exception in "
                                "batchKnnSearch -- falling back to CPU");
                }
            }
        }

        THEMIS_WARN("ZLUDA: batchKnnSearch requires CUDA PTX kernel -- "
                    "no PTX loaded, falling back to CPU");
        return {};
    }

private:
    static std::mutex s_callback_fn_mutex_;
    static ComputeDistancesFn s_compute_distances_fn_;
    static BatchKnnSearchFn s_batch_knn_fn_;
    static ZludaKernelFn s_zluda_kernel_fn_;

    void loadFunctions() {
        fnGetDeviceCount_ = (PFN_zludaGetDeviceCount)dlsym(zludaLib_, "cuDeviceGetCount");
        fnSetDevice_ = (PFN_zludaSetDevice)dlsym(zludaLib_, "cuDeviceSet");
        fnMalloc_ = (PFN_zludaMalloc)dlsym(zludaLib_, "cuMemAlloc");
        fnFree_ = (PFN_zludaFree)dlsym(zludaLib_, "cuMemFree");
        fnMemcpy_ = (PFN_zludaMemcpy)dlsym(zludaLib_, "cuMemcpy");
        fnStreamCreate_ = (PFN_zludaStreamCreate)dlsym(zludaLib_, "cuStreamCreate");
        fnStreamDestroy_ = (PFN_zludaStreamDestroy)dlsym(zludaLib_, "cuStreamDestroy");
        fnStreamSynchronize_ = (PFN_zludaStreamSynchronize)dlsym(zludaLib_, "cuStreamSynchronize");
        fnDeviceTotalMem_ = (PFN_zludaDeviceTotalMem)dlsym(zludaLib_, "cuDeviceTotalMem");
    }
    
    bool initialized_ = false;
    int deviceId_ = 0;
    void* zludaLib_ = nullptr;
    ZludaStream stream_ = nullptr;
    /// Actual VRAM capacity queried via cuDeviceTotalMem during initialize().
    /// Sentinel 8 GiB used when the function is unavailable through ZLUDA.
    size_t detected_memory_bytes_ = 8ULL * 1024 * 1024 * 1024;
    
    // Function pointers
    PFN_zludaGetDeviceCount fnGetDeviceCount_ = nullptr;
    PFN_zludaSetDevice fnSetDevice_ = nullptr;
    PFN_zludaMalloc fnMalloc_ = nullptr;
    PFN_zludaFree fnFree_ = nullptr;
    PFN_zludaMemcpy fnMemcpy_ = nullptr;
    PFN_zludaStreamCreate fnStreamCreate_ = nullptr;
    PFN_zludaStreamDestroy fnStreamDestroy_ = nullptr;
    PFN_zludaStreamSynchronize fnStreamSynchronize_ = nullptr;
    PFN_zludaDeviceTotalMem fnDeviceTotalMem_ = nullptr;
};

std::mutex ZLUDAVectorBackend::s_callback_fn_mutex_;
ZLUDAVectorBackend::ComputeDistancesFn ZLUDAVectorBackend::s_compute_distances_fn_;
ZLUDAVectorBackend::BatchKnnSearchFn ZLUDAVectorBackend::s_batch_knn_fn_;
ZludaKernelFn ZLUDAVectorBackend::s_zluda_kernel_fn_;

void ZLUDAVectorBackend::setComputeDistancesFn(ComputeDistancesFn fn) {
    std::lock_guard<std::mutex> lk(s_callback_fn_mutex_);
    s_compute_distances_fn_ = std::move(fn);
}

void ZLUDAVectorBackend::setBatchKnnSearchFn(BatchKnnSearchFn fn) {
    std::lock_guard<std::mutex> lk(s_callback_fn_mutex_);
    s_batch_knn_fn_ = std::move(fn);
}

void ZLUDAVectorBackend::setZludaKernelFnImpl(ZludaKernelFn fn) {
    std::lock_guard<std::mutex> lk(s_callback_fn_mutex_);
    s_zluda_kernel_fn_ = std::move(fn);
}

/// Free-function wrapper for injecting ZLUDA computeDistances callback bridges.
/// Thread-safe via backend static mutex; callback exceptions are handled
/// fail-closed in computeDistances() by returning an empty result.
void setZLUDAComputeDistancesFn(
    std::function<std::vector<float>(
        const float*, size_t, size_t, const float*, size_t, bool)> fn) {
    ZLUDAVectorBackend::setComputeDistancesFn(std::move(fn));
}

/// Free-function wrapper for injecting ZLUDA batchKnnSearch callback bridges.
/// Thread-safe via backend static mutex; callback exceptions are handled
/// fail-closed in batchKnnSearch() by returning an empty result.
void setZLUDABatchKnnSearchFn(
    std::function<std::vector<std::vector<std::pair<uint32_t, float>>>(
        const float*, size_t, size_t, const float*, size_t, size_t, bool)> fn) {
    ZLUDAVectorBackend::setBatchKnnSearchFn(std::move(fn));
}

/// @brief Public free-function bridge entry point for ZludaKernelFn.
/// Delegates to ZLUDAVectorBackend::setZludaKernelFnImpl().
/// @param fn Callable satisfying ZludaKernelFn; null clears the bridge.
/// @note Thread-safe.
void setZludaKernelFn(ZludaKernelFn fn) {
    ZLUDAVectorBackend::setZludaKernelFnImpl(std::move(fn));
}

[[nodiscard]] std::unique_ptr<IVectorBackend> createZLUDABackend() {
    return std::make_unique<ZLUDAVectorBackend>();
}

} // namespace acceleration
} // namespace themis

#endif // THEMIS_ENABLE_ZLUDA
