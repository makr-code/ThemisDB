/**
 * @file json_helpers.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.7
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <initializer_list>

namespace themis {
namespace json_util {

// ---------------------------------------------------------------------------
// safeGet – returns std::optional<T>, never throws
// ---------------------------------------------------------------------------

/**
 * @brief Try to read `j[key]` as type T.
 *
 * @return The value wrapped in std::optional, or std::nullopt if the key
 *         is absent or the stored value cannot be converted to T.
 */
template <typename T>
[[nodiscard]] inline std::optional<T> safeGet(
        const nlohmann::json& j,
        std::string_view key) noexcept
{
    try {
        const auto it = j.find(key);
        if (it == j.end()) {
          return std::nullopt;
        }
        return it->template get<T>();
    } catch (...) {
        return std::nullopt;
    }
}

// ---------------------------------------------------------------------------
// getOrDefault – returns value or caller-supplied default, never throws
// ---------------------------------------------------------------------------

/**
 * @brief Return `j[key]` as type T, or @p default_value when the key is
 *        absent or the value cannot be converted.
 */
template <typename T>
[[nodiscard]] inline T getOrDefault(
        const nlohmann::json& j,
        std::string_view key,
        T default_value) noexcept
{
    auto opt = safeGet<T>(j, key);
    return opt ? std::move(*opt) : std::move(default_value);
}

// ---------------------------------------------------------------------------
// requireField – returns value or throws std::invalid_argument
// ---------------------------------------------------------------------------

/**
 * @brief Return `j[key]` as type T.
 *
 * @throws std::invalid_argument if the key is absent.
 * @throws std::invalid_argument if the stored value cannot be converted to T.
 */
template <typename T>
[[nodiscard]] inline T requireField(
        const nlohmann::json& j,
        std::string_view key)
{
    const auto it = j.find(key);
    if (it == j.end()) {
        throw std::invalid_argument(
            std::string("Required JSON field missing: ") + std::string(key));
    }
    try {
        return it->template get<T>();
    } catch (const nlohmann::json::exception& ex) {
        throw std::invalid_argument(
            std::string("JSON field '") + std::string(key)
            + "' has wrong type: " + ex.what());
    }
}

// ---------------------------------------------------------------------------
// getNestedOrDefault – dot-path access, never throws
// ---------------------------------------------------------------------------

/**
 * @brief Traverse a sequence of keys into a nested JSON object and return the
 *        leaf value as T, or @p default_value on any failure.
 *
 * Example:  getNestedOrDefault<int>(j, {"server", "port"}, 8080)
 */
template <typename T>
[[nodiscard]] inline T getNestedOrDefault(
        const nlohmann::json& j,
        std::initializer_list<std::string_view> keys,
        T default_value) noexcept
{
    try {
        const nlohmann::json* cur = &j;
        for (auto&& k : keys) {
            const auto it = cur->find(k);
            if (it == cur->end()) {
              return default_value;
            }
            cur = &(*it);
        }
        return cur->template get<T>();
    } catch (...) {
        return default_value;
    }
}

// ---------------------------------------------------------------------------
// getOrDefaultStr – convenience overload for std::string with string literal
// ---------------------------------------------------------------------------

/**
 * @brief Return `j[key]` as std::string or @p default_value.
 *
 * Provided because `getOrDefault(j, key, "literal")` would otherwise
 * deduce T as `const char*` rather than `std::string`.
 */
[[nodiscard]] inline std::string getOrDefaultStr(
        const nlohmann::json& j,
        std::string_view key,
        std::string default_value = {}) noexcept
{
    return getOrDefault<std::string>(j, key, std::move(default_value));
}

// ---------------------------------------------------------------------------
// safeObject – return sub-object or empty object, never throws
// ---------------------------------------------------------------------------

/**
 * @brief Return `j[key]` if it is a JSON object, otherwise return an empty
 *        JSON object.  Useful to silently ignore missing configuration blocks.
 */
[[nodiscard]] inline nlohmann::json safeObject(
        const nlohmann::json& j,
        std::string_view key) noexcept
{
    try {
        const auto it = j.find(key);
        if (it != j.end() && it->is_object()) {
          return *it;
        }
    } catch (...) {}
    return nlohmann::json::object();
}

// ---------------------------------------------------------------------------
// safeArray – return sub-array or empty array, never throws
// ---------------------------------------------------------------------------

/**
 * @brief Return `j[key]` if it is a JSON array, otherwise return an empty
 *        JSON array.
 */
[[nodiscard]] inline nlohmann::json safeArray(
        const nlohmann::json& j,
        std::string_view key) noexcept
{
    try {
        const auto it = j.find(key);
        if (it != j.end() && it->is_array()) {
          return *it;
        }
    } catch (...) {}
    return nlohmann::json::array();
}

} // namespace json_util
} // namespace themis

