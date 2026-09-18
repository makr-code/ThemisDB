/**
 * @file async_bulk_uploader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class AsyncBulkUploader : public BulkUploadInterface {
public:
    explicit AsyncBulkUploader(
        std::shared_ptr<themis::content::ContentManager> content_manager,
        themis::content::AsyncIngestionConfig config = themis::content::AsyncIngestionConfig{}
    );

    ~AsyncBulkUploader() override;

    UploadResult upload(
        const std::vector<uint8_t>& content,
        const ContentMetadata& metadata
    ) override;

    std::vector<UploadResult> bulk_upload(
        const std::vector<std::vector<uint8_t>>& contents,
        const std::vector<ContentMetadata>& metadata_list
    ) override;

    bool cancel_upload(const std::string& content_id) override;

    UploadStatus get_upload_status(const std::string& content_id) const override;

    /**
     * @brief Start.
     */
    void start();

    void stop(bool wait_for_completion = true);

    /**
     * @brief Is running.
     * @return True when the operation succeeds.
     */
    bool is_running() const;

private:
    std::shared_ptr<themis::content::AsyncIngestionWorker> worker_;
    mutable std::mutex job_map_mutex_;
    std::unordered_map<std::string, std::string> content_to_job_map_;  // content_id -> job_id
    
    /**
     * @brief Map job status.
     * @param[in] status Input parameter.
     * @return Return value.
     */
    UploadStatus map_job_status(themis::content::IngestionJobStatus status) const;
};

}  // namespace themis::content::pipeline
