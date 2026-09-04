#include "maintenance/maintenance_schedule_store.h"
#include "maintenance/maintenance_api_contract.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <string>

namespace themis {
namespace maintenance {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MaintenanceScheduleStore::MaintenanceScheduleStore(IStorageEngine* engine)
    : engine_(engine)
{}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string MaintenanceScheduleStore::makeKey(const std::string& id) {
    std::string key = {};
    key.reserve(kKeyPrefix.size() + id.size());
    key.append(kKeyPrefix);
    key.append(id);
    return key;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

Result<void> MaintenanceScheduleStore::save(const MaintenanceScheduleEntry& entry) {
    const std::string key   = makeKey(entry.id);
    const std::string value = entry.toJson().dump();
    return engine_->put(key, value);
}

Result<void> MaintenanceScheduleStore::remove(const std::string& id) {
    return engine_->del(makeKey(id));
}

Result<void> MaintenanceScheduleStore::loadAll(
    std::map<std::string, MaintenanceScheduleEntry>& schedules)
{
    int loaded  = 0;
    int skipped = 0;

    // Collect entries into a temporary map; merge into schedules after scanning
    // so callers can hold an external mutex only for the merge step.
    std::map<std::string, MaintenanceScheduleEntry> tmp;

    auto scan_result = engine_->scanPrefix(
        kKeyPrefix,
        [&](std::string_view key, std::string_view value) -> bool {
            // Derive the schedule id from the RocksDB key by stripping the prefix.
            if (key.size() < kKeyPrefix.size()) {
                spdlog::warn("MaintenanceScheduleStore::loadAll: skipping entry "
                             "with malformed key '{}' (shorter than prefix)",
                             std::string(key));
                ++skipped;
                return true;
            }
            const std::string id_from_key(key.substr(kKeyPrefix.size()));

            try {
                auto j     = nlohmann::json::parse(value);
                auto entry = MaintenanceScheduleEntry::fromJson(j);

                // If the JSON payload's id is missing, recover it from the key.
                if (entry.id.empty()) {
                    spdlog::warn("MaintenanceScheduleStore::loadAll: recovering empty "
                                 "id for key '{}' from key suffix",
                                 std::string(key));
                    entry.id = id_from_key;
                } else if (entry.id != id_from_key) {
                    // Prefer the id encoded in the key; the payload may be stale.
                    spdlog::warn(
                        "MaintenanceScheduleStore::loadAll: id mismatch at key '{}' "
                        "(key-derived id '{}', payload id '{}'); using id from key",
                        std::string(key), id_from_key, entry.id);
                    entry.id = id_from_key;
                }

                tmp[entry.id] = std::move(entry);
                ++loaded;
            } catch (const nlohmann::json::exception& ex) {
                spdlog::error("MaintenanceScheduleStore::loadAll: corrupt "
                              "schedule entry at key '{}' – JSON parse error: {}",
                              std::string(key), ex.what());
                ++skipped;
            }
            return true; // continue scanning
        });

    if (!scan_result.has_value()) {
        return scan_result;
    }

    // If any entry was corrupt / unparseable, return PersistenceCorrupt so
    // callers can surface this rather than silently loading a partial set.
    if (skipped > 0) {
        spdlog::error("MaintenanceScheduleStore::loadAll: {} corrupt entry(ies) found; "
                      "returning PersistenceCorrupt (loaded {} good entry(ies))",
                      skipped, loaded);
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
            "MaintenanceScheduleStore::loadAll: PersistenceCorrupt — " +
            std::to_string(skipped) + " corrupt entry(ies) detected"));
    }

    // Merge loaded entries into the caller's map.
    for (auto& [id, entry] : tmp) {
        schedules[id] = std::move(entry);
    }

    spdlog::info("MaintenanceScheduleStore::loadAll: loaded {} schedule(s)", loaded);
    return OkVoid();
}

} // namespace maintenance
} // namespace themis
