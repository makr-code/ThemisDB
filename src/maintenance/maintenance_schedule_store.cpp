/**
 * @file maintenance_schedule_store.cpp
 * @brief Implementation of MaintenanceScheduleStore.
 */

#include "maintenance/maintenance_schedule_store.h"

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
    std::string key;
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
            try {
                auto j     = nlohmann::json::parse(value);
                auto entry = MaintenanceScheduleEntry::fromJson(j);
                if (!entry.id.empty()) {
                    tmp[entry.id] = std::move(entry);
                    ++loaded;
                } else {
                    spdlog::warn("MaintenanceScheduleStore::loadAll: skipping entry "
                                 "with empty id at key '{}' (corrupt or missing 'id' field)",
                                 std::string(key));
                    ++skipped;
                }
            } catch (const nlohmann::json::exception& ex) {
                spdlog::warn("MaintenanceScheduleStore::loadAll: skipping corrupt "
                             "schedule entry at key '{}' – JSON parse error: {}",
                             std::string(key), ex.what());
                ++skipped;
            }
            return true; // continue scanning
        });

    if (!scan_result.has_value()) {
        return scan_result;
    }

    // Merge loaded entries into the caller's map.
    for (auto& [id, entry] : tmp) {
        schedules[id] = std::move(entry);
    }

    spdlog::info("MaintenanceScheduleStore::loadAll: loaded {} schedule(s), "
                 "skipped {} corrupt entry(ies)", loaded, skipped);
    return OkVoid();
}

} // namespace maintenance
} // namespace themis
