/**
 * @file graphics_backends.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=19; TODO=1, Stub=17, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include "acceleration/metrics/backend_metrics.h"
#include "themis_export.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace acceleration {

// DirectX 12 Compute Shaders backend (Windows only)
/** @brief DirectX 12 Compute Shaders backend (Windows only). */
class DirectXVectorBackend : public IVectorBackend {
public:
    using AvailabilityFn = std::function<bool()>;
    using InitializeFn = std::function<bool()>;
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

    DirectXVectorBackend();
    ~DirectXVectorBackend() override;
    
    const char* name() const noexcept override { return "DirectX"; }
    BackendType type() const noexcept override { return BackendType::DIRECTX; }
    bool isAvailable() const noexcept override;
    
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;
    
    std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) override;
    
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) override;

    /// Register a non-DirectX availability bridge for stub builds.
    /// Thread-safe setter; passing empty function restores fail-closed default.
    static void setAvailabilityFn(AvailabilityFn fn) {
        std::lock_guard<std::mutex> lk(availabilityFnMutex());
        availabilityFnStorage() = std::move(fn);
    }
    /// Register a non-DirectX initialization bridge for stub builds.
    /// Thread-safe setter; passing empty function restores fail-closed default.
    static void setInitializeFn(InitializeFn fn) {
        std::lock_guard<std::mutex> lk(initializeFnMutex());
        initializeFnStorage() = std::move(fn);
    }
    /// Register a non-DirectX distance-compute bridge for stub builds.
    /// Thread-safe setter; passing empty function restores fail-closed default.
    static void setComputeDistancesFn(ComputeDistancesFn fn) {
        std::lock_guard<std::mutex> lk(computeDistancesFnMutex());
        computeDistancesFnStorage() = std::move(fn);
    }
    /// Register a non-DirectX batch-KNN bridge for stub builds.
    /// Thread-safe setter; passing empty function restores fail-closed default.
    static void setBatchKnnSearchFn(BatchKnnSearchFn fn) {
        std::lock_guard<std::mutex> lk(batchKnnSearchFnMutex());
        batchKnnSearchFnStorage() = std::move(fn);
    }

private:
    static std::mutex& availabilityFnMutex() {
        static std::mutex m;
        return m;
    }
    static AvailabilityFn& availabilityFnStorage() {
        static AvailabilityFn fn;
        return fn;
    }
    static std::mutex& initializeFnMutex() {
        static std::mutex m;
        return m;
    }
    static InitializeFn& initializeFnStorage() {
        static InitializeFn fn;
        return fn;
    }
    static std::mutex& computeDistancesFnMutex() {
        static std::mutex m;
        return m;
    }
    static ComputeDistancesFn& computeDistancesFnStorage() {
        static ComputeDistancesFn fn;
        return fn;
    }
    static std::mutex& batchKnnSearchFnMutex() {
        static std::mutex m;
        return m;
    }
    static BatchKnnSearchFn& batchKnnSearchFnStorage() {
        static BatchKnnSearchFn fn;
        return fn;
    }
    bool initialized_ = false;
    class DirectXVectorBackendImpl;
    std::unique_ptr<DirectXVectorBackendImpl> impl_;
};

// Vulkan Compute backend (cross-platform)
/** @brief Vulkan Compute backend (cross-platform). */
class VulkanVectorBackend : public IVectorBackend {
public:
    using AvailabilityFn = std::function<bool()>;
    using InitializeFn = std::function<bool()>;
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

    VulkanVectorBackend();
    ~VulkanVectorBackend() override;
    
    const char* name() const noexcept override { return "Vulkan"; }
    BackendType type() const noexcept override { return BackendType::VULKAN; }
    bool isAvailable() const noexcept override;
    
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;
    BackendHealthStatus getHealthStatus() const override;
    
    std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) override;
    
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) override;

    // Frozen kernel dispatch table — L2, cosine, inner-product, and top-K
    ANNKernelDispatch populateANNDispatch() const override;

    // ---- Vulkan-specific introspection --------------------------------

    // Returns true when the selected physical device advertises
    // VK_KHR_buffer_device_address (required for advanced buffer aliasing
    // and bindless GPU pointer operations).  On Apple Silicon via MoltenVK
    // this may return false even if Vulkan is otherwise functional.
    // Only meaningful after a successful initialize().
    bool hasBufferDeviceAddress() const noexcept;

    // Tunable workgroup dimensions for SPIR-V specialization constants.
    // Must be called before initialize() to take effect.
    // Calls after initialize() are silently ignored; zero values are rejected.
    // setWorkgroupSizeBatchSearch() additionally rejects values > 256 because
    // batch_search.comp declares shared float sharedQuery[256].
    void setWorkgroupSizeL2(uint32_t wgX, uint32_t wgY) noexcept;
    void setWorkgroupSizeBatchSearch(uint32_t wgX) noexcept;

    // Inspect current (pending or baked) workgroup sizes for testing/debugging.
    // Returns {wgX, wgY} for the L2 pipeline; {batchX, 1} for batch-search.
    std::pair<uint32_t, uint32_t> getWorkgroupSizeL2() const noexcept;
    uint32_t getWorkgroupSizeBatchSearch() const noexcept;

    /// Register a non-Vulkan availability bridge for stub builds.
    static void setAvailabilityFn(AvailabilityFn fn) {
        std::lock_guard<std::mutex> lk(availabilityFnMutex());
        availabilityFnStorage() = std::move(fn);
    }
    /// Register a non-Vulkan initialization bridge for stub builds.
    static void setInitializeFn(InitializeFn fn) {
        std::lock_guard<std::mutex> lk(initializeFnMutex());
        initializeFnStorage() = std::move(fn);
    }
    /// Register a non-Vulkan distance-compute bridge for stub builds.
    static void setComputeDistancesFn(ComputeDistancesFn fn) {
        std::lock_guard<std::mutex> lk(computeDistancesFnMutex());
        computeDistancesFnStorage() = std::move(fn);
    }
    /// Register a non-Vulkan batch-KNN bridge for stub builds.
    static void setBatchKnnSearchFn(BatchKnnSearchFn fn) {
        std::lock_guard<std::mutex> lk(batchKnnSearchFnMutex());
        batchKnnSearchFnStorage() = std::move(fn);
    }
    // ── STUB #169 bridge — runtime GLSL→SPIR-V compiler injection ──────────
    /// Callback type for injecting a shaderc/glslang-based GLSL→SPIR-V
    /// compiler so that compute shaders can be compiled at runtime without
    /// pre-built .spv files.
    ///
    /// Parameters: (glsl_source, shader_type)
    ///   shader_type is a string such as "compute", "vertex", "fragment".
    /// Must return a non-empty SPIR-V buffer or an empty vector on failure.
    using CompileGLSLFn = std::function<
        std::vector<uint32_t>(const std::string& /*glsl_source*/,
                              const std::string& /*shader_type*/)>;

    /// Inject (or remove) a runtime GLSL→SPIR-V compiler.  Pass nullptr /
    /// empty fn to restore the stub path (returns empty SPIR-V).
    /// Thread-safe.
    static void setCompileGLSLFn(CompileGLSLFn fn);

private:
    static std::mutex& availabilityFnMutex() {
        static std::mutex m;
        return m;
    }
    static AvailabilityFn& availabilityFnStorage() {
        static AvailabilityFn fn;
        return fn;
    }
    static std::mutex& initializeFnMutex() {
        static std::mutex m;
        return m;
    }
    static InitializeFn& initializeFnStorage() {
        static InitializeFn fn;
        return fn;
    }
    static std::mutex& computeDistancesFnMutex() {
        static std::mutex m;
        return m;
    }
    static ComputeDistancesFn& computeDistancesFnStorage() {
        static ComputeDistancesFn fn;
        return fn;
    }
    static std::mutex& batchKnnSearchFnMutex() {
        static std::mutex m;
        return m;
    }
    static BatchKnnSearchFn& batchKnnSearchFnStorage() {
        static BatchKnnSearchFn fn;
        return fn;
    }
    bool initialized_ = false;
    class VulkanVectorBackendImpl;
    std::unique_ptr<VulkanVectorBackendImpl> impl_;
    metrics::BackendMetrics metrics_{"vulkan"};
};

// Vulkan Geospatial Compute backend (cross-platform)
// Implements the IGeoBackend interface using Vulkan compute shaders for
// Haversine distance and point-in-polygon operations, providing the same
// geospatial compute capabilities as the CUDA geo backend.
/** @brief geospatial compute capabilities as the CUDA geo backend. */
class VulkanGeoBackend : public IGeoBackend {
public:
    VulkanGeoBackend();
    ~VulkanGeoBackend() override;

    const char* name() const noexcept override { return "VulkanGeo"; }
    BackendType type() const noexcept override { return BackendType::VULKAN; }
    bool isAvailable() const noexcept override;

    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;

    std::vector<float> batchDistances(
        const double* latitudes1,
        const double* longitudes1,
        const double* latitudes2,
        const double* longitudes2,
        size_t count,
        bool useHaversine = true
    ) override;

    std::vector<bool> batchPointInPolygon(
        const double* pointLats,
        const double* pointLons,
        size_t numPoints,
        const double* polygonCoords,
        size_t numPolygonVertices
    ) override;

    // Frozen kernel dispatch table — haversine distance and point-in-polygon
    GeoKernelDispatch populateGeoDispatch() const override;

private:
    bool initialized_ = false;
};

// OpenGL Compute Shaders backend (OpenGL 4.3+ compute shader acceleration)
//
// supportsAsync = false: all compute dispatch calls (computeDistances,
// batchKnnSearch) are fully synchronous — glMemoryBarrier + readback happen
// on the calling thread before the function returns.
/** @brief on the calling thread before the function returns. */
class OpenGLVectorBackend : public IVectorBackend {
public:
    using AvailabilityFn = std::function<bool()>;
    using InitializeFn = std::function<bool()>;
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

    OpenGLVectorBackend();
    ~OpenGLVectorBackend() override;
    
    const char* name() const noexcept override { return "OpenGL"; }
    BackendType type() const noexcept override { return BackendType::OPENGL; }
    bool isAvailable() const noexcept override;
    
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;
    
    std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) override;
    
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) override;

    /// Register a non-OpenGL availability bridge for stub builds.
    /// Thread-safe setter; passing empty function restores fail-closed default.
    static void setAvailabilityFn(AvailabilityFn fn) {
        std::lock_guard<std::mutex> lk(availabilityFnMutex());
        availabilityFnStorage() = std::move(fn);
    }
    /// Register a non-OpenGL initialization bridge for stub builds.
    /// Thread-safe setter; passing empty function restores fail-closed default.
    static void setInitializeFn(InitializeFn fn) {
        std::lock_guard<std::mutex> lk(initializeFnMutex());
        initializeFnStorage() = std::move(fn);
    }
    /// Register a non-OpenGL distance-compute bridge for stub builds.
    /// Thread-safe setter; passing empty function restores fail-closed default.
    static void setComputeDistancesFn(ComputeDistancesFn fn) {
        std::lock_guard<std::mutex> lk(computeDistancesFnMutex());
        computeDistancesFnStorage() = std::move(fn);
    }
    /// Register a non-OpenGL batch-KNN bridge for stub builds.
    /// Thread-safe setter; passing empty function restores fail-closed default.
    static void setBatchKnnSearchFn(BatchKnnSearchFn fn) {
        std::lock_guard<std::mutex> lk(batchKnnSearchFnMutex());
        batchKnnSearchFnStorage() = std::move(fn);
    }

private:
    static std::mutex& availabilityFnMutex() {
        static std::mutex m;
        return m;
    }
    static AvailabilityFn& availabilityFnStorage() {
        static AvailabilityFn fn;
        return fn;
    }
    static std::mutex& initializeFnMutex() {
        static std::mutex m;
        return m;
    }
    static InitializeFn& initializeFnStorage() {
        static InitializeFn fn;
        return fn;
    }
    static std::mutex& computeDistancesFnMutex() {
        static std::mutex m;
        return m;
    }
    static ComputeDistancesFn& computeDistancesFnStorage() {
        static ComputeDistancesFn fn;
        return fn;
    }
    static std::mutex& batchKnnSearchFnMutex() {
        static std::mutex m;
        return m;
    }
    static BatchKnnSearchFn& batchKnnSearchFnStorage() {
        static BatchKnnSearchFn fn;
        return fn;
    }
    bool initialized_ = false;
    class OpenGLVectorBackendImpl;
    std::unique_ptr<OpenGLVectorBackendImpl> impl_;
};

// OpenGL Geospatial Compute backend (OpenGL 4.3+ compute shader acceleration)
//
// Implements the IGeoBackend interface using GLSL 4.30 compute shaders for
// Haversine distance and point-in-polygon operations. Provides a CPU fallback
// (identical algorithm to VulkanGeoBackend) when no EGL/OpenGL 4.3 driver is
// available, so initialize() always succeeds on the current platform.
//
// supportsAsync = false: all dispatch is synchronous (glMemoryBarrier + readback
// on the calling thread).
/** @brief on the calling thread). */
class OpenGLGeoBackend : public IGeoBackend {
public:
    OpenGLGeoBackend();
    ~OpenGLGeoBackend() override;

    const char* name() const noexcept override { return "OpenGLGeo"; }
    BackendType type() const noexcept override { return BackendType::OPENGL; }
    bool isAvailable() const noexcept override;

    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;

    std::vector<float> batchDistances(
        const double* latitudes1,
        const double* longitudes1,
        const double* latitudes2,
        const double* longitudes2,
        size_t count,
        bool useHaversine = true
    ) override;

    std::vector<bool> batchPointInPolygon(
        const double* pointLats,
        const double* pointLons,
        size_t numPoints,
        const double* polygonCoords,
        size_t numPolygonVertices
    ) override;

private:
    bool initialized_ = false;
    class OpenGLGeoBackendImpl;
    std::unique_ptr<OpenGLGeoBackendImpl> impl_;
};

// OpenGL Graph Compute backend (OpenGL 4.3+ compute shader acceleration)
//
// Implements the IGraphBackend interface using GLSL 4.30 compute shaders for
// breadth-first search (wavefront-parallel BFS) and shortest-path computation
// (parallel Bellman-Ford). Adjacency is an N×N dense matrix stored in an SSBO.
// Falls back to CPU implementations when no EGL/OpenGL 4.3 driver is present.
//
// supportsAsync = false: all dispatch is synchronous.
/** @brief supportsAsync = false: all dispatch is synchronous. */
class OpenGLGraphBackend : public IGraphBackend {
public:
    OpenGLGraphBackend();
    ~OpenGLGraphBackend() override;

    const char* name() const noexcept override { return "OpenGLGraph"; }
    BackendType type() const noexcept override { return BackendType::OPENGL; }
    bool isAvailable() const noexcept override;

    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;

    // Batch BFS: returns reachable vertex lists (up to maxDepth hops) for each
    // start vertex. adjacency is an N×N dense matrix (adj[u*N+v] != 0 ⟹ edge u→v).
    std::vector<std::vector<uint32_t>> batchBFS(
        const uint32_t* adjacency,
        size_t numVertices,
        const uint32_t* startVertices,
        size_t numStarts,
        uint32_t maxDepth
    ) override;

    // Batch shortest path via Bellman-Ford. Returns vertex-sequence paths from
    // startVertices[i] to endVertices[i]; empty if unreachable.
    // weights is an N×N matrix (weight[u*N+v] for edge u→v).
    std::vector<std::vector<uint32_t>> batchShortestPath(
        const uint32_t* adjacency,
        const float* weights,
        size_t numVertices,
        const uint32_t* startVertices,
        const uint32_t* endVertices,
        size_t numPairs
    ) override;

private:
    bool initialized_ = false;
    class OpenGLGraphBackendImpl;
    std::unique_ptr<OpenGLGraphBackendImpl> impl_;
};

} // namespace acceleration
} // namespace themis

// ============================================================================
// Vulkan GLSL compiler injection — only available in THEMIS_ENABLE_VULKAN builds
// ============================================================================

#ifdef THEMIS_ENABLE_VULKAN
#include <functional>
#include <vector>
#include <string>

namespace themis {
namespace acceleration {

/**
 * @brief Injection type for a runtime GLSL→SPIR-V compiler (e.g. shaderc).
 *
 * Signature: `std::vector<uint32_t> fn(const std::string& glsl_source,
 *                                      const std::string& shader_type)`
 *
 * A non-empty return replaces the built-in empty-SPIR-V stub path.
 */
using GlslCompilerFn = std::function<
    std::vector<uint32_t>(const std::string& glsl_source,
                          const std::string& shader_type)>;

/**
 * @brief Inject a real GLSL-to-SPIR-V compiler backend.
 *
 * When @p fn is non-null, `compileGLSLtoSPIRV()` in `vulkan_backend_full.cpp`
 * delegates to it instead of returning an empty buffer.  Pass @p nullptr to
 * revert to the stub.  Thread-safe.
 *
 * Roadmap ref: src/acceleration/FUTURE_ENHANCEMENTS.md §Vulkan GLSL Compiler.
 */
THEMIS_BASE_API void setVulkanGlslCompilerFn(GlslCompilerFn fn);

} // namespace acceleration
} // namespace themis
#endif // THEMIS_ENABLE_VULKAN
