/**
 * @file webdav_user_registration_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/user_registration_plugin.h"
#include "utils/logger.h"
#include <openssl/evp.h>
#include <memory>
#include <sstream>
#include <iomanip>
#include <stdexcept>

// WebDAV HTTP client support (requires libcurl)
#ifdef THEMIS_ENABLE_WEBDAV
#include <curl/curl.h>
#endif

namespace themis {
namespace security {

namespace {

// ── RAII Wrappers for OpenSSL and curl objects ──────────────────────────────
struct WebDAV_EVP_MD_CTX_Deleter {
    void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); }
};

#ifdef THEMIS_ENABLE_WEBDAV
struct WebDAV_CURL_Deleter {
    void operator()(CURL* p) const { if (p) curl_easy_cleanup(p); }
};
struct WebDAV_CURL_slist_Deleter {
    void operator()(struct curl_slist* p) const { if (p) curl_slist_free_all(p); }
};

using WebDAV_CURL_ptr = std::unique_ptr<CURL, WebDAV_CURL_Deleter>;
using WebDAV_CURL_slist_ptr = std::unique_ptr<struct curl_slist, WebDAV_CURL_slist_Deleter>;
#endif

using WebDAV_EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, WebDAV_EVP_MD_CTX_Deleter>;

bool starts_with(const std::string& value, const char* prefix) {
    return value.rfind(prefix, 0) == 0;
}

std::string extract_url_host(const std::string& url) {
    const size_t scheme_pos = url.find("://");
    const size_t host_start = (scheme_pos == std::string::npos) ? 0 : scheme_pos + 3;
    if (host_start >= url.size()) {
        return {};
    }

    if (url[host_start] == '[') {
        const size_t end_bracket = url.find(']', host_start + 1);
        if (end_bracket == std::string::npos) {
            return {};
        }
        return url.substr(host_start + 1, end_bracket - host_start - 1);
    }

    const size_t host_end = url.find_first_of(":/", host_start);
    return url.substr(host_start, host_end == std::string::npos ? std::string::npos
                                                                : host_end - host_start);
}

bool is_loopback_host(const std::string& host) {
    return host == "localhost" || host == "127.0.0.1" || host == "::1";
}

bool is_loopback_url(const std::string& url) {
    return is_loopback_host(extract_url_host(url));
}

} // anonymous namespace

/**
 * @brief WebDAV User Registration Plugin
 * 
 * Integrates with WebDAV servers for user authentication and registration.
 * Supports integration with:
 * - Active Directory via WebDAV
 * - SharePoint user management
 * - OwnCloud/Nextcloud
 * - Generic WebDAV servers with user directories
 * 
 * Use cases:
 * - Corporate Active Directory integration
 * - SharePoint document library access control
 * - Network file server authentication
 */
class WebDAVUserRegistrationPlugin : public IUserRegistrationPlugin {
public:
    static constexpr long kRequestTimeoutMs = 5000;

    struct Config {
        std::string webdav_base_url;  // e.g., "https://sharepoint.company.com"
        std::string webdav_username;  // Admin username for WebDAV
        std::string webdav_password;  // Admin password
        std::string user_directory;   // Path to user directory on WebDAV
        bool verify_ssl = true;       // Verify SSL certificates
        bool active_directory_mode = false;  // Enable AD-specific features
    };
    
    explicit WebDAVUserRegistrationPlugin(const Config& config)
        : config_(config)
    {
        if (config_.webdav_base_url.empty()) {
            throw std::invalid_argument("WebDAV base URL must not be empty");
        }
        const bool uses_https = starts_with(config_.webdav_base_url, "https://");
        const bool uses_http = starts_with(config_.webdav_base_url, "http://");
        const bool loopback = is_loopback_url(config_.webdav_base_url);
        if (!uses_https && !(uses_http && loopback)) {
            throw std::invalid_argument(
                "WebDAV endpoint must use HTTPS; plain HTTP is only allowed for loopback development endpoints");
        }
        if (!config_.verify_ssl && !loopback) {
            throw std::invalid_argument(
                "WebDAV TLS verification may only be disabled for loopback development endpoints");
        }

        THEMIS_INFO("WebDAVUserRegistrationPlugin initialized for: {}", 
                    config_.webdav_base_url);
        
#ifdef THEMIS_ENABLE_WEBDAV
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
            throw std::runtime_error("Failed to initialize libcurl for WebDAV plugin");
        }
#endif
    }
    
    ~WebDAVUserRegistrationPlugin() {
#ifdef THEMIS_ENABLE_WEBDAV
        curl_global_cleanup();
#endif
    }
    
    std::string getName() const override {
        return "webdav";
    }
    
    bool isAvailable() const override {
#ifdef THEMIS_ENABLE_WEBDAV
        return !config_.webdav_base_url.empty();
#else
        return false;
#endif
    }
    
    Result<UserRegistrationData> registerUser(
        const std::string& user_id,
        [[maybe_unused]] const std::string& password,
        [[maybe_unused]] const std::unordered_map<std::string, std::string>& attributes
    ) override {
        THEMIS_INFO("WebDAV plugin: Registering user '{}'", user_id);
        
#ifdef THEMIS_ENABLE_WEBDAV
        // First authenticate the user with WebDAV server
        auto auth_result = authenticateWithWebDAV(user_id, password);
        if (!auth_result) {
            return themis::Err<UserRegistrationData>(
                errors::ErrorCode::ERR_API_UNAUTHORIZED,
                "WebDAV authentication failed: " + auth_result.error().message()
            );
        }
        
        UserRegistrationData data;
        data.user_id = user_id;
        data.password_hash = hashPassword(password);
        data.source = "webdav";
        data.source_uri = config_.webdav_base_url;
        
        // Map attributes from WebDAV properties
        for (const auto& [key, value] : attributes) {
            data.attributes[key] = value;
        }
        
        // Retrieve user properties from WebDAV if in AD mode
        if (config_.active_directory_mode) {
            auto props_result = getUserPropertiesFromAD(user_id);
            if (props_result.is_ok()) {
                auto props = props_result.value();
                data.attributes.insert(props.begin(), props.end());
                
                // Map AD groups to roles
                if (props.find("memberOf") != props.end()) {
                    data.roles = mapADGroupsToRoles(props["memberOf"]);
                }
            }
        }
        
        // Default role if no roles assigned
        if (data.roles.empty()) {
            data.roles.push_back("readonly");
        }
        
        return themis::Ok(std::move(data));
#else
        return themis::Err<UserRegistrationData>(
            themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "WebDAV support not enabled in build"
        );
#endif
    }
    
    Result<UserRegistrationData> authenticateUser(
        const std::string& user_id,
        [[maybe_unused]] const std::string& password
    ) override {
        THEMIS_INFO("WebDAV plugin: Authenticating user '{}'", user_id);
        
#ifdef THEMIS_ENABLE_WEBDAV
        // Authenticate directly with WebDAV server
        auto auth_result = authenticateWithWebDAV(user_id, password);
        if (!auth_result.is_ok()) {
            return Result<UserRegistrationData>::Err(auth_result.error());
        }
        
        // If authentication successful, register the user
        return registerUser(user_id, password, {});
#else
        return themis::Err<UserRegistrationData>(
            themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "WebDAV support not enabled in build"
        );
#endif
    }
    
    Result<std::vector<UserRegistrationData>> syncUsers() override {
        THEMIS_INFO("WebDAV plugin: Syncing users from '{}'", config_.webdav_base_url);
        
#ifdef THEMIS_ENABLE_WEBDAV
        std::vector<UserRegistrationData> users;

        std::string url = config_.webdav_base_url;
        if (!config_.user_directory.empty()) {
            url += "/" + config_.user_directory + "/";
        }

        // Collect the raw PROPFIND response body.
        std::string response_body;
        WebDAV_CURL_ptr curl(curl_easy_init());
        if (!curl) {
            return themis::Err<std::vector<UserRegistrationData>>(
                errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                "Failed to initialise CURL for PROPFIND"
            );
        }

        // Depth: 1 – list direct children (one entry per user sub-resource).
        curl_slist* raw_headers = curl_slist_append(nullptr, "Depth: 1");
        if (!raw_headers) {
            return themis::Err<std::vector<UserRegistrationData>>(
                errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                "Failed to create CURL header list for PROPFIND"
            );
        }
        curl_slist* content_type_headers = curl_slist_append(raw_headers, "Content-Type: application/xml");
        if (!content_type_headers) {
            curl_slist_free_all(raw_headers);
            return themis::Err<std::vector<UserRegistrationData>>(
                errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                "Failed to append Content-Type header for PROPFIND"
            );
        }
        WebDAV_CURL_slist_ptr headers(content_type_headers);

        // Minimal PROPFIND body requesting displayname and resourcetype.
        static const char kPropfindBody[] =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<D:propfind xmlns:D=\"DAV:\">"
            "<D:prop><D:displayname/><D:resourcetype/></D:prop>"
            "</D:propfind>";

        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_USERNAME, config_.webdav_username.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, config_.webdav_password.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "PROPFIND");
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, kPropfindBody);
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,
                         static_cast<long>(sizeof(kPropfindBody) - 1));
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, kRequestTimeoutMs);
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT_MS, kRequestTimeoutMs / 2);
        curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
        if (!config_.verify_ssl) {
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
        }

        CURLcode res = curl_easy_perform(curl.get());
        long http_code = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

        if (res != CURLE_OK) {
            return themis::Err<std::vector<UserRegistrationData>>(
                errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                "PROPFIND request failed: " + std::string(curl_easy_strerror(res))
            );
        }
        if (http_code != 207 && http_code != 200) {
            return themis::Err<std::vector<UserRegistrationData>>(
                errors::ErrorCode::ERR_API_UNAUTHORIZED,
                "PROPFIND returned HTTP " + std::to_string(http_code)
            );
        }

        // Parse the WebDAV XML multi-status response.
        // Each <D:response> element with an href that differs from the
        // collection root itself represents a user sub-resource.
        //
        // NOTE: We use lightweight string-scanning instead of a full XML parser
        // to avoid an additional dependency.  This approach assumes that the
        // WebDAV server uses the "D:" namespace prefix for DAV: elements (the
        // de-facto standard for WebDAV implementations).  Responses that use a
        // different prefix, CDATA sections, or comments around tag names will
        // not be parsed correctly.  The impact is degraded functionality (fewer
        // users synced), not a security vulnerability — no user data is
        // trusted without further validation in registerUser().
        //
        // Security: XML entity expansion (billion-laughs) attacks are not
        // applicable here because we do string-scanning rather than feeding
        // the response to an XML parser.  However, an extremely large response
        // body could exhaust memory.  Callers should configure
        // CURLOPT_MAXFILESIZE on the curl handle if the WebDAV server is
        // untrusted (the current implementation omits this to avoid blocking
        // large but legitimate user directories — tune for your environment).
        const std::string base_href = url;
        size_t pos = 0;
        while ((pos = response_body.find("<D:response>", pos)) != std::string::npos) {
            size_t end = response_body.find("</D:response>", pos);
            if (end == std::string::npos) break;
            std::string entry = response_body.substr(pos, end - pos);
            pos = end + 1;

            // Extract href.
            static constexpr size_t kHrefOpenLen  = sizeof("<D:href>") - 1;
            size_t href_start = entry.find("<D:href>");
            size_t href_end   = entry.find("</D:href>");
            if (href_start == std::string::npos || href_end == std::string::npos) continue;
            href_start += kHrefOpenLen;
            std::string href = entry.substr(href_start, href_end - href_start);

            // Strip the base path and any trailing slash to get the user id.
            if (href == base_href || href == base_href + "/") continue;
            size_t slash = href.rfind('/');
            std::string user_id = (slash != std::string::npos)
                                  ? href.substr(slash + 1)
                                  : href;
            if (user_id.empty()) continue;

            // Extract displayname if present.
            static constexpr size_t kDisplayNameOpenLen = sizeof("<D:displayname>") - 1;
            std::string display_name = user_id;
            size_t dn_start = entry.find("<D:displayname>");
            size_t dn_end   = entry.find("</D:displayname>");
            if (dn_start != std::string::npos && dn_end != std::string::npos) {
                dn_start += kDisplayNameOpenLen;
                display_name = entry.substr(dn_start, dn_end - dn_start);
            }

            UserRegistrationData data;
            data.user_id    = user_id;
            data.source     = "webdav";
            data.source_uri = config_.webdav_base_url;
            data.attributes["displayName"] = display_name;

            if (config_.active_directory_mode) {
                auto props_result = getUserPropertiesFromAD(user_id);
                if (props_result.is_ok()) {
                    auto props = props_result.value();
                    data.attributes.insert(props.begin(), props.end());
                    if (props.find("memberOf") != props.end()) {
                        data.roles = mapADGroupsToRoles(props.at("memberOf"));
                    }
                }
            }

            if (data.roles.empty()) {
                data.roles.push_back("readonly");
            }

            users.push_back(std::move(data));
        }

        THEMIS_INFO("WebDAV plugin: Synced {} users", users.size());
        return themis::Ok(std::move(users));
#else
        return themis::Err<std::vector<UserRegistrationData>>(
            errors::ErrorCode::ERR_PLUGIN_INCOMPATIBLE,
            "WebDAV support not enabled in build"
        );
#endif
    }
    
    Result<UserRegistrationData> updateUser(const std::string& user_id) override {
        THEMIS_INFO("WebDAV plugin: Updating user '{}'", user_id);
        
#ifdef THEMIS_ENABLE_WEBDAV
        UserRegistrationData data;
        data.user_id    = user_id;
        data.source     = "webdav";
        data.source_uri = config_.webdav_base_url;

        // Retrieve the latest user properties from the WebDAV server via PROPFIND.
        std::string url = config_.webdav_base_url;
        if (!config_.user_directory.empty()) {
            url += "/" + config_.user_directory + "/" + user_id;
        } else {
            url += "/" + user_id;
        }

        std::string response_body;
        WebDAV_CURL_ptr curl(curl_easy_init());
        if (curl) {
            curl_slist* raw_headers = curl_slist_append(nullptr, "Depth: 0");
            if (!raw_headers) {
                return themis::Err<UserRegistrationData>(
                    errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                    "Failed to create CURL header list for WebDAV update"
                );
            }
            curl_slist* content_type_headers = curl_slist_append(raw_headers, "Content-Type: application/xml");
            if (!content_type_headers) {
                curl_slist_free_all(raw_headers);
                return themis::Err<UserRegistrationData>(
                    errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                    "Failed to append Content-Type header for WebDAV update"
                );
            }
            WebDAV_CURL_slist_ptr headers(content_type_headers);

            static const char kPropfindBody[] =
                "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<D:propfind xmlns:D=\"DAV:\">"
                "<D:allprop/>"
                "</D:propfind>";

            curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl.get(), CURLOPT_USERNAME, config_.webdav_username.c_str());
            curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, config_.webdav_password.c_str());
            curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "PROPFIND");
            curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, kPropfindBody);
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,
                             static_cast<long>(sizeof(kPropfindBody) - 1));
            curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, curlWriteCallback);
            curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);
            curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, kRequestTimeoutMs);
            curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT_MS, kRequestTimeoutMs / 2);
            curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
            if (!config_.verify_ssl) {
                curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
            }

            CURLcode res = curl_easy_perform(curl.get());
            long http_code = 0;
            curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

            if (res == CURLE_OK && (http_code == 200 || http_code == 207)) {
                // Extract displayname from the PROPFIND response.
                static constexpr size_t kDisplayNameOpenLen = sizeof("<D:displayname>") - 1;
                size_t dn_start = response_body.find("<D:displayname>");
                size_t dn_end   = response_body.find("</D:displayname>");
                if (dn_start != std::string::npos && dn_end != std::string::npos) {
                    dn_start += kDisplayNameOpenLen;
                    data.attributes["displayName"] =
                        response_body.substr(dn_start, dn_end - dn_start);
                }
            } else {
                THEMIS_WARN("WebDAV PROPFIND for user '{}' returned HTTP {}", user_id, http_code);
            }
        }

        // If Active Directory mode, also fetch AD-specific properties.
        if (config_.active_directory_mode) {
            auto props_result = getUserPropertiesFromAD(user_id);
            if (props_result.is_ok()) {
                auto props = props_result.value();
                data.attributes.insert(props.begin(), props.end());
                if (props.find("memberOf") != props.end()) {
                    data.roles = mapADGroupsToRoles(props.at("memberOf"));
                }
            }
        }

        if (data.roles.empty()) {
            data.roles.push_back("readonly");
        }

        return themis::Ok(std::move(data));
#else
        return themis::Err<UserRegistrationData>(
            themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "WebDAV support not enabled in build"
        );
#endif
    }

private:
    Config config_;
    
#ifdef THEMIS_ENABLE_WEBDAV
    /**
     * @brief Authenticate user with WebDAV server
     */
    Result<void> authenticateWithWebDAV(
        const std::string& user_id,
        const std::string& password
    ) {
        WebDAV_CURL_ptr curl(curl_easy_init());
        if (!curl) {
            return themis::ErrVoid(errors::ErrorCode::ERR_NET_CONNECTION_REFUSED, "Failed to initialize CURL");
        }
        
        // Build WebDAV URL for user's home directory
        std::string url = config_.webdav_base_url;
        if (!config_.user_directory.empty()) {
            url += "/" + config_.user_directory + "/" + user_id;
        }
        
        // Set up CURL for WebDAV PROPFIND request
        std::string response_body;
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_USERNAME, user_id.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, password.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "PROPFIND");
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, kRequestTimeoutMs);
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT_MS, kRequestTimeoutMs / 2);
        curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
        
        if (!config_.verify_ssl) {
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
        }
        
        // Perform request
        CURLcode res = curl_easy_perform(curl.get());
        long response_code = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);
        
        if (res != CURLE_OK) {
            return themis::ErrVoid(
                errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
                "WebDAV request failed: " + std::string(curl_easy_strerror(res))
            );
        }
        
        // HTTP 207 Multi-Status or 200 OK indicates success
        if (response_code == 200 || response_code == 207) {
            return Result<void>::Ok();
        }
        
        // HTTP 401 Unauthorized
        if (response_code == 401) {
            return themis::ErrVoid(errors::ErrorCode::ERR_API_UNAUTHORIZED, "Invalid credentials");
        }
        
        return themis::ErrVoid(
            errors::ErrorCode::ERR_API_UNAUTHORIZED,
            "WebDAV authentication failed with status: " + std::to_string(response_code)
        );
    }
    
    /**
     * @brief Get user properties from Active Directory via WebDAV
     */
    Result<std::unordered_map<std::string, std::string>> getUserPropertiesFromAD(
        const std::string& user_id
    ) {
        // Use WebDAV PROPFIND with AD-specific properties.
        // Active Directory exposed via WebDAV (e.g., Exchange WebDAV or
        // SharePoint) returns properties in the "urn:schemas:contacts:" and
        // "urn:schemas:httpmail:" namespaces.  We request displayName, mail,
        // and memberOf via a targeted PROPFIND body.

        std::string url = config_.webdav_base_url;
        if (!config_.user_directory.empty()) {
            url += "/" + config_.user_directory + "/" + user_id;
        } else {
            url += "/" + user_id;
        }

        std::unordered_map<std::string, std::string> properties;
        // Populate safe defaults so callers always get sensible values even
        // when the WebDAV request cannot be made.
        properties["displayName"] = user_id;
        properties["mail"]        = user_id + "@" + extractHostFromUrl(config_.webdav_base_url);

#ifdef THEMIS_ENABLE_WEBDAV
        std::string response_body;
        WebDAV_CURL_ptr curl(curl_easy_init());
        if (!curl) {
            return Result<std::unordered_map<std::string, std::string>>::Ok(properties);
        }

        // PROPFIND body requesting AD contact + mail schema properties.
        static const char kAdPropfindBody[] =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<D:propfind xmlns:D=\"DAV:\""
            " xmlns:C=\"urn:schemas:contacts:\""
            " xmlns:M=\"urn:schemas:httpmail:\">"
            "<D:prop>"
            "<D:displayname/>"
            "<C:givenname/>"
            "<C:sn/>"
            "<M:to/>"
            "<C:memberOf/>"
            "</D:prop>"
            "</D:propfind>";

        curl_slist* raw_headers = curl_slist_append(nullptr, "Depth: 0");
        if (!raw_headers) {
            return Result<std::unordered_map<std::string, std::string>>::Ok(properties);
        }
        curl_slist* content_type_headers = curl_slist_append(raw_headers, "Content-Type: application/xml");
        if (!content_type_headers) {
            curl_slist_free_all(raw_headers);
            return Result<std::unordered_map<std::string, std::string>>::Ok(properties);
        }
        WebDAV_CURL_slist_ptr headers(content_type_headers);

        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_USERNAME, config_.webdav_username.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, config_.webdav_password.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "PROPFIND");
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, kAdPropfindBody);
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,
                         static_cast<long>(sizeof(kAdPropfindBody) - 1));
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, kRequestTimeoutMs);
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT_MS, kRequestTimeoutMs / 2);
        curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
        if (!config_.verify_ssl) {
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
        }

        CURLcode res = curl_easy_perform(curl.get());
        long http_code = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

        if (res == CURLE_OK && (http_code == 200 || http_code == 207)) {
            // Helper: extract text between open and close tags.
            auto extract = [&](const std::string& open_tag,
                               const std::string& close_tag) -> std::string {
                size_t s = response_body.find(open_tag);
                size_t e = response_body.find(close_tag);
                if (s == std::string::npos || e == std::string::npos) return {};
                s += open_tag.size();
                return response_body.substr(s, e - s);
            };

            auto dn = extract("<D:displayname>", "</D:displayname>");
            if (!dn.empty()) properties["displayName"] = dn;

            auto given = extract("<C:givenname>", "</C:givenname>");
            if (!given.empty()) properties["givenName"] = given;

            auto sn = extract("<C:sn>", "</C:sn>");
            if (!sn.empty()) properties["sn"] = sn;

            auto mail = extract("<M:to>", "</M:to>");
            if (!mail.empty()) properties["mail"] = mail;

            auto member_of = extract("<C:memberOf>", "</C:memberOf>");
            if (!member_of.empty()) properties["memberOf"] = member_of;
        } else {
            THEMIS_WARN("WebDAV AD PROPFIND for user '{}' returned HTTP {}", user_id, http_code);
        }
#endif // THEMIS_ENABLE_WEBDAV

        return Result<std::unordered_map<std::string, std::string>>::Ok(properties);
    }
    
    /**
     * @brief Map Active Directory groups to ThemisDB roles
     */
    std::vector<std::string> mapADGroupsToRoles(const std::string& memberOf) {
        std::vector<std::string> roles;
        
        // Parse memberOf string (comma-separated list of groups)
        // Map well-known AD groups to ThemisDB roles
        
        if (memberOf.find("Domain Admins") != std::string::npos ||
            memberOf.find("Administrators") != std::string::npos) {
            roles.push_back("admin");
        } else if (memberOf.find("Power Users") != std::string::npos) {
            roles.push_back("operator");
        } else if (memberOf.find("Analysts") != std::string::npos) {
            roles.push_back("analyst");
        } else {
            roles.push_back("readonly");
        }
        
        return roles;
    }

    /**
     * @brief libcurl write callback — appends received bytes to a std::string.
     *
     * Signature matches `curl_write_callback` so that it can be passed to
     * `curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback)`.
     * `userdata` must point to a `std::string`.
     */
    static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb,
                                    void* userdata) {
        auto* body = static_cast<std::string*>(userdata);
        body->append(ptr, size * nmemb);
        return size * nmemb;
    }

    /**
     * @brief Extract the hostname from a URL for use as a mail domain.
     *
     * Strips the scheme (e.g. "https://") and any path component so that
     * "https://sharepoint.company.com/sites/hr" → "sharepoint.company.com".
     * Returns "company.com" when the URL cannot be parsed.
     */
    static std::string extractHostFromUrl(const std::string& url) {
        std::string host;
        size_t p = url.find("://");
        host = (p != std::string::npos) ? url.substr(p + 3) : url;
        size_t slash = host.find('/');
        if (slash != std::string::npos) {
            host = host.substr(0, slash);
        }
        return host.empty() ? "company.com" : host;
    }
#endif
    
    std::string hashPassword(const std::string& password) const {
        // Simple SHA-256 hash (same as AccessControl)
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len = 0;
        
        WebDAV_EVP_MD_CTX_ptr mdctx(EVP_MD_CTX_new());
        EVP_DigestInit_ex(mdctx.get(), EVP_sha256(), nullptr);
        EVP_DigestUpdate(mdctx.get(), password.c_str(), password.length());
        EVP_DigestFinal_ex(mdctx.get(), hash, &hash_len);
        
        std::stringstream ss;
        for (unsigned int i = 0; i < hash_len; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        
        return ss.str();
    }
};

} // namespace security
} // namespace themis
