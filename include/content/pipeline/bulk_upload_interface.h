// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis::content::pipeline {

/**
 * @brief Bulk upload interface for efficient content ingestion
 * 
 * This is a placeholder class for GAP-005 implementation.
 * Future enhancements:
 * - Parallel upload processing
 * - Resume capability for interrupted uploads
 * - Batch optimization and deduplication
 * - Progress tracking and callbacks
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
        std::string content_id;
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
     * @brief Upload a single content item (placeholder)
     * @param content Content data to upload
     * @param metadata Content metadata
     * @return Upload result
     */
    virtual UploadResult upload(const std::vector<uint8_t>& content,
                                const ContentMetadata& metadata);

    /**
     * @brief Upload multiple content items in batch (placeholder)
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
     * @param content_id ID of the content to cancel
     * @return true if cancellation was successful
     */
    virtual bool cancel_upload(const std::string& content_id);

    /**
     * @brief Get status of an upload operation
     * @param content_id ID of the content
     * @return Current upload status
     */
    virtual UploadStatus get_upload_status(const std::string& content_id) const;

protected:
    ProgressCallback progress_callback_;
};

}  // namespace themis::content::pipeline
