/**
 * @file expected.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// Try to include tl/expected.hpp if available, otherwise use std::optional fallback
#if __has_include(<tl/expected.hpp>)
  #include <tl/expected.hpp>
  #define HAS_TL_EXPECTED 1
#else
  // Fallback: provide a lightweight tl::expected implementation.
  #include <optional>
  #include <stdexcept>
  #include <type_traits>
  #include <utility>
  #include <variant>
  #define HAS_TL_EXPECTED 0

namespace tl {

template<typename E>
/** @brief Unexpected. */
class unexpected {
public:
    using error_type = E;

    unexpected(const E& e)
        : error_(e) {}

    unexpected(E&& e)
        : error_(std::move(e)) {}

    const E& value() const& { return error_; }
    E& value() & { return error_; }
    E&& value() && { return std::move(error_); }

private:
    E error_;
};

template<typename E>
unexpected(E) -> unexpected<E>;

template<typename T, typename E>
/** @brief Expected. */
class expected {
public:
    using value_type = T;
    using error_type = E;

    expected(const T& value)
        : storage_(value) {}

    expected(T&& value)
        : storage_(std::move(value)) {}

    expected(const unexpected<E>& unexp)
        : storage_(unexp.value()) {}

    expected(unexpected<E>&& unexp)
        : storage_(std::move(unexp).value()) {}

    expected(const expected&) = default;
    expected(expected&&) noexcept(std::is_nothrow_move_constructible_v<std::variant<T, E>>) noexcept = default;
    expected& operator=(const expected&) = default;
    expected& operator=(expected&&) noexcept(std::is_nothrow_move_assignable_v<std::variant<T, E>>) noexcept = default;

    [[nodiscard]] bool has_value() const noexcept { return std::holds_alternative<T>(storage_); }
    explicit operator bool() const noexcept { return has_value(); }

    T& value() & {
        if (!has_value()) {
            throw std::logic_error("bad expected access");
        }
        return std::get<T>(storage_);
    }

    const T& value() const& {
        if (!has_value()) {
            throw std::logic_error("bad expected access");
        }
        return std::get<T>(storage_);
    }

    T&& value() && {
        if (!has_value()) {
            throw std::logic_error("bad expected access");
        }
        return std::get<T>(std::move(storage_));
    }

    T& operator*() & { return value(); }
    const T& operator*() const& { return value(); }
    T&& operator*() && { return std::move(value()); }

    E& error() & {
        if (has_value()) {
            throw std::logic_error("expected has value, no error present");
        }
        return std::get<E>(storage_);
    }

    const E& error() const& {
        if (has_value()) {
            throw std::logic_error("expected has value, no error present");
        }
        return std::get<E>(storage_);
    }

    E&& error() && {
        if (has_value()) {
            throw std::logic_error("expected has value, no error present");
        }
        return std::get<E>(std::move(storage_));
    }

    template<typename U>
    T value_or(U&& default_value) const& {
        if (has_value()) {
            return std::get<T>(storage_);
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    template<typename F>
    auto and_then(F&& f) & {
        using result_type = decltype(f(value()));
        if (has_value()) {
            return f(value());
        }
        return result_type(unexpected<E>(error()));
    }

    template<typename F>
    auto and_then(F&& f) const& {
        using result_type = decltype(f(value()));
        if (has_value()) {
            return f(value());
        }
        return result_type(unexpected<E>(error()));
    }

private:
    std::variant<T, E> storage_;
};

template<typename E>
/** @brief Expected< void, e >. */
class expected<void, E> {
public:
    using value_type = void;
    using error_type = E;

    expected()
        : has_value_(true), error_() {}

    expected(const unexpected<E>& unexp)
        : has_value_(false), error_(unexp.value()) {}

    expected(unexpected<E>&& unexp)
        : has_value_(false), error_(std::move(unexp).value()) {}

    [[nodiscard]] bool has_value() const noexcept { return has_value_; }
    explicit operator bool() const noexcept { return has_value_; }

    void value() const {
        if (!has_value_) {
            throw std::logic_error("bad expected access");
        }
    }

    E& error() & {
        if (has_value_) {
            throw std::logic_error("expected has value, no error present");
        }
        return error_;
    }

    const E& error() const& {
        if (has_value_) {
            throw std::logic_error("expected has value, no error present");
        }
        return error_;
    }

    E&& error() && {
        if (has_value_) {
            throw std::logic_error("expected has value, no error present");
        }
        return std::move(error_);
    }

    template<typename F>
    auto and_then(F&& f) & {
        using result_type = decltype(f());
        if (has_value_) {
            return f();
        }
        return result_type(unexpected<E>(error_));
    }

    template<typename F>
    auto and_then(F&& f) const& {
        using result_type = decltype(f());
        if (has_value_) {
            return f();
        }
        return result_type(unexpected<E>(error_));
    }

private:
    bool has_value_;
    E error_;
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
