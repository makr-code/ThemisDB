/**
 * @file remote_registry_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Remote plugin registry client for ThemisDB.
//
// Supports authenticated access to a remote plugin registry, plugin listing,
// download with SHA-256 integrity verification, and hand-off to ModuleLoader.
//
// See src/base/ROADMAP.md – Long-term: Remote plugin loading from authenticated registry

#pragma once

#include "themis/base/module_loader.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace modules {

// =============================================================================
// RegistryConfig – connection and authentication settings
// =============================================================================

/**
 * @brief Configuration for the remote plugin registry client.
 */
struct RegistryConfig {
    /// Base URL of the registry (e.g., "https://registry.example.com/api/v1").
    std::string registry_url;

    /// Bearer token sent as "Authorization: Bearer <token>".
    /// Takes precedence over api_key when both are set.
    std::string auth_token;

    /// API key sent as "X-API-Key: <key>" header.
    std::string api_key;

    /// Directory where downloaded plugin binaries are stored.
    std::string download_dir = "/tmp/themis_plugins";

    /// HTTP request timeout in milliseconds.
    int timeout_ms = 30000;

    /// Maximum number of retry attempts on transient HTTP errors.
    int max_retries = 3;

    /// Total wall-clock budget (ms) across all retry attempts for a single
    /// request.  The retry loop is aborted when this limit is reached, even if
    /// max_retries has not been exhausted.  Default: 30 000 ms.
    int max_total_retry_time_ms = 30000;

    /// Verify SSL/TLS certificates (set to false only in test environments).
    bool verify_ssl = true;

    /// Optional custom CA bundle path (empty = use system default).
    std::string ca_bundle_path;

    /// Optional TLS public-key pin expressed as a SHA-256 fingerprint in the
    /// format accepted by libcurl's CURLOPT_PINNEDPUBLICKEY:
    ///   "sha256//<base64-encoded-SHA256-of-SubjectPublicKeyInfo-DER>"
    ///
    /// When non-empty the TLS handshake is rejected unless the server presents
    /// a certificate whose public key matches this pin — regardless of the CA
    /// chain.  This prevents MITM attacks even when a CA is compromised.
    ///
    /// Example:
    ///   cfg.pinned_public_key = "sha256//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    ///
    /// Obtain the pin with:
    ///   openssl s_client -connect registry.example.com:443 | \
    ///   openssl x509 -pubkey -noout | openssl pkey -pubin -outform DER | \
    ///   openssl dgst -sha256 -binary | base64
    ///
    /// Leave empty to rely solely on CA-based certificate verification.
    std::string pinned_public_key;
};

// =============================================================================
// RegistryPluginEntry – metadata for a single plugin in the registry
// =============================================================================

/**
 * @brief Metadata for a single plugin entry returned by the registry.
 *
 * The registry returns entries in JSON format, e.g.:
 * @code{.json}
 * {
 *   "name": "themis_analytics",
 *   "version": "1.2.0",
 *   "description": "Analytics extension for ThemisDB",
 *   "download_url": "https://registry.example.com/plugins/themis_analytics-1.2.0.so",
 *   "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
 *   "min_themis_version": "1.0.0"
 * }
 * @endcode
 */
struct RegistryPluginEntry {
    std::string name;                ///< Unique plugin name (e.g., "themis_analytics")
    std::string version;             ///< Semantic version (e.g., "1.2.0")
    std::string description;         ///< Human-readable description
    std::string download_url;        ///< Full URL to download the binary
    std::string sha256;              ///< Expected SHA-256 hex digest of the binary
    std::string min_themis_version;  ///< Minimum compatible ThemisDB version
};

// =============================================================================
// PluginDownloadResult – outcome of a download + integrity check
// =============================================================================

/**
 * @brief Result of downloading a plugin from the remote registry.
 */
struct PluginDownloadResult {
    bool        success       = false;
    std::string local_path;           ///< Where the binary was saved on disk
    std::string plugin_name;          ///< Name from RegistryPluginEntry
    std::string version;              ///< Version from RegistryPluginEntry
    std::string error_message;        ///< Non-empty on failure
};

// =============================================================================
// RequestStats – observability for the last HTTP request
// =============================================================================

/**
 * @brief Statistics for the most recent HTTP request issued by
 *        RemoteRegistryClient.
 *
 * Populated by both httpGet() and httpGetBinary() so callers can inspect
 * retry counts and error details without relying on log output.
 */
struct RequestStats {
    /// Number of HTTP attempts made (1 = no retries were needed).
    int attempts = 0;

    /// Last error description; empty when the final attempt succeeded.
    std::string last_error;
};

// =============================================================================
// RemoteRegistryClient – authenticated access to a plugin registry
// =============================================================================

/**
 * @brief Client for loading plugins from an authenticated remote registry.
 *
 * Features:
 *  - Bearer token / API key authentication
 *  - Plugin listing via HTTP GET /plugins
 *  - Plugin binary download with SHA-256 integrity verification
 *  - Integration with ModuleLoader for final loading
 *
 * Thread safety: all public methods are safe to call from multiple threads.
 *
 * Async usage: the *Async variants (listPluginsAsync, fetchPluginAsync,
 * downloadPluginAsync) return a std::future and run the entire operation —
 * including retry back-off sleeps — on a detached worker thread, so the
 * calling thread is never blocked.  The client must be owned by a
 * std::shared_ptr for those methods to work; they call shared_from_this()
 * internally and throw std::bad_weak_ptr if the client is stack-allocated.
 *
 * Typical usage:
 * @code
 *   auto client = std::make_shared<RemoteRegistryClient>(cfg);
 *
 *   // Synchronous
 *   auto plugins = client->listPlugins();
 *
 *   // Asynchronous (calling thread released immediately)
 *   auto future = client->listPluginsAsync();
 *   auto plugins = future.get();
 *   // Synchronous usage (calling thread blocks during retries):
 *   RemoteRegistryClient client(cfg);
 *   auto plugins = client.listPlugins();
 *
 *   // Async usage (calling thread is released during retry backoffs):
 *   auto client = std::make_shared<RemoteRegistryClient>(cfg);
 *   auto fut = client->listPluginsAsync();
 *   // … do other work …
 *   auto plugins = fut.get();
 *
 *   for (const auto& entry : plugins) {
 *       auto result = client->downloadPlugin(entry);
 *       if (result.success) {
 *           loader.loadModule(result.local_path, entry.name);
 *       }
 *   }
 * @endcode
 */
class RemoteRegistryClient : public std::enable_shared_from_this<RemoteRegistryClient> {
public:
    explicit RemoteRegistryClient(const RegistryConfig& config);
    ~RemoteRegistryClient();

    RemoteRegistryClient(const RemoteRegistryClient&)            = delete;
    RemoteRegistryClient& operator=(const RemoteRegistryClient&) = delete;

    std::vector<RegistryPluginEntry> listPlugins();
    std::future<std::vector<RegistryPluginEntry>> listPluginsAsync();

    std::optional<RegistryPluginEntry> fetchPlugin(const std::string& name);
    std::future<std::optional<RegistryPluginEntry>> fetchPluginAsync(const std::string& name);

    PluginDownloadResult downloadPlugin(const RegistryPluginEntry& entry);
    std::future<PluginDownloadResult> downloadPluginAsync(const RegistryPluginEntry& entry);

    ModuleVerificationResult downloadAndLoad(const RegistryPluginEntry& entry,
                                             ModuleLoader& loader);

    const RegistryConfig& config() const { return config_; }

    std::future<std::string> httpGetAsync(const std::string& url);
    std::future<bool> httpGetBinaryAsync(const std::string& url,
                                         const std::string& out_path);

    static void setBackoffDispatcher(
        std::function<std::future<void>(std::chrono::milliseconds)> dispatcher);

    RequestStats lastRequestStats() const;

private:
    RegistryConfig config_;

    mutable std::mutex stats_mutex_;
    RequestStats       last_stats_;

    std::string httpGet(const std::string& url);
    bool        httpGetBinary(const std::string& url, const std::string& out_path);

    static bool verifyIntegrity(const std::string& file_path,
                                const std::string& expected_sha256);
    std::string buildAuthorizationHeader() const;
    static void asyncBackoffSleep(int ms);
    static bool parseEntry(const nlohmann::json& obj, RegistryPluginEntry& out);
};

} // namespace modules
} // namespace themis

