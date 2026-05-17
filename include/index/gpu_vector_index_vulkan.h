#pragma once

#include "index/gpu_vector_index.h"

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace themis {
namespace index {

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

} // namespace index
} // namespace themis
