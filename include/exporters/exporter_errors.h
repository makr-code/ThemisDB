/**
 * @file exporter_errors.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: exporter_errors.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 168
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "utils/error_registry.h"
#include <stdexcept>
#include <string>

namespace themis::exporters {

/// Base exception class for all exporter errors
class ExporterException : public std::runtime_error {
public:
    explicit ExporterException(
        errors::ErrorCode code,
        const std::string& message,
        const std::string& context = ""
    ) : std::runtime_error(message),
        error_code_(code),
        context_(context) {}
    
    errors::ErrorCode getErrorCode() const { return error_code_; }
    const std::string& getContext() const { return context_; }
    
protected:
    errors::ErrorCode error_code_;
    std::string context_;
};

/// Schema validation error
class SchemaValidationException : public ExporterException {
public:
    explicit SchemaValidationException(
        const std::string& message,
        const std::string& entity_id = "",
        const std::string& validation_error = ""
    ) : ExporterException(
            errors::ErrorCode::ERR_EXPORT_SCHEMA_VALIDATION_FAILED,
            message,
            "entity_id=" + entity_id + ", validation_error=" + validation_error
        ),
        entity_id_(entity_id),
        validation_error_(validation_error) {}
    
    const std::string& getEntityId() const { return entity_id_; }
    const std::string& getValidationError() const { return validation_error_; }
    
private:
    std::string entity_id_;
    std::string validation_error_;
};

/// I/O error during export
class ExportIOException : public ExporterException {
public:
    explicit ExportIOException(
        const std::string& message,
        const std::string& file_path = "",
        int errno_value = 0
    ) : ExporterException(
            errors::ErrorCode::ERR_EXPORT_IO_ERROR,
            message,
            "file_path=" + file_path + ", errno=" + std::to_string(errno_value)
        ),
        file_path_(file_path),
        errno_value_(errno_value) {}
    
    const std::string& getFilePath() const { return file_path_; }
    int getErrno() const { return errno_value_; }
    
private:
    std::string file_path_;
    int errno_value_;
};

/// Size limit exceeded error
class SizeLimitException : public ExporterException {
public:
    explicit SizeLimitException(
        const std::string& message,
        size_t current_size,
        size_t max_size
    ) : ExporterException(
            errors::ErrorCode::ERR_EXPORT_SIZE_LIMIT_EXCEEDED,
            message,
            "current_size=" + std::to_string(current_size) + 
            ", max_size=" + std::to_string(max_size)
        ),
        current_size_(current_size),
        max_size_(max_size) {}
    
    size_t getCurrentSize() const { return current_size_; }
    size_t getMaxSize() const { return max_size_; }
    
private:
    size_t current_size_;
    size_t max_size_;
};

/// Quality filter failure
class QualityFilterException : public ExporterException {
public:
    explicit QualityFilterException(
        const std::string& message,
        const std::string& entity_id = "",
        const std::string& reason = ""
    ) : ExporterException(
            errors::ErrorCode::ERR_EXPORT_QUALITY_FILTER_FAILED,
            message,
            "entity_id=" + entity_id + ", reason=" + reason
        ),
        entity_id_(entity_id),
        reason_(reason) {}
    
    const std::string& getEntityId() const { return entity_id_; }
    const std::string& getReason() const { return reason_; }
    
private:
    std::string entity_id_;
    std::string reason_;
};

/// Format configuration error
class FormatException : public ExporterException {
public:
    explicit FormatException(
        const std::string& message,
        const std::string& format_name = ""
    ) : ExporterException(
            errors::ErrorCode::ERR_EXPORT_FORMAT_INVALID,
            message,
            "format=" + format_name
        ),
        format_name_(format_name) {}
    
    const std::string& getFormatName() const { return format_name_; }
    
private:
    std::string format_name_;
};

/// Configuration error
class ConfigException : public ExporterException {
public:
    explicit ConfigException(
        const std::string& message,
        const std::string& config_key = ""
    ) : ExporterException(
            errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID,
            message,
            "config_key=" + config_key
        ),
        config_key_(config_key) {}
    
    const std::string& getConfigKey() const { return config_key_; }
    
private:
    std::string config_key_;
};

/// @brief Policy denial exception — thrown when an export is rejected by PolicyEngine.
///
/// This is the concrete typed wrapper used by exporter paths that surface
/// `ERR_EXPORT_POLICY_DENIED` via `ExporterException`.
/// Catching `ExporterException` remains the stable contract for policy-blocked
/// export operations.
///
/// @note isResumableError(ExporterErrorCode::EXPORT_ABORTED) == false —
///       policy denials are fail-closed and never resumable.  The operator or
///       requesting user must resolve the policy conflict before re-attempting.
class PolicyDeniedException : public ExporterException {
public:
    /// @brief Construct a PolicyDeniedException.
    /// @param denial_reason      Human-readable denial reason from
    ///                           PolicyEngine::checkExportPermission().
    /// @param requesting_user    Identity of the user/service that requested
    ///                           the export (may be empty).
    /// @param collection         Name of the collection being exported
    ///                           (may be empty).
    explicit PolicyDeniedException(
        const std::string& denial_reason,
        const std::string& requesting_user = "",
        const std::string& collection = ""
    ) : ExporterException(
            errors::ErrorCode::ERR_EXPORT_POLICY_DENIED,
            "Export denied by policy: " + denial_reason,
            "user=" + requesting_user + ", collection=" + collection
        ),
        denial_reason_(denial_reason),
        requesting_user_(requesting_user),
        collection_(collection) {}

    /// @return The denial reason supplied by PolicyEngine.
    const std::string& getDenialReason() const { return denial_reason_; }

    /// @return The requesting user identity.
    const std::string& getRequestingUser() const { return requesting_user_; }

    /// @return The collection name that was being exported.
    const std::string& getCollection() const { return collection_; }

private:
    std::string denial_reason_;
    std::string requesting_user_;
    std::string collection_;
};

} // namespace themis::exporters
