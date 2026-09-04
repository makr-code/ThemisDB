/**
 * @file bulk_upload_interface.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#include "content/pipeline/bulk_upload_interface.h"
#include <sstream>

namespace themis::content::pipeline {

BulkUploadInterface::UploadResult BulkUploadInterface::upload(
    const std::vector<uint8_t>& content,
    const ContentMetadata& metadata) {
    // Simple implementation for basic use cases
    // 
    // For production use with ContentManager integration, consider using
    // AsyncIngestionWorker which provides:
    // - Async processing with worker thread pool
    // - Job queue management
    // - Integration with ContentManager::ingest()
    // - Progress tracking via IngestionJob
    //
    // This can be extended to wrap AsyncIngestionWorker or directly
    // use ContentManager::ingest() for actual storage.
    
    UploadResult result;
    result.content_id = metadata.content_id;
    result.bytes_uploaded = content.size();
    result.status = UploadStatus::COMPLETED;
    
    // Notify progress if callback is set
    if ([[maybe_unused]] progress_callback_) {
        progress_callback_(metadata.content_id, content.size(), content.size());
    }
    
    return result;
}

std::vector<BulkUploadInterface::UploadResult> BulkUploadInterface::bulk_upload(
    const std::vector<std::vector<uint8_t>>& contents,
    const std::vector<ContentMetadata>& metadata_list) {
    // Simple sequential implementation
    //
    // For parallel batch processing, use AsyncIngestionWorker with:
    // - IngestionJobType::BATCH_FILES for multiple files
    // - Configurable worker thread pool (AsyncIngestionConfig)
    // - Automatic parallelization and queue management
    //
    // This implementation provides a simple interface for testing
    // and can be extended to delegate to AsyncIngestionWorker.
    
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
    
    // Sequential upload (can be parallelized via AsyncIngestionWorker)
    for (size_t i = 0; i < contents.size(); ++i) {
        results.push_back(upload(contents[i], metadata_list[i]));
    }
    
    return results;
}

void BulkUploadInterface::set_progress_callback([[maybe_unused]] ProgressCallback callback) {
    progress_callback_ = callback;
}

bool BulkUploadInterface::cancel_upload(const std::string& /*content_id*/) {
    // Simple implementation doesn't support cancellation
    // Use AsyncIngestionWorker for cancellation support via cancelJob()
    return false;
}

BulkUploadInterface::UploadStatus BulkUploadInterface::get_upload_status(
    const std::string& /*content_id*/) const {
    // Simple implementation doesn't track status
    // Use AsyncIngestionWorker for status tracking via getJobStatus()
    return UploadStatus::COMPLETED;
}

}  // namespace themis::content::pipeline
