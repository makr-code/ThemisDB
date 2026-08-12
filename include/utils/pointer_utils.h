/**
 * @file pointer_utils.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <stdexcept>
#include <optional>
#include <functional>
#include <type_traits>
#include <spdlog/spdlog.h>

namespace themis {
namespace utils {
namespace pointer {

/**
 * @brief Validates that a pointer is non-null, throws exception if null
 * 
 * @tparam T Type of the pointer
 * @param ptr Pointer to check
 * @param message Error message to use if null
 * @return T* The same pointer (guaranteed non-null)
 * @throws std::runtime_error if ptr is nullptr
 * 
 * @par Example
 * SomeType* ptr = get_pointer();
 * auto* validated = require_non_null(ptr, "get_pointer returned null");
 * validated->method(); // Safe - guaranteed non-null
 */
template<typename T>
T* require_non_null(T* ptr, const char* message = "Null pointer") {
    if (!ptr) {
        spdlog::error("Null pointer check failed: {}", message);
        throw std::runtime_error(message);
    }
    return ptr;
}

/**
 * @brief Converts a raw pointer to std::optional
 * 
 * @tparam T Type of the pointer
 * @param ptr Pointer to convert
 * @return std::optional<T*> Optional containing pointer or nullopt
 * 
 * @par Example
 * SomeType* ptr = get_pointer();
 * if (auto opt = as_optional(ptr)) {
 *     (*opt)->method(); // Safe usage
 * }
 */
template<typename T>
std::optional<T*> as_optional(T* ptr) noexcept {
    return ptr ? std::optional<T*>(ptr) : std::nullopt;
}

/**
 * @brief Safely invokes a function on a pointer if non-null
 * 
 * @tparam T Type of the pointer
 * @tparam Func Function type
 * @param ptr Pointer to dereference
 * @param func Function to invoke on dereferenced pointer
 * @return std::optional with result, or nullopt if ptr is null
 * 
 * @par Example
 * SomeType* ptr = get_pointer();
 * auto result = safe_invoke(ptr, [](SomeType& obj) {
 *     return obj.compute();
 * });
 * if (result) {
 *     // Use result.value()
 * }
 */
template<typename T, typename Func>
auto safe_invoke(T* ptr, Func&& func)
    -> std::optional<std::conditional_t<std::is_void_v<decltype(func(*ptr))>, std::monostate, decltype(func(*ptr))>> {
    using ResultType = decltype(func(*ptr));
    using OptionalType = std::optional<std::conditional_t<std::is_void_v<ResultType>, std::monostate, ResultType>>;

    if (!ptr) {
        return std::nullopt;
    }

    if constexpr (std::is_void_v<ResultType>) {
        func(*ptr);
        return OptionalType{std::monostate{}};
    } else {
        return OptionalType{func(*ptr)};
    }
}

/**
 * @brief RAII wrapper for C API pointers with custom deleter
 * 
 * @tparam T Type of the pointer
 * @tparam Deleter Deleter function type
 * @param ptr C API pointer to wrap
 * @param deleter Cleanup function
 * @return std::unique_ptr with custom deleter
 * @throws std::runtime_error if ptr is nullptr
 * 
 * @par Example
 * auto file_ptr = wrap_c_ptr(fopen("file.txt", "r"), [](FILE* f) {
 *     if (f) fclose(f);
 * });
 * // File automatically closed on scope exit
 */
template<typename T, typename Deleter>
std::unique_ptr<T, Deleter> wrap_c_ptr(T* ptr, Deleter deleter) {
    if (!ptr) {
        spdlog::error("C API returned null pointer");
        throw std::runtime_error("C API returned null pointer");
    }
    return std::unique_ptr<T, Deleter>(ptr, deleter);
}

/**
 * @brief Validates dynamic_cast result, returns optional
 * 
 * @tparam Derived Target type to cast to
 * @tparam Base Source type
 * @param base Base pointer to cast
 * @return std::optional<Derived*> Optional containing casted pointer or nullopt
 * 
 * @par Example
 * Base* base = get_base();
 * if (auto derived = safe_dynamic_cast<Derived>(base)) {
 *     (*derived)->derived_method();
 * } else {
 *     spdlog::warn("Dynamic cast failed");
 * }
 */
template<typename Derived, typename Base>
std::optional<Derived*> safe_dynamic_cast(Base* base) noexcept {
    if (!base) {
        return std::nullopt;
    }
    Derived* derived = dynamic_cast<Derived*>(base);
    return derived ? std::optional<Derived*>(derived) : std::nullopt;
}

/**
 * @brief Validates shared_ptr from weak_ptr::lock()
 * 
 * @tparam T Type of the shared pointer
 * @param weak Weak pointer to lock
 * @param message Warning message if lock fails
 * @return std::optional<std::shared_ptr<T>> Locked shared_ptr or nullopt
 * 
 * @par Example
 * std::weak_ptr<Resource> weak = get_weak();
 * if (auto shared = safe_lock(weak, "Resource expired")) {
 *     (*shared)->use();
 * }
 */
template<typename T>
std::optional<std::shared_ptr<T>> safe_lock(
    const std::weak_ptr<T>& weak,
    const char* message = "weak_ptr lock failed"
) noexcept {
    auto shared = weak.lock();
    if (!shared) {
        spdlog::debug("weak_ptr lock failed: {}", message);
        return std::nullopt;
    }
    return shared;
}

/**
 * @brief Validates container access with bounds checking
 * 
 * @tparam Container Container type (e.g., std::vector, std::map)
 * @tparam Key Key/index type
 * @param container Container to access
 * @param key Key or index
 * @return std::optional with value or nullopt if not found
 * 
 * @par Example
 * std::map<std::string, int> map;
 * if (auto val = safe_at(map, "key")) {
 *     // Use val.value()
 * }
 */
template<typename Container, typename Key>
auto safe_at(const Container& container, const Key& key) 
    -> std::optional<typename Container::mapped_type> {
    auto it = container.find(key);
    if (it != container.end()) {
        return it->second;
    }
    return std::nullopt;
}

// Overload for vector/array types with operator[]
template<typename T>
std::optional<std::reference_wrapper<const T>> safe_at(
    const std::vector<T>& vec, 
    size_t index
) noexcept {
    if (index < vec.size()) {
        return std::cref(vec[index]);
    }
    return std::nullopt;
}

template<typename T>
std::optional<std::reference_wrapper<T>> safe_at(
    std::vector<T>& vec, 
    size_t index
) noexcept {
    if (index < vec.size()) {
        return std::ref(vec[index]);
    }
    return std::nullopt;
}

} // namespace pointer
} // namespace utils
} // namespace themis
