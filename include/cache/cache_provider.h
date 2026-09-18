/**
 * @file cache_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace themis { namespace cache {

struct CacheValue {
    std::string payload;   // serialized JSON (entity or result page)
    uint64_t version{0};   // monotone version (e.g., WAL index)
    uint64_t ts_ms{0};     // insert timestamp (ms)
};

class CacheProvider {
public:
    /**
     * @brief Cache Provider.
     * @return Return value.
     */
    virtual ~CacheProvider() = default;
    [[nodiscard]] virtual bool Get(std::string_view key, CacheValue& out) = 0;
    /**
     * @brief Put.
     * @param[in] key Input parameter.
     * @param[in] v Input parameter.
     * @param[in] ttl_ms Input parameter.
     */
    virtual void Put(std::string_view key, const CacheValue& v, uint64_t ttl_ms) = 0;
    /**
     * @brief Invalidate.
     * @param[in] key Input parameter.
     */
    virtual void Invalidate(std::string_view key) = 0;
};

/**
 * @brief Make Entity Key.
 * @param[in] urn Input parameter.
 * @return Return value.
 * @details Implements makeEntityKey without additional internal calls.
 */
inline std::string makeEntityKey(const std::string& urn) { return urn; }

}} // namespace themis::cache
