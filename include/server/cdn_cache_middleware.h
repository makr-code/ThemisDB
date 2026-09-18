/**
 * @file cdn_cache_middleware.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast/http.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

enum class CacheDirective {
    PUBLIC,     ///< public – CDN and browser may cache
    PRIVATE,    ///< private – browser only, not shared caches
    NO_CACHE,   ///< no-cache – revalidate with origin on every use
    NO_STORE,   ///< no-store – do not persist at all (sensitive data)
};

struct CdnRoutePolicy {
    CacheDirective directive{CacheDirective::NO_STORE};

    uint32_t max_age_seconds{0};

    uint32_t cdn_max_age_seconds{0};

    uint32_t stale_while_revalidate_seconds{0};

    uint32_t stale_if_error_seconds{0};

    std::string surrogate_keys;

    bool enable_etag{false};

    bool emit_cdn_cache_control{true};

    bool emit_surrogate_control{false};
};

class CdnCacheMiddleware {
public:
    CdnCacheMiddleware() = default;


    /**
     * @brief Register a retention policy.
     * @param[in] path_prefix Input parameter.
     * @param[in] policy Retention policy definition to store.
     */
    void registerPolicy(const std::string& path_prefix, const CdnRoutePolicy& policy);


    /**
     * @brief Apply.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void apply(
        const http::request<http::string_body>&  req,
        http::response<http::string_body>&        res) const;

    /**
     * @brief Check Conditional.
     * @param[in] req Input parameter.
     * @param[in] res Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkConditional(
        const http::request<http::string_body>&  req,
        const http::response<http::string_body>& res) const;


    /**
     * @brief Generate ETag.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    static std::string generateETag(const std::string& body);

    /**
     * @brief Build Cache Control Value.
     * @param[in] policy Input parameter.
     * @param[in] is_write Input parameter.
     * @return Return value.
     */
    static std::string buildCacheControlValue(const CdnRoutePolicy& policy, bool is_write);

private:
    std::unordered_map<std::string, CdnRoutePolicy> policies_;

    /**
     * @brief Find Policy.
     * @param[in] path Input parameter.
     * @return Pointer to the result.
     */
    const CdnRoutePolicy* findPolicy(const std::string& path) const;

    /**
     * @brief Extract Path.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    static std::string extractPath(const http::request<http::string_body>& req);

    /**
     * @brief Is Write Method.
     * @param[in] method Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isWriteMethod(http::verb method);

    /**
     * @brief Is Server Error.
     * @param[in] status Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isServerError(http::status status);
};

} // namespace server
} // namespace themis
