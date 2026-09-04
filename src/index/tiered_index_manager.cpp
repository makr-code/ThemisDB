/**
 * @file tiered_index_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Cold/Warm Tier Index Migration — implementation
//
// See include/index/tiered_index_manager.h for design notes.

#include "index/tiered_index_manager.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace index {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TieredIndexManager::TieredIndexManager(std::string warm_base_dir,
                                         std::string cold_base_dir)
    : warm_base_dir_(std::move(warm_base_dir))
    , cold_base_dir_(std::move(cold_base_dir))
    , export_fn_([](const std::string&, const std::string&) { return true; })
    , import_fn_([](const std::string&, const std::string&) { return true; })
{}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void TieredIndexManager::setPolicy(const TierMigrationPolicy& policy) {
    std::unique_lock<std::shared_mutex> lk(registry_mutex_);
    policy_ = policy;
}

TierMigrationPolicy TieredIndexManager::policy() const {
    std::shared_lock<std::shared_mutex> lk(registry_mutex_);
    return policy_;
}

void TieredIndexManager::setExportFn(ExportFn fn) {
    std::unique_lock<std::shared_mutex> lk(registry_mutex_);
    export_fn_ = fn ? std::move(fn)
                    : [](const std::string&, const std::string&) { return true; };
}

void TieredIndexManager::setImportFn(ImportFn fn) {
    std::unique_lock<std::shared_mutex> lk(registry_mutex_);
    import_fn_ = fn ? std::move(fn)
                    : [](const std::string&, const std::string&) { return true; };
}

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------

bool TieredIndexManager::registerIndex(const std::string& name,
                                         const std::string& data_path,
                                         uint64_t           size_bytes) {
    return registerIndex(name, IndexTierMeta::Tier::HOT, data_path, size_bytes);
}

bool TieredIndexManager::registerIndex(const std::string&  name,
                                         IndexTierMeta::Tier tier,
                                         const std::string&  data_path,
                                         uint64_t            size_bytes) {
    if (name.empty()) {
      return false;
    }

    std::unique_lock<std::shared_mutex> lk(registry_mutex_);
    if (registry_.count(name)) return false;   // already registered

    IndexTierMeta meta;
    meta.tier         = tier;
    meta.data_path    = data_path;
    meta.last_access  = std::chrono::steady_clock::now();
    meta.access_count = 0;
    meta.size_bytes   = size_bytes;
    registry_.emplace(name, std::move(meta));
    return true;
}

bool TieredIndexManager::unregisterIndex(const std::string& name) {
    std::unique_lock<std::shared_mutex> lk(registry_mutex_);
    return registry_.erase(name) > 0;
}

bool TieredIndexManager::hasIndex(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lk(registry_mutex_);
    return registry_.count(name) > 0;
}

std::optional<IndexTierMeta> TieredIndexManager::getMetadata(
        const std::string& name) const {
    std::shared_lock<std::shared_mutex> lk(registry_mutex_);
    auto it = registry_.find(name);
    if (it == registry_.end()) {
      return std::nullopt;
    }
    return it->second;
}

std::vector<std::string> TieredIndexManager::listIndexes() const {
    std::shared_lock<std::shared_mutex> lk(registry_mutex_);
    std::vector<std::string> names;
    names.reserve(registry_.size());
    for (const auto& [k, _] : registry_) {
      names.push_back(k);
    }
    // Sort for deterministic iteration order (registry_ is unordered_map)
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> TieredIndexManager::listIndexesByTier(
        IndexTierMeta::Tier tier) const {
    std::shared_lock<std::shared_mutex> lk(registry_mutex_);
    std::vector<std::string> names;
    names.reserve(registry_.size());
    for (const auto& [k, v] : registry_) {
        if (v.tier == tier) {
          names.push_back(k);
        }
    }
    // Sort for deterministic iteration order (registry_ is unordered_map)
    std::sort(names.begin(), names.end());
    return names;
}

// ---------------------------------------------------------------------------
// Access tracking
// ---------------------------------------------------------------------------

bool TieredIndexManager::recordAccess(const std::string& name) {
    std::unique_lock<std::shared_mutex> lk(registry_mutex_);
    auto it = registry_.find(name);
    if (it == registry_.end()) {
      return false;
    }
    it->second.last_access = std::chrono::steady_clock::now();
    ++it->second.access_count;
    return true;
}

bool TieredIndexManager::resetAccessCount(const std::string& name) {
    std::unique_lock<std::shared_mutex> lk(registry_mutex_);
    auto it = registry_.find(name);
    if (it == registry_.end()) {
      return false;
    }
    it->second.access_count = 0;
    return true;
}

// ---------------------------------------------------------------------------
// Manual migration (public wrappers)
// ---------------------------------------------------------------------------

MigrationResult TieredIndexManager::migrateTo(const std::string&  name,
                                                IndexTierMeta::Tier target) {
    // Snapshot current tier under lock.
    IndexTierMeta::Tier current{};
    {
        std::unique_lock<std::shared_mutex> lk(registry_mutex_);
        auto it = registry_.find(name);
        if (it == registry_.end()) {
            return MigrationResult::Err(name,
                                        target,
                                        target,
                                        MigrationDiagnosticCode::INDEX_NOT_FOUND,
                                        "index not found");
        }
        current = it->second.tier;
    }

    if (current == target) {
        // Already in requested tier – treat as success.
        return MigrationResult::Ok(name, current, target);
    }

    return doMigrate(name, current, target);
}

MigrationResult TieredIndexManager::promoteToHot(const std::string& name) {
    return migrateTo(name, IndexTierMeta::Tier::HOT);
}

MigrationResult TieredIndexManager::demoteToWarm(const std::string& name) {
    return migrateTo(name, IndexTierMeta::Tier::WARM);
}

MigrationResult TieredIndexManager::demoteToCold(const std::string& name) {
    return migrateTo(name, IndexTierMeta::Tier::COLD);
}

// ---------------------------------------------------------------------------
// Automatic migration pass
// ---------------------------------------------------------------------------

std::vector<MigrationResult> TieredIndexManager::runMigrationPass() {
    using Tier = IndexTierMeta::Tier;

    // Snapshot registry state under lock to avoid holding the mutex during
    // potentially long export/import callbacks.
    TierMigrationPolicy pol;
    std::vector<std::pair<std::string, IndexTierMeta>> snapshot;
    {
        std::unique_lock<std::shared_mutex> lk(registry_mutex_);
        pol = policy_;
        snapshot.reserve(registry_.size());
        for (const auto& [k, v] : registry_) {
          snapshot.emplace_back(k, v);
        }
    }

    const auto now = std::chrono::steady_clock::now();

    std::vector<MigrationResult> results;
    results.reserve(snapshot.size());

    for (const auto& [name, meta] : snapshot) {
        const auto idle_secs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                now - meta.last_access).count());

        // ---- Demotion checks (age-based) -----------------------------------
        if (meta.tier == Tier::HOT && idle_secs >= pol.hot_to_warm_idle_secs) {
            results.push_back(doMigrate(name, Tier::HOT, Tier::WARM));
            continue;
        }

        if (meta.tier == Tier::WARM && idle_secs >= pol.warm_to_cold_idle_secs) {
            results.push_back(doMigrate(name, Tier::WARM, Tier::COLD));
            continue;
        }

        // ---- Promotion checks (access-count based) -------------------------
        // Only consider accesses within the promotion window.
        const bool in_window = (idle_secs <= pol.promotion_window_secs);

        if (meta.tier == Tier::WARM && in_window
                && meta.access_count >= pol.warm_to_hot_access_threshold) {
            results.push_back(doMigrate(name, Tier::WARM, Tier::HOT));
            continue;
        }

        if (meta.tier == Tier::COLD && in_window
                && meta.access_count >= pol.cold_to_warm_access_threshold) {
            results.push_back(doMigrate(name, Tier::COLD, Tier::WARM));
            continue;
        }
    }

    return results;
}

// ---------------------------------------------------------------------------
// Tier path helpers
// ---------------------------------------------------------------------------

std::string TieredIndexManager::warmPath(const std::string& name) const {
    return warm_base_dir_ + "/" + name;
}

std::string TieredIndexManager::coldPath(const std::string& name) const {
    return cold_base_dir_ + "/" + name;
}

std::string TieredIndexManager::pathForTier(const std::string&  name,
                                              IndexTierMeta::Tier tier) const {
    using Tier = IndexTierMeta::Tier;
    switch (tier) {
        case Tier::WARM: return warmPath(name);
        case Tier::COLD: return coldPath(name);
        default:         return {};          // HOT: live path managed by caller
    }
}

// ---------------------------------------------------------------------------
// doMigrate – execute migration, update registry
// ---------------------------------------------------------------------------

MigrationResult TieredIndexManager::doMigrate(const std::string&  name,
                                                IndexTierMeta::Tier from,
                                                IndexTierMeta::Tier to) {
    // Capture callbacks under lock so we can call them outside.
    ExportFn export_fn;
    ImportFn import_fn;
    std::string live_path;
    {
        std::unique_lock<std::shared_mutex> lk(registry_mutex_);
        auto it = registry_.find(name);
        if (it == registry_.end()) {
            return MigrationResult::Err(name,
                                        from,
                                        to,
                                        MigrationDiagnosticCode::INDEX_NOT_FOUND,
                                        "index not found during migration");
        }
        if (it->second.tier != from) {
            std::ostringstream oss;
            oss << "migration aborted: tier changed from "
                << IndexTierMeta::tierName(from) << " to "
                << IndexTierMeta::tierName(it->second.tier);
            return MigrationResult::Err(name,
                                        from,
                                        to,
                                        MigrationDiagnosticCode::TIER_MISMATCH,
                                        oss.str(),
                                        it->second.data_path,
                                        pathForTier(name, to));
        }
        export_fn = export_fn_;
        import_fn = import_fn_;
        live_path = it->second.data_path;
    }

    const std::string dest_path = pathForTier(name, to);
    const std::string src_path  = pathForTier(name, from);
    // Always report the registered path (live_path) in migration results
    const std::string target_path = live_path;
    const std::string source_path = live_path;
    // Actual filesystem paths for export/import callbacks
    const std::string export_dest = dest_path.empty() ? live_path : dest_path;
    const std::string import_src  = src_path.empty() ? live_path : src_path;

    const bool is_demotion = (static_cast<int>(to) > static_cast<int>(from));

    if (is_demotion) {
        // Export (serialize) the index to the destination tier path.
        try {
            if (!export_fn(name, export_dest)) {
                std::ostringstream oss;
                oss << "export failed while demoting from "
                    << IndexTierMeta::tierName(from) << " to "
                    << IndexTierMeta::tierName(to)
                    << " (target=" << export_dest << ")";
                return MigrationResult::Err(
                    name,
                    from,
                    to,
                    MigrationDiagnosticCode::EXPORT_FAILED,
                    oss.str(),
                    source_path,
                    target_path);
            }
        } catch (const std::exception& e) {
            std::ostringstream oss;
            oss << "export threw while demoting from "
                << IndexTierMeta::tierName(from) << " to "
                << IndexTierMeta::tierName(to)
                << " (target=" << export_dest << "): "
                << e.what();
            return MigrationResult::Err(
                name,
                from,
                to,
                MigrationDiagnosticCode::EXPORT_FAILED,
                oss.str(),
                source_path,
                target_path);
        } catch (...) {
            std::ostringstream oss;
            oss << "export threw non-standard exception while demoting from "
                << IndexTierMeta::tierName(from) << " to "
                << IndexTierMeta::tierName(to)
                << " (target=" << export_dest << ")";
            return MigrationResult::Err(
                name,
                from,
                to,
                MigrationDiagnosticCode::EXPORT_FAILED,
                oss.str(),
                source_path,
                target_path);
        }
    } else {
        // Promotion: import (deserialize) from the source tier path.
        try {
            if (!import_fn(name, import_src)) {
                std::ostringstream oss;
                oss << "import failed while promoting from "
                    << IndexTierMeta::tierName(from) << " to "
                    << IndexTierMeta::tierName(to)
                    << " (source=" << import_src << ")";
                return MigrationResult::Err(
                    name,
                    from,
                    to,
                    MigrationDiagnosticCode::IMPORT_FAILED,
                    oss.str(),
                    source_path,
                    target_path);
            }
        } catch (const std::exception& e) {
            std::ostringstream oss;
            oss << "import threw while promoting from "
                << IndexTierMeta::tierName(from) << " to "
                << IndexTierMeta::tierName(to)
                << " (source=" << import_src << "): "
                << e.what();
            return MigrationResult::Err(
                name,
                from,
                to,
                MigrationDiagnosticCode::IMPORT_FAILED,
                oss.str(),
                source_path,
                target_path);
        } catch (...) {
            std::ostringstream oss;
            oss << "import threw non-standard exception while promoting from "
                << IndexTierMeta::tierName(from) << " to "
                << IndexTierMeta::tierName(to)
                << " (source=" << import_src << ")";
            return MigrationResult::Err(
                name,
                from,
                to,
                MigrationDiagnosticCode::IMPORT_FAILED,
                oss.str(),
                source_path,
                target_path);
        }
    }

    // Update registry.
    {
        std::unique_lock<std::shared_mutex> lk(registry_mutex_);
        auto it = registry_.find(name);
        if (it == registry_.end()) {
            return MigrationResult::Err(name,
                                        from,
                                        to,
                                        MigrationDiagnosticCode::INDEX_NOT_FOUND,
                                        "index disappeared before migration state could be updated",
                                        source_path,
                                        target_path);
        }
        if (it->second.tier != from) {
            std::ostringstream oss;
            oss << "migration state update aborted: tier changed from "
                << IndexTierMeta::tierName(from) << " to "
                << IndexTierMeta::tierName(it->second.tier);
            return MigrationResult::Err(name,
                                        from,
                                        to,
                                        MigrationDiagnosticCode::TIER_MISMATCH,
                                        oss.str(),
                                        source_path,
                                        target_path);
        }
        it->second.tier = to;
        it->second.data_path = target_path;
        it->second.last_access = std::chrono::steady_clock::now();
        it->second.access_count = 0;
    }

    return MigrationResult::Ok(name, from, to, source_path, target_path);
}

} // namespace index
} // namespace themis
