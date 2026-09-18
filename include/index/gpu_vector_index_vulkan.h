/**
 * @file gpu_vector_index_vulkan.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    /**
     * @brief TBD: Describe VulkanVectorIndexBackend.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit VulkanVectorIndexBackend(const GPUVectorIndex::Config& config);
    ~VulkanVectorIndexBackend();

    /**
     * @brief TBD: Describe initialize.
     * @param[in] dimension Input parameter.
     * @return True on success.
     */
    bool initialize(int dimension);
    /**
     * @brief TBD: Describe shutdown.
     */
    void shutdown();
    /**
     * @brief TBD: Describe uploadVectors.
     * @param[in] vectors Input parameter.
     * @return True on success.
     */
    bool uploadVectors(const std::vector<std::vector<float>>& vectors);
    std::vector<std::pair<float, size_t>> searchIndices(const std::vector<float>& query, size_t k);
    std::vector<std::vector<std::pair<float, size_t>>> searchBatchIndices(
        const std::vector<std::vector<float>>& queries, size_t k);
    /**
     * @brief TBD: Describe search.
     * @param[in] query Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    std::vector<GPUVectorIndex::SearchResult> search(const std::vector<float>& query, size_t k);
    /**
     * @brief TBD: Describe searchBatch.
     * @param[in] queries Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<GPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k);
    /**
     * @brief TBD: Describe getStatistics.
     * @return Return value.
     */
    GPUVectorIndex::Statistics getStatistics() const;
    /**
     * @brief TBD: Describe isInitialized.
     * @return True on success.
     */
    bool isInitialized() const;

    using InitializeFn = std::function<bool(int)>;
    using UploadFn = std::function<bool(const std::vector<std::vector<float>>&)>;
    using SearchFn = std::function<std::vector<GPUVectorIndex::SearchResult>(
        const std::vector<float>&, size_t)>;
    using SearchBatchFn = std::function<std::vector<std::vector<GPUVectorIndex::SearchResult>>(
        const std::vector<std::vector<float>>&, size_t)>;

    /**
     * @brief TBD: Describe setInitializeFn.
     * @param[in] fn Input parameter.
     */
    static void setInitializeFn(InitializeFn fn);
    /**
     * @brief TBD: Describe setUploadFn.
     * @param[in] fn Input parameter.
     */
    static void setUploadFn(UploadFn fn);
    /**
     * @brief TBD: Describe setSearchFn.
     * @param[in] fn Input parameter.
     */
    static void setSearchFn(SearchFn fn);
    /**
     * @brief TBD: Describe setSearchBatchFn.
     * @param[in] fn Input parameter.
     */
    static void setSearchBatchFn(SearchBatchFn fn);

    /**
     * @brief TBD: Describe initializeFnMutex.
     * @return Return value.
     */
    static std::mutex& initializeFnMutex();
    /**
     * @brief TBD: Describe initializeFnStorage.
     * @return Return value.
     */
    static InitializeFn& initializeFnStorage();
    /**
     * @brief TBD: Describe uploadFnMutex.
     * @return Return value.
     */
    static std::mutex& uploadFnMutex();
    /**
     * @brief TBD: Describe uploadFnStorage.
     * @return Return value.
     */
    static UploadFn& uploadFnStorage();
    /**
     * @brief TBD: Describe searchFnMutex.
     * @return Return value.
     */
    static std::mutex& searchFnMutex();
    /**
     * @brief TBD: Describe searchFnStorage.
     * @return Return value.
     */
    static SearchFn& searchFnStorage();
    /**
     * @brief TBD: Describe searchBatchFnMutex.
     * @return Return value.
     */
    static std::mutex& searchBatchFnMutex();
    /**
     * @brief TBD: Describe searchBatchFnStorage.
     * @return Return value.
     */
    static SearchBatchFn& searchBatchFnStorage();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // THEMIS_VULKAN_VECTOR_INDEX_BACKEND_DECLARED

} // namespace index
} // namespace themis
