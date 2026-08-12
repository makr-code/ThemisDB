/**
 * @file gpu_vector_index_vulkan.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/gpu_vector_index.h"

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace themis {
namespace index {

#ifndef THEMIS_VULKAN_VECTOR_INDEX_BACKEND_DECLARED
#define THEMIS_VULKAN_VECTOR_INDEX_BACKEND_DECLARED

/**
 * @brief Vulkan backend for GPU vector indexing.
 *
 * This class provides a Vulkan compute implementation for vector upload and
 * nearest-neighbor search. A callback bridge is included so non-Vulkan builds
 * can remain link-compatible via injected fallback handlers.
 */
class VulkanVectorIndexBackend {
public:
    explicit VulkanVectorIndexBackend(const GPUVectorIndex::Config& config);
    ~VulkanVectorIndexBackend();

    bool initialize(int dimension);
    void shutdown();
    bool uploadVectors(const std::vector<std::vector<float>>& vectors);
    std::vector<std::pair<float, size_t>> searchIndices(const std::vector<float>& query, size_t k);
    std::vector<std::vector<std::pair<float, size_t>>> searchBatchIndices(
        const std::vector<std::vector<float>>& queries, size_t k);
    std::vector<GPUVectorIndex::SearchResult> search(const std::vector<float>& query, size_t k);
    std::vector<std::vector<GPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k);
    GPUVectorIndex::Statistics getStatistics() const;
    bool isInitialized() const;

    using InitializeFn = std::function<bool(int)>;
    using UploadFn = std::function<bool(const std::vector<std::vector<float>>&)>;
    using SearchFn = std::function<std::vector<GPUVectorIndex::SearchResult>(
        const std::vector<float>&, size_t)>;
    using SearchBatchFn = std::function<std::vector<std::vector<GPUVectorIndex::SearchResult>>(
        const std::vector<std::vector<float>>&, size_t)>;

    static void setInitializeFn(InitializeFn fn);
    static void setUploadFn(UploadFn fn);
    static void setSearchFn(SearchFn fn);
    static void setSearchBatchFn(SearchBatchFn fn);

    static std::mutex& initializeFnMutex();
    static InitializeFn& initializeFnStorage();
    static std::mutex& uploadFnMutex();
    static UploadFn& uploadFnStorage();
    static std::mutex& searchFnMutex();
    static SearchFn& searchFnStorage();
    static std::mutex& searchBatchFnMutex();
    static SearchBatchFn& searchBatchFnStorage();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // THEMIS_VULKAN_VECTOR_INDEX_BACKEND_DECLARED

} // namespace index
} // namespace themis
