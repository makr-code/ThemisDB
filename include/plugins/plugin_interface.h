/**
 * @file plugin_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#ifndef THEMISDB_PLUGIN_INTERFACE_H
#define THEMISDB_PLUGIN_INTERFACE_H

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <tuple>
#include <cstdio>
#include <optional>
#include <algorithm>
#include <nlohmann/json.hpp>

/**
 * @brief Unified Plugin Interface for ThemisDB
 * 
 * This interface unifies existing plugin loaders:
 * - acceleration/plugin_loader.h (Compute backends)
 * - security/hsm_provider_pkcs11.cpp (PKCS#11 dynamic loading)
 * - acceleration/zluda_backend.cpp (ZLUDA dynamic loading)
 * 
 * Benefits:
 * - Single plugin architecture for all components
 * - Consistent security verification
 * - Unified plugin discovery and lifecycle
 * - Shared code for DLL loading (Windows/Linux/macOS)
 */

// Platform-specific export macros
#ifndef THEMIS_PLUGIN_EXPORT
#ifdef _WIN32
    #ifdef THEMIS_PLUGIN_EXPORTS
        #define THEMIS_PLUGIN_EXPORT __declspec(dllexport)
    #else
        #define THEMIS_PLUGIN_EXPORT __declspec(dllimport)
    #endif
#else
    #define THEMIS_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif
#endif

namespace themis {
namespace plugins {

/**
 * @brief Plugin Type Categories
 * 
 * Maps to existing plugin systems:
 * - COMPUTE_BACKEND -> acceleration::BackendPlugin
 * - BLOB_STORAGE -> New blob storage backends
 * - IMPORTER -> New data importers
 * - HSM_PROVIDER -> security::HSMProvider (PKCS#11)
 * - LLM_BACKEND      -> llm::ILLMPlugin (v1.5.0+)
 * - AUDIO_PROCESSING -> whisper::WhisperPlugin (v2.0.0+)
 * - IMAGE_GENERATION -> stable_diffusion::SDPlugin (v2.0.0+)
 * - RESOURCE_LIMIT_POLICY -> plugins::IEditionPolicyPlugin (v2.2.0+)
 */
enum class PluginType {
    COMPUTE_BACKEND,   // Vector/Graph/Geo acceleration (existing)
    BLOB_STORAGE,      // Storage backends (Filesystem, S3, Azure, WebDAV)
    IMPORTER,          // Data importers (PostgreSQL, MySQL, CSV)
    EXPORTER,          // Data exporters
    HSM_PROVIDER,      // Hardware Security Modules (PKCS#11)
    EMBEDDING,         // Embedding providers (Sentence-BERT, OpenAI)
    LLM_BACKEND,       // LLM backends (llama.cpp, vLLM, etc.) - v1.5.0+
    AUDIO_PROCESSING,  // Audio transcription/processing (whisper.cpp, etc.) - v2.0.0
    IMAGE_GENERATION,  // Image generation (stable-diffusion.cpp, etc.) - v2.0.0
    AGENTIC_TOOL,      // Agentic tool plugins loaded by ToolRegistry (JSON in/out) - v2.1.0
    INGESTION_STEP,    // Ingestion workflow step plugins (IIngestionStep) - v2.0.0
    RESOURCE_LIMIT_POLICY, // Signed edition-upgrade: provides IVRAMPolicy or IShardLimitPolicy — v2.2.0
    CUSTOM             // Custom plugins
};

/**
* @brief Plugin Lifecycle State Machine (Phase 2 Hardening)
*
* Explicit state transitions for plugin lifecycle management. Enforced
* during load, unload, and reload operations.
*
* State machine invariants:
*   - A plugin starts in UNLOADED state
*   - Transitions must follow explicit paths; no direct jumps allowed
*   - Failed transitions leave the plugin in a consistent state
*   - UNLOADING transitions are atomic (no partial unload)
*
* Valid transitions:
*   - UNLOADED → LOADING (load operation initiates)
*   - LOADING → LOADED (successful load)
*   - LOADING → UNLOADED (load failure or rollback)
*   - LOADED → UNLOADING (unload operation initiates)
*   - UNLOADING → UNLOADED (unload successful)
*   - LOADED → LOADED (reload without state change)
*
* @see src/plugins/ROADMAP.md — Phase 2A implementation
*/
enum class PluginLifecycleState : uint8_t {
    /**
     * Initial state: plugin has never been loaded or has been unloaded.
     * Operations allowed: load
     */
    UNLOADED = 0,

    /**
     * Intermediate state: plugin is being loaded.
     * Duration: synchronous operation; transitions to LOADED or UNLOADED.
     * Operations allowed: none (loading in progress)
     */
    LOADING = 1,

    /**
     * Steady state: plugin is fully initialized and available.
     * Operations allowed: query, reload, unload
     */
    LOADED = 2,

    /**
     * Intermediate state: plugin is being unloaded.
     * Duration: synchronous operation; transitions to UNLOADED.
     * Operations allowed: none (unloading in progress)
     */
    UNLOADING = 3,

    /**
     * Error/unknown state. Should not occur in production; indicates
     * an internal consistency failure.
     */
    UNKNOWN = 255
};

/**
* @brief Convert PluginLifecycleState to human-readable string.
* @param state The lifecycle state.
* @return String representation of the state (never nullptr).
*/
inline const char* lifecycleStateToString(PluginLifecycleState state) {
    switch (state) {
        case PluginLifecycleState::UNLOADED:  return "UNLOADED";
        case PluginLifecycleState::LOADING:   return "LOADING";
        case PluginLifecycleState::LOADED:    return "LOADED";
        case PluginLifecycleState::UNLOADING: return "UNLOADING";
        case PluginLifecycleState::UNKNOWN:   return "UNKNOWN";
        default:                               return "INVALID";
    }
}

/**
* @brief Validate a plugin lifecycle state transition.
*
* Checks if transitioning from @a from_state to @a to_state is allowed
* according to the plugin lifecycle state machine rules.
*
* Valid transitions:
*   - UNLOADED → LOADING
*   - LOADING → LOADED | UNLOADED
*   - LOADED → LOADED (reload) | UNLOADING
*   - UNLOADING → UNLOADED
*
* @param from_state Current state.
* @param to_state Desired next state.
* @return true if transition is allowed; false otherwise.
*/
inline bool isValidLifecycleTransition(PluginLifecycleState from_state,
                                      PluginLifecycleState to_state) {
    if (from_state == to_state && to_state == PluginLifecycleState::LOADED) {
        // Reload case: LOADED → LOADED is allowed
        return true;
    }
    switch (from_state) {
        case PluginLifecycleState::UNLOADED:
            return to_state == PluginLifecycleState::LOADING;
        case PluginLifecycleState::LOADING:
            return to_state == PluginLifecycleState::LOADED ||
                   to_state == PluginLifecycleState::UNLOADED;
        case PluginLifecycleState::LOADED:
            return to_state == PluginLifecycleState::UNLOADING;
        case PluginLifecycleState::UNLOADING:
            return to_state == PluginLifecycleState::UNLOADED;
        default:
            return false;
    }
}

/**
* @brief Plugin Capabilities
*/
struct PluginCapabilities {
    bool supports_streaming = false;
    bool supports_batching = false;
    bool supports_transactions = false;
    bool thread_safe = false;
    bool gpu_accelerated = false;
    bool provides_vram_policy  = false;  ///< Supplies an IVRAMPolicy for EditionManager (v2.2.0+)
    bool provides_shard_policy = false;  ///< Supplies an IShardLimitPolicy for EditionManager (v2.2.0+)
};

/**
 * @brief Inclusive version range constraint for plugin capability negotiation.
 *
 * Both bounds are optional: an empty string means "no constraint" on that bound.
 * Example: {min_version="1.0.0", max_version="2.0.0"} matches any version
 * from 1.0.0 to 2.0.0 inclusive.
 */
struct PluginVersionRange {
    std::string min_version;  ///< Inclusive lower bound; "" = no lower bound
    std::string max_version;  ///< Inclusive upper bound; "" = no upper bound

    bool isUnconstrained() const {
        return min_version.empty() && max_version.empty();
    }
};

/**
 * @brief A single capability requirement with an optional plugin version range.
 *
 * The capability_name maps to the boolean flags in PluginCapabilities:
 *   "streaming", "batching", "transactions", "thread_safe", "gpu_accelerated"
 *
 * The version_range constrains the plugin version (IThemisPlugin::getVersion()),
 * enabling callers to express requirements such as "I need streaming support
 * and the plugin must be at least v1.2.0".
 */
struct PluginCapabilityRequirement {
    std::string capability_name;      ///< Named capability (see PluginCapabilities)
    PluginVersionRange version_range; ///< Required plugin version range (optional)
};

/**
 * @brief Result of a plugin capability negotiation pass.
 */
struct PluginNegotiationResult {
    bool success = false;
    std::vector<std::string> satisfied;    ///< Requirements that were met
    std::vector<std::string> unsatisfied;  ///< Requirements that were NOT met
    std::string error_message;
};

/**
 * @brief Base Plugin Interface
 * 
 * All plugins must implement this interface.
 * Type-specific plugins should also implement their domain interface
 * (e.g., IBlobStorageBackend, IImporter, etc.)
 */
class IThemisPlugin {
public:
    virtual ~IThemisPlugin() = default;
    
    /**
     * @brief Get plugin name
     */
    [[nodiscard]] virtual const char* getName() const = 0;
    
    /**
     * @brief Get plugin version (semantic versioning)
     */
    [[nodiscard]] virtual const char* getVersion() const = 0;
    
    /**
     * @brief Get plugin type
     */
    [[nodiscard]] virtual PluginType getType() const = 0;
    
    /**
     * @brief Get plugin capabilities
     */
    [[nodiscard]] virtual PluginCapabilities getCapabilities() const = 0;
    
    /**
     * @brief Initialize plugin with configuration JSON
     * @param config_json Configuration as JSON string
     * @return true if initialized successfully
     */
    [[nodiscard]] virtual bool initialize(const char* config_json) = 0;
    
    /**
     * @brief Shutdown plugin and release resources
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Get plugin instance (type-specific)
     * @return Pointer to plugin implementation (must be cast to specific type)
     * 
     * For COMPUTE_BACKEND: Cast to acceleration::BackendPlugin*
     * For BLOB_STORAGE: Cast to storage::IBlobStorageBackend*
     * For IMPORTER: Cast to importers::IImporter*
     */
    [[nodiscard]] virtual void* getInstance() = 0;
};

/**
 * @brief Stateful Plugin Interface
 * 
 * Optional interface for plugins that need state preservation during hot-reload.
 * Plugins implementing this interface can save their state before unload
 * and restore it after reload, enabling zero-downtime updates.
 * 
 * Thread-Safety: Implementations must be thread-safe
 */
class IStatefulPlugin {
public:
    virtual ~IStatefulPlugin() = default;
    
    /**
     * @brief Save plugin state before reload
     * 
     * Called by PluginManager before unloading during hot-reload.
     * The returned state will be passed back to restoreState() after reload.
     * 
     * @return Serialized state as JSON string, or empty string if no state
     * @throws std::exception on serialization error (will be logged, not fatal)
     */
    [[nodiscard]] virtual std::string saveState() = 0;
    
    /**
     * @brief Restore plugin state after reload
     * 
     * Called by PluginManager after successful reload.
     * The plugin should restore its internal state from the provided data.
     * 
     * @param state Previously saved state from saveState()
     * @return true if state restored successfully, false otherwise
     * @note If restoration fails, plugin remains loaded with default state
     */
    [[nodiscard]] virtual bool restoreState(const std::string& state) = 0;
};

/**
 * @brief Runtime plugin capability negotiator.
 *
 * Checks whether a plugin satisfies a set of PluginCapabilityRequirements,
 * including version-range constraints on the plugin version.
 *
 * Thread-Safety: All methods are stateless and thread-safe.
 *
 * Example usage:
 * @code
 *   PluginCapabilityNegotiator::negotiate(plugin, {
 *       {"streaming",     {"1.0.0", ""}},
 *       {"gpu_accelerated", {"", ""}},
 *   });
 * @endcode
 */
class PluginCapabilityNegotiator {
public:
    /**
     * @brief Negotiate capabilities between a plugin and a list of requirements.
     *
     * For each requirement:
     *   1. Checks that the named capability flag is enabled in the plugin.
     *   2. Checks that the plugin version falls within the required version range.
     *
     * @param plugin       Plugin to inspect (via getVersion() / getCapabilities()).
     * @param requirements List of capability requirements to satisfy.
     * @return PluginNegotiationResult with success flag and per-requirement details.
     */
    static PluginNegotiationResult negotiate(
        const IThemisPlugin& plugin,
        const std::vector<PluginCapabilityRequirement>& requirements)
    {
        PluginNegotiationResult result;
        const char* raw_version = plugin.getVersion();
        const std::string version = raw_version ? raw_version : "";
        const PluginCapabilities caps = plugin.getCapabilities();

        for (const auto& req : requirements) {
            bool cap_ok = checkCapability(req.capability_name, caps);
            bool ver_ok = isVersionInRange(version, req.version_range);

            if (cap_ok && ver_ok) {
                result.satisfied.push_back(req.capability_name);
            } else {
                result.unsatisfied.push_back(req.capability_name);
                if (!result.error_message.empty()) {
                    result.error_message += "; ";
                }
                if (!cap_ok) {
                    result.error_message += "capability '" + req.capability_name
                        + "' not supported";
                } else {
                    result.error_message += "plugin version '" + version
                        + "' out of range [" + req.version_range.min_version
                        + ", " + req.version_range.max_version
                        + "] for '" + req.capability_name + "'";
                }
            }
        }

        result.success = result.unsatisfied.empty();
        return result;
    }

    /**
     * @brief Check whether a plugin version satisfies a version range.
     *
     * Uses semantic versioning (major.minor.patch).  Non-parseable or empty
     * version strings are treated as (0, 0, 0) and satisfy only unconstrained
     * ranges.
     *
     * @param version Plugin version string (e.g. "1.2.3").
     * @param range   Required version range.
     * @return true if version satisfies range.
     */
    static bool isVersionInRange(const std::string& version,
                                 const PluginVersionRange& range)
    {
        if (range.isUnconstrained()) {
            return true;
        }
        // An unversioned plugin satisfies only unconstrained ranges.
        if (version.empty()) {
            return false;
        }
        auto ver = parseVersion(version);
        if (!range.min_version.empty() && ver < parseVersion(range.min_version)) {
            return false;
        }
        if (!range.max_version.empty() && ver > parseVersion(range.max_version)) {
            return false;
        }
        return true;
    }

    /**
     * @brief Check whether a named capability is enabled.
     *
     * Recognised names: "streaming", "batching", "transactions",
     *                   "thread_safe", "gpu_accelerated",
     *                   "provides_vram_policy", "provides_shard_policy".
     * Unknown names always return false.
     *
     * @param name Capability name.
     * @param caps Plugin capabilities struct.
     * @return true if capability is enabled.
     */
    static bool checkCapability(const std::string& name,
                                const PluginCapabilities& caps)
    {
        if (name == "streaming") {
          return caps.supports_streaming;
        }
        if (name == "batching") {
          return caps.supports_batching;
        }
        if (name == "transactions") {
          return caps.supports_transactions;
        }
        if (name == "thread_safe") {
          return caps.thread_safe;
        }
        if (name == "gpu_accelerated") {
          return caps.gpu_accelerated;
        }
        if (name == "provides_vram_policy") {
          return caps.provides_vram_policy;
        }
        if (name == "provides_shard_policy") {
          return caps.provides_shard_policy;
        }
        return false;
    }

private:
    using Version3 = std::tuple<int, int, int>;

    static Version3 parseVersion(const std::string& v) {
        int major = 0, minor = 0, patch = 0;
        if (!v.empty()) {
            // Pre-initialised to 0; partial parses (e.g. "1.2.x") degrade
            // gracefully, matching the behaviour in module_dependency_resolver.cpp.
            (void)std::sscanf(v.c_str(), "%d.%d.%d", &major, &minor, &patch);
        }
        return {major, minor, patch};
    }
};

/**
 * @brief Plugin Entry Points
 * 
 * Every plugin DLL must export these two functions:
 */
typedef IThemisPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(IThemisPlugin*);

/**
 * @brief Plugin Manifest (parsed from plugin.json)
 */
struct PluginManifest {
    std::string name;
    std::string version;
    std::string description;
    PluginType type;
    std::string visibility = "public";
    
    // Platform-specific binaries
    std::string binary_windows;  // .dll
    std::string binary_linux;    // .so
    std::string binary_macos;    // .dylib
    
    // Dependencies
    std::vector<std::string> dependencies;
    
    // Capabilities
    PluginCapabilities capabilities;
    
    // Auto-load on startup?
    bool auto_load = false;
    
    // Load priority (lower = higher priority)
    int load_priority = 100;
    
    // Config schema (JSON Schema)
    std::string config_schema;
    
    // Expected SHA-256 hash of the binary (hex-encoded).
    // When set, the plugin manager verifies the on-disk binary hash before loading.
    // Leave empty to skip hash enforcement (development/unsigned builds).
    std::string expected_hash;

    // Private/public rollout and compatibility metadata.
    std::vector<std::string> allowed_editions;
    std::string license_feature;
    std::string min_themisdb_version;
    std::string max_themisdb_version;
    std::string compatible_core_abi;
};

/**
 * @brief Publisher signature metadata for marketplace plugins.
 */
struct PluginSignatureInfo {
    std::string fingerprint;  ///< Hex-encoded Ed25519 public key fingerprint
    std::string algorithm;    ///< Signing algorithm (e.g. "ed25519")
    std::string signed_at;    ///< ISO 8601 UTC timestamp
};

/**
 * @brief Extended plugin manifest for marketplace-distributed plugins.
 *
 * Adds marketplace-specific metadata fields on top of the base PluginManifest.
 * The JSON Schema for this format is defined in
 * `include/plugins/manifest_schema_v2.json`.
 *
 * Example plugin.json (marketplace edition):
 * @code
 * {
 *   "name": "s3_blob_storage",
 *   "version": "1.0.0",
 *   "type": "blob_storage",
 *   "description": "AWS S3 blob storage backend",
 *   "author": "ThemisDB Team",
 *   "license": "Proprietary",
 *   "homepage": "https://marketplace.themisdb.io/plugins/s3_blob_storage",
 *   "tags": ["storage", "aws", "s3"],
 *   "category": "storage",
 *   "min_themisdb_version": "1.2.0",
 *   "allowed_editions": ["enterprise", "hyperscaler"],
 *   "license_feature": "private_connector_pack",
 *   "binary": { "linux": "themis_blob_s3.so" },
 *   "verified_publisher": true
 * }
 * @endcode
 */
struct MarketplaceManifest : public PluginManifest {
    // Publisher information
    std::string author;           ///< Publisher/author name
    std::string license;          ///< SPDX license identifier
    std::string homepage;         ///< Plugin homepage URL
    std::string repository;       ///< Source code repository URL
    std::string documentation;    ///< Documentation URL or relative path

    // Marketplace discovery
    std::vector<std::string> tags;  ///< Searchable tags
    std::string category;           ///< Marketplace category
    std::string marketplace_id;     ///< UUID assigned by the marketplace

    // Compatibility
    std::string min_themis_version;  ///< Minimum required ThemisDB version
    std::string max_themis_version;  ///< Maximum tested ThemisDB version

    // Trust
    bool verified_publisher = false;  ///< Publisher verified by ThemisDB
    PluginSignatureInfo signature;    ///< Publisher signature metadata
};

/**
 * @brief Manifest Validation Error Codes (Wave C Batch 2)
 * 
 * Error codes [8700-8799] reserved for plugin manifest edition/license/boundary validation.
 * These codes enable fail-closed validation of edition restrictions, license gates,
 * and public/private visibility boundaries.
 * 
 * Usage: Check against these codes when manifest validation fails during plugin load.
 */
enum class ManifestErrorCode {
    // Success
    MANIFEST_OK = 0,
    
    // Edition and license validation (8700-8719)
    PLUGIN_EDITION_MISMATCH = 8700,      ///< Plugin's allowed_editions does not include current edition
    PLUGIN_LICENSE_DENIED = 8701,        ///< license_feature required but not granted by license gate
    PLUGIN_LICENSE_FEATURE_INVALID = 8702, ///< license_feature field format invalid
    PLUGIN_ALLOWED_EDITIONS_MALFORMED = 8703, ///< allowed_editions not an array or invalid values
    
    // Public/private boundary violations (8720-8739)
    PLUGIN_PRIVATE_IN_COMMUNITY = 8720,  ///< visibility="private" but edition="community" (fail-closed)
    PLUGIN_PATH_VISIBILITY_MISMATCH = 8721, ///< Plugin path contains "private/" but not marked private
    PLUGIN_RESTRICTED_NO_CONTEXT = 8722, ///< visibility="restricted" without scoped checkout context
    
    // Reserved (8740-8799)
    PLUGIN_MANIFEST_VALIDATION_ERROR = 8799 ///< Generic manifest validation error
};

/**
 * @brief Validates plugin manifests against the ThemisDB marketplace schema v2.
 *
 * Provides structural and semantic validation of plugin.json content without
 * requiring a third-party JSON Schema library.  Validation rules correspond
 * 1:1 to the constraints in `include/plugins/manifest_schema_v2.json`.
 *
 * Performance target: < 2 ms per manifest on modern hardware.
 *
 * Thread-Safety: All methods are stateless and thread-safe.
 *
 * Example usage:
 * @code
 *   nlohmann::json j = nlohmann::json::parse(file);
 *   auto result = ManifestSchemaValidator::validate(j);
 *   if (!result.valid) {
 *       for (const auto& err : result.errors) { log(err); }
 *   }
 *   auto manifest = ManifestSchemaValidator::parseMarketplaceManifest(j);
 * @endcode
 */
class ManifestSchemaValidator {
public:
    using json = nlohmann::json;

    struct ValidationResult {
        bool valid = false;
        std::vector<std::string> errors;
    };

    /**
     * @brief Validate a parsed JSON object against the marketplace manifest schema.
     *
     * Checks required fields, type constraints, value ranges, and pattern rules
     * as defined in manifest_schema_v2.json.
     *
     * @param j  Parsed JSON object representing the plugin manifest.
     * @return   ValidationResult with valid=true on success, or a list of errors.
     */
    static ValidationResult validate(const json& j) {
        ValidationResult result = {};

        if (!j.is_object()) {
            result.errors.push_back("Manifest root must be a JSON object");
            return result;
        }

        // ---- Required fields ----
        checkRequiredString(j, "name", 1, 128, result);
        checkRequiredString(j, "version", 1, 64, result);
        checkRequiredString(j, "description", 1, 512, result);

        // type: required enum
        static const std::vector<std::string> valid_types = {
            "compute_backend", "blob_storage", "importer", "exporter",
            "hsm_provider", "embedding", "llm_backend", "custom"
        };
        if (!j.contains("type") || !j["type"].is_string()) {
            result.errors.push_back("Required field 'type' must be a string");
        } else if (std::find(valid_types.begin(), valid_types.end(),
                             j["type"].get<std::string>()) == valid_types.end()) {
            result.errors.push_back("Field 'type' has invalid value '"
                + j["type"].get<std::string>() + "'");
        }

        // binary: required object with at least one platform entry
        if (!j.contains("binary") || !j["binary"].is_object()) {
            result.errors.push_back("Required field 'binary' must be an object");
        } else {
            const auto& bin = j["binary"];
            if (bin.empty()) {
                result.errors.push_back("Field 'binary' must have at least one platform entry");
            }
            static const std::vector<std::string> platforms = {"windows", "linux", "macos"};
            for (const auto& [k, v] : bin.items()) {
                if (std::find(platforms.begin(), platforms.end(), k) == platforms.end()) {
                    result.errors.push_back("Unknown platform '" + k + "' in 'binary'");
                } else if (!v.is_string()) {
                    result.errors.push_back("'binary." + k + "' must be a string");
                }
            }
        }

        // ---- Optional scalar fields ----
        checkOptionalString(j, "author", 0, 256, result);
        checkOptionalString(j, "license", 0, 64, result);
        checkOptionalString(j, "homepage", 0, 512, result);
        checkOptionalString(j, "repository", 0, 512, result);
        checkOptionalString(j, "documentation", 0, 512, result);
        checkOptionalString(j, "marketplace_id", 0, 64, result);
        checkOptionalString(j, "min_themis_version", 0, 32, result);
        checkOptionalString(j, "max_themis_version", 0, 32, result);
        checkOptionalString(j, "min_themisdb_version", 0, 32, result);
        checkOptionalString(j, "max_themisdb_version", 0, 32, result);
        checkOptionalString(j, "license_feature", 0, 128, result);
        checkOptionalString(j, "compatible_core_abi", 0, 64, result);
        checkOptionalString(j, "visibility", 0, 32, result);
        checkOptionalString(j, "expected_hash", 0, 128, result);

        // expected_hash, when present, must be exactly 64 hex characters (SHA-256)
        if (j.contains("expected_hash") && j["expected_hash"].is_string()) {
            const auto& h = j["expected_hash"].get<std::string>();
            if (!h.empty() && h.size() != 64) {
                result.errors.push_back(
                    "Field 'expected_hash' must be exactly 64 hex characters (SHA-256)");
            }
        }

        if (j.contains("auto_load") && !j["auto_load"].is_boolean()) {
            result.errors.push_back("Field 'auto_load' must be a boolean");
        }
        if (j.contains("verified_publisher") && !j["verified_publisher"].is_boolean()) {
            result.errors.push_back("Field 'verified_publisher' must be a boolean");
        }
        if (j.contains("load_priority")) {
            if (!j["load_priority"].is_number_integer()) {
                result.errors.push_back("Field 'load_priority' must be an integer");
            } else {
                int prio = j["load_priority"].get<int>();
                if (prio < 0 || prio > 1000) {
                    result.errors.push_back("Field 'load_priority' must be between 0 and 1000");
                }
            }
        }

        // ---- Optional object fields ----
        if (j.contains("capabilities") && !j["capabilities"].is_object()) {
            result.errors.push_back("Field 'capabilities' must be an object");
        }
        if (j.contains("config_schema") && !j["config_schema"].is_object()) {
            result.errors.push_back("Field 'config_schema' must be an object");
        }
        if (j.contains("configuration") && !j["configuration"].is_object()) {
            result.errors.push_back("Field 'configuration' must be an object");
        }
        if (j.contains("presets") && !j["presets"].is_object()) {
            result.errors.push_back("Field 'presets' must be an object");
        }

        // ---- Optional array fields ----
        if (j.contains("tags")) {
            if (!j["tags"].is_array()) {
                result.errors.push_back("Field 'tags' must be an array");
            } else if (j["tags"].size() > 16) {
                result.errors.push_back("Field 'tags' must not have more than 16 items");
            }
        }
        if (j.contains("allowed_editions")) {
            static const std::vector<std::string> valid_editions = {
                "minimal", "community", "enterprise", "hyperscaler", "military"
            };
            if (!j["allowed_editions"].is_array()) {
                result.errors.push_back("Field 'allowed_editions' must be an array");
            } else {
                for (const auto& ed : j["allowed_editions"]) {
                    if (!ed.is_string()) {
                        result.errors.push_back("Field 'allowed_editions' entries must be strings");
                        continue;
                    }
                    if (std::find(valid_editions.begin(), valid_editions.end(),
                                  ed.get<std::string>()) == valid_editions.end()) {
                        result.errors.push_back("Field 'allowed_editions' contains invalid edition '" +
                                                ed.get<std::string>() + "'");
                    }
                }
            }
        }
        if (j.contains("supported_formats") && !j["supported_formats"].is_array()) {
            result.errors.push_back("Field 'supported_formats' must be an array");
        }
        if (j.contains("examples") && !j["examples"].is_array()) {
            result.errors.push_back("Field 'examples' must be an array");
        }
        if (j.contains("dependencies") && !j["dependencies"].is_array()
                && !j["dependencies"].is_object()) {
            result.errors.push_back("Field 'dependencies' must be an array or object");
        }

        // ---- signature sub-object ----
        if (j.contains("signature")) {
            if (!j["signature"].is_object()) {
                result.errors.push_back("Field 'signature' must be an object");
            } else {
                const auto& sig = j["signature"];
                static const std::vector<std::string> sig_required = {
                    "fingerprint", "algorithm", "signed_at"
                };
                for (const auto& field : sig_required) {
                    if (!sig.contains(field) || !sig[field].is_string()
                            || sig[field].get<std::string>().empty()) {
                        result.errors.push_back(
                            "Field 'signature." + field + "' is required and must be a non-empty string");
                    }
                }
                // Fingerprint must be at least 16 hex characters (matches schema minLength: 16)
                if (sig.contains("fingerprint") && sig["fingerprint"].is_string()) {
                    const auto fp = sig["fingerprint"].get<std::string>();
                    if (fp.size() < 16) {
                        result.errors.push_back(
                            "Field 'signature.fingerprint' must be at least 16 characters");
                    }
                }
                static const std::vector<std::string> valid_algos = {
                    "ed25519", "ecdsa-p256", "rsa-pss-sha256"
                };
                if (sig.contains("algorithm") && sig["algorithm"].is_string()) {
                    const auto algo = sig["algorithm"].get<std::string>();
                    if (std::find(valid_algos.begin(), valid_algos.end(), algo) == valid_algos.end()) {
                        result.errors.push_back(
                            "Field 'signature.algorithm' has invalid value '" + algo + "'");
                    }
                }
            }
        }

        // category enum validation
        if (j.contains("category")) {
            static const std::vector<std::string> valid_cats = {
                "storage", "compute", "security", "data-import", "data-export",
                "machine-learning", "observability", "replication", "custom"
            };
            if (!j["category"].is_string()) {
                result.errors.push_back("Field 'category' must be a string");
            } else if (std::find(valid_cats.begin(), valid_cats.end(),
                                 j["category"].get<std::string>()) == valid_cats.end()) {
                result.errors.push_back("Field 'category' has invalid value '"
                    + j["category"].get<std::string>() + "'");
            }
        }

        if (j.contains("visibility")) {
            static const std::vector<std::string> valid_visibility = {
                "public", "private", "restricted"
            };
            if (!j["visibility"].is_string()) {
                result.errors.push_back("Field 'visibility' must be a string");
            } else if (std::find(valid_visibility.begin(), valid_visibility.end(),
                                 j["visibility"].get<std::string>()) == valid_visibility.end()) {
                result.errors.push_back("Field 'visibility' has invalid value '" +
                    j["visibility"].get<std::string>() + "'");
            }
        }

        result.valid = result.errors.empty();
        return result;
    }

    /**
     * @brief Parse a JSON object into a MarketplaceManifest.
     *
     * Assumes the JSON has already been validated; returns std::nullopt if a
     * required field is missing or has the wrong type.
     *
     * @param j  Parsed JSON object.
     * @return   Populated MarketplaceManifest, or std::nullopt on parse failure.
     */
    static std::optional<MarketplaceManifest> parseMarketplaceManifest(const json& j) {
        auto result = validate(j);
        if (!result.valid) {
            return std::nullopt;
        }

        MarketplaceManifest m;
        m.name        = j.value("name", "");
        m.version     = j.value("version", "");
        m.description = j.value("description", "");
        m.visibility  = j.value("visibility", "public");

        // Resolve type string
        const std::string type_str = j.value("type", "custom");
        if      (type_str == "compute_backend") {
          m.type = PluginType::COMPUTE_BACKEND;
        }
        else if (type_str == "blob_storage")    m.type = PluginType::BLOB_STORAGE;
        else if (type_str == "importer")        m.type = PluginType::IMPORTER;
        else if (type_str == "exporter")        m.type = PluginType::EXPORTER;
        else if (type_str == "hsm_provider")    m.type = PluginType::HSM_PROVIDER;
        else if (type_str == "embedding")       m.type = PluginType::EMBEDDING;
        else if (type_str == "llm_backend")     m.type = PluginType::LLM_BACKEND;
        else if (type_str == "agentic_tool")    m.type = PluginType::AGENTIC_TOOL;
        else if (type_str == "ingestion_step")  m.type = PluginType::INGESTION_STEP;
        else                                    m.type = PluginType::CUSTOM;

        if (j.contains("binary") && j["binary"].is_object()) {
            m.binary_windows = j["binary"].value("windows", "");
            m.binary_linux   = j["binary"].value("linux",   "");
            m.binary_macos   = j["binary"].value("macos",   "");
        }

        if (j.contains("dependencies") && j["dependencies"].is_array()) {
            for (const auto& dep : j["dependencies"]) {
                if (dep.is_string()) {
                    m.dependencies.push_back(dep.get<std::string>());
                }
            }
        }

        if (j.contains("capabilities") && j["capabilities"].is_object()) {
            const auto& caps = j["capabilities"];
            m.capabilities.supports_streaming   = caps.value("streaming",     false);
            m.capabilities.supports_batching    = caps.value("batching",      false);
            m.capabilities.supports_transactions = caps.value("transactions",  false);
            m.capabilities.thread_safe          = caps.value("thread_safe",   false);
            m.capabilities.gpu_accelerated      = caps.value("gpu_accelerated", false);
        }

        m.auto_load      = j.value("auto_load",    false);
        m.load_priority  = j.value("load_priority", 100);
        m.expected_hash  = j.value("expected_hash", "");
        m.license_feature = j.value("license_feature", "");
        m.compatible_core_abi = j.value("compatible_core_abi", "");

        if (j.contains("config_schema") && j["config_schema"].is_object()) {
            m.config_schema = j["config_schema"].dump();
        }

        // Marketplace-specific fields
        m.author             = j.value("author",             "");
        m.license            = j.value("license",            "");
        m.homepage           = j.value("homepage",           "");
        m.repository         = j.value("repository",         "");
        m.documentation      = j.value("documentation",      "");
        m.category           = j.value("category",           "");
        m.marketplace_id     = j.value("marketplace_id",     "");
        m.min_themis_version = j.value("min_themis_version",
            j.value("min_themisdb_version", ""));
        m.max_themis_version = j.value("max_themis_version",
            j.value("max_themisdb_version", ""));
        m.min_themisdb_version = m.min_themis_version;
        m.max_themisdb_version = m.max_themis_version;
        m.verified_publisher = j.value("verified_publisher", false);

        if (j.contains("tags") && j["tags"].is_array()) {
            for (const auto& t : j["tags"]) {
                if (t.is_string()) {
                    m.tags.push_back(t.get<std::string>());
                }
            }
        }

        if (j.contains("allowed_editions") && j["allowed_editions"].is_array()) {
            for (const auto& ed : j["allowed_editions"]) {
                if (ed.is_string()) {
                    m.allowed_editions.push_back(ed.get<std::string>());
                }
            }
        }

        if (j.contains("signature") && j["signature"].is_object()) {
            const auto& sig = j["signature"];
            m.signature.fingerprint = sig.value("fingerprint", "");
            m.signature.algorithm   = sig.value("algorithm",   "");
            m.signature.signed_at   = sig.value("signed_at",   "");
        }

        return m;
    }

private:
    static void checkRequiredString(const json& j, const std::string& field,
                                    std::size_t min_len, std::size_t max_len,
                                    ValidationResult& out) {
        if (!j.contains(field) || !j[field].is_string()) {
            out.errors.push_back("Required field '" + field + "' must be a string");
            return;
        }
        const auto& val = j[field].get<std::string>();
        if (val.size() < min_len) {
            out.errors.push_back("Field '" + field + "' must not be empty");
        }
        if (val.size() > max_len) {
            out.errors.push_back("Field '" + field + "' exceeds maximum length of "
                + std::to_string(max_len));
        }
    }

    static void checkOptionalString(const json& j, const std::string& field,
                                    std::size_t /*min_len*/, std::size_t max_len,
                                    ValidationResult& out) {
        if (!j.contains(field)) {
          return;
        }
        if (!j[field].is_string()) {
            out.errors.push_back("Field '" + field + "' must be a string");
            return;
        }
        if (j[field].get<std::string>().size() > max_len) {
            out.errors.push_back("Field '" + field + "' exceeds maximum length of "
                + std::to_string(max_len));
        }
    }
};

} // namespace plugins
} // namespace themis

/**
 * @brief Convenience macro for plugin implementation
 * 
 * Usage:
 * ```cpp
 * class MyPlugin : public IThemisPlugin { ... };
 * 
 * THEMIS_PLUGIN_IMPL(MyPlugin)
 * ```
 */
#define THEMIS_PLUGIN_IMPL(PluginClass) \
    extern "C" { \
        THEMIS_PLUGIN_EXPORT themis::plugins::IThemisPlugin* createPlugin() { \
            return new PluginClass(); \
        } \
        THEMIS_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin* plugin) { \
            delete plugin; \
        } \
    }

#endif  // THEMISDB_PLUGIN_INTERFACE_H
