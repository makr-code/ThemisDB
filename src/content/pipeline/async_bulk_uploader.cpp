/**
 * @file async_bulk_uploader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#include "content/pipeline/async_bulk_uploader.h"
#include <chrono>

namespace themis::content::pipeline {

AsyncBulkUploader::AsyncBulkUploader(
    std::shared_ptr<themis::content::ContentManager> content_manager,
    themis::content::AsyncIngestionConfig config
) {
    worker_ = std::make_shared<themis::content::AsyncIngestionWorker>(
        content_manager,
        config
    );
}

AsyncBulkUploader::~AsyncBulkUploader() {
    if (worker_ && is_running()) {
        stop(false);  // Don't wait for completion on destruction
    }
}

AsyncBulkUploader::UploadResult AsyncBulkUploader::upload(
    const std::vector<uint8_t>& content,
    const ContentMetadata& metadata
) {
    UploadResult result;
    result.content_id = metadata.content_id;
    result.bytes_uploaded = 0;
    
    if (!worker_ || !is_running()) {
        result.status = UploadStatus::FAILED;
        result.error_message = "Worker not running";
        return result;
    }
    
    try {
        // Convert to string for AsyncIngestionWorker
        std::string blob(content.begin(), content.end());
        
        // Submit to AsyncIngestionWorker
        std::string job_id = worker_->submitFile(
            blob,
            metadata.content_id,  // Use content_id as filename
            metadata.content_type,
            "",  // user_context
            nlohmann::json::object()
        );
        
        // Track job
        {
            std::lock_guard<std::mutex> lock(job_map_mutex_);
            content_to_job_map_[metadata.content_id] = job_id;
        }
        
        result.status = UploadStatus::IN_PROGRESS;
        result.bytes_uploaded = content.size();
        
        // Call progress callback if set
        if ([[maybe_unused]] progress_callback_) {
            progress_callback_(metadata.content_id, content.size(), content.size());
        }
        
    } catch (const std::exception& e) {
        result.status = UploadStatus::FAILED;
        result.error_message = e.what();
    }
    
    return result;
}

std::vector<AsyncBulkUploader::UploadResult> AsyncBulkUploader::bulk_upload(
    const std::vector<std::vector<uint8_t>>& contents,
    const std::vector<ContentMetadata>& metadata_list
) {
    std::vector<UploadResult> results;
    
    if (contents.size() != metadata_list.size()) {
        // Return error results if sizes don't match
        for (size_t i = 0; i < contents.size(); ++i) {
            UploadResult result;
            result.status = UploadStatus::FAILED;
            result.error_message = "Metadata count mismatch";
            if (i < metadata_list.size()) {
                result.content_id = metadata_list[i].content_id;
            }
            results.push_back(result);
        }
        return results;
    }
    
    if (!worker_ || !is_running()) {
        for (size_t i = 0; i < contents.size(); ++i) {
            UploadResult result;
            result.status = UploadStatus::FAILED;
            result.error_message = "Worker not running";
            result.content_id = metadata_list[i].content_id;
            results.push_back(result);
        }
        return results;
    }
    
    // Prepare batch for AsyncIngestionWorker
    std::vector<std::pair<std::string, std::string>> files;
    for (size_t i = 0; i < contents.size(); ++i) {
        std::string blob(contents[i].begin(), contents[i].end());
        files.push_back({metadata_list[i].content_id, blob});
    }
    
    try {
        // Submit batch job
        std::string job_id = worker_->submitBatch(files, "", nlohmann::json::object());
        
        // Track all content items with same job_id
        {
            std::lock_guard<std::mutex> lock(job_map_mutex_);
            for (const auto& meta : metadata_list) {
                content_to_job_map_[meta.content_id] = job_id;
            }
        }
        
        // Create results
        for (size_t i = 0; i < contents.size(); ++i) {
            UploadResult result;
            result.content_id = metadata_list[i].content_id;
            result.status = UploadStatus::IN_PROGRESS;
            result.bytes_uploaded = contents[i].size();
            results.push_back(result);
            
            // Call progress callback if set
            if ([[maybe_unused]] progress_callback_) {
                progress_callback_(
                    metadata_list[i].content_id,
                    contents[i].size(),
                    contents[i].size()
                );
            }
        }
        
    } catch (const std::exception& e) {
        for (const auto& meta : metadata_list) {
            UploadResult result;
            result.content_id = meta.content_id;
            result.status = UploadStatus::FAILED;
            result.error_message = e.what();
            results.push_back(result);
        }
    }
    
    return results;
}

bool AsyncBulkUploader::cancel_upload(const std::string& content_id) {
    if (!worker_) {
        return false;
    }
    
    std::string job_id;
    {
        std::lock_guard<std::mutex> lock(job_map_mutex_);
        auto it = content_to_job_map_.find(content_id);
        if (it == content_to_job_map_.end()) {
            return false;
        }
        job_id = it->second;
    }
    
    return worker_->cancelJob(job_id);
}

AsyncBulkUploader::UploadStatus AsyncBulkUploader::get_upload_status(
    const std::string& content_id
) const {
    if (!worker_) {
        return UploadStatus::FAILED;
    }
    
    std::string job_id;
    {
        std::lock_guard<std::mutex> lock(job_map_mutex_);
        auto it = content_to_job_map_.find(content_id);
        if (it == content_to_job_map_.end()) {
            return UploadStatus::PENDING;
        }
        job_id = it->second;
    }
    
    auto job_status = worker_->getJobStatus(job_id);
    if (!job_status.has_value()) {
        return UploadStatus::PENDING;
    }
    
    return map_job_status(job_status->status);
}

void AsyncBulkUploader::start() {
    if (worker_) {
        worker_->start();
    }
}

void AsyncBulkUploader::stop(bool wait_for_completion) {
    if (worker_) {
        worker_->stop(wait_for_completion);
    }
}

bool AsyncBulkUploader::is_running() const {
    return worker_ && worker_->isRunning();
}

AsyncBulkUploader::UploadStatus AsyncBulkUploader::map_job_status(
    themis::content::IngestionJobStatus status
) const {
    using themis::content::IngestionJobStatus;
    
    switch (status) {
        case IngestionJobStatus::QUEUED:
            return UploadStatus::PENDING;
        case IngestionJobStatus::PROCESSING:
            return UploadStatus::IN_PROGRESS;
        case IngestionJobStatus::COMPLETED:
            return UploadStatus::COMPLETED;
        case IngestionJobStatus::FAILED:
            return UploadStatus::FAILED;
        case IngestionJobStatus::CANCELLED:
            return UploadStatus::CANCELLED;
        default:
            return UploadStatus::PENDING;
    }
}

}  // namespace themis::content::pipeline
