/**
 * @file delta_update_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "updates/release_manifest.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace updates {

/**
 * @brief Supported binary patch algorithms
 */
enum class PatchAlgorithm {
    BSDIFF,     ///< bsdiff – best compression ratio, slower generation
    XDELTA3,    ///< xdelta3 – fast, good compression
    VCDIFF,     ///< RFC 3284 VCDIFF encoding, compressed with zstd (pure C++)
    ZSTD_DICT,  ///< zstd dictionary-based compression (base file as dict)
};

/**
 * @brief Delta descriptor for a single file in a release
 * 
 * Supports optional patch ordering constraints via depends_on and apply_order.
 * When a manifest has enforce_order=true, patches are applied in dependency order
 * as determined by topological sort (see DeltaManifest::enforce_order).
 */
struct FileDelta {
    std::string path;         ///< Relative file path (e.g. "bin/themis_server")
    std::string base_hash;    ///< SHA-256 of the base (from) file
    std::string target_hash;  ///< SHA-256 of the reconstructed target file
    std::string patch_url;    ///< URL to download the binary patch
    uint64_t patch_size  = 0; ///< Compressed patch size in bytes
    uint64_t target_size = 0; ///< Expected size of the reconstructed file
    PatchAlgorithm algorithm  = PatchAlgorithm::ZSTD_DICT;
    
    /// @brief Paths this file depends on (other FileDelta::path values).
    /// Empty means no ordering dependencies.
    /// Used only if DeltaManifest::enforce_order is true.
    std::vector<std::string> depends_on;
    
    /// @brief Explicit ordering hint: lower values applied first.
    /// 0 (default) = no preference; ties broken by manifest order.
    /// Used only if DeltaManifest::enforce_order is true.
    uint32_t apply_order = 0;

    json toJson() const;
    static std::optional<FileDelta> fromJson(const json& j);
};

/**
 * @brief Manifest describing a delta (binary diff) update between two versions
 *
 * A DeltaManifest is stored alongside the normal ReleaseManifest and describes
 * how to reconstruct the target release by applying binary patches to the
 * currently installed files instead of downloading full replacements.
 *
 * Expected bandwidth savings: 70-90 % for typical incremental releases.
 */
struct DeltaManifest {
    std::string from_version; ///< Base version (e.g. "1.4.0")
    std::string to_version;   ///< Target version (e.g. "1.5.0")
    std::vector<FileDelta> deltas;

    /// Convenience: total bytes that need to be downloaded (sum of patch_size)
    uint64_t totalPatchSize() const;

    /// Convenience: total bytes of the reconstructed files (sum of target_size)
    uint64_t totalTargetSize() const;

    json toJson() const;
    static std::optional<DeltaManifest> fromJson(const json& j);
};

/**
 * @brief Result of applying a DeltaManifest to the installed files
 */
struct DeltaApplyResult {
    bool success = false;
    std::string error_message;
    std::vector<std::string> files_patched;   ///< Files updated via delta patch
    std::vector<std::string> files_fallback;  ///< Files that fell back to full download
};

/**
 * @brief Engine that generates and applies binary delta patches
 *
 * ### Usage (apply path)
 * ```cpp
 * DeltaUpdateEngine engine(install_dir, download_dir);
 * engine.setProgressCallback([](int pct, const std::string& msg) { ... });
 *
 * auto delta = engine.findDelta("1.4.0", "1.5.0");
 * if (delta) {
 *     auto result = engine.applyDelta(*delta);
 *     if (!result.success) {
 *         // engine already attempted per-file fallback; caller may trigger full update
 *     }
 * }
 * ```
 *
 * ### Thread-safety
 * Not thread-safe. A single DeltaUpdateEngine instance must not be used
 * concurrently from multiple threads (consistent with HotReloadEngine).
 */
class DeltaUpdateEngine {
public:
    /**
     * @param install_directory  Directory where current binaries live
     * @param download_directory Scratch directory for downloaded patches
     */
    explicit DeltaUpdateEngine(
        std::string install_directory  = ".",
        std::string download_directory = "/tmp/themis_updates"
    );

    ~DeltaUpdateEngine();

    // ── Delta lookup ─────────────────────────────────────────────────────────

    /**
     * @brief Check whether a delta manifest exists for a given version pair
     * @param from_version Currently installed version
     * @param to_version   Target version
     * @return DeltaManifest if a delta is available, nullopt otherwise
     */
    std::optional<DeltaManifest> findDelta(
        const std::string& from_version,
        const std::string& to_version) const;

    /**
     * @brief Register a delta manifest (e.g. parsed from a release asset)
     */
    void registerDelta(const DeltaManifest& manifest);

    // ── Patch application ────────────────────────────────────────────────────

    /**
     * @brief Apply all file deltas in a DeltaManifest
     *
     * For each FileDelta:
     *  1. Validate the relative path (rejects traversal attempts).
     *  2. Verify that the installed base file matches base_hash.
     *  3. Locate the pre-downloaded patch at
     *     <download_directory>/<path>.patch  (caller is responsible for
     *     fetching patches via the utils HTTP client before calling this).
     *  4. Apply the patch algorithm to produce the target file.
     *  5. Verify the reconstructed file against target_hash and target_size.
     *  6. Atomically replace the installed file.
     *
     * If any single file fails steps 1-4 the engine falls back to a sentinel
     * error in DeltaApplyResult::files_fallback without aborting other files.
     *
     * @param manifest  DeltaManifest returned by findDelta()
     * @return          Aggregate result
     */
    DeltaApplyResult applyDelta(const DeltaManifest& manifest);

    // ── Patch generation ─────────────────────────────────────────────────────

    /**
     * @brief Generate a binary patch between two local files
     *
     * Intended for CI/CD pipelines that produce release artifacts.
     *
     * @param base_path    Path to the old (base) file
     * @param target_path  Path to the new (target) file
     * @param patch_path   Destination path for the generated patch
     * @param algorithm    Patch algorithm to use (default: ZSTD_DICT)
     * @return true on success
     */
    bool generatePatch(
        const std::string& base_path,
        const std::string& target_path,
        const std::string& patch_path,
        PatchAlgorithm algorithm = PatchAlgorithm::ZSTD_DICT);

    /**
     * @brief Apply a single binary patch file to a base file
     *
     * @param base_path   Path to the base (old) file
     * @param patch_path  Path to the downloaded patch file
     * @param target_path Destination path for the reconstructed file
     * @return true on success
     */
    bool applyPatch(
        const std::string& base_path,
        const std::string& patch_path,
        const std::string& target_path);

    // ── Observability ────────────────────────────────────────────────────────

    /**
     * @brief Register a progress callback invoked during patch application
     * @param callback Function receiving (percentage 0-100, message)
     */
    void setProgressCallback(
        std::function<void(int, const std::string&)> callback);

private:
    std::string install_dir_;
    std::string download_dir_;
    std::function<void(int, const std::string&)> progress_cb_;
    std::vector<DeltaManifest> registered_deltas_;

    void reportProgress(int pct, const std::string& msg);

    // Per-algorithm generate/apply helpers
    bool generatePatchZstdDict(
        const std::vector<uint8_t>& base,
        const std::vector<uint8_t>& target,
        const std::string& patch_path);

    bool applyPatchZstdDict(
        const std::vector<uint8_t>& base,
        const std::string& patch_path,
        const std::string& target_path);

    bool generatePatchVcdiff(
        const std::vector<uint8_t>& base,
        const std::vector<uint8_t>& target,
        const std::string& patch_path);

    bool applyPatchVcdiff(
        const std::vector<uint8_t>& base,
        const std::string& patch_path,
        const std::string& target_path);

    // Utility
    static std::string calculateHash(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::vector<uint8_t>& data);
    static bool atomicWriteFile(const std::string& path, const std::vector<uint8_t>& data);
};

// Helpers for converting PatchAlgorithm to/from string
std::string patchAlgorithmToString(PatchAlgorithm algo);
std::optional<PatchAlgorithm> patchAlgorithmFromString(const std::string& s);

} // namespace updates
} // namespace themis

