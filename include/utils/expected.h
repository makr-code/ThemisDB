/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            expected.h                                         ║
  Version:         0.0.36                                             ║
  Last Modified:   2026-03-30 04:12:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     196                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

// Try to include tl/expected.hpp if available.
// If unavailable, provide a minimal local fallback that supports the subset
// used by this codebase (construction from value/unexpected, bool checks,
// value()/error(), operator*()).
#if __has_include(<tl/expected.hpp>)
  #include <tl/expected.hpp>
  #define HAS_TL_EXPECTED 1
#else
  #include <optional>
  #include <stdexcept>
  #include <type_traits>
  #include <utility>
  #include <variant>
  #define HAS_TL_EXPECTED 0

namespace tl {

template<typename E>
class unexpected {
public:
    explicit unexpected(E e) : error_(std::move(e)) {}

    E& value() { return error_; }
    const E& value() const { return error_; }

private:
    E error_;
};

template<typename T, typename E>
class expected {
public:
    template<typename, typename>
    friend class expected;

    template<typename U = T, typename = std::enable_if_t<std::is_default_constructible_v<U>>>
    expected() : data_(T{}) {}

    expected(const T& v) : data_(v) {}
    expected(T&& v) : data_(std::move(v)) {}
    expected(unexpected<E> u) : data_(std::move(u.value())) {}

    template<
        typename U,
        typename = std::enable_if_t<
            std::is_convertible_v<U, T> && !std::is_same_v<U, T>
        >
    >
    expected(const expected<U, E>& other) {
        if (other.has_value()) {
            data_ = T(other.value());
        } else {
            data_ = other.error();
        }
    }

    template<
        typename U,
        typename = std::enable_if_t<
            std::is_convertible_v<U, T> && !std::is_same_v<U, T>
        >
    >
    expected& operator=(const expected<U, E>& other) {
        if (other.has_value()) {
            data_ = T(other.value());
        } else {
            data_ = other.error();
        }
        return *this;
    }

    bool has_value() const { return std::holds_alternative<T>(data_); }
    explicit operator bool() const { return has_value(); }

    T& value() {
        if (!has_value()) {
            throw std::logic_error("bad expected access");
        }
        return std::get<T>(data_);
    }

    const T& value() const {
        if (!has_value()) {
            throw std::logic_error("bad expected access");
        }
        return std::get<T>(data_);
    }

    E& error() {
        if (has_value()) {
            throw std::logic_error("no expected error");
        }
        return std::get<E>(data_);
    }

    const E& error() const {
        if (has_value()) {
            throw std::logic_error("no expected error");
        }
        return std::get<E>(data_);
    }

    T& operator*() { return value(); }
    const T& operator*() const { return value(); }
    T* operator->() { return &value(); }
    const T* operator->() const { return &value(); }

private:
    std::variant<T, E> data_;
};

template<typename E>
class expected<void, E> {
public:
    expected() = default;
    expected(unexpected<E> u) : error_(std::move(u.value())) {}

    bool has_value() const { return !error_.has_value(); }
    explicit operator bool() const { return has_value(); }

    void value() const {
        if (!has_value()) {
            throw std::logic_error("bad expected access");
        }
    }

    E& error() {
        if (has_value()) {
            throw std::logic_error("no expected error");
        }
        return *error_;
    }

    const E& error() const {
        if (has_value()) {
            throw std::logic_error("no expected error");
        }
        return *error_;
    }

private:
    std::optional<E> error_;
};

} // namespace tl
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
