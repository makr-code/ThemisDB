/**
 * @file ingestion_plugin.cpp
 * @brief Plugin system for custom content ingestion and processing pipelines.
 * @version 0.0.47
 * @note Maturity: 🔴 ALPHA
 * @note Score: 69/100
 * @note Status: Beta; Plugin loading framework in place; dynamic configuration and hot-reload under development
 * @note This block is auto-generated and will be overwritten.
 */
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

