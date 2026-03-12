/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            remote_registry_client.h                           ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:55:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     243                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 27a08eb54  2026-02-27  feat(base): implement remote plugin loading from authenti... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    ///     openssl x509 -pubkey -noout | openssl pkey -pubin -outform DER | \
    ///     openssl dgst -sha256 -binary | base64
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
 * Typical usage:
 * @code
 *   RegistryConfig cfg;
 *   cfg.registry_url = "https://registry.example.com/api/v1";
 *   cfg.auth_token   = "my-secret-token";
 *   cfg.download_dir = "/opt/themis/plugins";
 *
 *   RemoteRegistryClient client(cfg);
 *
 *   auto plugins = client.listPlugins();
 *   for (const auto& entry : plugins) {
 *       auto result = client.downloadPlugin(entry);
 *       if (result.success) {
 *           loader.loadModule(result.local_path, entry.name);
 *       }
 *   }
 * @endcode
 */
class RemoteRegistryClient {
public:
    explicit RemoteRegistryClient(const RegistryConfig& config);
    ~RemoteRegistryClient();

    RemoteRegistryClient(const RemoteRegistryClient&)            = delete;
    RemoteRegistryClient& operator=(const RemoteRegistryClient&) = delete;

    // -------------------------------------------------------------------------
    // Registry queries
    // -------------------------------------------------------------------------

    /**
     * @brief Fetch the list of available plugins from the registry.
     *
     * Issues GET <registry_url>/plugins and parses the JSON array.
     *
     * @return Vector of plugin entries (may be empty on error).
     */
    std::vector<RegistryPluginEntry> listPlugins();

    /**
     * @brief Fetch metadata for a single plugin by name.
     *
     * Issues GET <registry_url>/plugins/<name>.
     *
     * @param name Plugin name to look up.
     * @return Plugin entry if found, or std::nullopt on error/not found.
     */
    std::optional<RegistryPluginEntry> fetchPlugin(const std::string& name);

    // -------------------------------------------------------------------------
    // Download
    // -------------------------------------------------------------------------

    /**
     * @brief Download a plugin binary and verify its SHA-256 hash.
     *
     * The binary is saved to config.download_dir/<name>-<version>.<ext>.
     * After download the SHA-256 digest is compared to entry.sha256; if they
     * differ the local file is removed and an error is returned.
     *
     * @param entry    Plugin entry (from listPlugins or fetchPlugin).
     * @return PluginDownloadResult describing success, local path, or error.
     */
    PluginDownloadResult downloadPlugin(const RegistryPluginEntry& entry);

    // -------------------------------------------------------------------------
    // Combined download + load
    // -------------------------------------------------------------------------

    /**
     * @brief Download a plugin from the registry and load it via ModuleLoader.
     *
     * Convenience wrapper combining downloadPlugin() and
     * loader.loadModule().
     *
     * @param entry  Plugin entry.
     * @param loader ModuleLoader to use for the final load step.
     * @return ModuleVerificationResult from ModuleLoader.
     */
    ModuleVerificationResult downloadAndLoad(const RegistryPluginEntry& entry,
                                             ModuleLoader& loader);

    // -------------------------------------------------------------------------
    // Configuration access
    // -------------------------------------------------------------------------

    /// Return the current configuration.
    const RegistryConfig& config() const { return config_; }

    // -------------------------------------------------------------------------
    // Observability
    // -------------------------------------------------------------------------

    /**
     * @brief Return statistics from the most recent httpGet or httpGetBinary
     *        call.
     *
     * The returned struct reflects the last completed request (success or
     * failure).  Thread-safe: safe to call concurrently with other public
     * methods.
     */
    RequestStats lastRequestStats() const;

private:
    RegistryConfig config_;

    // Stats for the most recently completed HTTP request.
    mutable std::mutex stats_mutex_;
    RequestStats       last_stats_;

    // HTTP helpers
    std::string httpGet(const std::string& url);
    bool        httpGetBinary(const std::string& url, const std::string& out_path);

    // Integrity
    static bool verifyIntegrity(const std::string& file_path,
                                const std::string& expected_sha256);

    // Auth header building (returns header value for Authorization or X-API-Key)
    std::string buildAuthorizationHeader() const;

    // Perform a blocking back-off sleep for `ms` milliseconds.
    static void asyncBackoffSleep(int ms);

    // Parse a single JSON object into a RegistryPluginEntry (returns false on
    // missing mandatory fields).
    static bool parseEntry(const nlohmann::json& obj, RegistryPluginEntry& out);
};

} // namespace modules
} // namespace themis
