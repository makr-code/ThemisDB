/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            expected.h                                         ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:28:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     196                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

// Try to include tl/expected.hpp if available, otherwise use std::optional fallback
#if __has_include(<tl/expected.hpp>)
  #include <tl/expected.hpp>
  #define HAS_TL_EXPECTED 1
#else
  // Fallback: use std::optional + std::bad_optional_access style approach
  #include <optional>
  #define HAS_TL_EXPECTED 0
#endif

#include "utils/error_registry.h"
#include <string>
#include <system_error>
#include <type_traits>

namespace themis {

/**
 * Error class wrapping ErrorCode with optional context information
 * 
 * Provides a production-ready error type that:
 * - Carries structured error codes from ErrorRegistry
 * - Includes dynamic context (e.g., file paths, resource IDs)
 * - Supports error chaining and propagation
 */
class Error {
public:
    // Constructors
    Error() : code_(errors::ErrorCode::ERR_UNKNOWN) {}
    
    explicit Error(errors::ErrorCode code) 
        : code_(code) {}
    
    Error(errors::ErrorCode code, std::string context)
        : code_(code), context_(std::move(context)) {}
    
    // Accessors
    errors::ErrorCode code() const { return code_; }
    const std::string& context() const { return context_; }
    
    // Get full error message (template + context)
    std::string message() const {
        auto& registry = errors::ErrorRegistry::getInstance();
        auto metadata = registry.getError(code_);
        
        if (context_.empty()) {
            return metadata.message_template;
        }
        
        // Simply append context to template without fmt::format
        // (fmt::format causes constexpr evaluation issues)
        return metadata.message_template + ": " + context_;
    }
    
    // Get error metadata
    errors::ErrorMetadata metadata() const {
        auto& registry = errors::ErrorRegistry::getInstance();
        return registry.getError(code_);
    }
    
    // Comparison
    bool operator==(const Error& other) const {
        return code_ == other.code_;
    }
    
    bool operator!=(const Error& other) const {
        return !(*this == other);
    }
    
private:
    errors::ErrorCode code_;
    std::string context_;
};

/**
 * Production-ready Result<T> type alias using tl::expected
 * 
 * This provides:
 * - Zero-overhead error propagation (no exceptions)
 * - Type-safe error handling (compiler-enforced checking)
 * - Composable error handling with monadic operations
 * - Forward-compatible with C++23 std::expected
 * 
 * Usage examples:
 * 
 *   // Function returning Result
 *   Result<std::string> readFile(const std::string& path) {
 *       if (!fileExists(path)) {
 *           return tl::unexpected(Error(ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, path));
 *       }
 *       return fileContents;
 *   }
 * 
 *   // Caller checking result
 *   auto result = readFile("config.yaml");
 *   if (result) {
 *       processContent(*result);
 *   } else {
 *       spdlog::error("Failed to read file: {}", result.error().message());
 *   }
 * 
 *   // Monadic error propagation
 *   return readFile(path)
 *       .and_then([](const std::string& content) { return parseYaml(content); })
 *       .and_then([](const Config& cfg) { return validateConfig(cfg); });
 */
template<typename T>
using Result = tl::expected<T, Error>;

/**
 * Helper function to create an error result
 * 
 * Usage: return Err(ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, filepath);
 */
template<typename T>
Result<T> Err(errors::ErrorCode code, std::string context = "") {
    return tl::unexpected(Error(code, std::move(context)));
}

/**
 * Helper function to create a success result
 * 
 * Usage: return Ok(value);
 */
template<typename T>
Result<std::decay_t<T>> Ok(T&& value) {
    return Result<std::decay_t<T>>(std::forward<T>(value));
}

// Specialization for void-like operations
inline Result<void> OkVoid() {
    return Result<void>();
}

inline Result<void> ErrVoid(errors::ErrorCode code, std::string context = "") {
    return tl::unexpected(Error(code, std::move(context)));
}

/**
 * Legacy pattern conversion helpers
 * These help migrate existing code patterns to Result<T>
 */

// Convert nullable pointer to Result
template<typename T>
Result<T*> fromNullable(T* ptr, errors::ErrorCode errorCode, std::string context = "") {
    if (ptr == nullptr) {
        return tl::unexpected(Error(errorCode, std::move(context)));
    }
    return ptr;
}

// Convert bool + message to Result<void>
inline Result<void> fromBoolStatus(bool ok, const std::string& errorMessage, 
                                     errors::ErrorCode errorCode) {
    if (!ok) {
        return tl::unexpected(Error(errorCode, errorMessage));
    }
    return Result<void>();
}

// Convert std::optional to Result
template<typename T>
Result<T> fromOptional(std::optional<T>&& opt, errors::ErrorCode errorCode, 
                       std::string context = "") {
    if (!opt.has_value()) {
        return tl::unexpected(Error(errorCode, std::move(context)));
    }
    return std::move(*opt);
}

} // namespace themis
