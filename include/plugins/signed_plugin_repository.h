/**
 * @file signed_plugin_repository.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "plugins/plugin_interface.h"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace plugins {

/**
 * @brief A pinned public key entry for plugin repository signature verification.
 *
 * Key pinning binds a specific Ed25519 public key to a trusted plugin publisher
 * or repository authority.  Only catalog entries whose detached signature was
 * produced by one of the pinned keys are accepted.
 *
 * Key pinning prevents man-in-the-middle and registry-substitution attacks by
 * binding the repository to an explicit, administrator-controlled set of public
 * keys — analogous to HTTP Public Key Pinning (HPKP) for TLS or GPG keyring
 * management in package managers such as apt/dnf.
 */
struct PinnedKey {
    /// Hex-encoded SHA-256 fingerprint of the raw 32-byte Ed25519 public key.
    std::string fingerprint;

    /// Raw 32-byte Ed25519 public key.
    std::vector<uint8_t> public_key;

    /// Human-readable label, e.g. "ThemisDB Official Repository".
    std::string label;

    /// When false the key is retained for audit purposes but not used for
    /// verification (allows graceful key rotation without hard removal).
    bool active = true;
};

/**
 * @brief A single signed entry in the plugin repository catalog.
 *
 * Each entry pairs a @ref MarketplaceManifest with a detached Ed25519
 * signature so that the catalog can be verified independently of any
 * transport-layer security.
 */
struct RepositoryEntry {
    /// Full plugin manifest (name, version, binary hashes, capabilities …).
    MarketplaceManifest manifest;

    /// Base64-encoded detached Ed25519 signature over the canonical manifest
    /// JSON produced by @ref SignedPluginRepository::canonicalManifestJson.
    std::string signature_b64;

    /// Fingerprint (hex SHA-256) of the @ref PinnedKey that produced the
    /// signature.  Used to look up the matching key during verification.
    std::string key_fingerprint;
};

/**
 * @brief Signed plugin repository with key pinning.
 *
 * Manages an in-process catalog of @ref RepositoryEntry objects.  Each entry
 * must carry a valid Ed25519 signature that was produced by one of the pinned
 * keys before it is accepted into the catalog.
 *
 * ### Typical usage
 * @code
 * SignedPluginRepository repo;
 *
 * // 1. Pin the official ThemisDB repository key (loaded from config / certs).
 * PinnedKey official;
 * official.fingerprint  = "a1b2c3..."; // pre-computed SHA-256 of the raw key
 * official.public_key   = load_bytes("/etc/themis/certs/repo_pub.bin");
 * official.label        = "ThemisDB Official";
 * repo.addPinnedKey(official);
 *
 * // 2. Load a signed index received from the registry endpoint.
 * for (const auto& entry : fetch_index_entries()) {
 *     repo.addEntry(entry);  // rejected if signature invalid or key not pinned
 * }
 *
 * // 3. Look up a plugin at load time.
 * auto entry = repo.findEntry("s3_blob_storage", "1.2.0");
 * if (entry) { use(*entry); }
 * @endcode
 *
 * ### Thread-safety
 * All public methods are thread-safe; internal state is protected by a single
 * mutex.
 *
 * ### Performance target
 * Ed25519 signature verification: < 1 ms per entry on modern hardware
 * (OpenSSL EVP_DigestVerify one-shot).
 */
class SignedPluginRepository {
public:
    SignedPluginRepository() = default;
    ~SignedPluginRepository() = default;

    // Non-copyable, movable
    SignedPluginRepository(const SignedPluginRepository&) = delete;
    SignedPluginRepository& operator=(const SignedPluginRepository&) = delete;
    SignedPluginRepository(SignedPluginRepository&&) = default;
    SignedPluginRepository& operator=(SignedPluginRepository&&) = default;

    // -------------------------------------------------------------------------
    // Key management
    // -------------------------------------------------------------------------

    /**
     * @brief Add a trusted (pinned) public key.
     *
     * If a key with the same fingerprint already exists it is replaced.
     * Callers should pre-compute the fingerprint with @ref computeKeyFingerprint
     * and store it alongside the key in configuration or PKI infrastructure.
     *
     * @param key  Key to pin.
     */
    void addPinnedKey(PinnedKey key);

    /**
     * @brief Deactivate and remove a pinned key by its fingerprint.
     *
     * @param fingerprint  Hex-encoded SHA-256 fingerprint to remove.
     * @return true if a key was found and removed, false if not found.
     */
    bool removePinnedKey(const std::string& fingerprint);

    /**
     * @brief Check whether a key with the given fingerprint is currently pinned
     *        and active.
     */
    bool hasPinnedKey(const std::string& fingerprint) const;

    /**
     * @brief Return a snapshot of all pinned keys (active and inactive).
     */
    std::vector<PinnedKey> getPinnedKeys() const;

    // -------------------------------------------------------------------------
    // Catalog management
    // -------------------------------------------------------------------------

    /**
     * @brief Verify @p entry and, if valid, add it to the catalog.
     *
     * An entry is accepted only when:
     *  - its @c key_fingerprint resolves to an *active* pinned key, and
     *  - the Ed25519 @c signature_b64 verifies against the canonical manifest
     *    JSON (@ref canonicalManifestJson) using that key.
     *
     * Adding an entry with the same (name, version) pair as an existing entry
     * replaces the old entry.
     *
     * @param entry  Catalog entry to add.
     * @return true on success, false if verification failed.
     */
    bool addEntry(const RepositoryEntry& entry);

    /**
     * @brief Verify @p entry's signature without adding it to the catalog.
     *
     * @param entry  Entry to verify.
     * @return true if signature is valid and the signing key is pinned and
     *         active.
     */
    bool verifyEntry(const RepositoryEntry& entry) const;

    /**
     * @brief Look up the latest (highest semver) entry for a given plugin name.
     *
     * @param name     Plugin name (case-sensitive).
     * @param version  Optional exact version string.  When empty the entry with
     *                 the lexicographically greatest version is returned.
     * @return The matching entry, or @c std::nullopt if not found.
     */
    std::optional<RepositoryEntry> findEntry(
        const std::string& name,
        const std::string& version = "") const;

    /**
     * @brief Return all catalog entries whose name matches @p name.
     */
    std::vector<RepositoryEntry> findByName(const std::string& name) const;

    /**
     * @brief Return a snapshot of all catalog entries.
     */
    std::vector<RepositoryEntry> listEntries() const;

    /**
     * @brief Remove all entries from the catalog (pinned keys are retained).
     */
    void clear();

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /**
     * @brief Compute the hex-encoded SHA-256 fingerprint of a raw public key.
     *
     * This is the canonical fingerprint format used by @ref PinnedKey::fingerprint
     * and @ref RepositoryEntry::key_fingerprint.
     *
     * @param public_key  Raw key bytes (32 bytes for Ed25519).
     * @return Lowercase hex string (64 characters for SHA-256), or empty on
     *         error.
     */
    static std::string computeKeyFingerprint(const std::vector<uint8_t>& public_key);

    /**
     * @brief Produce the canonical JSON string used as the signing payload.
     *
     * Fields are serialised in alphabetical key order with no extraneous
     * whitespace to ensure a deterministic byte sequence independent of JSON
     * library or platform.
     *
     * Covered fields (subset of @ref MarketplaceManifest), serialised in
     * alphabetical key order:
     *   author, binary_linux, binary_macos, binary_windows, description,
     *   expected_hash, license, min_themis_version, name, type,
     *   verified_publisher, version.
     *
     * @param manifest  Manifest to serialise.
     * @return Canonical JSON string.
     */
    static std::string canonicalManifestJson(const MarketplaceManifest& manifest);

private:
    mutable std::mutex mutex_;
    std::vector<PinnedKey>      pinned_keys_;
    std::vector<RepositoryEntry> entries_;

    /// Find a pinned key by fingerprint (caller must hold mutex_).
    const PinnedKey* findPinnedKeyLocked(const std::string& fingerprint) const;

    /// Verify entry signature and key-pin status.
    /// Caller **must** hold mutex_ before calling this method.
    bool verifyEntryLocked(const RepositoryEntry& entry) const;

    /// Verify an Ed25519 signature using the given raw public key.
    bool verifyEd25519Signature(
        const std::vector<uint8_t>& public_key,
        const std::string& message,
        const std::vector<uint8_t>& signature) const;
};

} // namespace plugins
} // namespace themis
