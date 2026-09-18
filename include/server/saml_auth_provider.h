/**
 * @file saml_auth_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/saml_authenticator.h"

#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>

namespace themis {
namespace server {

class SamlAuthProvider {
public:
    struct Config {
        auth::SAMLConfig saml;

        std::string idp_slo_url;

        std::string sp_slo_url;

        std::string org_name;
        std::string org_display_name;
        std::string org_url;

        std::string contact_email;

        std::function<std::string(const auth::SAMLClaims&)> token_factory;
    };

    /**
     * @brief Saml Auth Provider.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit SamlAuthProvider(const Config& config);

    ~SamlAuthProvider() = default;

    // Non-copyable, movable
    SamlAuthProvider(const SamlAuthProvider&) = delete;
    SamlAuthProvider& operator=(const SamlAuthProvider&) = delete;
    SamlAuthProvider(SamlAuthProvider&&) noexcept = default;
    SamlAuthProvider& operator=(SamlAuthProvider&&) noexcept = default;

    // -----------------------------------------------------------------------
    // HTTP handlers – each returns a JSON result with an embedded status_code
    // field on error (mirroring the SessionApiHandler convention).
    // -----------------------------------------------------------------------

    nlohmann::json handleLogin(const std::string& relay_state = "");

    nlohmann::json handleAcs(
        const std::string& saml_response_b64,
        const std::string& relay_state = "",
        const std::string& in_response_to = "");

    nlohmann::json handleSlo(const std::string& session_index = "");

    /**
     * @brief Build Metadata Xml.
     * @return Return value.
     */
    std::string buildMetadataXml() const;

    void setClockForTesting(
        std::function<std::chrono::system_clock::time_point()> clock);

private:
    Config config_;
    std::unique_ptr<auth::SAMLAuthenticator> authenticator_;

    mutable std::mutex pending_mutex_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> pending_requests_;

    /**
     * @brief Evict Expired Pending Requests.
     */
    void evictExpiredPendingRequests();

    /**
     * @brief Make Error.
     * @param[in] status_code Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    static nlohmann::json makeError(int status_code, const std::string& message);

    /**
     * @brief Default Token Factory.
     * @param[in] claims Input parameter.
     * @return Return value.
     */
    static std::string defaultTokenFactory(const auth::SAMLClaims& claims);

    /**
     * @brief Url Encode.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    static std::string urlEncode(const std::string& input);
};

} // namespace server
} // namespace themis

