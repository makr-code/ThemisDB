/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingestion_plugin.cpp                               ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     74                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
