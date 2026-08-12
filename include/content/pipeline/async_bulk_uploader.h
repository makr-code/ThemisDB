/**
 * @file async_bulk_uploader.h
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

#include "content/pipeline/bulk_upload_interface.h"
#include "content/async_ingestion_worker.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace themis::content::pipeline {

/**
 * @brief AsyncIngestionWorker adapter for BulkUploadInterface
 * 
 * This adapter integrates BulkUploadInterface with ThemisDB's production-ready
 * AsyncIngestionWorker, providing:
 * - Multi-threaded parallel processing
 * - Job queue management
 * - ContentManager integration for actual storage
 * - Advanced progress tracking and cancellation
 * 
 * Use this for production deployments requiring high-throughput batch uploads.
 */
class AsyncBulkUploader : public BulkUploadInterface {
public:
    /**
     * @brief Construct async uploader with ContentManager
     * 
     * @param content_manager ContentManager instance for storage
     * @param config Worker configuration (thread count, queue size, etc.)
     */
    explicit AsyncBulkUploader(
        std::shared_ptr<themis::content::ContentManager> content_manager,
        themis::content::AsyncIngestionConfig config = themis::content::AsyncIngestionConfig{}
    );

    ~AsyncBulkUploader() override;

    /**
     * @brief Upload a single content item asynchronously
     * 
     * Delegates to AsyncIngestionWorker for parallel processing.
     * 
     * @param content Content data to upload
     * @param metadata Content metadata
     * @return Upload result (with job_id for tracking)
     */
    UploadResult upload(
        const std::vector<uint8_t>& content,
        const ContentMetadata& metadata
    ) override;

    /**
     * @brief Upload multiple content items in parallel
     * 
     * Uses AsyncIngestionWorker's batch processing with configurable parallelism.
     * 
     * @param contents Vector of content data to upload
     * @param metadata_list Vector of metadata for each content item
     * @return Vector of upload results
     */
    std::vector<UploadResult> bulk_upload(
        const std::vector<std::vector<uint8_t>>& contents,
        const std::vector<ContentMetadata>& metadata_list
    ) override;

    /**
     * @brief Cancel an ongoing upload operation
     * 
     * Uses AsyncIngestionWorker's cancellation mechanism.
     * 
     * @param content_id ID of the content to cancel
     * @return true if cancellation was successful
     */
    bool cancel_upload(const std::string& content_id) override;

    /**
     * @brief Get status of an upload operation
     * 
     * Queries AsyncIngestionWorker for job status.
     * 
     * @param content_id ID of the content
     * @return Current upload status
     */
    UploadStatus get_upload_status(const std::string& content_id) const override;

    /**
     * @brief Start the async worker threads
     */
    void start();

    /**
     * @brief Stop the async worker threads
     * @param wait_for_completion If true, waits for all jobs to finish
     */
    void stop(bool wait_for_completion = true);

    /**
     * @brief Check if worker is running
     */
    bool is_running() const;

private:
    std::shared_ptr<themis::content::AsyncIngestionWorker> worker_;
    mutable std::mutex job_map_mutex_;
    std::unordered_map<std::string, std::string> content_to_job_map_;  // content_id -> job_id
    
    UploadStatus map_job_status(themis::content::IngestionJobStatus status) const;
};

}  // namespace themis::content::pipeline
