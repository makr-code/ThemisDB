/**
 * @file ingestion_plugin.cpp
 * @brief Plugin system for custom content ingestion and processing pipelines.
 * @version 0.0.47
 * @note Maturity: 🟡 BETA
 * @note Score: 69/100
 * @note Gap Summary: total=16; TODO=3, Stub=2, Unimpl=2, Mock=1, Sim=0, Debt=2, C=2, H=4, M=8, L=0
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

