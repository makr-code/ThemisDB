/**
 * @file safe_access.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <optional>
#include <functional>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include "utils/logger.h"

namespace themis {
namespace utils {

/**
 * @brief Safe vector element access with bounds checking
 * 
 * Returns std::nullopt if index is out of bounds, preventing crashes.
 * 
 * @tparam T Element type
 * @param vec Vector to access
 * @param index Index to retrieve
 * @return std::optional with reference wrapper to element, or nullopt if out of bounds
 * 
 * @code
 * std::vector<int> data = {1, 2, 3};
 * if (auto val = safe_get(data, 1)) {
 *     process(val->get());
 * }
 * @endcode
 */
template<typename T>
std::optional<std::reference_wrapper<T>> 
safe_get(std::vector<T>& vec, size_t index) {
    if (index >= vec.size()) {
        THEMIS_WARN("safe_get: index {} out of bounds (size {})", index, vec.size());
        return std::nullopt;
    }
    return std::ref(vec[index]);
}

/**
 * @brief Safe vector element access (const version)
 */
template<typename T>
std::optional<std::reference_wrapper<const T>> 
safe_get(const std::vector<T>& vec, size_t index) {
    if (index >= vec.size()) {
        THEMIS_WARN("safe_get: index {} out of bounds (size {})", index, vec.size());
        return std::nullopt;
    }
    return std::cref(vec[index]);
}

/**
 * @brief Safe map value access with key checking
 * 
 * Returns std::nullopt if key is not found, preventing crashes.
 * 
 * @tparam K Key type
 * @tparam V Value type
 * @param map Map to access
 * @param key Key to look up
 * @return std::optional with reference wrapper to value, or nullopt if key not found
 * 
 * @code
 * std::map<std::string, Config> configs;
 * if (auto cfg = safe_get(configs, "prod")) {
 *     use(cfg->get());
 * }
 * @endcode
 */
template<typename K, typename V>
std::optional<std::reference_wrapper<V>>
safe_get(std::map<K, V>& map, const K& key) {
    auto it = map.find(key);
    if (it == map.end()) {
        THEMIS_DEBUG("safe_get: key not found in map");
        return std::nullopt;
    }
    return std::ref(it->second);
}

/**
 * @brief Safe map value access (const version)
 */
template<typename K, typename V>
std::optional<std::reference_wrapper<const V>>
safe_get(const std::map<K, V>& map, const K& key) {
    auto it = map.find(key);
    if (it == map.end()) {
        THEMIS_DEBUG("safe_get: key not found in map");
        return std::nullopt;
    }
    return std::cref(it->second);
}

/**
 * @brief Safe raw pointer dereference with null checking
 * 
 * Returns std::nullopt if pointer is null, preventing null dereference crashes.
 * 
 * @tparam T Pointed-to type
 * @param ptr Pointer to check and dereference
 * @return std::optional with reference wrapper to object, or nullopt if ptr is null
 * 
 * @code
 * Node* node = getNode();
 * if (auto n = safe_deref(node)) {
 *     process(n->get());
 * }
 * @endcode
 */
template<typename T>
std::optional<std::reference_wrapper<T>>
safe_deref(T* ptr) {
    if (ptr == nullptr) {
        THEMIS_WARN("safe_deref: null pointer dereference prevented");
        return std::nullopt;
    }
    return std::ref(*ptr);
}

/**
 * @brief Safe raw pointer dereference (const version)
 */
template<typename T>
std::optional<std::reference_wrapper<const T>>
safe_deref(const T* ptr) {
    if (ptr == nullptr) {
        THEMIS_WARN("safe_deref: null pointer dereference prevented");
        return std::nullopt;
    }
    return std::cref(*ptr);
}

/**
 * @brief Safe smart pointer get with null checking
 * 
 * Throws std::runtime_error if smart pointer is null.
 * Use this for situations where null is a programming error.
 * 
 * @tparam T Pointed-to type
 * @param ptr Smart pointer to check
 * @param context Context string for error message
 * @return Raw pointer (guaranteed non-null)
 * @throws std::runtime_error if ptr is null
 * 
 * @code
 * auto resource = checked_get(resourcePtr, "resource initialization");
 * resource->use();  // Safe - will throw if null
 * @endcode
 */
template<typename T>
T* checked_get(const std::shared_ptr<T>& ptr, const char* context = "unknown") {
    if (!ptr) {
        std::string msg = std::string("checked_get: null shared_ptr in context '") + context + "'";
        THEMIS_ERROR("{}", msg);
        throw std::runtime_error(msg);
    }
    return ptr.get();
}

/**
 * @brief Safe smart pointer get (unique_ptr version)
 */
template<typename T>
T* checked_get(const std::unique_ptr<T>& ptr, const char* context = "unknown") {
    if (!ptr) {
        std::string msg = std::string("checked_get: null unique_ptr in context '") + context + "'";
        THEMIS_ERROR("{}", msg);
        throw std::runtime_error(msg);
    }
    return ptr.get();
}

/**
 * @brief Safe dynamic_cast with optional result
 * 
 * Returns std::nullopt if cast fails, making null checks explicit.
 * 
 * @tparam Target Target type for cast
 * @tparam Source Source type
 * @param ptr Pointer to cast
 * @return std::optional with pointer to Target, or nullopt if cast fails
 * 
 * @code
 * Base* base = getBase();
 * if (auto derived = safe_cast<Derived>(base)) {
 *     derived.value()->specificMethod();
 * }
 * @endcode
 */
template<typename Target, typename Source>
std::optional<Target*> safe_cast(Source* ptr) {
    if (!ptr) {
        THEMIS_DEBUG("safe_cast: null pointer passed");
        return std::nullopt;
    }
    Target* result = dynamic_cast<Target*>(ptr);
    if (!result) {
        THEMIS_DEBUG("safe_cast: cast failed from {} to {}", 
                     typeid(Source).name(), typeid(Target).name());
        return std::nullopt;
    }
    return result;
}

/**
 * @brief Safe dynamic_cast (const version)
 */
template<typename Target, typename Source>
std::optional<const Target*> safe_cast(const Source* ptr) {
    if (!ptr) {
        THEMIS_DEBUG("safe_cast: null pointer passed");
        return std::nullopt;
    }
    const Target* result = dynamic_cast<const Target*>(ptr);
    if (!result) {
        THEMIS_DEBUG("safe_cast: cast failed from {} to {}", 
                     typeid(Source).name(), typeid(Target).name());
        return std::nullopt;
    }
    return result;
}

} // namespace utils
} // namespace themis

