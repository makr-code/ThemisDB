/**
 * @file bulk_upload_interface.h
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

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis::content::pipeline {

class BulkUploadInterface {
public:
    enum class UploadStatus {
        PENDING,
        IN_PROGRESS,
        COMPLETED,
        FAILED,
        CANCELLED
    };

    struct ContentMetadata {
        std::string content_id;
        std::string content_type;
        size_t content_size = 0;
        bool compressed = false;
        size_t chunk_count = 0;
    };

    struct UploadResult {
        UploadStatus status = UploadStatus::PENDING;
        std::string content_id = {};
        std::string error_message;
        size_t bytes_uploaded = 0;
    };

    using ProgressCallback = std::function<void(const std::string& content_id,
                                                 size_t bytes_uploaded,
                                                 size_t total_bytes)>;

    BulkUploadInterface() = default;
    /**
     * @brief Bulk Upload Interface.
     * @return Return value.
     */
    virtual ~BulkUploadInterface() = default;

    /**
     * @brief Upload.
     * @param[in] content Input parameter.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    virtual UploadResult upload(const std::vector<uint8_t>& content,
                                const ContentMetadata& metadata);

    /**
     * @brief Bulk upload.
     * @param[in] contents Input parameter.
     * @param[in] metadata_list Input parameter.
     * @return Return value.
     */
    virtual std::vector<UploadResult> bulk_upload(
        const std::vector<std::vector<uint8_t>>& contents,
        const std::vector<ContentMetadata>& metadata_list);

    /**
     * @brief Set progress callback.
     * @param[in] callback Input parameter.
     */
    void set_progress_callback(ProgressCallback callback);

    /**
     * @brief Cancel upload.
     * @param[in] content_id Identifier of the content.
     * @return True when the operation succeeds.
     */
    virtual bool cancel_upload(const std::string& content_id);

    /**
     * @brief Get upload status.
     * @param[in] content_id Identifier of the content.
     * @return Return value.
     */
    virtual UploadStatus get_upload_status(const std::string& content_id) const;

protected:
    ProgressCallback progress_callback_;
};

}  // namespace themis::content::pipeline
