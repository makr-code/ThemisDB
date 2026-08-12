/**
 * @file tiered_index_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// Cold/Warm Tier Index Migration for ThemisDB
//
// Manages the lifecycle of indexes across three storage tiers:
//
//   HOT  – Indexes fully loaded in memory (or on fast NVMe SSD). Lowest
//           query latency, highest memory cost.
//   WARM – Indexes serialised to a local disk path but not held in memory.
//           Moderate latency (file I/O on first access), lower memory cost.
//   COLD – Indexes archived to an object-storage URI (s3://, gcs://, etc.)
//           or a slow local archive path. Highest latency; accessed rarely.
//
// Migration is driven by two configurable policies:
//   1. Age-based   – move index down a tier when last accessed > N seconds ago
//   2. Access-based – promote index up a tier when access count exceeds threshold
//
// Thread safety: all public methods are thread-safe (mutex-protected registry).
//
// Usage:
//   TieredIndexManager mgr(warm_dir, cold_dir);
//   mgr.setPolicy(policy);
//   mgr.registerIndex("my_index", IndexTierMeta::Tier::HOT, "/path/to/data");
//   mgr.recordAccess("my_index");
//   mgr.runMigrationPass();  // demote stale / promote hot indexes

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace index {

// ---------------------------------------------------------------------------
// IndexTierMeta – per-index tier state
// ---------------------------------------------------------------------------

/// Metadata describing an index's current tier and access statistics.
struct IndexTierMeta {
    enum class Tier { HOT = 0, WARM = 1, COLD = 2 };

    /// Human-readable tier name for logging / API responses.
    static const char* tierName(Tier t) noexcept {
        switch (t) {
            case Tier::HOT:  return "hot";
            case Tier::WARM: return "warm";
            case Tier::COLD: return "cold";
        }
        return "unknown";
    }

    Tier tier = Tier::HOT;

    /// Filesystem or object-storage path where index data resides on this tier.
    /// For HOT tier this is the live data directory; for WARM/COLD it is the
    /// exported snapshot location.
    std::string data_path;

    /// Wall-clock time of the most recent access (query or write).
    std::chrono::steady_clock::time_point last_access;

    /// Total number of accesses since registration (or last reset).
    uint64_t access_count = 0;

    /// Approximate size in bytes (informational; 0 means unknown).
    uint64_t size_bytes = 0;
};

// ---------------------------------------------------------------------------
// TierMigrationPolicy – thresholds that drive automatic migrations
// ---------------------------------------------------------------------------

/// Configuration for the automatic migration pass.
struct TierMigrationPolicy {
    /// Seconds of inactivity before a HOT index is demoted to WARM.
    uint64_t hot_to_warm_idle_secs = 3600;      // 1 hour

    /// Seconds of inactivity before a WARM index is demoted to COLD.
    uint64_t warm_to_cold_idle_secs = 86400;    // 24 hours

    /// Access-count threshold: when a WARM index is accessed at least this
    /// many times within `promotion_window_secs`, promote it to HOT.
    uint64_t warm_to_hot_access_threshold = 10;

    /// Access-count threshold: when a COLD index is accessed at least this
    /// many times within `promotion_window_secs`, promote it to WARM.
    uint64_t cold_to_warm_access_threshold = 3;

    /// Time window (seconds) for the access-count promotion check.
    uint64_t promotion_window_secs = 300;       // 5 minutes
};

// ---------------------------------------------------------------------------
// MigrationResult – outcome of a single index migration
// ---------------------------------------------------------------------------

enum class MigrationDiagnosticCode {
    NONE = 0,
    INDEX_NOT_FOUND,
    TIER_MISMATCH,
    EXPORT_FAILED,
    IMPORT_FAILED
};

struct MigrationResult {
    bool ok = true;
    MigrationDiagnosticCode code = MigrationDiagnosticCode::NONE;
    std::string message;
    std::string index_name;
    std::string source_path;
    std::string target_path;
    IndexTierMeta::Tier from_tier{};
    IndexTierMeta::Tier to_tier{};

    static MigrationResult Ok(std::string name,
                              IndexTierMeta::Tier from,
                              IndexTierMeta::Tier to,
                              std::string source = {},
                              std::string target = {}) {
        return {true,
                MigrationDiagnosticCode::NONE,
                "",
                std::move(name),
                std::move(source),
                std::move(target),
                from,
                to};
    }
    static MigrationResult Err(std::string name,
                               IndexTierMeta::Tier from,
                               IndexTierMeta::Tier to,
                               MigrationDiagnosticCode diagnostic_code,
                               std::string msg,
                               std::string source = {},
                               std::string target = {}) {
        return {false,
                diagnostic_code,
                std::move(msg),
                std::move(name),
                std::move(source),
                std::move(target),
                from,
                to};
    }
};

// ---------------------------------------------------------------------------
// TieredIndexManager
// ---------------------------------------------------------------------------

/// Manages cold/warm/hot tier migration for named index data snapshots.
///
/// The manager does NOT own the live index objects (VectorIndexManager,
/// SecondaryIndexManager, etc.); it coordinates where their serialised data
/// is stored and uses caller-supplied serialize/deserialize callbacks when
/// a migration is requested.
class TieredIndexManager {
public:
    /// Callback type invoked when an index must be exported (demoted).
    /// @param index_name  Logical name of the index.
    /// @param dest_path   Target path to write the serialised snapshot.
    /// @returns true on success.
    using ExportFn  = std::function<bool(const std::string& index_name,
                                          const std::string& dest_path)>;

    /// Callback type invoked when an index must be re-loaded (promoted).
    /// @param index_name  Logical name of the index.
    /// @param src_path    Path of the serialised snapshot to load.
    /// @returns true on success.
    using ImportFn  = std::function<bool(const std::string& index_name,
                                          const std::string& src_path)>;

    /// @param warm_base_dir  Root directory used for WARM tier snapshots.
    /// @param cold_base_dir  Root directory (or URI prefix) for COLD tier archives.
    explicit TieredIndexManager(std::string warm_base_dir,
                                 std::string cold_base_dir);

    ~TieredIndexManager() = default;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /// Replace the current migration policy.
    void setPolicy(const TierMigrationPolicy& policy);

    /// Return the active migration policy.
    TierMigrationPolicy policy() const;

    /// Set the export callback (called on demotion).  Defaults to a no-op
    /// that always returns true (useful for tests or read-only registries).
    void setExportFn(ExportFn fn);

    /// Set the import callback (called on promotion).  Defaults to a no-op.
    void setImportFn(ImportFn fn);

    // -----------------------------------------------------------------------
    // Registry
    // -----------------------------------------------------------------------

    /// Register a new index in the HOT tier.
    /// @param name       Logical index name (must be unique).
    /// @param data_path  Live data path for this index.
    /// @param size_bytes Estimated size in bytes (optional).
    /// @returns true if registration succeeded; false if name already exists.
    bool registerIndex(const std::string& name,
                       const std::string& data_path,
                       uint64_t           size_bytes = 0);

    /// Register an existing index in a specific tier (e.g., recovered from disk).
    bool registerIndex(const std::string&   name,
                       IndexTierMeta::Tier  tier,
                       const std::string&   data_path,
                       uint64_t             size_bytes = 0);

    /// Remove an index from the registry (does not delete data on disk).
    /// @returns true if the index existed and was removed.
    bool unregisterIndex(const std::string& name);

    /// Return true if @p name is currently registered.
    bool hasIndex(const std::string& name) const;

    /// Return a snapshot of the metadata for a registered index, or nullopt.
    /// The returned value is a copy; it will not update as accesses occur.
    std::optional<IndexTierMeta> getMetadata(const std::string& name) const;

    /// Return names of all registered indexes.
    std::vector<std::string> listIndexes() const;

    /// Return names of all indexes in a specific tier.
    std::vector<std::string> listIndexesByTier(IndexTierMeta::Tier tier) const;

    // -----------------------------------------------------------------------
    // Access tracking
    // -----------------------------------------------------------------------

    /// Record that @p name was accessed.  Updates last_access and access_count.
    /// @returns true if the index was found; false otherwise.
    bool recordAccess(const std::string& name);

    /// Reset the access counter for @p name to zero.
    bool resetAccessCount(const std::string& name);

    // -----------------------------------------------------------------------
    // Manual migration
    // -----------------------------------------------------------------------

    /// Manually migrate @p name to the specified tier.
    MigrationResult migrateTo(const std::string& name, IndexTierMeta::Tier target);

    /// Convenience wrappers.
    MigrationResult promoteToHot(const std::string& name);
    MigrationResult demoteToWarm(const std::string& name);
    MigrationResult demoteToCold(const std::string& name);

    // -----------------------------------------------------------------------
    // Automatic migration pass
    // -----------------------------------------------------------------------

    /// Evaluate all registered indexes against the active policy and migrate
    /// those that qualify.  Returns the list of migrations performed.
    ///
    /// Call this periodically (e.g., from a background scheduler) or on-demand.
    std::vector<MigrationResult> runMigrationPass();

    // -----------------------------------------------------------------------
    // Tier path helpers
    // -----------------------------------------------------------------------

    /// Build the warm-tier snapshot path for @p name.
    std::string warmPath(const std::string& name) const;

    /// Build the cold-tier snapshot path for @p name.
    std::string coldPath(const std::string& name) const;

private:
    // Derive the destination path for a tier.
    std::string pathForTier(const std::string& name, IndexTierMeta::Tier tier) const;

    // Execute the actual migration (calls export_fn_ / import_fn_ as needed).
    // Caller must NOT hold registry_mutex_.
    MigrationResult doMigrate(const std::string& name,
                               IndexTierMeta::Tier from,
                               IndexTierMeta::Tier to);

    mutable std::shared_mutex registry_mutex_;
    std::unordered_map<std::string, IndexTierMeta> registry_;

    std::string warm_base_dir_;
    std::string cold_base_dir_;

    TierMigrationPolicy policy_;

    ExportFn export_fn_;
    ImportFn import_fn_;
};

} // namespace index
} // namespace themis
