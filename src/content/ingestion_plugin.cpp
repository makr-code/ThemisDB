/*
 * ThemisDB | File: ingestion_plugin.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 58
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file ingestion_plugin.cpp
 * @brief Ingestion Plugin Support Implementation
 * 
 * JSON serialization and helpers for IngestionSource.
 * 
 * @author ThemisDB Team
 * @date February 2026
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

