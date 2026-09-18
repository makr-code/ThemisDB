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

class VulkanVectorIndexBackend {
public:
    /**
     * @brief Vulkan Vector Index Backend.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit VulkanVectorIndexBackend(const GPUVectorIndex::Config& config);
    ~VulkanVectorIndexBackend();

    /**
     * @brief Initialize.
     * @param[in] dimension Input parameter.
     * @return True when the operation succeeds.
     */
    bool initialize(int dimension);
    /**
     * @brief Shutdown.
     */
    void shutdown();
    /**
     * @brief Upload Vectors.
     * @param[in] vectors Input parameter.
     * @return True when the operation succeeds.
     */
    bool uploadVectors(const std::vector<std::vector<float>>& vectors);
    std::vector<std::pair<float, size_t>> searchIndices(const std::vector<float>& query, size_t k);
    std::vector<std::vector<std::pair<float, size_t>>> searchBatchIndices(
        const std::vector<std::vector<float>>& queries, size_t k);
    /**
     * @brief Search.
     * @param[in] query Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    std::vector<GPUVectorIndex::SearchResult> search(const std::vector<float>& query, size_t k);
    /**
     * @brief Search Batch.
     * @param[in] queries Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<GPUVectorIndex::SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k);
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    GPUVectorIndex::Statistics getStatistics() const;
    /**
     * @brief Is Initialized.
     * @return True when the operation succeeds.
     */
    bool isInitialized() const;

    using InitializeFn = std::function<bool(int)>;
    using UploadFn = std::function<bool(const std::vector<std::vector<float>>&)>;
    using SearchFn = std::function<std::vector<GPUVectorIndex::SearchResult>(
        const std::vector<float>&, size_t)>;
    using SearchBatchFn = std::function<std::vector<std::vector<GPUVectorIndex::SearchResult>>(
        const std::vector<std::vector<float>>&, size_t)>;

    /**
     * @brief Set Initialize Fn.
     * @param[in] fn Input parameter.
     */
    static void setInitializeFn(InitializeFn fn);
    /**
     * @brief Set Upload Fn.
     * @param[in] fn Input parameter.
     */
    static void setUploadFn(UploadFn fn);
    /**
     * @brief Set Search Fn.
     * @param[in] fn Input parameter.
     */
    static void setSearchFn(SearchFn fn);
    /**
     * @brief Set Search Batch Fn.
     * @param[in] fn Input parameter.
     */
    static void setSearchBatchFn(SearchBatchFn fn);

    /**
     * @brief Initialize Fn Mutex.
     * @return Return value.
     */
    static std::mutex& initializeFnMutex();
    /**
     * @brief Initialize Fn Storage.
     * @return Return value.
     */
    static InitializeFn& initializeFnStorage();
    /**
     * @brief Upload Fn Mutex.
     * @return Return value.
     */
    static std::mutex& uploadFnMutex();
    /**
     * @brief Upload Fn Storage.
     * @return Return value.
     */
    static UploadFn& uploadFnStorage();
    /**
     * @brief Search Fn Mutex.
     * @return Return value.
     */
    static std::mutex& searchFnMutex();
    /**
     * @brief Search Fn Storage.
     * @return Return value.
     */
    static SearchFn& searchFnStorage();
    /**
     * @brief Search Batch Fn Mutex.
     * @return Return value.
     */
    static std::mutex& searchBatchFnMutex();
    /**
     * @brief Search Batch Fn Storage.
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
