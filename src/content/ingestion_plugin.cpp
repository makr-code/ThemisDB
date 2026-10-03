/** @file ingestion_plugin.cpp @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata. @author makr-code @version 0.0.1 @date 2026-09-18 21:43:08 @note Maturity: 🟢 PRODUCTION-READY @note Score: 100/100 @note Lines: 47 @note Module Context: src/content @note Ownership Scope: production-code @note Primary Symbols: none-detected @note PR History (last 5): #5950 [CP-1] Content Module Batch... (2026-08-15) @note Governance: BranchModel=develop-first; CanonicalBranches=develop,community,military @note Release Context: GateModel=WaveA→B→C→D on develop @note Status: Production Ready @note Generator: .github/scripts/code_maturity_header_writer.py @note This block is auto-generated and will be overwritten. */

#include "content/ingestion_plugin.h"
#include "content/async_ingestion_worker.h"

namespace themis {
namespace content {

// ============================================================================
// IngestionSource JSON Serialization
// ============================================================================

json IngestionSource::toJson() const {
    return json{
        {"source_id", source_id},
        {"plugin_name", plugin_name},
        {"type", static_cast<int>(type)},
        {"location", location},
        {"config", config},
        {"priority", priority},
        {"tags", tags},
        {"incremental", incremental}
    };
}

/**
 * @brief From Json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: at(), value(), json::object().
 */
IngestionSource IngestionSource::fromJson(const json& j) {
    IngestionSource source;
    source.source_id = j.at("source_id").get<std::string>();
    source.plugin_name = j.at("plugin_name").get<std::string>();
    source.type = static_cast<IngestionJobType>(j.at("type").get<int>());
    source.location = j.at("location").get<std::string>();
    source.config = j.value("config", json::object());
    source.priority = j.value("priority", 0);
    source.tags = j.value("tags", std::vector<std::string>{});
    source.incremental = j.value("incremental", true);
    return source;
}

} // namespace content
} // namespace themis

