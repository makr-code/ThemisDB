/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            exporter_errors.h                                  ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:24:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     182                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

} // namespace themis::exporters
