/**
 * @file delta_update_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=22, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "updates/delta_update_engine.h"
#include "updates/batch5_safety_helpers.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

#include <openssl/evp.h>

#ifdef THEMIS_HAS_ZSTD
#include <zstd.h>
#endif

namespace themis {
namespace updates {

namespace fs = std::filesystem;

// ============================================================================
// RAII Wrapper for EVP_MD_CTX (Error Code: 7464-7465)
// ============================================================================

/**
 * @brief RAII wrapper for EVP_MD_CTX to ensure cleanup in all execution paths.
 * 
 * Guarantees exception-safe resource cleanup of OpenSSL EVP context.
 * Prevents resource leaks even during early returns or exceptions.
 * 
 * @error_code 7464 EVP_MD_CTX resource leak in exception path
 */
class EvpMdCtxRaii {
public:
    explicit EvpMdCtxRaii(EVP_MD_CTX* ctx = nullptr) : ctx_(ctx) {}
    
    ~EvpMdCtxRaii() {
        if (ctx_) {
            EVP_MD_CTX_free(ctx_);
        }
    }
    
    // Non-copyable
    EvpMdCtxRaii(const EvpMdCtxRaii&) = delete;
    EvpMdCtxRaii& operator=(const EvpMdCtxRaii&) = delete;
    
    // Movable
    EvpMdCtxRaii(EvpMdCtxRaii&& other) noexcept : ctx_(other.release()) {}
    EvpMdCtxRaii& operator=(EvpMdCtxRaii&& other) noexcept {
        if (this != &other) {
            if (ctx_) {
              EVP_MD_CTX_free(ctx_);
            }
            ctx_ = other.release();
        }
        return *this;
    }
    
    EVP_MD_CTX* get() const noexcept { return ctx_; }
    EVP_MD_CTX* release() noexcept {
        EVP_MD_CTX* tmp = ctx_;
        ctx_ = nullptr;
        return tmp;
    }
    
private:
    EVP_MD_CTX* ctx_ = nullptr;
};

// ============================================================================
// Security helper: path traversal prevention
// ============================================================================

/**
 * @brief Validate a relative file path from an untrusted manifest.
 *
 * Rejects paths that:
 *  - are empty
 *  - are absolute (start with '/')
 *  - contain ".." components (directory traversal)
 *  - contain null bytes
 *
 * After constructing the full path we additionally verify it is lexically
 * contained within the expected base directory using weakly_canonical.
 *
 * @param rel_path  Relative path from a FileDelta
 * @param base_dir  The directory the file must reside within
 * @return true if safe; false if the path should be rejected
 */
static bool isSafePath(const std::string& rel_path, const std::string& base_dir) {
    if (rel_path.empty()) {
      return false;
    }
    // Reject absolute paths and null bytes
    if (rel_path[0] == '/' || rel_path[0] == '\\' || rel_path.find('\0') != std::string::npos) {
      return false;
    }

    // Reject absolute/drive-rooted paths and any ".." component.
    fs::path p(rel_path);
    if (p.is_absolute() || p.has_root_name() || p.has_root_directory()) {
      return false;
    }
    for (const auto& component : p) {
        if (component == "..") {
          return false;
        }
    }

    // Final check: the resolved path must be inside base_dir.
    try {
        auto full = fs::weakly_canonical(fs::path(base_dir) / p);
        auto base = fs::weakly_canonical(fs::path(base_dir));

        // Use lexical relation instead of raw string prefix checks so Windows
        // path separators and drive handling are evaluated correctly.
        const auto rel = full.lexically_relative(base);
        if (rel.empty() || rel.is_absolute()) {
          return false;
        }
        for (const auto& component : rel) {
            if (component == "..") {
              return false;
            }
        }
    } catch (...) {
        return false;
    }
    return true;
}

// ============================================================================
// Patch file format
// ============================================================================
//
// ZSTD_DICT patch:
//   [8 bytes] magic  = "TDLTZSTD"
//   [8 bytes] original_size (little-endian uint64)
//   [N bytes] zstd-compressed target using base as raw dictionary
//
// VCDIFF patch:
//   [8 bytes] magic  = "TDLTVCDIFF"  (first 8 bytes only)
//   Actually we use:  "TDLTVCD\x01"
//   [8 bytes] original_size
//   [N bytes] VCDIFF instruction stream, compressed with zstd
//
//   VCDIFF instruction encoding:
//     Each instruction is one of:
//       ADD  [1 byte type=0x01] [4 bytes length] [length bytes data]
//       COPY [1 byte type=0x02] [4 bytes offset] [4 bytes length]
//     Instructions are concatenated without extra framing.

static constexpr uint8_t MAGIC_ZSTD[8] = {'T','D','L','T','Z','S','T','D'};
static constexpr uint8_t MAGIC_VCD[8]  = {'T','D','L','T','V','C','D','\x01'};
static constexpr uint8_t INSTR_ADD  = 0x01;
static constexpr uint8_t INSTR_COPY = 0x02;

// ============================================================================
// Utility functions
// ============================================================================

std::string DeltaUpdateEngine::calculateHash(const std::vector<uint8_t>& data) {
    // Use RAII wrapper for EVP_MD_CTX (Error Code: 7465)
    EvpMdCtxRaii ctx(EVP_MD_CTX_new());
    if (!ctx.get()) {
      return "";
    }

    if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
        return "";
    }
    if (EVP_DigestUpdate(ctx.get(), data.data(),static_cast<int>(data.size())) != 1) {
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hashLen = 0;
    if (EVP_DigestFinal_ex(ctx.get(), hash, &hashLen) != 1) {
        return "";
    }

    std::ostringstream ss = {};
    for (unsigned int i = 0; i < hashLen; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::vector<uint8_t> DeltaUpdateEngine::readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

bool DeltaUpdateEngine::writeFile(const std::string& path,
                                  const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
      return false;
    }
    f.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
    return f.good();
}

bool DeltaUpdateEngine::atomicWriteFile(const std::string& path,
                                        const std::vector<uint8_t>& data) {
    std::string tmp = path + ".tmp";
    if (!writeFile(tmp, data)) {
      return false;
    }
    try {
        fs::rename(tmp, path);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("atomicWriteFile rename failed for {}: {}", path, e.what());
        fs::remove(tmp);
        return false;
    }
}

// ============================================================================
// PatchAlgorithm string conversion
// ============================================================================

std::string patchAlgorithmToString(PatchAlgorithm algo) {
    switch (algo) {
        case PatchAlgorithm::BSDIFF:    return "bsdiff";
        case PatchAlgorithm::XDELTA3:   return "xdelta3";
        case PatchAlgorithm::VCDIFF:    return "vcdiff";
        case PatchAlgorithm::ZSTD_DICT: return "zstd_dict";
        default:                        return "unknown";
    }
}

std::optional<PatchAlgorithm> patchAlgorithmFromString(const std::string& s) {
    if (s == "bsdiff") {
      return PatchAlgorithm::BSDIFF;
    }
    if (s == "xdelta3") {
      return PatchAlgorithm::XDELTA3;
    }
    if (s == "vcdiff") {
      return PatchAlgorithm::VCDIFF;
    }
    if (s == "zstd_dict") {
      return PatchAlgorithm::ZSTD_DICT;
    }
    return std::nullopt;
}

// ============================================================================
// FileDelta JSON
// ============================================================================

json FileDelta::toJson() const {
    json j;
    j["path"]        = path;
    j["base_hash"]   = base_hash;
    j["target_hash"] = target_hash;
    j["patch_url"]   = patch_url;
    j["patch_size"]  = patch_size;
    j["target_size"] = target_size;
    j["algorithm"]   = patchAlgorithmToString(algorithm);
    
    // Serialize ordering fields (empty by default for backward compatibility)
    if (!depends_on.empty()) {
        j["depends_on"] = depends_on;
    }
    if (apply_order != 0) {
        j["apply_order"] = apply_order;
    }
    
    return j;
}

std::optional<FileDelta> FileDelta::fromJson(const json& j) {
    try {
        FileDelta fd;
        fd.path        = j.value("path",        "");
        fd.base_hash   = j.value("base_hash",   "");
        fd.target_hash = j.value("target_hash", "");
        fd.patch_url   = j.value("patch_url",   "");
        fd.patch_size  = j.value("patch_size",  static_cast<uint64_t>(0));
        fd.target_size = j.value("target_size", static_cast<uint64_t>(0));

        auto algo_str = j.value("algorithm", "zstd_dict");
        auto algo     = patchAlgorithmFromString(algo_str);
        fd.algorithm  = algo.value_or(PatchAlgorithm::ZSTD_DICT);
        
        // Deserialize ordering fields
        if (j.contains("depends_on") && j["depends_on"].is_array()) {
            fd.depends_on = j["depends_on"].get<std::vector<std::string>>();
        }
        fd.apply_order = j.value("apply_order", static_cast<uint32_t>(0));

        return fd;
    } catch (...) {
        return std::nullopt;
    }
}

// ============================================================================
// DeltaManifest
// ============================================================================

uint64_t DeltaManifest::totalPatchSize() const {
    uint64_t total = 0;
    for (const auto& d : deltas) {
      total += d.patch_size;
    }
    return total;
}

uint64_t DeltaManifest::totalTargetSize() const {
    uint64_t total = 0;
    for (const auto& d : deltas) {
      total += d.target_size;
    }
    return total;
}

json DeltaManifest::toJson() const {
    json j;
    j["from_version"] = from_version;
    j["to_version"]   = to_version;
    j["deltas"]       = json::array();
    for (const auto& d : deltas) {
        j["deltas"].push_back(d.toJson());
    }
    
    // Serialize ordering fields (omit if defaults for backward compatibility)
    if (enforce_order) {
        j["enforce_order"] = enforce_order;
    }
    if (!implicit_dependencies.empty()) {
        j["implicit_dependencies"] = implicit_dependencies;
    }
    
    return j;
}

std::optional<DeltaManifest> DeltaManifest::fromJson(const json& j) {
    try {
        DeltaManifest dm;
        dm.from_version = j.value("from_version", "");
        dm.to_version   = j.value("to_version",   "");

        if (j.contains("deltas") && j["deltas"].is_array()) {
            for (const auto& dj : j["deltas"]) {
                auto fd = FileDelta::fromJson(dj);
                if (fd) {
                  dm.deltas.push_back(*fd);
                }
            }
        }
        
        // Deserialize ordering fields
        dm.enforce_order = j.value("enforce_order", false);
        if (j.contains("implicit_dependencies") && j["implicit_dependencies"].is_array()) {
            dm.implicit_dependencies = j["implicit_dependencies"].get<std::vector<std::string>>();
        }
        
        return dm;
    } catch (...) {
        return std::nullopt;
    }
}

// ============================================================================
// DeltaUpdateEngine
// ============================================================================

DeltaUpdateEngine::DeltaUpdateEngine(std::string install_directory,
                                     std::string download_directory)
    : install_dir_(std::move(install_directory))
    , download_dir_(std::move(download_directory)) {
    fs::create_directories(download_dir_);
}

DeltaUpdateEngine::~DeltaUpdateEngine() = default;

void DeltaUpdateEngine::reportProgress(int pct, const std::string& msg) {
    LOG_DEBUG("DeltaUpdateEngine: {}% - {}", pct, msg);
    if (progress_cb_) {
      progress_cb_(pct, msg);
    }
}

// ============================================================================
// Patch Ordering Enforcement (UPD-IMPL-003)
// ============================================================================

bool DeltaUpdateEngine::validateDependencies(const DeltaManifest& manifest) {
    // Build a set of all available patch paths
    std::unordered_set<std::string> available_paths = {};

    for (const auto& fd : manifest.deltas) {
        available_paths.insert(fd.path);
    }
    
    // Check that all dependencies exist in the manifest
    for (const auto& fd : manifest.deltas) {
        for (const auto& dep : fd.depends_on) {
            if (available_paths.find(dep) == available_paths.end()) {
                LOG_ERROR(
                    "Patch ordering: dependency '{}' for '{}' not found in manifest (7404)",
                    dep, fd.path);
                return false;  // Error code 7404: Dependency file missing
            }
        }
        
        // Also check implicit dependencies
        for (const auto& dep : manifest.implicit_dependencies) {
            if (available_paths.find(dep) == available_paths.end()) {
                LOG_ERROR(
                    "Patch ordering: implicit dependency '{}' for '{}' not found in manifest (7404)",
                    dep, fd.path);
                return false;  // Error code 7404: Dependency file missing
            }
        }
    }
    
    return true;
}

bool DeltaUpdateEngine::hasCircularDependency(const std::vector<FileDelta>& deltas) {
    // Build adjacency list and in-degree count
    std::unordered_map<std::string, std::vector<std::string>> adj_list;
    std::unordered_map<std::string, int> in_degree;
    std::unordered_map<std::string, size_t> delta_indices;  // For quick lookup
    
    for (size_t i = 0; i <static_cast<int>(deltas.size()); ++i) {
        delta_indices[deltas[i].path] = i;
        in_degree[deltas[i].path] = 0;
    }
    
    // Build adjacency list and compute in-degrees
    for (const auto& fd : deltas) {
        for (const auto& dep : fd.depends_on) {
            // fd depends on dep, so dep -> fd is an edge in dependency graph
            adj_list[dep].push_back(fd.path);
            in_degree[fd.path]++;
        }
    }
    
    // Kahn's algorithm: if we can't process all nodes, there's a cycle
    std::queue<std::string> queue = {};

    for (const auto& [path, degree] : in_degree) {
        if (degree == 0) {
            queue.push(path);
        }
    }
    
    int processed = 0;
    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();
        ++processed;
        
        for (const auto& neighbor : adj_list[current]) {
            --in_degree[neighbor];
            if (in_degree[neighbor] == 0) {
                queue.push(neighbor);
            }
        }
    }
    
    // If we couldn't process all nodes, there's a cycle
    bool has_cycle = (processed != static_cast<int>(deltas.size()));
    if (has_cycle) {
        LOG_ERROR("Patch ordering: circular dependency detected (7402)");
    }
    return has_cycle;
}

std::vector<FileDelta> DeltaUpdateEngine::computeApplyOrder(const DeltaManifest& manifest) {
    std::vector<FileDelta> result;
    
    // Return empty list on any validation error
    if (!validateDependencies(manifest)) {
        return result;  // Error code 7404: Dependency file missing
    }
    
    if (hasCircularDependency(manifest.deltas)) {
        return result;  // Error code 7402: Circular dependency
    }
    
    // Build adjacency list and in-degree count for topological sort
    std::unordered_map<std::string, std::vector<std::string>> adj_list;
    std::unordered_map<std::string, int> in_degree;
    std::unordered_map<std::string, FileDelta> delta_by_path;

    // Initialize data structures
    for (const auto& fd : manifest.deltas) {
        delta_by_path[fd.path] = fd;
        in_degree[fd.path] = 0;
    }

    // Build adjacency list and compute in-degrees based on explicit dependencies
    for (const auto& fd : manifest.deltas) {
        for (const auto& dep : fd.depends_on) {
            // fd depends on dep, so dep -> fd is an edge
            adj_list[dep].push_back(fd.path);
            in_degree[fd.path]++;
        }

        // Also add implicit dependencies
        for (const auto& dep : manifest.implicit_dependencies) {
            // Only if not already in explicit depends_on
            if (std::find(fd.depends_on.begin(), fd.depends_on.end(), dep) == fd.depends_on.end()) {
                adj_list[dep].push_back(fd.path);
                in_degree[fd.path]++;
            }
        }
    }

    // Kahn's algorithm: use a min-heap keyed by (apply_order, path) for
    // deterministic tie-breaking regardless of hash-map iteration order.
    using Entry = std::pair<uint32_t, std::string>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> ready;
    for (const auto& fd : manifest.deltas) {
        if (in_degree[fd.path] == 0) {
            ready.push({fd.apply_order, fd.path});
        }
    }

    while (!ready.empty()) {
        auto [order, current] = ready.top();
        ready.pop();

        result.push_back(delta_by_path[current]);

        // Collect newly-ready neighbours and push with their apply_order key
        for (const auto& neighbor : adj_list[current]) {
            if (--in_degree[neighbor] == 0) {
                ready.push({delta_by_path[neighbor].apply_order, neighbor});
            }
        }
    }

    // Cycle detection: if we didn't process every delta the graph has a cycle.
    if (static_cast<int>(result.size()) != manifest.deltas.size()) {
        LOG_ERROR("computeApplyOrder: cycle detected – processed {}/{} patches; aborting",
                  result.size(),static_cast<int>(manifest.deltas.size()));
        return {};
    }

    LOG_INFO("Patch ordering computed: {} patches in dependency order",static_cast<int>(result.size()));
    return result;
}

void DeltaUpdateEngine::setProgressCallback(
    std::function<void(int, const std::string&)> callback) {
    progress_cb_ = std::move([[maybe_unused]] callback);
}

// ── Delta registry ────────────────────────────────────────────────────────

void DeltaUpdateEngine::registerDelta(const DeltaManifest& manifest) {
    // Replace existing entry for the same version pair if present
    for (auto& existing : registered_deltas_) {
        if (existing.from_version == manifest.from_version &&
            existing.to_version   == manifest.to_version) {
            existing = manifest;
            return;
        }
    }
    registered_deltas_.push_back(manifest);
}

std::optional<DeltaManifest> DeltaUpdateEngine::findDelta(
    const std::string& from_version,
    const std::string& to_version) const {
    for (const auto& dm : registered_deltas_) {
        if (dm.from_version == from_version && dm.to_version == to_version) {
            return dm;
        }
    }
    return std::nullopt;
}

// ── Patch application (public) ────────────────────────────────────────────

DeltaApplyResult DeltaUpdateEngine::applyDelta(const DeltaManifest& manifest) {
    DeltaApplyResult result;
    result.success = true;

    // --- Ordering Enforcement (UPD-IMPL-003) ---
    // If enforce_order is true, compute and apply patches in dependency order
    std::vector<FileDelta> deltas_to_apply = manifest.deltas;
    
    if (manifest.enforce_order) {
        LOG_INFO("Patch ordering enforcement enabled; computing apply order...");
        deltas_to_apply = computeApplyOrder(manifest);
        
        if (deltas_to_apply.empty() && !manifest.deltas.empty()) {
            LOG_ERROR("Failed to compute patch order (circular dependency or missing dependency)");
            result.success = false;
            result.error_message = "Patch ordering failed: circular or missing dependencies";
            return result;
        }
    }

    size_t total = deltas_to_apply.size();
    size_t idx   = 0;

    for (const auto& fd : deltas_to_apply) {
        ++idx;
        int pct = static_cast<int>(idx * 100 / (total > 0 ? total : 1));
        reportProgress(pct, "Patching " + fd.path);

        // --- 0. Validate relative path (path traversal prevention) ---
        if (!isSafePath(fd.path, install_dir_)) {
            LOG_ERROR("Unsafe path rejected in delta manifest: '{}'", fd.path);
            result.files_fallback.push_back(fd.path);
            continue;
        }

        // --- 1. Load base file ---
        // Use fs::path for portable path handling (Error Code: 7466)
        fs::path base_path = fs::path(install_dir_) / fd.path;
        auto base_data = readFile(base_path.string());

        // --- 2. Verify base hash ---
        if (!fd.base_hash.empty()) {
           auto actual_base_hash = calculateHash(base_data);
           if (actual_base_hash != fd.base_hash) {
               LOG_WARN("Base hash mismatch for {}: expected {} got {}",
                   fd.path, fd.base_hash, actual_base_hash);
               result.files_fallback.push_back(fd.path);
               continue;
           }
        }

        // --- 3. Locate patch file (must be pre-downloaded) ---
        fs::path patch_path = fs::path(download_dir_) / (fd.path + ".patch");

        if (!fs::exists(patch_path)) {
           LOG_WARN("Patch file not found for {}: {}", fd.path, patch_path.string());
           result.files_fallback.push_back(fd.path);
           continue;
        }

        // --- 4. Reconstruct target ---
        fs::path recon_path = fs::path(download_dir_) / (fd.path + ".patched");
        fs::create_directories(recon_path.parent_path());

        if (!applyPatch(base_path.string(), patch_path.string(), recon_path.string())) {
           LOG_WARN("applyPatch failed for {}", fd.path);
           result.files_fallback.push_back(fd.path);
           fs::remove(recon_path);
           continue;
        }

        // --- 5. Verify reconstructed target hash ---
        auto target_data = readFile(recon_path.string());
        if (!fd.target_hash.empty()) {
           auto actual_target_hash = calculateHash(target_data);
           if (actual_target_hash != fd.target_hash) {
               LOG_WARN("Target hash mismatch for {}: expected {} got {}",
                   fd.path, fd.target_hash, actual_target_hash);
                result.files_fallback.push_back(fd.path);
                fs::remove(recon_path);
                continue;
            }
        }

        // --- 6. Verify size ---
        if (fd.target_size > 0 && static_cast<int>(target_data.size()) != fd.target_size) {
            LOG_WARN("Target size mismatch for {}: expected {} got {}",
                fd.path, fd.target_size,static_cast<int>(target_data.size()));
            result.files_fallback.push_back(fd.path);
            fs::remove(recon_path);
            continue;
        }

        // --- 7. Atomic install ---
        fs::path install_path = fs::path(install_dir_) / fd.path;  // Use fs::path (Error Code: 7467)
        fs::create_directories(install_path.parent_path());

        try {
            fs::rename(recon_path, install_path);
            result.files_patched.push_back(fd.path);
            LOG_INFO("Delta patched: {}", fd.path);
        } catch (const std::exception& e) {
            LOG_WARN("Atomic rename failed for {}: {}", fd.path, e.what());
            fs::remove(recon_path);
            result.files_fallback.push_back(fd.path);
        }
    }

    // Partial fallback is still a success at the engine level (caller decides
    // whether to run a full-download for the fallback files).
    result.success = true;
    if (!result.files_fallback.empty()) {
        result.error_message =
            std::to_string(result.files_fallback.size()) +
            " file(s) fell back to full download";
        LOG_WARN("DeltaUpdateEngine: {}", result.error_message);
    }
    return result;
}

// ── applyPatch (public) ───────────────────────────────────────────────────

bool DeltaUpdateEngine::applyPatch(const std::string& base_path,
                                   const std::string& patch_path,
                                   const std::string& target_path) {
    // Read the patch header to determine algorithm
    std::ifstream pf(patch_path, std::ios::binary);
    if (!pf) {
        LOG_ERROR("Cannot open patch file: {}", patch_path);
        return false;
    }

    uint8_t magic[8];
    if (!pf.read(reinterpret_cast<char*>(magic), 8)) {
        LOG_ERROR("Patch file too short: {}", patch_path);
        return false;
    }
    pf.close();

    auto base = readFile(base_path);

    if (std::memcmp(magic, MAGIC_ZSTD, 8) == 0) {
        return applyPatchZstdDict(base, patch_path, target_path);
    }
    if (std::memcmp(magic, MAGIC_VCD, 8) == 0) {
        return applyPatchVcdiff(base, patch_path, target_path);
    }

    LOG_ERROR("Unknown patch magic in: {}", patch_path);
    return false;
}

// ── generatePatch (public) ────────────────────────────────────────────────

bool DeltaUpdateEngine::generatePatch(const std::string& base_path,
                                      const std::string& target_path,
                                      const std::string& patch_path,
                                      PatchAlgorithm algorithm) {
    auto base   = readFile(base_path);
    auto target = readFile(target_path);

    if (base.empty() && !fs::exists(base_path)) {
        LOG_ERROR("Base file not found: {}", base_path);
        return false;
    }
    if (target.empty() && !fs::exists(target_path)) {
        LOG_ERROR("Target file not found: {}", target_path);
        return false;
    }

    fs::create_directories(fs::path(patch_path).parent_path());

    switch (algorithm) {
        case PatchAlgorithm::ZSTD_DICT:
            return generatePatchZstdDict(base, target, patch_path);
        case PatchAlgorithm::VCDIFF:
            return generatePatchVcdiff(base, target, patch_path);
        case PatchAlgorithm::BSDIFF:
        [[fallthrough]];\n        case PatchAlgorithm::XDELTA3:
            // These algorithms need external libraries not bundled with ThemisDB.
            // Fall back to ZSTD_DICT which is always available.
            LOG_WARN("Algorithm {} not compiled in – falling back to ZSTD_DICT",
                patchAlgorithmToString(algorithm));
            return generatePatchZstdDict(base, target, patch_path);
    }
    return false;
}

// ── ZSTD_DICT implementation ──────────────────────────────────────────────

bool DeltaUpdateEngine::generatePatchZstdDict(
    const std::vector<uint8_t>& base,
    const std::vector<uint8_t>& target,
    const std::string& patch_path) {
#ifdef THEMIS_HAS_ZSTD
    // Use the base file as a raw dictionary to compress the target.
    // This is standard zstd dictionary mode – efficient when files share
    // large common regions (typical for binary updates).

    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    if (!cctx) {
        LOG_ERROR("ZSTD_createCCtx failed");
        return false;
    }

    // Create dictionary from base data
    ZSTD_CDict* cdict = ZSTD_createCDict(
        base.data(),static_cast<int>(base.size()), ZSTD_CLEVEL_DEFAULT);
    if (!cdict) {
        ZSTD_freeCCtx(cctx);
        LOG_ERROR("ZSTD_createCDict failed");
        return false;
    }

    size_t bound        = ZSTD_compressBound(target.size());
    std::vector<uint8_t> compressed(bound);

    size_t compressed_size = ZSTD_compress_usingCDict(
        cctx,
        compressed.data(), bound,
        target.data(),static_cast<int>(target.size()),
        cdict);

    ZSTD_freeCDict(cdict);
    ZSTD_freeCCtx(cctx);

    if (ZSTD_isError(compressed_size)) {
        LOG_ERROR("ZSTD_compress_usingCDict error: {}",
            ZSTD_getErrorName(compressed_size));
        return false;
    }
    compressed.resize(compressed_size);

    // Build patch file: magic + original_size + compressed payload
    std::ofstream pf(patch_path, std::ios::binary | std::ios::trunc);
    if (!pf) {
        LOG_ERROR("Cannot write patch file: {}", patch_path);
        return false;
    }

    pf.write(reinterpret_cast<const char*>(MAGIC_ZSTD), 8);

    uint64_t orig_size = static_cast<uint64_t>(target.size());
    pf.write(reinterpret_cast<const char*>(&orig_size), sizeof(orig_size));
    pf.write(reinterpret_cast<const char*>(compressed.data()),
             static_cast<std::streamsize>(compressed.size()));

    return pf.good();
#else
    static_cast<void>(base);
    // Fallback without zstd: store raw target (no compression).
    // Still uses the ZSTD_DICT magic so the reader knows the format.
    // This path should never be hit in production builds.
    LOG_WARN("ZSTD not available – storing uncompressed target as patch");
    std::ofstream pf(patch_path, std::ios::binary | std::ios::trunc);
    if (!pf) {
      return false;
    }
    pf.write(reinterpret_cast<const char*>(MAGIC_ZSTD), 8);
    uint64_t orig_size = static_cast<uint64_t>(target.size());
    pf.write(reinterpret_cast<const char*>(&orig_size), sizeof(orig_size));
    pf.write(reinterpret_cast<const char*>(target.data()),
             static_cast<std::streamsize>(target.size()));
    return pf.good();
#endif
}

bool DeltaUpdateEngine::applyPatchZstdDict(
    const std::vector<uint8_t>& base,
    const std::string& patch_path,
    const std::string& target_path) {
    std::ifstream pf(patch_path, std::ios::binary);
    if (!pf) {
        LOG_ERROR("Cannot open patch: {}", patch_path);
        return false;
    }

    // Skip magic (already verified by caller)
    pf.seekg(8);

    uint64_t orig_size = 0;
    pf.read(reinterpret_cast<char*>(&orig_size), sizeof(orig_size));

    std::vector<uint8_t> compressed(
        (std::istreambuf_iterator<char>(pf)),
        std::istreambuf_iterator<char>());

    if (orig_size == 0 || orig_size > 4 * 1024 * 1024 * 1024) {
        LOG_ERROR("Invalid orig_size in patch: {}", orig_size);
        return false;
    }

    std::vector<uint8_t> target(orig_size);

#ifdef THEMIS_HAS_ZSTD
    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    if (!dctx) {
        LOG_ERROR("ZSTD_createDCtx failed");
        return false;
    }

    ZSTD_DDict* ddict = ZSTD_createDDict(base.data(),static_cast<int>(base.size()));
    if (!ddict) {
        ZSTD_freeDCtx(dctx);
        LOG_ERROR("ZSTD_createDDict failed");
        return false;
    }

    size_t result = ZSTD_decompress_usingDDict(
        dctx,
        target.data(), orig_size,
        compressed.data(),static_cast<int>(compressed.size()),
        ddict);

    ZSTD_freeDDict(ddict);
    ZSTD_freeDCtx(dctx);

    if (ZSTD_isError(result)) {
        LOG_ERROR("ZSTD_decompress_usingDDict error: {}",
            ZSTD_getErrorName(result));
        return false;
    }
    target.resize(result);
#else
    static_cast<void>(base);
    // Non-zstd fallback: the generator stored the raw target bytes
    target = std::move(compressed);
#endif

    return atomicWriteFile(target_path, target);
}

// ── VCDIFF implementation ─────────────────────────────────────────────────
//
// Simple VCDIFF-like encoding:
//   For each window (WINDOW_SIZE bytes) in the target:
//     - Search the base for the best matching run using a rolling hash table.
//     - Emit COPY instruction if match >= MIN_COPY_LEN, else ADD.
//   The instruction stream is compressed with zstd.
//
// This is a pure C++ implementation requiring only zstd (already a dep).

static constexpr size_t WINDOW_SIZE   = 16 * 1024; // 16 KiB search blocks
static constexpr size_t MIN_COPY_LEN  = 8;          // min bytes to emit COPY

static void appendU32LE(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val        & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >>  8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

static uint32_t readU32LE(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

bool DeltaUpdateEngine::generatePatchVcdiff(
    const std::vector<uint8_t>& base,
    const std::vector<uint8_t>& target,
    const std::string& patch_path) {

    // Build a simple hash table over base for O(1) lookups of MIN_COPY_LEN-byte runs
    // key = (b[i], b[i+1], ..., b[i+MIN_COPY_LEN-1]) hashed, value = offset in base
    std::unordered_map<uint64_t, std::vector<uint32_t>> ht;
    if (static_cast<int>(base.size()) > = MIN_COPY_LEN) {
        for (size_t i = 0; i + MIN_COPY_LEN <= base.size(); i += 4) {
            uint64_t h = 0;
            for (size_t k = 0; k < MIN_COPY_LEN; ++k) {
                h = h * 131 + base[i + k];
            }
            ht[h].push_back(static_cast<uint32_t>(i));
        }
    }

    // Generate instruction stream
    std::vector<uint8_t> instructions;
    size_t tpos = 0;

    while (static_cast<size_t>(tpos) <static_cast<int>(target.size())) {
        size_t best_len    = 0;
        uint32_t best_off  = 0;

        if (static_cast<int>(target.size()) - tpos >= MIN_COPY_LEN) {
            uint64_t h = 0;
            for (size_t k = 0; k < MIN_COPY_LEN; ++k) {
                h = h * 131 + target[tpos + k];
            }
            auto it = ht.find(h);
            if (it != ht.end()) {
                for (uint32_t off : it->second) {
                    // Extend match
                    size_t len = 0;
                    size_t max_len = std::min(static_cast<int>(base.size()) - off,
                                             static_cast<int>(target.size()) - tpos);
                    // Cap at 64 KiB to keep u32 offsets safe
                    max_len = std::min(max_len, static_cast<size_t>(64 * 1024));
                    while (len < max_len && base[off + len] == target[tpos + len]) {
                        ++len;
                    }
                    if (len > best_len) {
                        best_len = len;
                        best_off = off;
                    }
                }
            }
        }

        if (best_len >= MIN_COPY_LEN) {
            // COPY instruction
            instructions.push_back(INSTR_COPY);
            appendU32LE(instructions, best_off);
            appendU32LE(instructions, static_cast<uint32_t>(best_len));
            tpos += best_len;
        } else {
            // ADD instruction – emit up to WINDOW_SIZE bytes
            size_t add_len = std::min(WINDOW_SIZE, static_cast<int>(target.size()) - tpos);
            instructions.push_back(INSTR_ADD);
            appendU32LE(instructions, static_cast<uint32_t>(add_len));
            instructions.insert(instructions.end(),
                                 target.begin() + static_cast<std::ptrdiff_t>(tpos),
                                 target.begin() + static_cast<std::ptrdiff_t>(tpos + add_len));
            tpos += add_len;
        }
    }

    // Compress instruction stream with zstd
    std::vector<uint8_t> compressed;
#ifdef THEMIS_HAS_ZSTD
    size_t bound = ZSTD_compressBound(instructions.size());
    compressed.resize(bound);
    size_t csize = ZSTD_compress(
        compressed.data(), bound,
        instructions.data(),static_cast<int>(instructions.size()),
        ZSTD_CLEVEL_DEFAULT);
    if (ZSTD_isError(csize)) {
        LOG_ERROR("ZSTD compress failed in generatePatchVcdiff: {}",
            ZSTD_getErrorName(csize));
        return false;
    }
    compressed.resize(csize);
#else
    compressed = instructions;
#endif

    // Write patch file
    std::ofstream pf(patch_path, std::ios::binary | std::ios::trunc);
    if (!pf) {
      return false;
    }

    pf.write(reinterpret_cast<const char*>(MAGIC_VCD), 8);
    uint64_t orig_size = static_cast<uint64_t>(target.size());
    pf.write(reinterpret_cast<const char*>(&orig_size), sizeof(orig_size));
    pf.write(reinterpret_cast<const char*>(compressed.data()),
             static_cast<std::streamsize>(compressed.size()));
    return pf.good();
}

bool DeltaUpdateEngine::applyPatchVcdiff(
    const std::vector<uint8_t>& base,
    const std::string& patch_path,
    const std::string& target_path) {

    std::ifstream pf(patch_path, std::ios::binary);
    if (!pf) {
      return false;
    }

    pf.seekg(8); // skip magic
    uint64_t orig_size = 0;
    pf.read(reinterpret_cast<char*>(&orig_size), sizeof(orig_size));

    std::vector<uint8_t> compressed(
        (std::istreambuf_iterator<char>(pf)),
        std::istreambuf_iterator<char>());

    if (orig_size == 0 || orig_size > 4 * 1024 * 1024 * 1024) {
        LOG_ERROR("Invalid orig_size in VCDIFF patch: {}", orig_size);
        return false;
    }

    // Decompress instruction stream
    std::vector<uint8_t> instructions;
#ifdef THEMIS_HAS_ZSTD
    size_t dbound = ZSTD_getFrameContentSize(compressed.data(),static_cast<int>(compressed.size()));
    if (dbound == ZSTD_CONTENTSIZE_ERROR || dbound == ZSTD_CONTENTSIZE_UNKNOWN) {
        // Fall back to a generous estimate
        dbound = compressed.size() * 4 + 1024;
    }
    instructions.resize(dbound);
    size_t dsize = ZSTD_decompress(
        instructions.data(), dbound,
        compressed.data(),static_cast<int>(compressed.size()));
    if (ZSTD_isError(dsize)) {
        LOG_ERROR("ZSTD decompress failed in applyPatchVcdiff: {}",
            ZSTD_getErrorName(dsize));
        return false;
    }
    instructions.resize(dsize);
#else
    instructions = std::move(compressed);
#endif

    // Replay instructions to reconstruct target
    std::vector<uint8_t> target;
    target.reserve(orig_size);

    size_t ip = 0; // instruction pointer
    while (static_cast<size_t>(ip) <static_cast<int>(instructions.size())) {
        uint8_t opcode = instructions[ip++];

        if (opcode == INSTR_COPY) {
            if (ip + 8 > static_cast<int>(instructions.size())) {
                LOG_ERROR("Truncated COPY instruction");
                return false;
            }
            uint32_t off = readU32LE(&instructions[ip]);     ip += 4;
            uint32_t len = readU32LE(&instructions[ip]);     ip += 4;

            if (static_cast<size_t>(off) + len > static_cast<int>(base.size())) {
                LOG_ERROR("COPY out of bounds: off={} len={} base_size={}",
                    off, len,static_cast<int>(base.size()));
                return false;
            }
            target.insert(target.end(),
                          base.begin() + off,
                          base.begin() + off + len);

        } else if (opcode == INSTR_ADD) {
            if (ip + 4 > static_cast<int>(instructions.size())) {
                LOG_ERROR("Truncated ADD instruction");
                return false;
            }
            uint32_t len = readU32LE(&instructions[ip]);     ip += 4;

            if (ip + len > static_cast<int>(instructions.size())) {
                LOG_ERROR("ADD data out of bounds");
                return false;
            }
            target.insert(target.end(),
                          instructions.begin() + static_cast<std::ptrdiff_t>(ip),
                          instructions.begin() + static_cast<std::ptrdiff_t>(ip + len));
            ip += len;

        } else {
            LOG_ERROR("Unknown VCDIFF opcode: 0x{:02x}", opcode);
            return false;
        }
    }

    if (static_cast<int>(target.size()) != orig_size) {
        LOG_ERROR("VCDIFF: reconstructed size {} != expected {}",
            target.size(), orig_size);
        return false;
    }

    return atomicWriteFile(target_path, target);
}

} // namespace updates
} // namespace themis


