/**
 * @file bulk_upload_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis::content::pipeline {

/**
 * @brief Bulk upload interface for efficient content ingestion
 * 
 * This class provides a simplified, pipeline-specific interface for
 * batch content upload operations. It can be used as a facade or
 * integrated with ThemisDB's AsyncIngestionWorker for production use.
 * 
 * For production deployments, consider using AsyncIngestionWorker directly
 * (include/content/async_ingestion_worker.h) which provides:
 * - Multi-threaded processing with configurable worker pools
 * - Job queue management with priority support
 * - Advanced progress tracking and cancellation
 * - Archive extraction and batch file processing
 * 
 * This interface is designed for:
 * - Simple batch upload scenarios
 * - Testing and development
 * - Pipeline-specific upload patterns
 * - Integration point for custom upload strategies
 * 
 * Future enhancements:
 * - Integration adapter for AsyncIngestionWorker
 * - Resume capability for interrupted uploads
 * - Batch optimization and deduplication
 * - Multi-modal content handling
 * - Compaction strategies
 */
class BulkUploadInterface {
public:
    /**
     * @brief Status of an upload operation
     */
    enum class UploadStatus {
        PENDING,
        IN_PROGRESS,
        COMPLETED,
        FAILED,
        CANCELLED
    };

    /**
     * @brief Metadata for a content item
     */
    struct ContentMetadata {
        std::string content_id;
        std::string content_type;
        size_t content_size = 0;
        bool compressed = false;
        size_t chunk_count = 0;
    };

    /**
     * @brief Upload operation result
     */
    struct UploadResult {
        UploadStatus status = UploadStatus::PENDING;
        std::string content_id = {};
        std::string error_message;
        size_t bytes_uploaded = 0;
    };

    /**
     * @brief Progress callback signature
     */
    using ProgressCallback = std::function<void(const std::string& content_id,
                                                 size_t bytes_uploaded,
                                                 size_t total_bytes)>;

    BulkUploadInterface() = default;
    virtual ~BulkUploadInterface() = default;

    /**
     * @brief Upload a single content item
     * 
     * Simple implementation for basic use cases. For production with
     * ContentManager integration, consider AsyncIngestionWorker.
     * 
     * @param content Content data to upload
     * @param metadata Content metadata
     * @return Upload result
     */
    virtual UploadResult upload(const std::vector<uint8_t>& content,
                                const ContentMetadata& metadata);

    /**
     * @brief Upload multiple content items in batch
     * 
     * Simple sequential implementation. For parallel processing,
     * use AsyncIngestionWorker with BATCH_FILES job type.
     * 
     * @param contents Vector of content data to upload
     * @param metadata_list Vector of metadata for each content item
     * @return Vector of upload results
     */
    virtual std::vector<UploadResult> bulk_upload(
        const std::vector<std::vector<uint8_t>>& contents,
        const std::vector<ContentMetadata>& metadata_list);

    /**
     * @brief Set progress callback
     * @param callback Callback function for progress updates
     */
    void set_progress_callback(ProgressCallback callback);

    /**
     * @brief Cancel an ongoing upload operation
     * 
     * Note: Current simple implementation doesn't support cancellation.
     * Use AsyncIngestionWorker for advanced cancellation support.
     * 
     * @param content_id ID of the content to cancel
     * @return true if cancellation was successful
     */
    virtual bool cancel_upload(const std::string& content_id);

    /**
     * @brief Get status of an upload operation
     * 
     * Note: Current implementation doesn't track status persistently.
     * Use AsyncIngestionWorker for comprehensive status tracking.
     * 
     * @param content_id ID of the content
     * @return Current upload status
     */
    virtual UploadStatus get_upload_status(const std::string& content_id) const;

protected:
    ProgressCallback progress_callback_;
};

}  // namespace themis::content::pipeline
