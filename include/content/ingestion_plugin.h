/*
 * ThemisDB | File: ingestion_plugin.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 115
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #1221 feat: Plugin-based multi-so... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

// Forward declarations
struct IngestionJob;
enum class IngestionJobType;

/**
 * @brief Ingestion plugin interface
 * 
 * Plugins implement this interface to add new data sources
 * to AsyncIngestionWorker.
 * 
 * Example plugin: HuggingFaceIngestionPlugin
 * 
 * Thread-Safety: Plugin implementations must be thread-safe.
 */
class IngestionPlugin {
public:
    virtual ~IngestionPlugin() = default;
    
    /**
     * @brief Plugin name (unique identifier)
     */
    [[nodiscard]] virtual std::string name() const = 0;
    
    /**
     * @brief Plugin version
     */
    [[nodiscard]] virtual std::string version() const = 0;
    
    /**
     * @brief Job types this plugin can handle
     */
    [[nodiscard]] virtual std::vector<IngestionJobType> supportedTypes() const = 0;
    
    /**
     * @brief Process an ingestion job
     * 
     * Plugin should:
     * 1. Fetch data from source
     * 2. Update job.progress
     * 3. Store content via ContentManager
     * 4. Update job.content_ids
     * 5. Set job.status to COMPLETED or FAILED
     * 
     * @param job Job to process (will be modified in-place)
     */
    virtual void processJob(IngestionJob& job) = 0;
    
    /**
     * @brief Estimate job size (for progress tracking)
     * 
     * @return Estimated number of items to process, or 0 if unknown
     */
    [[nodiscard]] virtual size_t estimateJobSize(const IngestionJob& job) = 0;
    
    /**
     * @brief Get plugin configuration
     */
    [[nodiscard]] virtual json getConfig() const = 0;
    
    /**
     * @brief Set plugin configuration
     */
    virtual void setConfig(const json& config) = 0;
};

/**
 * @brief Data source configuration
 * 
 * Describes an external data source that can be ingested
 * via a registered plugin.
 */
struct IngestionSource {
    std::string source_id;          ///< Unique identifier
    std::string plugin_name;        ///< Which plugin handles this
    IngestionJobType type;          ///< Job type
    std::string location;           ///< URL, path, connection string
    json config;                    ///< Plugin-specific configuration
    int priority = 0;               ///< Higher = preferred in conflicts
    std::vector<std::string> tags;  ///< Classification tags
    bool incremental = true;        ///< Only fetch new data
    
    /**
     * @brief Serialize to JSON
     */
    json toJson() const;
    
    /**
     * @brief Deserialize from JSON
     */
    static IngestionSource fromJson(const json& j);
};

} // namespace content
} // namespace themis
