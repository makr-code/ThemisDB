/**
 * @file oci_registry_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/expected.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace plugins {

/**
 * @brief Parsed OCI image reference.
 *
 * An OCI reference has the form:
 *   [registry/][namespace/]name[:tag][\@digest]
 *
 * Examples:
 *   - "ghcr.io/themisdb/plugins/s3_blob:1.2.0"
 *   - "registry.example.com/myplugin@sha256:abc123..."
 */
struct OciReference {
    std::string registry;    ///< Hostname (+ optional port), e.g. "ghcr.io"
    std::string name;        ///< Repository path, e.g. "themisdb/plugins/s3_blob"
    std::string tag;         ///< Tag, e.g. "1.2.0" (defaults to "latest")
    std::string digest;      ///< Optional digest, e.g. "sha256:abc123…"

    /**
     * @brief Parse a raw OCI reference string into its components.
     *
     * Accepts references of the form:
     *   [registry/]name[:tag][\@digest]
     *
     * If no registry is detected (no dot or colon before the first slash),
     * "registry-1.docker.io" is used as a default.
     *
     * @param raw  Raw reference string.
     * @return     Populated OciReference, or an error if the string is empty
     *             or syntactically invalid.
     */
    static Result<OciReference> parse(const std::string& raw);

    /// Reconstruct the canonical reference string (registry/name:tag).
    std::string toString() const;
};

/**
 * @brief Authentication configuration for an OCI registry.
 */
struct OciAuthConfig {
    std::string username;      ///< Basic-auth username (leave empty for anonymous)
    std::string password;      ///< Basic-auth password or token
    std::string bearer_token;  ///< Pre-obtained Bearer token (overrides username/password)
};

/**
 * @brief Descriptor for a single layer inside an OCI manifest.
 */
struct OciManifestLayer {
    std::string media_type;  ///< e.g. "application/vnd.themisdb.plugin.v1.binary"
    std::string digest;      ///< "sha256:<hex>"
    int64_t     size = 0;    ///< Byte size of the compressed layer
};

/**
 * @brief Parsed OCI Image Manifest (schema version 2).
 */
struct OciManifest {
    int schema_version = 2;
    std::string media_type;
    OciManifestLayer config;
    std::vector<OciManifestLayer> layers;

    /// Raw JSON string of the manifest (needed for digest verification).
    std::string raw_json;
};

/**
 * @brief Media type used for ThemisDB plugin binary layers in OCI images.
 *
 * When packaging a plugin as an OCI artifact, the binary layer MUST carry
 * this media type so that @ref OciRegistryClient can identify and download
 * the correct layer.
 */
static constexpr const char* THEMIS_PLUGIN_LAYER_MEDIA_TYPE =
    "application/vnd.themisdb.plugin.v1.binary";

/**
 * @brief Client for pulling ThemisDB plugins from OCI-compliant registries.
 *
 * Implements a minimal subset of the OCI Distribution Specification
 * (https://github.com/opencontainers/distribution-spec) sufficient to:
 *   1. Authenticate with a registry (anonymous or Bearer token).
 *   2. Fetch and parse an OCI image manifest.
 *   3. Download and verify (SHA-256) the plugin binary layer.
 *   4. Store the binary in a local cache directory.
 *
 * ### Typical usage
 * @code
 * OciRegistryClient client;
 * client.setAuth("ghcr.io", {"", "", "ghp_TOKEN"});
 *
 * auto ref = OciReference::parse("ghcr.io/themisdb/plugins/s3_blob:1.2.0");
 * auto path = client.pullPluginBinary(*ref, "/var/cache/themis/plugins");
 * if (path) {
 *     auto result = PluginManager::instance().loadPluginFromPath(*path);
 * }
 * @endcode
 *
 * ### Thread-safety
 * Individual method calls are thread-safe. Each call creates its own
 * libcurl easy handle; the shared state (auth config) is protected by a
 * mutex.
 *
 * ### Performance target
 * Network I/O dominates; no in-process bottleneck beyond standard libcurl.
 */
class OciRegistryClient {
public:
    OciRegistryClient();
    ~OciRegistryClient();

    // Non-copyable
    OciRegistryClient(const OciRegistryClient&) = delete;
    OciRegistryClient& operator=(const OciRegistryClient&) = delete;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /**
     * @brief Set authentication credentials for a specific registry host.
     *
     * @param registry  Registry hostname, e.g. "ghcr.io".
     * @param auth      Authentication configuration.
     */
    void setAuth(const std::string& registry, OciAuthConfig auth);

    /**
     * @brief Set the HTTP connect/read timeout in seconds (default: 30).
     */
    void setTimeout(long timeout_seconds);

    // -------------------------------------------------------------------------
    // Core API
    // -------------------------------------------------------------------------

    /**
     * @brief Fetch and parse the OCI manifest for a reference.
     *
     * @param ref  Parsed OCI reference.
     * @return     Parsed @ref OciManifest or error.
     */
    Result<OciManifest> fetchManifest(const OciReference& ref);

    /**
     * @brief Download the plugin binary layer and save it to @p dest_dir.
     *
     * Looks for a layer with media type @ref THEMIS_PLUGIN_LAYER_MEDIA_TYPE
     * in the manifest and downloads the corresponding blob. The SHA-256
     * digest is verified before the file is moved into place.
     *
     * @param ref       Parsed OCI reference.
     * @param dest_dir  Local directory where the binary is stored.
     * @return          Absolute path of the saved plugin binary, or an error.
     */
    Result<std::string> pullPluginBinary(
        const OciReference& ref,
        const std::string& dest_dir);

private:
    // Perform a GET request and return the response body.
    Result<std::string> httpGet(
        const std::string& url,
        const std::vector<std::string>& extra_headers = {});

    // Perform a GET request and write the body to a file.
    Result<void> httpGetToFile(
        const std::string& url,
        const std::string& dest_path,
        const std::vector<std::string>& extra_headers = {});

    // Obtain (or refresh) a Bearer token via the WWW-Authenticate challenge.
    Result<std::string> obtainBearerToken(
        const std::string& registry,
        const std::string& scope);

    // Verify the SHA-256 digest of a file matches the expected "sha256:<hex>" string.
    static bool verifyDigest(const std::string& file_path, const std::string& expected_digest);

    // Build the base API URL for a registry.
    static std::string registryBaseUrl(const std::string& registry);

    std::unordered_map<std::string, OciAuthConfig> auth_configs_;
    mutable std::mutex mutex_;
    long timeout_seconds_ = 30;
};

} // namespace plugins
} // namespace themis
